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
#include "electroslag/graphics/graphics_types.hpp"
#include "electroslag/graphics/context_capability_interface.hpp"
#include "electroslag/graphics/graphics_debugger_interface.hpp"
#include "electroslag/graphics/frame_buffer_interface.hpp"
#include "electroslag/graphics/primitive_stream_interface.hpp"
#include "electroslag/graphics/shader_program_interface.hpp"
#include "electroslag/graphics/buffer_interface.hpp"
#include "electroslag/graphics/sync_interface.hpp"

namespace electroslag {
    namespace graphics {
        class context_interface
            : public context_capability_interface
            , public graphics_debugger_interface {
        public:
            // The graphics interface is currently hard coded to double buffer.
            // (Yes the driver may do otherwise, but we ask for 2.)
            static constexpr int const display_buffer_count = 2;

            virtual ~context_interface()
            {}

            virtual void initialize(graphics_initialize_params const* params) = 0;
            virtual void shutdown() = 0;

            // Object binding
            virtual void unbind_all_objects() = 0;

            virtual frame_buffer_interface::ref& get_display_frame_buffer() = 0;
            virtual frame_buffer_interface::ref& get_frame_buffer() = 0;
            virtual void bind_frame_buffer(frame_buffer_interface::ref const& fbi) = 0;

            virtual primitive_stream_interface::ref& get_primitive_stream() = 0;
            virtual void bind_primitive_stream(primitive_stream_interface::ref const& prim_stream) = 0;

            virtual shader_program_interface::ref& get_shader_program() = 0;
            virtual void bind_shader_program(shader_program_interface::ref const& shader) = 0;

            virtual buffer_interface::ref& get_uniform_buffer(int binding) = 0;
            virtual void bind_uniform_buffer(buffer_interface::ref const& ubo, int binding) = 0;
            virtual void bind_uniform_buffer_range(buffer_interface::ref const& ubo, int binding, int start, int end = -1) = 0;

            virtual void set_sync_point(sync_interface::ref& sync) = 0;

            // State changes
            virtual void set_depth_test(depth_test_params const* depth_test) = 0;
            // TODO: Add a depth range state
            virtual void set_blending(blending_params const* blend) = 0;
            // TODO: Add a blend constant color state

            // Operations
            virtual void clear_color(float red, float green, float blue, float alpha) = 0;
            virtual void clear_depth_stencil(float depth, int stencil = 0) = 0;

            virtual void draw(int element_count = 0, int index_buffer_start_offset = 0, int index_value_offset = 0) = 0;

            virtual void swap() = 0;

#if !defined(ELECTROSLAG_BUILD_SHIP)
            virtual void push_debug_group(std::string const& name) = 0;
            virtual void pop_debug_group(std::string const& name) = 0;
#endif
        };
    }
}
