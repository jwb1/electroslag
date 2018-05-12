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
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/renderer/renderer_types.hpp"
#include "electroslag/renderer/camera.hpp"
#include "electroslag/renderer/renderer.hpp"
#include "electroslag/threading/thread_pool.hpp"

namespace electroslag {
    namespace renderer {
        // static
        bool camera::supported_component_bits(int component_bits)
        {
            // Must have the camera specific bits; a default transform can be provided.
            switch (component_bits) {
            case renderable_descriptor_component_bits_camera:
            case renderable_descriptor_component_bits_camera | renderable_descriptor_component_bits_transform:
                return (true);

            default:
                return (false);
            }
        }

        camera::camera(
            renderable_descriptor::ref const& desc,
            transform_descriptor::ref const& transform,
            unsigned long long name_hash
            )
            : named_object(name_hash)
            , m_camera_mode(camera_mode_unknown)
            , m_near_distance(0.0f) // TODO: Sensible defaults for camera parameters.
            , m_far_distance(0.0f)
            , m_field_of_view(0.0f)
            , m_size_change_delegate(0)
            , m_world_to_eye_dirty(true)
            , m_fbo_tracks_window_dimensions(false)
        {
            ELECTROSLAG_CHECK(desc->get_component_bits() & renderable_descriptor_component_bits_camera);
            camera_descriptor::ref const& cam_desc(desc->get_camera_component());

            // Pointers to various state / objects.
            graphics::graphics_interface* g = graphics::get_graphics();
            ui::window_interface* window = ui::get_ui()->get_window();
            ui::window_dimensions const* dimensions = window->get_dimensions();

            // Set up the frame buffer the camera renders to.
            int target_width = dimensions->width;
            int target_height = dimensions->height;
            bool bind_delegate = false;
            if (cam_desc->get_camera_render_target() == camera_render_target_window) {
                bind_delegate = true;
            }
            else if (cam_desc->get_camera_render_target() == camera_render_target_fbo) {
                if (cam_desc->get_fbo_width() > 0 && cam_desc->get_fbo_height() > 0) {
                    target_width = cam_desc->get_fbo_width();
                    target_height = cam_desc->get_fbo_height();
                }
                else {
                    bind_delegate = true;
                    m_fbo_tracks_window_dimensions = true;
                }

                m_frame_buffer = g->create_frame_buffer(
                    cam_desc->get_fbo_attribs(),
                    target_width,
                    target_height
                    );
            }
            else {
                throw load_object_failure("invalid camera render target");
            }

            // Set up the projection matrix.
            m_camera_mode = cam_desc->get_camera_mode();
            m_near_distance = cam_desc->get_near_distance();
            m_far_distance = cam_desc->get_far_distance();
            m_field_of_view = cam_desc->get_field_of_view();

            compute_eye_to_clip(
                static_cast<float>(target_width),
                static_cast<float>(target_height)
                );

            // Set up the view matrix.
            bool transform_component = (desc->get_component_bits() & renderable_descriptor_component_bits_transform) != 0;
            bool transform_instance = transform.is_valid();
            if (transform_component && transform_instance) {
                // Quaternion rotation is not commutative. (Component rotation before instance.)
                m_rotation.reset_value(transform->get_rotation() * desc->get_transform_component()->get_rotation());
                m_translate.reset_value(transform->get_translate() * desc->get_transform_component()->get_translate());
            }
            else if (transform_instance) {
                m_rotation.reset_value(transform->get_rotation());
                m_translate.reset_value(transform->get_translate());
            }
            else if (transform_component) {
                m_rotation.reset_value(desc->get_transform_component()->get_rotation());
                m_translate.reset_value(desc->get_transform_component()->get_translate());
            }
            else {
                m_rotation.reset_value(glm::f32quat(1.0f, 0.0f, 0.0f, 0.0f));
                m_translate.reset_value(glm::f32vec3(1.0f, 1.0f, 1.0f));
            }

            // Ready for window size change events.
            if (bind_delegate) {
                m_size_change_delegate = ui::window_interface::size_changed_delegate::create_from_method<
                    camera,
                    &camera::on_window_resize
                    >(this);
                window->size_changed.bind(m_size_change_delegate, event_bind_mode_reference_listener);
            }
        }

        camera::~camera()
        {
            if (m_size_change_delegate) {
                ui::window_interface* window = ui::get_ui()->get_window();
                window->size_changed.unbind(m_size_change_delegate);
                delete m_size_change_delegate;
                m_size_change_delegate = 0;
            }
        }

        void camera::set_controller(
            unsigned long long name_hash,
            animation::property_controller_interface::ref& controller
            )
        {
            if (name_hash == m_rotation.get_name_hash()) {
                m_rotation.set_controller(controller);
            }
            else if (name_hash == m_translate.get_name_hash()) {
                m_translate.set_controller(controller);
            }
            else {
                throw std::runtime_error("Invalid property name hash");
            }
        }

