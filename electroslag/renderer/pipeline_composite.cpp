//  Electroslag Interactive Graphics System
//  Copyright 2018 Joshua Buckman
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#include "electroslag/precomp.hpp"
#include "electroslag/threading/thread_pool.hpp"
#include "electroslag/graphics/serialized_graphics_types.hpp"
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/renderer/pipeline_composite.hpp"
#include "electroslag/renderer/renderer.hpp"
#include "electroslag/renderer/uniform_buffer_manager.hpp"
#include "electroslag/renderer/texture_manager.hpp"

namespace electroslag {
    namespace renderer {
        pipeline_composite::pipeline_composite(
            pipeline_descriptor::ref const& desc,
            graphics::shader_field_map::ref const& vertex_attrib_field_map
            )
            : m_initialization_step(initialization_step_create_shaders)
            , m_desc(desc)
            , m_vertex_attrib_field_map(vertex_attrib_field_map)
            , m_dynamic_ubo_size(0)
            , m_depth_test(*desc->get_depth_test_params())
            , m_blending(*desc->get_blending_params())
        {}

        void pipeline_composite::initialize_step()
        {
            // Initialization happens over the course of several frames; each step
            // depends on the completion of the graphics API calls from the prior step.
            switch (m_initialization_step) {
            case initialization_step_create_shaders:
                m_shader = get_renderer_internal()->get_shader_program_manager()->get_shader_program(
                    m_desc->get_shader(),
                    m_vertex_attrib_field_map
                    );
                m_sync_operation = graphics::get_graphics()->get_render_policy()->get_system_sync();
                m_initialization_step = initialization_step_wait_for_shaders;
                break;

            case initialization_step_wait_for_shaders:
                if (m_sync_operation->is_signaled()) {
                    m_initialization_step = initialization_step_create_textures;
                    // Intentional fall through!
                }
                else {
                    break;
                }

            case initialization_step_create_textures: {
                graphics::shader_program_descriptor::ref shader_desc(m_shader->get_descriptor());

                // For each stage; create textures that are referenced via static UBO.
                bool wait_for_textures = false;
                wait_for_textures |= create_stage_textures(shader_desc->get_vertex_shader());
                wait_for_textures |= create_stage_textures(shader_desc->get_tessellation_control_shader());
                wait_for_textures |= create_stage_textures(shader_desc->get_tessellation_evaluation_shader());
                wait_for_textures |= create_stage_textures(shader_desc->get_geometry_shader());
                wait_for_textures |= create_stage_textures(shader_desc->get_fragment_shader());
                wait_for_textures |= create_stage_textures(shader_desc->get_compute_shader());

                if (wait_for_textures) {
                    m_sync_operation = graphics::get_graphics()->get_render_policy()->get_system_sync();
                    m_initialization_step = initialization_step_wait_for_textures;
                }
                else {
                    // TODO: Could we goto the case instead of waiting for another initialize_step that we
                    // don't need?
                    m_initialization_step = initialization_step_create_ubo;
                }
                break;
            }

            case initialization_step_wait_for_textures:
                if (m_sync_operation->is_signaled()) {
                    m_initialization_step = initialization_step_create_ubo;
                    // Intentional fall through!
                }
                else {
                    break;
                }

            case initialization_step_create_ubo: {
                graphics::shader_program_descriptor::ref shader_desc(m_shader->get_descriptor());
                int min_ubo_alignment = graphics::get_graphics()->get_context_capability()->get_min_ubo_offset_alignment();
                m_dynamic_ubo_size = 0;

                // For each stage; set up UBOs.
                bool wait_for_ubos = false;
                wait_for_ubos |= create_stage_ubo(shader_desc->get_vertex_shader(), min_ubo_alignment);
                wait_for_ubos |= create_stage_ubo(shader_desc->get_tessellation_control_shader(), min_ubo_alignment);
                wait_for_ubos |= create_stage_ubo(shader_desc->get_tessellation_evaluation_shader(), min_ubo_alignment);
                wait_for_ubos |= create_stage_ubo(shader_desc->get_geometry_shader(), min_ubo_alignment);
                wait_for_ubos |= create_stage_ubo(shader_desc->get_fragment_shader(), min_ubo_alignment);
                wait_for_ubos |= create_stage_ubo(shader_desc->get_compute_shader(), min_ubo_alignment);

                if (wait_for_ubos) {
                    m_sync_operation = graphics::get_graphics()->get_render_policy()->get_system_sync();
                    m_initialization_step = initialization_step_wait_for_ubo;
                }
                else {
                    m_initialization_step = initialization_step_ready;
                }
                break;
            }

            case initialization_step_wait_for_ubo:
                if (m_sync_operation->is_signaled()) {
                    m_initialization_step = initialization_step_ready;
                }
                break;
            }

            // No need to keep these references after initialization is done.
            if (m_initialization_step == initialization_step_ready) {
                m_sync_operation.reset();
                m_desc.reset();
                m_vertex_attrib_field_map.reset();
            }
        }

