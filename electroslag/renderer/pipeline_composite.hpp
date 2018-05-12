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

#pragma once
#include "electroslag/graphics/shader_field_map.hpp"
#include "electroslag/renderer/pipeline_interface.hpp"
#include "electroslag/renderer/pipeline_descriptor.hpp"

namespace electroslag {
    namespace renderer {
        class pipeline_composite : public pipeline_interface {
        public:
            typedef reference<pipeline_composite> ref;

            static ref create(
                pipeline_descriptor::ref const& desc,
                graphics::shader_field_map::ref const& vertex_attrib_field_map
                )
            {
                return (new pipeline_composite(desc, vertex_attrib_field_map));
            }

            // Implement field_source_interface
            virtual bool locate_field_source(graphics::shader_field const* field, void const** field_source) const;

            // Implement pipeline_interface
            virtual void initialize_step();

            virtual bool ready() const;

            virtual bool is_transparent() const;

            virtual int get_dynamic_ubo_size() const
            {
                return (m_dynamic_ubo_size);
            }

            virtual graphics::shader_program_interface::ref const& get_shader() const
            {
                return (m_shader);
            }

            virtual void bind_pipeline(
                graphics::context_interface* context,
                frame_details* this_frame_details,
                int dynamic_ubo_base_offset
                ) const;

        private:
            template<class T>
            void write_field(
                graphics::shader_field const* field,
                serialize::serializable_map::ref const& field_initializer,
                referenced_buffer_interface::accessor& data_accessor
                )
            {
                // This is the slow path for writing a UBO field from an initializer; to be used
                // only in static UBO initialization!
                unsigned long long field_obj_hash = 0;
                if (field_initializer->locate_value(field->get_hash(), &field_obj_hash)) {
                    T* field_obj = dynamic_cast<T*>(
                        serialize::get_database()->find_object(field_obj_hash)
                        );
                    field->write_uniform(
                        static_cast<byte*>(data_accessor.get_pointer()),
                        field_obj
                        );
                }
            }

            pipeline_composite(
                pipeline_descriptor::ref const& desc,
                graphics::shader_field_map::ref const& vertex_attrib_field_map
                );

            bool create_stage_textures(
                graphics::shader_stage_descriptor::ref const& stage
                );
            bool create_stage_ubo(
                graphics::shader_stage_descriptor::ref const& stage,
                int min_ubo_alignmen
                );
            int create_dynamic_ubo(
                graphics::uniform_buffer_descriptor::ref const& ubo_desc,
                int current_dynamic_ubo_offset,
                int min_ubo_alignment
                );
            bool create_static_ubo(
                graphics::uniform_buffer_descriptor::ref const& ubo_desc
                );
            void create_static_ubo_initializer(
                referenced_buffer_from_sizeof::ref& initializer_data,
                serialize::serializable_map::ref const& field_initializer,
                graphics::uniform_buffer_descriptor::ref const& ubo_desc
                );

            // Initialization data.
            enum initialization_step {
                initialization_step_unknown = -1,
                initialization_step_create_shaders = 0,
                initialization_step_wait_for_shaders = 1,
                initialization_step_create_textures = 2,
                initialization_step_wait_for_textures = 3,
                initialization_step_create_ubo = 4,
                initialization_step_wait_for_ubo = 5,
                initialization_step_ready = 6
            };
            initialization_step m_initialization_step;
            graphics::sync_interface::ref m_sync_operation;
            pipeline_descriptor::ref m_desc;
            graphics::shader_field_map::ref m_vertex_attrib_field_map;

            // Shader.
            graphics::shader_program_interface::ref m_shader;

            // All of the necessary static UBO bindings.
            struct ubo_binding {
                ubo_binding(
                    graphics::buffer_interface::ref& new_buffer,
                    int new_binding
                    )
                    : buffer(new_buffer)
                    , binding(new_binding)
                {}

                graphics::buffer_interface::ref buffer;
                int binding;
            };
            typedef std::vector<ubo_binding> ubo_binding_vector;
            ubo_binding_vector m_ubo_bindings;

            // Dynamic UBO are bound via a given offset into the current dynamic UBO.
            int m_dynamic_ubo_size;

            struct dynamic_ubo_binding {
                dynamic_ubo_binding(int new_binding, int new_offset)
                    : binding(new_binding)
                    , binding_offset(new_offset)
                {}

                int binding;
                int binding_offset;
            };
            typedef std::vector<dynamic_ubo_binding> dynamic_ubo_binding_vector;
            dynamic_ubo_binding_vector m_dynamic_ubo_bindings;

            // Remember the initializer for all dynamic ubo fields.
            typedef std::unique_ptr<void, void(*)(void*)> field_value_ptr;

            static void malloc_deleter(void* ptr)
            {
                std::free(ptr);
            }

            typedef std::unordered_map<
                unsigned long long,
                field_value_ptr,
                prehashed_key<unsigned long long>,
                std::equal_to<unsigned long long>
            > field_value_table;
            field_value_table m_field_values;

            // Rendering state.
            graphics::depth_test_params m_depth_test;
            graphics::blending_params m_blending;

            // Disallowed operations:
            pipeline_composite();
            explicit pipeline_composite(pipeline_composite const&);
            pipeline_composite& operator =(pipeline_composite const&);
        };
    }
}
