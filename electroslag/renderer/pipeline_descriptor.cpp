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
#include "electroslag/renderer/renderer_types.hpp"
#include "electroslag/renderer/pipeline_descriptor.hpp"
#include "electroslag/graphics/serialized_graphics_types.hpp"

namespace electroslag {
    namespace renderer {
        pipeline_descriptor::pipeline_descriptor(serialize::archive_reader_interface* ar)
        {
            if (!ar->read_enumeration("pipeline_type", &m_pipeline_type, pipeline_type_strings)) {
                throw load_object_failure("pipeline_type");
            }

            unsigned long long shader_name_hash = 0;
            if (!ar->read_name_hash("shader", &shader_name_hash)) {
                throw load_object_failure("shader");
            }

            m_shader = serialize::get_database()->find_object_ref<graphics::shader_program_descriptor>(
                shader_name_hash
                );

            int32_t ubo_value_count = 0;
            if (!ar->read_int32("ubo_count", &ubo_value_count) || ubo_value_count < 0) {
                throw load_object_failure("ubo_count");
            }

            if (ubo_value_count > 0) {
                m_ubo_value_vector.reserve(ubo_value_count);

                serialize::enumeration_namer ubo_value_namer(ubo_value_count, 'u');
                std::string ubo_value_name;
                std::string tc_name;
                while (!ubo_value_namer.used_all_names()) {
                    ubo_value_name = ubo_value_namer.get_next_name();

                    unsigned long long ubo_name_hash = 0;
                    ar->read_name_hash(ubo_value_name + "_ubo", &ubo_name_hash);

                    ubo_value_vector::iterator ubo_value(m_ubo_value_vector.emplace(
                        m_ubo_value_vector.end(),
                        serialize::get_database()->find_object_ref<graphics::uniform_buffer_descriptor>(ubo_name_hash)
                        ));

                    unsigned long long initializer_name_hash = 0;
                    ar->read_name_hash(ubo_value_name + "_init", &initializer_name_hash);

                    if (initializer_name_hash) {
                        ubo_value->initializer = serialize::get_database()->find_object_ref<serialize::serializable_map>(initializer_name_hash);
                    }

                    ar->read_boolean(ubo_value_name + "_static", &(ubo_value->static_data));
                }
            }

            if (!ar->read_uint32("depth_test_value", &(m_depth_test.value))) {
#if !defined(ELECTROSLAG_BUILD_SHIP)
                unsigned long long depth_hash = 0;
                ar->read_name_hash("depth_test", &depth_hash);

                m_depth_test = dynamic_cast<graphics::serialized_depth_test_params*>(
                    serialize::get_database()->find_object(depth_hash)
                    )->get();
#else
                throw load_object_failure("depth_test_value");
#endif
            }
            
            if (!ar->read_uint32("blending_value", &(m_blending.value))) {
#if !defined(ELECTROSLAG_BUILD_SHIP)
                unsigned long long blending_hash = 0;
                ar->read_name_hash("blending", &blending_hash);

                m_blending = dynamic_cast<graphics::serialized_blending_params*>(
                    serialize::get_database()->find_object(blending_hash)
                    )->get();
#else
                throw load_object_failure("blending_value");
#endif
            }
        }

        void pipeline_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            m_shader->save_to_archive(ar);

            ubo_value_vector::iterator u(m_ubo_value_vector.begin());
            while (u != m_ubo_value_vector.end()) {
                if (u->initializer.is_valid()) {
                    u->initializer->save_to_archive(ar);
                }
                ++u;
            }

            serializable_object::save_to_archive(ar);

            ar->write_int32("pipeline_type", m_pipeline_type);
            ar->write_name_hash("shader", m_shader->get_hash());

            int32_t ubo_value_count = static_cast<int32_t>(m_ubo_value_vector.size());
            ar->write_int32("ubo_count", ubo_value_count);

