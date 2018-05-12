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
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/serialize/serializable_map.hpp"
#include "electroslag/graphics/shader_field.hpp"
#include "electroslag/graphics/shader_program_descriptor.hpp"
#include "electroslag/graphics/uniform_buffer_descriptor.hpp"
#include "electroslag/graphics/texture_descriptor.hpp"
#include "electroslag/renderer/renderer_types.hpp"

namespace electroslag {
    namespace renderer {
        class pipeline_descriptor
            : public referenced_object
            , public serialize::serializable_object<pipeline_descriptor> {
        public:
            typedef reference<pipeline_descriptor> ref;

            static ref create()
            {
                return (ref(new pipeline_descriptor()));
            }

            virtual ~pipeline_descriptor()
            {}

            // Implement serializable_object
            explicit pipeline_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            pipeline_type get_pipeline_type() const
            {
                return (m_pipeline_type);
            }

            void set_pipeline_type(pipeline_type type)
            {
                ELECTROSLAG_CHECK(type > pipeline_type_unknown && type < pipeline_type_count);
                m_pipeline_type = type;
            }

            graphics::shader_program_descriptor::ref const& get_shader() const
            {
                return (m_shader);
            }

            graphics::shader_program_descriptor::ref get_shader()
            {
                return (m_shader);
            }

            void set_shader(graphics::shader_program_descriptor::ref const& s);

            serialize::serializable_map::ref const& get_ubo_initializer(graphics::uniform_buffer_descriptor::ref const& ubo) const;
            serialize::serializable_map::ref get_ubo_initializer(graphics::uniform_buffer_descriptor::ref const& ubo);

            void set_ubo_initializer(
                graphics::uniform_buffer_descriptor::ref const& ubo,
                serialize::serializable_map::ref const& initalizer
                );

            bool is_ubo_static_data(graphics::uniform_buffer_descriptor::ref const& ubo) const;

            void set_ubo_static_data(
                graphics::uniform_buffer_descriptor::ref const& ubo,
                bool static_data
                );

            graphics::depth_test_params const* get_depth_test_params() const
            {
                return (&m_depth_test);
            }

            void set_depth_test_params(graphics::depth_test_params const* depth_test)
            {
                m_depth_test.value = depth_test->value;
            }

            bool is_semi_transparent() const
            {
                return (m_blending.enable);
            }

            graphics::blending_params const* get_blending_params() const
            {
                return (&m_blending);
            }

            void set_blending_params(graphics::blending_params const* blending)
            {
                m_blending.value = blending->value;
            }

        private:
            pipeline_descriptor()
                : m_pipeline_type(pipeline_type_unknown)
            {}

            void gather_ubo(graphics::shader_stage_descriptor::ref& stage);

            pipeline_type m_pipeline_type;
            graphics::shader_program_descriptor::ref m_shader;

            struct uniform_block_value {
                explicit uniform_block_value(
                    graphics::uniform_buffer_descriptor::ref const& new_ubo
                    )
                    : ubo(new_ubo)
                    , static_data(false)
                {}

                graphics::uniform_buffer_descriptor::ref ubo;
                serialize::serializable_map::ref initializer;
                bool static_data;
            };
            typedef std::vector<uniform_block_value> ubo_value_vector;
            ubo_value_vector m_ubo_value_vector;

            graphics::depth_test_params m_depth_test;
            graphics::blending_params m_blending;

            // Disallowed operations:
            explicit pipeline_descriptor(pipeline_descriptor const&);
            pipeline_descriptor& operator =(pipeline_descriptor const&);
        };
    }
}
