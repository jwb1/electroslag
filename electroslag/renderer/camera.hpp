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
#include "electroslag/named_object.hpp"
#include "electroslag/math/plane.hpp"
#include "electroslag/ui/ui_interface.hpp"
#include "electroslag/graphics/frame_buffer_interface.hpp"
#include "electroslag/graphics/context_interface.hpp"
#include "electroslag/animation/property.hpp"
#include "electroslag/animation/animated_object.hpp"
#include "electroslag/renderer/renderer_types.hpp"
#include "electroslag/renderer/mesh_interface.hpp"
#include "electroslag/renderer/renderable_descriptor.hpp"

namespace electroslag {
    namespace renderer {
        class camera
            : public animation::animated_object
            , public named_object {
        public:
            typedef reference<camera> ref;

            static bool supported_component_bits(int component_bits);

            static camera::ref create(
                renderable_descriptor::ref const& desc,
                transform_descriptor::ref const& transform,
                unsigned long long name_hash
                )
            {
                return (ref(new camera(desc, transform, name_hash)));
            }

            virtual ~camera();

            // Implement animated_object
            virtual void set_controller(unsigned long long name_hash, animation::property_controller_interface::ref& controller);
            virtual void clear_controller(unsigned long long name_hash);

            // Camera specific features
            glm::f32mat4x4 const& get_world_to_clip() const
            {
                return (m_world_to_clip);
            }

            bool view_frustum_cull(mesh_interface::ref& mesh, float* out_camera_distance) const;

            class camera_transform_work_item : public frame_work_item {
            public:
                typedef reference<camera_transform_work_item> ref;

                camera_transform_work_item(
                    camera::ref& this_camera,
                    frame_details* this_frame_details
                    )
                    : m_this_camera(this_camera)
                    , m_this_frame_details(this_frame_details)
                {}

                virtual void execute()
                {
                    m_this_camera->transform_camera(m_this_frame_details);
                }

            protected:
                camera::ref m_this_camera;
                frame_details* m_this_frame_details;

            private:
                // Disallowed operations:
                camera_transform_work_item();
                explicit camera_transform_work_item(camera_transform_work_item const&);
                camera_transform_work_item& operator =(camera_transform_work_item const&);
            };

            camera_transform_work_item::ref make_transform_work_item(
                frame_details* this_frame_details
                );

            class setup_camera_command : public graphics::command {
            public:
                setup_camera_command(
                    camera::ref& this_camera,
                    frame_details* this_frame_details
                    )
                    : m_this_camera(this_camera)
                    , m_this_frame_details(this_frame_details)
                {}

                virtual void execute(graphics::context_interface* context)
                {
                    m_this_camera->setup_camera(context, m_this_frame_details);
                }

            private:
                camera::ref m_this_camera;
                frame_details* m_this_frame_details;
            };

            void setup_camera(
                graphics::context_interface* context,
                frame_details* this_frame_details
                );

        protected:
            camera(
                renderable_descriptor::ref const& desc,
                transform_descriptor::ref const& transform,
                unsigned long long name_hash
                );

        private:
            void transform_camera(frame_details* this_frame_details);

            void compute_eye_to_clip(float target_width, float target_height);
            void on_window_resize(ui::window_dimensions const* dimensions);

            // Cached from the descriptor used to create this camera.
            camera_mode m_camera_mode;
            float m_near_distance;
            float m_far_distance;
            float m_field_of_view;

            // Position the camera in world space; used to create the view transform matrix.
            animation::quat_property<hash_string("rotation")> m_rotation;
            animation::vec3_property<hash_string("translate")> m_translate;

            glm::f32mat4x4 m_world_to_eye;

            // The projection matrix is determined when the camera is created, and just
            // concatenated with the view matrix.
            glm::f32mat4x4 m_eye_to_clip;
            
            // This is a cache of m_eye_to_clip * m_world_to_eye
            glm::f32mat4x4 m_world_to_clip;

            // The frame buffer the camera is rendering to.
            ui::window_interface::size_changed_delegate* m_size_change_delegate;
            graphics::frame_buffer_interface::ref m_frame_buffer;

            // These planes represent the view frustum in eye space.
            enum view_frustum_plane_index {
                view_frustum_plane_index_invalid = -1,
                view_frustum_plane_index_left = 0,
                view_frustum_plane_index_right = 1,
                view_frustum_plane_index_top = 2,
                view_frustum_plane_index_bottom = 3,
                view_frustum_plane_index_far = 4,
                view_frustum_plane_index_near = 5,

                view_frustum_plane_index_count
            };

            math::plane m_eye_frustum_planes[view_frustum_plane_index_count];
            math::plane m_world_frustum_planes[view_frustum_plane_index_count];

            // Dirty bits.
            bool m_world_to_eye_dirty;
            bool m_fbo_tracks_window_dimensions;

            // Disallowed operations:
            camera();
            explicit camera(camera const&);
            camera& operator =(camera const&);
        };
    }
}