            serialize::enumeration_namer ubo_value_namer(ubo_value_count, 'u');
            std::string ubo_value_name;
            u = m_ubo_value_vector.begin();
            while (u != m_ubo_value_vector.end()) {
                ubo_value_name = ubo_value_namer.get_next_name();

                ar->write_name_hash(ubo_value_name + "_ubo", u->ubo->get_hash());
                if (u->initializer.is_valid()) {
                    ar->write_name_hash(ubo_value_name + "_init", u->initializer->get_hash());
                }
                else {
                    ar->write_name_hash(ubo_value_name + "_init", 0);
                }

                ar->write_boolean(ubo_value_name + "_static", u->static_data);
                ++u;
            }

            ar->write_uint32("depth_test_value", m_depth_test.value);
            ar->write_uint32("blending_value", m_blending.value);
        }

        void pipeline_descriptor::set_shader(graphics::shader_program_descriptor::ref const& s)
        {
            m_shader = s;

            m_ubo_value_vector.clear();

            gather_ubo(m_shader->get_vertex_shader());
            gather_ubo(m_shader->get_tessellation_control_shader());
            gather_ubo(m_shader->get_tessellation_evaluation_shader());
            gather_ubo(m_shader->get_geometry_shader());
            gather_ubo(m_shader->get_fragment_shader());
            gather_ubo(m_shader->get_compute_shader());
        }

        serialize::serializable_map::ref const& pipeline_descriptor::get_ubo_initializer(
            graphics::uniform_buffer_descriptor::ref const& ubo
            ) const
        {
            ubo_value_vector::const_iterator u(m_ubo_value_vector.begin());
            unsigned long long ubo_hash = ubo->get_hash();
            while (u != m_ubo_value_vector.end()) {
                if (u->ubo->get_hash() == ubo_hash) {
                    return (u->initializer);
                }
                ++u;
            }

            throw std::logic_error("ubo not found");
        }

        serialize::serializable_map::ref pipeline_descriptor::get_ubo_initializer(
            graphics::uniform_buffer_descriptor::ref const& ubo
            )
        {
            ubo_value_vector::const_iterator u(m_ubo_value_vector.begin());
            unsigned long long ubo_hash = ubo->get_hash();
            while (u != m_ubo_value_vector.end()) {
                if (u->ubo->get_hash() == ubo_hash) {
                    return (u->initializer);
                }
                ++u;
            }

            throw std::logic_error("ubo not found");
        }

        void pipeline_descriptor::set_ubo_initializer(
            graphics::uniform_buffer_descriptor::ref const& ubo,
            serialize::serializable_map::ref const& initializer
            )
        {
            ubo_value_vector::iterator u(m_ubo_value_vector.begin());
            unsigned long long ubo_hash = ubo->get_hash();
            while (u != m_ubo_value_vector.end()) {
                if (u->ubo->get_hash() == ubo_hash) {
                    u->initializer = initializer;
                    break;
                }
                ++u;
            }
        }

        bool pipeline_descriptor::is_ubo_static_data(graphics::uniform_buffer_descriptor::ref const& ubo) const
        {
            ubo_value_vector::const_iterator u(m_ubo_value_vector.begin());
            unsigned long long ubo_hash = ubo->get_hash();
            while (u != m_ubo_value_vector.end()) {
                if (u->ubo->get_hash() == ubo_hash) {
                    return (u->static_data);
                }
                ++u;
            }

            throw std::logic_error("ubo not found");
        }

        void pipeline_descriptor::set_ubo_static_data(graphics::uniform_buffer_descriptor::ref const& ubo, bool static_data)
        {
            ubo_value_vector::iterator u(m_ubo_value_vector.begin());
            unsigned long long ubo_hash = ubo->get_hash();
            while (u != m_ubo_value_vector.end()) {
                if (u->ubo->get_hash() == ubo_hash) {
                    u->static_data = static_data;
                    break;
                }
                ++u;
            }
        }

        void pipeline_descriptor::gather_ubo(graphics::shader_stage_descriptor::ref& stage)
        {
            if (!stage.is_valid()) {
                return;
            }

            graphics::shader_stage_descriptor::uniform_buffer_iterator u(stage->begin_uniform_buffers());
            while (u != stage->end_uniform_buffers()) {
                m_ubo_value_vector.emplace_back(*u);
                ++u;
            }
        }
    }
}
