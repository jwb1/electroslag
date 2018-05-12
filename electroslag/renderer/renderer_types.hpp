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
#include "electroslag/threading/work_item_interface.hpp"
#include "electroslag/graphics/sync_interface.hpp"
#include "electroslag/graphics/buffer_interface.hpp"

namespace electroslag {
    namespace renderer {
        enum pass_type {
            pass_type_unknown = -1,
            pass_type_geometry,
            pass_type_pixel_processing,

            pass_type_count  // Ensure this is the last enum entry
        };

        enum pipeline_type {
            pipeline_type_unknown = -1,
            pipeline_type_forward_geometry,

            pipeline_type_count // Ensure this is the last enum entry
        };

        static char const* const pipeline_type_strings[pipeline_type_count] = {
            "pipeline_type_forward_geometry"
        };

        enum renderable_descriptor_component_bits {
            renderable_descriptor_component_bits_transform = 1,
            renderable_descriptor_component_bits_geometry = 2,
            renderable_descriptor_component_bits_geometry_shape_only = 4,
            renderable_descriptor_component_bits_pipeline = 8,
            renderable_descriptor_component_bits_camera = 16,

            renderable_descriptor_component_bits_count = 5 // Has to be hand edited!
        };

        static char const* const renderable_descriptor_component_strings[renderable_descriptor_component_bits_count] = {
            "renderable_descriptor_component_bits_transform",
            "renderable_descriptor_component_bits_geometry",
            "renderable_descriptor_component_bits_geometry_shape_only",
            "renderable_descriptor_component_bits_pipeline",
            "renderable_descriptor_component_bits_camera"
        };

        static renderable_descriptor_component_bits const renderable_descriptor_component_bits_values[renderable_descriptor_component_bits_count] = {
            renderable_descriptor_component_bits_transform,
            renderable_descriptor_component_bits_geometry,
            renderable_descriptor_component_bits_geometry_shape_only,
            renderable_descriptor_component_bits_pipeline,
            renderable_descriptor_component_bits_camera
        };

        enum camera_mode {
            camera_mode_unknown = -1,
            camera_mode_perspective,
            camera_mode_orthographic,

            camera_mode_count // Ensure this is the last enum entry
        };

        static char const* const camera_mode_strings[camera_mode_count] = {
            "camera_mode_perspective",
            "camera_mode_orthographic"
        };

        enum camera_render_target {
            camera_render_target_unknown = -1,
            camera_render_target_window,
            camera_render_target_fbo,

            camera_render_target_count // Ensure this is the last enum entry
        };

        static char const* const camera_render_target_strings[camera_render_target_count] = {
            "camera_render_target_window",
            "camera_render_target_fbo"
        };

        class renderer;
        struct frame_details {
            void reset()
            {
                millisec_elapsed = 0;
                r = 0;

                dynamic_ubo_size = 0;
                dynamic_ubo.reset();
                mapped_dynamic_ubo = 0;

                total_meshes = 0;
                completed_meshes.store(0);

                // Don't reset the sync object; its re-used frame to frame.
            }

            // High level details about the frame.
            int millisec_elapsed;
            renderer* r;

            // Sync object the GPU signals when done rendering the frame.
            graphics::sync_interface::ref sync;

            // Per-frame (aka "dynamic") UBOs are created from this one single buffer.
            int dynamic_ubo_size;
            graphics::buffer_interface::ref dynamic_ubo;
            byte* mapped_dynamic_ubo;

            // Track mesh render work item completion.
            int total_meshes;
            std::atomic<int> completed_meshes;
        };

        typedef threading::work_item_interface frame_work_item;
        typedef frame_work_item::work_item_vector frame_work_item_vector;
    }
}
