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
#include "electroslag/graphics/graphics_types.hpp"
#include "electroslag/renderer/renderer_types.hpp"

namespace electroslag {
    namespace renderer {
        class camera_descriptor
            : public referenced_object
            , public serialize::serializable_object<camera_descriptor> {
        public:
            typedef reference<camera_descriptor> ref;

            static ref create()
            {
                return (ref(new camera_descriptor()));
            }

            // Implement serializable_object
            explicit camera_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            camera_mode get_camera_mode() const
            {
                return (m_camera_mode);
            }

            void set_camera_mode(camera_mode mode)
            {
                m_camera_mode = mode;
            }

            float get_near_distance() const
            {
                return (m_near_distance);
            }

            void set_near_distance(float near_distance)
            {
                m_near_distance = near_distance;
            }

            float get_far_distance() const
            {
                return (m_far_distance);
            }

            void set_far_distance(float far_distance)
            {
                m_far_distance = far_distance;
            }

            float get_field_of_view() const
            {
                return (m_field_of_view);
            }

            void set_field_of_view(float field_of_view)
            {
                m_field_of_view = field_of_view;
            }

            camera_render_target get_camera_render_target() const
            {
                return (m_target);
            }

            void set_camera_render_target(camera_render_target target)
            {
                m_target = target;
            }

            graphics::frame_buffer_attribs const* get_fbo_attribs() const
            {
                return (&m_fbo_attribs);
            }

            void set_fbo_attribs(graphics::frame_buffer_attribs const* fbo_attribs)
            {
                m_fbo_attribs = *fbo_attribs;
            }

            int get_fbo_width() const
            {
                return (m_fbo_width);
            }

            void set_fbo_width(int fbo_width)
            {
                m_fbo_width = fbo_width;
            }

            int get_fbo_height() const
            {
                return (m_fbo_height);
            }

            void set_fbo_height(int fbo_height)
            {
                m_fbo_height = fbo_height;
            }

        private:
            camera_descriptor()
                : m_camera_mode(camera_mode_unknown)
                , m_near_distance(0.0f) // TODO: Sensible defaults for camera parameters.
                , m_far_distance(0.0f)
                , m_field_of_view(0.0f)
                , m_target(camera_render_target_unknown)
                , m_fbo_width(-1)
                , m_fbo_height(-1)
            {}

            camera_mode m_camera_mode;
            float m_near_distance;
            float m_far_distance;
            float m_field_of_view; // radians
            camera_render_target m_target;
            graphics::frame_buffer_attribs m_fbo_attribs;
            int m_fbo_width;
            int m_fbo_height;

            // Disallowed operations:
            explicit camera_descriptor(camera_descriptor const&);
            camera_descriptor& operator =(camera_descriptor const&);
        };
    }
}
