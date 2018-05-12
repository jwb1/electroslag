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
#include "electroslag/graphics/shader_stage_descriptor.hpp"
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/serializable_buffer.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace graphics {
        class shader_program_descriptor
            : public referenced_object
            , public serialize::serializable_object<shader_program_descriptor> {
        public:
            typedef reference<shader_program_descriptor> ref;

            static ref create()
            {
                return (ref(new shader_program_descriptor()));
            }

            static ref clone(ref const& clone_ref)
            {
                return (ref(new shader_program_descriptor(*clone_ref.get_pointer())));
            }

            // Implement serializable_object
            explicit shader_program_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            shader_stage_descriptor::ref const& get_vertex_shader() const
            {
                return (m_vertex);
            }

            shader_stage_descriptor::ref& get_vertex_shader()
            {
                return (m_vertex);
            }

            void set_vertex_shader(shader_stage_descriptor::ref const& vertex)
            {
                ELECTROSLAG_CHECK(!vertex.is_valid() || vertex->get_stage_flag() == shader_stage_vertex);
                m_vertex = vertex;
            }

            shader_stage_descriptor::ref const& get_tessellation_control_shader() const
            {
                return (m_tessellation_control);
            }

            shader_stage_descriptor::ref& get_tessellation_control_shader()
            {
                return (m_tessellation_control);
            }

            void set_tessellation_control_shader(shader_stage_descriptor::ref const& tessellation_control)
            {
                ELECTROSLAG_CHECK(!tessellation_control.is_valid() || tessellation_control->get_stage_flag() == shader_stage_tessellation_control);
                m_tessellation_control = tessellation_control;
            }

            shader_stage_descriptor::ref const& get_tessellation_evaluation_shader() const
            {
                return (m_tessellation_evaluation);
            }

            shader_stage_descriptor::ref& get_tessellation_evaluation_shader()
            {
                return (m_tessellation_evaluation);
            }

            void set_tessellation_evaluation_shader(shader_stage_descriptor::ref const& tessellation_evaluation)
            {
                ELECTROSLAG_CHECK(!tessellation_evaluation.is_valid() || tessellation_evaluation->get_stage_flag() == shader_stage_tessellation_evaluation);
                m_tessellation_evaluation = tessellation_evaluation;
            }

            shader_stage_descriptor::ref const& get_geometry_shader() const
            {
                return (m_geometry);
            }

            shader_stage_descriptor::ref& get_geometry_shader()
            {
                return (m_geometry);
            }

            void set_geometry_shader(shader_stage_descriptor::ref const& geometry)
            {
                ELECTROSLAG_CHECK(!geometry.is_valid() || geometry->get_stage_flag() == shader_stage_geometry);
                m_geometry = geometry;
            }

            shader_stage_descriptor::ref const& get_fragment_shader() const
            {
                return (m_fragment);
            }

            shader_stage_descriptor::ref& get_fragment_shader()
            {
                return (m_fragment);
            }

            void set_fragment_shader(shader_stage_descriptor::ref const& fragment)
            {
                ELECTROSLAG_CHECK(!fragment.is_valid() || fragment->get_stage_flag() == shader_stage_fragment);
                m_fragment = fragment;
            }

            shader_stage_descriptor::ref const& get_compute_shader() const
            {
                return (m_compute);
            }

            shader_stage_descriptor::ref& get_compute_shader()
            {
                return (m_compute);
            }

            void set_compute_shader(shader_stage_descriptor::ref const& compute)
            {
                ELECTROSLAG_CHECK(!compute.is_valid() || compute->get_stage_flag() == shader_stage_compute);
                m_compute = compute;
            }

            shader_field_map::ref const& get_vertex_fields() const
            {
                return (m_fields);
            }

            shader_field_map::ref& get_vertex_fields()
            {
                return (m_fields);
            }

            void set_fields(shader_field_map::ref const& fields)
            {
                m_fields = fields;
            }

            serialize::serializable_buffer::ref const& get_shader_defines() const
            {
                return (m_shader_defines);
            }

            serialize::serializable_buffer::ref& get_shader_defines()
            {
                return (m_shader_defines);
            }

            void set_shader_defines(serialize::serializable_buffer::ref const& defines)
            {
                m_shader_defines = defines;
            }

        private:
            shader_program_descriptor()
            {}

            explicit shader_program_descriptor(shader_program_descriptor const& copy_object);

            shader_stage_descriptor::ref m_vertex;
            shader_stage_descriptor::ref m_tessellation_control;
            shader_stage_descriptor::ref m_tessellation_evaluation;
            shader_stage_descriptor::ref m_geometry;
            shader_stage_descriptor::ref m_fragment;
            shader_stage_descriptor::ref m_compute;

            shader_field_map::ref m_fields;

            serialize::serializable_buffer::ref m_shader_defines;

            // Disallowed operations:
            shader_program_descriptor& operator =(shader_program_descriptor const&);
        };
    }
}