        void camera::clear_controller(unsigned long long name_hash)
        {
            if (name_hash == m_rotation.get_name_hash()) {
                m_rotation.clear_controller();
            }
            else if (name_hash == m_translate.get_name_hash()) {
                m_translate.clear_controller();
            }
            else {
                throw std::runtime_error("Invalid property name hash");
            }
        }

        bool camera::view_frustum_cull(mesh_interface::ref& mesh, float* out_camera_distance) const
        {
            // Return true if culled, false if not. If the mesh is not culled, also return an
            // approximate distance from the camera to the nearest point on the mesh. The
            // distance is intended for coarse depth sorts only.
            ELECTROSLAG_CHECK(!m_world_to_eye_dirty);
            *out_camera_distance = m_far_distance;

            math::f32aabb const& world_aabb = mesh->get_world_aabb();
            glm::f32vec3 const& world_aabb_min = world_aabb.get_min_corner();
            glm::f32vec3 const& world_aabb_max = world_aabb.get_max_corner();

            glm::f32vec3 const aabb_points[8] = {
                { world_aabb_min.x, world_aabb_min.y, world_aabb_min.z },
                { world_aabb_max.x, world_aabb_min.y, world_aabb_min.z },
                { world_aabb_min.x, world_aabb_max.y, world_aabb_min.z },
                { world_aabb_max.x, world_aabb_max.y, world_aabb_min.z },
                { world_aabb_min.x, world_aabb_min.y, world_aabb_max.z },
                { world_aabb_max.x, world_aabb_min.y, world_aabb_max.z },
                { world_aabb_min.x, world_aabb_max.y, world_aabb_max.z },
                { world_aabb_max.x, world_aabb_max.y, world_aabb_max.z }
            };

            for (int plane = 0; plane < view_frustum_plane_index_count; ++plane) {
                int point = 0;
                for (; point < _countof(aabb_points); ++point) {
                    float distance = m_world_frustum_planes[plane].signed_distance(aabb_points[point]);
                    if (distance > 0.0f) {
                        // When culling against the near plane, remember the closest point.
                        if (plane == view_frustum_plane_index_near) {
                            if (distance < *out_camera_distance) {
                                *out_camera_distance = distance;
                            }
                        }
                        // If one point is "in" then the mesh can't be culled by this plane.
                        break;
                    }
                }
                if (point == 8) {
                    return (true);
                }
            }

            // Convert from distance to near plane to distance to camera.
            *out_camera_distance += m_near_distance;
            return (false);
        }

        camera::camera_transform_work_item::ref camera::make_transform_work_item(
            frame_details* this_frame_details
            )
        {
            return (threading::get_frame_thread_pool()->enqueue_work_item<camera_transform_work_item>(
                ref(this),
                this_frame_details
                ));
        }

        void camera::transform_camera(frame_details* this_frame_details)
        {
            // Animate properties
            m_world_to_eye_dirty |= (m_translate.update(this_frame_details->millisec_elapsed) == animation::property_value_state_changed);
            m_world_to_eye_dirty |= (m_rotation.update(this_frame_details->millisec_elapsed) == animation::property_value_state_changed);

            if (m_world_to_eye_dirty) {
                // These transforms are specified in the same way as meshes; but we want
                // a transform from world to "local" space of the camera, which is the
                // inverse of the transforms in the file.
                m_world_to_eye = glm::mat4_cast(glm::inverse(m_rotation.get_value()));
                m_world_to_eye = m_world_to_eye * glm::translate(-(m_translate.get_value()));

                m_world_to_clip = m_eye_to_clip * m_world_to_eye;

                // Don't inverse twice to bring eye space frustum planes to world space.
                for (int plane = 0; plane < view_frustum_plane_index_count; ++plane) {
                    m_world_frustum_planes[plane] = m_eye_frustum_planes[plane].transform(m_translate.get_value(), m_rotation.get_value());
                }

                m_world_to_eye_dirty = false;
            }
        }

        void camera::setup_camera(
            graphics::context_interface* context,
            frame_details*
            )
        {
            if (m_frame_buffer.is_valid()) {
                context->bind_frame_buffer(m_frame_buffer);
            }
            else {
                context->bind_frame_buffer(context->get_display_frame_buffer());
            }
            context->clear_color(0.0f, 0.0f, 0.0f, 0.0f);
            context->clear_depth_stencil(1.0f, 0);
        }

