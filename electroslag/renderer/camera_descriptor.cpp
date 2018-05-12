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
#include "electroslag/renderer/camera_descriptor.hpp"

namespace electroslag {
    namespace renderer {
        camera_descriptor::camera_descriptor(serialize::archive_reader_interface* ar)
        {
            if (!ar->read_enumeration("camera_mode", &m_camera_mode, camera_mode_strings)) {
                throw load_object_failure("camera_mode");
            }
            if (!ar->read_float("near", &m_near_distance)) {
                throw load_object_failure("near");
            }
            if (!ar->read_float("far", &m_far_distance)) {
                throw load_object_failure("far");
            }

            if (m_camera_mode == camera_mode_perspective) {
                // If the fov is not specified in radians, try degrees.
                if (!ar->read_float("field_of_view", &m_field_of_view)) {
                    float field_of_view_deg = 0.0f;
                    if (ar->read_float("field_of_view_deg", &field_of_view_deg)) {
                        m_field_of_view = glm::radians(field_of_view_deg);
                    }
                    else {
                        throw load_object_failure("field_of_view");
                    }
                }
            }

            if (!ar->read_enumeration("camera_render_target", &m_target, camera_render_target_strings)) {
                throw load_object_failure("camera_render_target");
            }

            if (m_target == camera_render_target_fbo) {
                if (!ar->read_enumeration("fbo_color_format", &m_fbo_attribs.color_format, graphics::frame_buffer_color_format_strings)) {
                    throw load_object_failure("fbo_color_format");
                }

                ar->read_enumeration("fbo_msaa", &m_fbo_attribs.msaa, graphics::frame_buffer_msaa_strings);

                if (!ar->read_enumeration("fbo_depth_format", &m_fbo_attribs.depth_stencil_format, graphics::frame_buffer_depth_stencil_format_strings)) {
                    throw load_object_failure("fbo_color_format");
                }

                ar->read_boolean("fbo_depth_shadow_hint", &m_fbo_attribs.depth_stencil_shadow_hint);

                if (!ar->read_int32("fbo_width", &m_fbo_width)) {
                    throw load_object_failure("fbo_width");
                }
                if (!ar->read_int32("fbo_height", &m_fbo_height)) {
                    throw load_object_failure("fbo_height");
                }
            }
        }

        void camera_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            serializable_object::save_to_archive(ar);

            ar->write_int32("camera_mode", m_camera_mode);
            ar->write_float("near", m_near_distance);
            ar->write_float("far", m_far_distance);

            if (m_camera_mode == camera_mode_perspective) {
                ar->write_float("field_of_view", m_field_of_view);
            }

            ar->write_int32("camera_render_target", m_target);

            if (m_target == camera_render_target_fbo) {
                ar->write_int32("fbo_color_format", m_fbo_attribs.color_format);
                ar->write_int32("fbo_msaa", m_fbo_attribs.msaa);
                ar->write_int32("fbo_depth_format", m_fbo_attribs.depth_stencil_format);
                ar->write_boolean("fbo_depth_shadow_hint", m_fbo_attribs.depth_stencil_shadow_hint);
                ar->write_int32("fbo_width", m_fbo_width);
                ar->write_int32("fbo_height", m_fbo_height);
            }
        }
    }
}
