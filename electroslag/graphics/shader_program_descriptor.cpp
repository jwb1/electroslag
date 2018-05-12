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
#include "electroslag/graphics/shader_program_descriptor.hpp"

namespace electroslag {
    namespace graphics {
        shader_program_descriptor::shader_program_descriptor(serialize::archive_reader_interface* ar)
        {
            serialize::database* db = serialize::get_database();

            shader_stage_bits stage_bits = static_cast<shader_stage_bits>(0);
            if (!ar->read_enumeration_flags("stages", &stage_bits, shader_stage_strings, shader_stage_bits_values)) {
                throw load_object_failure("stages");
            }

            unsigned long long name_hash = 0;
            if (stage_bits & shader_stage_bits_vertex) {
                if (!ar->read_name_hash("v_stage", &name_hash)) {
                    throw load_object_failure("v_stage");
                }
                m_vertex = db->find_object_ref<shader_stage_descriptor>(name_hash);
            }

            if (stage_bits & shader_stage_bits_tessellation_control) {
                if (!ar->read_name_hash("tc_stage", &name_hash)) {
                    throw load_object_failure("tc_stage");
                }
                m_tessellation_control = db->find_object_ref<shader_stage_descriptor>(name_hash);
            }

            if (stage_bits & shader_stage_bits_tessellation_evaluation) {
                if (!ar->read_name_hash("te_stage", &name_hash)) {
                    throw load_object_failure("te_stage");
                }
                m_tessellation_evaluation = db->find_object_ref<shader_stage_descriptor>(name_hash);
            }

            if (stage_bits & shader_stage_bits_geometry) {
                if (!ar->read_name_hash("g_stage", &name_hash)) {
                    throw load_object_failure("g_stage");
                }
                m_geometry = db->find_object_ref<shader_stage_descriptor>(name_hash);
            }

            if (stage_bits & shader_stage_bits_fragment) {
                if (!ar->read_name_hash("f_stage", &name_hash)) {
                    throw load_object_failure("f_stage");
                }
                m_fragment = db->find_object_ref<shader_stage_descriptor>(name_hash);
            }

            if (stage_bits & shader_stage_bits_compute) {
                if (!ar->read_name_hash("c_stage", &name_hash)) {
                    throw load_object_failure("c_stage");
                }
                m_compute = db->find_object_ref<shader_stage_descriptor>(name_hash);
            }

            unsigned long long field_map_hash = 0;
            if (!ar->read_name_hash("vertex_field_map", &field_map_hash)) {
                throw load_object_failure("vertex_field_map");
            }

            m_fields = db->find_object_ref<shader_field_map>(field_map_hash);

            unsigned long long shader_define_hash = 0;
            ar->read_name_hash("shader_defines", &shader_define_hash);
            if (shader_define_hash != 0) {
                m_shader_defines = db->find_object_ref<serialize::serializable_buffer>(shader_define_hash);
            }
        }

        shader_program_descriptor::shader_program_descriptor(shader_program_descriptor const& copy_object)
            : referenced_object()
            , serializable_object(copy_object)
            , m_fields(copy_object.m_fields)
        {
            if (copy_object.get_vertex_shader().is_valid()) {
                m_vertex = shader_stage_descriptor::clone(copy_object.get_vertex_shader());
            }

            if (copy_object.get_tessellation_control_shader().is_valid()) {
                m_tessellation_control = shader_stage_descriptor::clone(copy_object.get_tessellation_control_shader());
            }

            if (copy_object.get_tessellation_evaluation_shader().is_valid()) {
                m_tessellation_evaluation = shader_stage_descriptor::clone(copy_object.get_tessellation_evaluation_shader());
            }

            if (copy_object.get_geometry_shader().is_valid()) {
                m_geometry = shader_stage_descriptor::clone(copy_object.get_geometry_shader());
            }

            if (copy_object.get_fragment_shader().is_valid()) {
                m_fragment = shader_stage_descriptor::clone(copy_object.get_fragment_shader());
            }

            if (copy_object.get_compute_shader().is_valid()) {
                m_compute = shader_stage_descriptor::clone(copy_object.get_compute_shader());
            }

            if (copy_object.get_shader_defines().is_valid()) {
                m_shader_defines = serialize::serializable_buffer::clone(copy_object.get_shader_defines());
            }
        }

        void shader_program_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            // Write the stage objects
            uint32_t stage_bits = 0;
            if (m_vertex.is_valid()) {
                m_vertex->save_to_archive(ar);
                stage_bits |= shader_stage_bits_vertex;
            }

            if (m_tessellation_control.is_valid()) {
                m_tessellation_control->save_to_archive(ar);
                stage_bits |= shader_stage_bits_tessellation_control;
            }

            if (m_tessellation_evaluation.is_valid()) {
                m_tessellation_evaluation->save_to_archive(ar);
                stage_bits |= shader_stage_bits_tessellation_evaluation;
            }

            if (m_geometry.is_valid()) {
                m_geometry->save_to_archive(ar);
                stage_bits |= shader_stage_bits_geometry;
            }

            if (m_fragment.is_valid()) {
                m_fragment->save_to_archive(ar);
                stage_bits |= shader_stage_bits_fragment;
            }

            if (m_compute.is_valid()) {
                m_compute->save_to_archive(ar);
                stage_bits |= shader_stage_bits_compute;
            }

            // Write in the shader_field_map for the vertex attributes referenced by the vertex shader.
            m_fields->save_to_archive(ar);

            // Save the preprocessor macros, if any.
            if (m_shader_defines.is_valid()) {
                m_shader_defines->save_to_archive(ar);
            }

            // Next write the shader_program itself.
            serializable_object::save_to_archive(ar);

            ar->write_uint32("stages", stage_bits);

            // Reference the shader stages.
            if (m_vertex.is_valid()) {
                ar->write_name_hash("v_stage", m_vertex->get_hash());
            }

            if (m_tessellation_control.is_valid()) {
                ar->write_name_hash("tc_stage", m_tessellation_control->get_hash());
            }

            if (m_tessellation_evaluation.is_valid()) {
                ar->write_name_hash("te_stage", m_tessellation_evaluation->get_hash());
            }

            if (m_geometry.is_valid()) {
                ar->write_name_hash("g_stage", m_geometry->get_hash());
            }

            if (m_fragment.is_valid()) {
                ar->write_name_hash("f_stage", m_fragment->get_hash());
            }

            if (m_compute.is_valid()) {
                ar->write_name_hash("c_stage", m_compute->get_hash());
            }

            // Reference the vertex attribute array.
            ar->write_name_hash("vertex_field_map", m_fields->get_hash());

            // Reference the shader pre-processor macros, if any.
            if (m_shader_defines.is_valid()) {
                ar->write_name_hash("shader_defines", m_shader_defines->get_hash());
            }
            else {
                ar->write_name_hash("shader_defines", 0);
            }
        }
    }
}