        void camera::compute_eye_to_clip(float target_width, float target_height)
        {
            // TODO: support infinite far clipping planes
            enum frustum_points {
                frustum_points_invalid = -1,
                frustum_points_left_top_near,
                frustum_points_left_bottom_near,
                frustum_points_right_top_near,
                frustum_points_right_bottom_near,
                frustum_points_left_top_far,
                frustum_points_left_bottom_far,
                frustum_points_right_top_far,
                frustum_points_right_bottom_far,

                frustum_points_count
            };

            glm::f32vec3 frustum_points[frustum_points_count];
            if (m_camera_mode == camera_mode_orthographic) {
                // Set up projection matrix.
                m_eye_to_clip = glm::ortho(
                    -1.0f, // left
                     1.0f, // right
                    -1.0f, // bottom
                     1.0f, // top
                    m_near_distance,
                    m_far_distance
                    );

                // Compute frustum points.
                frustum_points[frustum_points_left_top_near]     = glm::f32vec3(-1.0f,  1.0f, -m_near_distance);
                frustum_points[frustum_points_left_bottom_near]  = glm::f32vec3(-1.0f, -1.0f, -m_near_distance);
                frustum_points[frustum_points_right_top_near]    = glm::f32vec3( 1.0f,  1.0f, -m_near_distance);
                frustum_points[frustum_points_right_bottom_near] = glm::f32vec3( 1.0f, -1.0f, -m_near_distance);
                frustum_points[frustum_points_left_top_far]      = glm::f32vec3(-1.0f,  1.0f, -m_far_distance);
                frustum_points[frustum_points_left_bottom_far]   = glm::f32vec3(-1.0f, -1.0f, -m_far_distance);
                frustum_points[frustum_points_right_top_far]     = glm::f32vec3( 1.0f,  1.0f, -m_far_distance);
                frustum_points[frustum_points_right_bottom_far]  = glm::f32vec3( 1.0f, -1.0f, -m_far_distance);
            }
            else if (m_camera_mode == camera_mode_perspective) {
                // Set up projection matrix.
                float aspect_ratio = target_width / target_height;
                m_eye_to_clip = glm::perspective(
                    m_field_of_view,
                    aspect_ratio,
                    m_near_distance,
                    m_far_distance
                    );

                // Compute frustum points.
                float tan_half_fov = tanf(m_field_of_view / 2.0f);
                float ny = m_near_distance * tan_half_fov;
                float fy = m_far_distance * tan_half_fov;
                float nx = ny * aspect_ratio;
                float fx = fy * aspect_ratio;

                frustum_points[frustum_points_left_top_near]     = glm::f32vec3(-nx,  ny, -m_near_distance);
                frustum_points[frustum_points_left_bottom_near]  = glm::f32vec3(-nx, -ny, -m_near_distance);
                frustum_points[frustum_points_right_top_near]    = glm::f32vec3( nx,  ny, -m_near_distance);
                frustum_points[frustum_points_right_bottom_near] = glm::f32vec3( nx, -ny, -m_near_distance);
                frustum_points[frustum_points_left_top_far]      = glm::f32vec3(-fx,  fy, -m_far_distance);
                frustum_points[frustum_points_left_bottom_far]   = glm::f32vec3(-fx, -fy, -m_far_distance);
                frustum_points[frustum_points_right_top_far]     = glm::f32vec3( fx,  fy, -m_far_distance);
                frustum_points[frustum_points_right_bottom_far]  = glm::f32vec3( fx, -fy, -m_far_distance);
            }
            else {
                throw load_object_failure("invalid camera mode");
            }

            // Turn frustum points in to planes. Right hand winding for plane normal.
            m_eye_frustum_planes[view_frustum_plane_index_left].set(
                frustum_points[frustum_points_left_bottom_near],
                frustum_points[frustum_points_left_bottom_far],
                frustum_points[frustum_points_left_top_far]
                );
            m_eye_frustum_planes[view_frustum_plane_index_right].set(
                frustum_points[frustum_points_right_top_near],
                frustum_points[frustum_points_right_top_far],
                frustum_points[frustum_points_right_bottom_far]
                );
            m_eye_frustum_planes[view_frustum_plane_index_top].set(
                frustum_points[frustum_points_left_top_near],
                frustum_points[frustum_points_left_top_far],
                frustum_points[frustum_points_right_top_far]
                );
            m_eye_frustum_planes[view_frustum_plane_index_bottom].set(
                frustum_points[frustum_points_left_bottom_near],
                frustum_points[frustum_points_right_bottom_near],
                frustum_points[frustum_points_right_bottom_far]
                );
            m_eye_frustum_planes[view_frustum_plane_index_far].set(
                frustum_points[frustum_points_right_top_far],
                frustum_points[frustum_points_left_top_far],
                frustum_points[frustum_points_left_bottom_far]
                );
            m_eye_frustum_planes[view_frustum_plane_index_near].set(
                frustum_points[frustum_points_left_bottom_near],
                frustum_points[frustum_points_left_top_near],
                frustum_points[frustum_points_right_top_near]
                );

            m_world_to_eye_dirty = true;
        }

        void camera::on_window_resize(ui::window_dimensions const* dimensions)
        {
            graphics::graphics_interface* g = graphics::get_graphics();

            if (m_fbo_tracks_window_dimensions) {
                graphics::frame_buffer_attribs attribs(*m_frame_buffer->get_attribs());

                m_frame_buffer = g->create_frame_buffer(
                    &attribs,
                    dimensions->width,
                    dimensions->height
                    );
            }

            compute_eye_to_clip(
                static_cast<float>(dimensions->width),
                static_cast<float>(dimensions->height)
                );
        }
    }
}
