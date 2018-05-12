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
#include "electroslag/graphics/serialized_graphics_types.hpp"

namespace electroslag {
    namespace graphics {
        serialized_depth_test_params::serialized_depth_test_params(serialize::archive_reader_interface* archive)
        {
            depth_test_mode test_mode = depth_test_mode_unknown;
            if (!archive->read_enumeration("test_mode", &test_mode, depth_test_mode_strings)) {
                throw load_object_failure("test_mode");
            }

            bool write_enable = false;
            if (!archive->read_boolean("write_enable", &write_enable)) {
                throw load_object_failure("write_enable");
            }

            bool test_enable = false;
            if (!archive->read_boolean("test_enable", &test_enable)) {
                throw load_object_failure("test_enable");
            }

            m_depth_test_params.test_mode = test_mode;
            m_depth_test_params.write_enable = write_enable;
            m_depth_test_params.test_enable = test_enable;
        }

        void serialized_depth_test_params::save_to_archive(serialize::archive_writer_interface* archive)
        {
            serializable_object::save_to_archive(archive);
            archive->write_int32("test_mode", m_depth_test_params.test_mode);
            archive->write_boolean("write_enable", m_depth_test_params.write_enable);
            archive->write_boolean("test_enable", m_depth_test_params.test_enable);
        }

        serialized_blending_params::serialized_blending_params(serialize::archive_reader_interface* archive)
        {
            bool enable = false;
            if (!archive->read_boolean("enable", &enable)) {
                throw load_object_failure("enable");
            }

            blending_mode color_mode = blending_mode_unknown;
            blending_operand color_op1 = blending_operand_unknown;
            blending_operand color_op2 = blending_operand_unknown;

            blending_mode alpha_mode = blending_mode_unknown;
            blending_operand alpha_op1 = blending_operand_unknown;
            blending_operand alpha_op2 = blending_operand_unknown;

            if (enable) {
                archive->read_enumeration("color_mode", &color_mode, blending_mode_strings);
                archive->read_enumeration("color_op1", &color_op1, blending_operand_strings);
                archive->read_enumeration("color_op2", &color_op2, blending_operand_strings);

                archive->read_enumeration("alpha_mode", &alpha_mode, blending_mode_strings);
                archive->read_enumeration("alpha_op1", &alpha_op1, blending_operand_strings);
                archive->read_enumeration("alpha_op2", &alpha_op2, blending_operand_strings);
            }

            m_blending_params.color_mode = color_mode;
            m_blending_params.color_op1 = color_op1;
            m_blending_params.color_op2 = color_op2;
            m_blending_params.alpha_mode = alpha_mode;
            m_blending_params.alpha_op1 = alpha_op1;
            m_blending_params.alpha_op2 = alpha_op2;
            m_blending_params.enable = enable;
        }

        void serialized_blending_params::save_to_archive(serialize::archive_writer_interface* archive)
        {
            serializable_object::save_to_archive(archive);
            archive->write_boolean("enable", m_blending_params.enable);

            if (m_blending_params.enable) {
                archive->write_int32("color_mode", m_blending_params.color_mode);
                archive->write_int32("color_op1", m_blending_params.color_op1);
                archive->write_int32("color_op2", m_blending_params.color_op2);
                archive->write_int32("alpha_mode", m_blending_params.alpha_mode);
                archive->write_int32("alpha_op1", m_blending_params.alpha_op1);
                archive->write_int32("alpha_op2", m_blending_params.alpha_op2);
            }
        }

        serialized_sampler_params::serialized_sampler_params(serialize::archive_reader_interface* archive)
        {
            texture_filter magnification_filter = texture_filter_default;
            archive->read_enumeration("magnification_filter", &magnification_filter, texture_filter_strings);
            m_sampler_params.magnification_filter = magnification_filter;

            texture_filter minification_filter = texture_filter_default;
            archive->read_enumeration("minification_filter", &minification_filter, texture_filter_strings);
            m_sampler_params.minification_filter = minification_filter;

            texture_filter mip_filter = texture_filter_default;
            archive->read_enumeration("mip_filter", &mip_filter, texture_filter_strings);
            m_sampler_params.mip_filter = mip_filter;

            texture_coord_wrap s_wrap_mode = texture_coord_wrap_default;
            archive->read_enumeration("s_wrap_mode", &s_wrap_mode, texture_coord_wrap_strings);
            m_sampler_params.s_wrap_mode = s_wrap_mode;

            texture_coord_wrap t_wrap_mode = texture_coord_wrap_default;
            archive->read_enumeration("t_wrap_mode", &t_wrap_mode, texture_coord_wrap_strings);
            m_sampler_params.t_wrap_mode = t_wrap_mode;

            texture_coord_wrap u_wrap_mode = texture_coord_wrap_default;
            archive->read_enumeration("u_wrap_mode", &u_wrap_mode, texture_coord_wrap_strings);
            m_sampler_params.u_wrap_mode = u_wrap_mode;
        }

        void serialized_sampler_params::save_to_archive(serialize::archive_writer_interface* archive)
        {
            serializable_object::save_to_archive(archive);
            archive->write_int32("magnification_filter", m_sampler_params.magnification_filter);
            archive->write_int32("minification_filter", m_sampler_params.minification_filter);
            archive->write_int32("mip_filter", m_sampler_params.mip_filter);

            archive->write_int32("s_wrap_mode", m_sampler_params.s_wrap_mode);
            archive->write_int32("t_wrap_mode", m_sampler_params.t_wrap_mode);
            archive->write_int32("u_wrap_mode", m_sampler_params.u_wrap_mode);
        }
    }
}