        bool pipeline_composite::locate_field_source(graphics::shader_field const* field, void const** field_source) const
        {
            // Look up the initializer for this field.
            field_value_table::const_iterator f(m_field_values.find(field->get_hash()));
            if (f != m_field_values.end()) {
                // The safety of returning this pointer is tied to the fact that the caller is
                // assumed to hold a reference on the pipeline_composite.
                *field_source = f->second.get();
                return (true);
            }
            else {
                return (false);
            }
        }

        bool pipeline_composite::ready() const
        {
            return (m_initialization_step == initialization_step_ready);
        }

        bool pipeline_composite::is_transparent() const
        {
            return (m_blending.enable);
        }

        void pipeline_composite::bind_pipeline(
            graphics::context_interface* context,
            frame_details* this_frame_details,
            int dynamic_ubo_base_offset
            ) const
        {
            ELECTROSLAG_CHECK(graphics::get_graphics()->get_render_thread()->is_running());
            ELECTROSLAG_CHECK(ready());

            context->set_blending(&m_blending);
            context->set_depth_test(&m_depth_test);
            context->bind_shader_program(m_shader);

            ubo_binding_vector::const_iterator u(m_ubo_bindings.begin());
            while (u != m_ubo_bindings.end()) {
                context->bind_uniform_buffer(u->buffer, u->binding);
                ++u;
            }

            dynamic_ubo_binding_vector::const_iterator d(m_dynamic_ubo_bindings.begin());
            while (d != m_dynamic_ubo_bindings.end()) {
                context->bind_uniform_buffer_range(
                    this_frame_details->dynamic_ubo,
                    d->binding,
                    d->binding_offset + dynamic_ubo_base_offset
                    );
                ++d;
            }
        }

        bool pipeline_composite::create_stage_textures(
            graphics::shader_stage_descriptor::ref const& stage
            )
        {
            bool need_wait = false;
            if (!stage.is_valid()) {
                return (need_wait);
            }

            texture_manager* texture_manager = get_renderer_internal()->get_texture_manager();

            graphics::shader_stage_descriptor::const_uniform_buffer_iterator u(stage->begin_uniform_buffers());
            while (u != stage->end_uniform_buffers()) {
                // Each UBO may have an initializer; that's where textures will be referenced.
                serialize::serializable_map::ref field_initializer(m_desc->get_ubo_initializer(*u));
                if (field_initializer.is_valid()) {
                    // Find texture handle fields in the UBO
                    graphics::shader_field_map::ref const& ubo_field_map((*u)->get_fields());
                    graphics::shader_field_map::const_iterator f(ubo_field_map->begin());
                    while (f != ubo_field_map->end()) {
                        graphics::shader_field const* field = f->second;

                        if (field->get_field_type() == graphics::field_type_texture_handle) {

                            // Look up the initializer for this field.
                            unsigned long long field_obj_hash = 0;
                            if (field_initializer->locate_value(field->get_hash(), &field_obj_hash)) {

                                // Prime the texture manager by getting this texture async; we can then wait for
                                // all texture creations together.
                                graphics::texture_descriptor::ref texture_desc(
                                    serialize::get_database()->find_object_ref<graphics::texture_descriptor>(field_obj_hash)
                                    );
                                texture_manager->get_texture(texture_desc);
                                need_wait = true;
                            }
                        }
                        ++f;
                    }
                }

                ++u;
            }
            return (need_wait);
        }

        bool pipeline_composite::create_stage_ubo(
            graphics::shader_stage_descriptor::ref const& stage,
            int min_ubo_alignment
            )
        {
            bool need_wait = false;
            if (!stage.is_valid()) {
                return (need_wait);
            }

            int current_dynamic_ubo_offset = m_dynamic_ubo_size;
            graphics::shader_stage_descriptor::const_uniform_buffer_iterator u(stage->begin_uniform_buffers());
            while (u != stage->end_uniform_buffers()) {
                if (m_desc->is_ubo_static_data(*u)) {
                    need_wait |= create_static_ubo(*u);
                }
                else {
                    current_dynamic_ubo_offset = create_dynamic_ubo(*u, current_dynamic_ubo_offset, min_ubo_alignment);
                }
                ++u;
            }
            m_dynamic_ubo_size += current_dynamic_ubo_offset;
            return (need_wait);
        }

        int pipeline_composite::create_dynamic_ubo(
            graphics::uniform_buffer_descriptor::ref const& ubo_desc,
            int current_dynamic_ubo_offset,
            int min_ubo_alignment
            )
        {
            // Remember the binding information necessary for this dynamic UBO.
            m_dynamic_ubo_bindings.emplace_back(ubo_desc->get_binding(), current_dynamic_ubo_offset);

            // Advance the dynamic UBO offset.
            current_dynamic_ubo_offset += ubo_desc->get_size();
            current_dynamic_ubo_offset = next_multiple_of_2(current_dynamic_ubo_offset, min_ubo_alignment);

            // Extract the field initializers and remember them.
            serialize::serializable_map::ref field_initializer(m_desc->get_ubo_initializer(ubo_desc));

            if (field_initializer.is_valid()) {
                graphics::shader_field_map::ref const& ubo_field_map(ubo_desc->get_fields());
                graphics::shader_field_map::const_iterator f(ubo_field_map->begin());
                while (f != ubo_field_map->end()) {
                    graphics::shader_field const* field = f->second;

                    unsigned long long field_obj_hash = 0;
                    if (field_initializer->locate_value(field->get_hash(), &field_obj_hash)) {
                        // TODO: Why is this a copy instead of just a pointer? Worried about unload?
                        int field_size = graphics::field_type_util::get_bytes(field->get_field_type());
                        field_value_ptr field_copy(std::malloc(field_size), malloc_deleter);

                        memcpy(field_copy.get(), serialize::get_database()->find_object(field_obj_hash), field_size);

                        // Use std::move to move the unique_ptr into the map.
                        m_field_values.insert(std::make_pair(field->get_hash(), std::move(field_copy)));
                    }
                    ++f;
                }
            }
            
            return (current_dynamic_ubo_offset);
        }

        bool pipeline_composite::create_static_ubo(
            graphics::uniform_buffer_descriptor::ref const& ubo_desc
            )
        {
            bool need_wait = false;

            // Building an initializer for a static UBO is not free, so check if we have the
            // right buffer already by hash first.
            serialize::serializable_map::ref field_initializer(m_desc->get_ubo_initializer(ubo_desc));
            ELECTROSLAG_CHECK(field_initializer.is_valid());

            // TODO: XOR of hashes; This could be a bad idea. Can I check for collisions?
            unsigned long long ubo_hash = ubo_desc->get_hash() ^ field_initializer->get_hash();

            uniform_buffer_manager* ubo_manager = get_renderer_internal()->get_ubo_manager();

            graphics::buffer_interface::ref static_ubo(ubo_manager->get_buffer(ubo_hash));

            if (!static_ubo.is_valid()) {
                graphics::buffer_descriptor::ref initializer(graphics::buffer_descriptor::create());
                initializer->set_hash(ubo_hash);
                initializer->set_buffer_memory_caching(graphics::buffer_memory_caching_static);
                initializer->set_buffer_memory_map(graphics::buffer_memory_map_static);

                referenced_buffer_from_sizeof::ref initializer_data(referenced_buffer_from_sizeof::create(ubo_desc->get_size()));
                create_static_ubo_initializer(initializer_data, field_initializer, ubo_desc);
                initializer->set_initialized_data(initializer_data);

                static_ubo = ubo_manager->get_buffer(initializer);
                need_wait = true;
            }

            m_ubo_bindings.emplace_back(static_ubo, ubo_desc->get_binding());

            return (need_wait);
        }

        void pipeline_composite::create_static_ubo_initializer(
            referenced_buffer_from_sizeof::ref& initializer_data,
            serialize::serializable_map::ref const& field_initializer,
            graphics::uniform_buffer_descriptor::ref const& ubo_desc
            )
        {
            referenced_buffer_from_sizeof::accessor data_accessor(initializer_data);

            texture_manager* texture_manager = get_renderer_internal()->get_texture_manager();

            // The initializer must be updated with the texture handles before the UBO is created.
            graphics::shader_field_map::ref const& ubo_field_map(ubo_desc->get_fields());
            graphics::shader_field_map::const_iterator f(ubo_field_map->begin());
            while (f != ubo_field_map->end()) {
                graphics::shader_field const* field = f->second;

                switch (field->get_field_type()) {
                case graphics::field_type_vec2:
                    write_field<graphics::field_structs::vec2>(
                        field,
                        field_initializer,
                        data_accessor
                        );
                    break;

                case graphics::field_type_vec3:
                    write_field<graphics::field_structs::vec3>(
                        field,
                        field_initializer,
                        data_accessor
                        );
                    break;

                case graphics::field_type_vec4:
                    write_field<graphics::field_structs::vec4>(
                        field,
                        field_initializer,
                        data_accessor
                        );
                    break;

                case graphics::field_type_uvec2:
                    write_field<graphics::field_structs::uvec2>(
                        field,
                        field_initializer,
                        data_accessor
                        );
                    break;

                case graphics::field_type_mat4:
                    write_field<graphics::field_structs::mat4>(
                        field,
                        field_initializer,
                        data_accessor
                        );
                    break;

                case graphics::field_type_texture_handle: {
                    unsigned long long field_obj_hash = 0;
                    if (field_initializer->locate_value(field->get_hash(), &field_obj_hash)) {
                        graphics::texture_descriptor::ref connected_texture_desc(
                            serialize::get_database()->find_object_ref<graphics::texture_descriptor>(field_obj_hash)
                            );

                        // Assumes texture creation is done by now!
                        graphics::texture_interface::ref texture(texture_manager->get_texture(
                            connected_texture_desc
                            ));

                        graphics::field_structs::texture_handle handle(texture->get_handle());
                        field->write_uniform(
                            static_cast<byte*>(data_accessor.get_pointer()),
                            &handle
                            );
                    }
                    break;
                }
                }
                ++f;
            }
        }
    }
}
