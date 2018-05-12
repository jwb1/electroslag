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
#if !defined(ELECTROSLAG_BUILD_SHIP)
#include "electroslag/dynamic_library.hpp"
#endif
#include "electroslag/graphics/context_interface.hpp"

namespace electroslag {
    namespace graphics {
        class context_opengl : public context_interface {
        public:
            static void check_opengl_error()
            {
#if defined(ELECTROSLAG_BUILD_DEBUG)
                GLenum error_code = gl::GetError();
                if (error_code != gl::NO_ERROR_) {
                    throw opengl_api_failure("glGetError", error_code);
                }
#endif
            }

            static void clear_opengl_error()
            {
                while (gl::GetError() != gl::NO_ERROR_);
            }

            context_opengl()
                : m_max_total_uniform_bindings(0)
                , m_min_ubo_offset_alignment(0)
#if !defined(ELECTROSLAG_BUILD_SHIP)
                , m_graphics_debugger_attached(false)
#endif
#if defined(_WIN32)
                , m_hdc(0)
                , m_hglrc(0)
#endif
            {
                memset(m_max_stage_uniform_bindings, 0, sizeof(m_max_stage_uniform_bindings));
            }

            virtual ~context_opengl()
            {}

            // Implement context_capability_interface
            virtual int get_max_total_uniform_bindings() const
            {
                return (m_max_total_uniform_bindings);
            }

            virtual int get_max_stage_uniform_bindings(shader_stage stage) const
            {
                ELECTROSLAG_CHECK(stage > shader_stage_unknown && stage < shader_stage_count);
                return (m_max_stage_uniform_bindings[stage]);
            }

            virtual int get_min_ubo_offset_alignment() const
            {
                return (m_min_ubo_offset_alignment);
            }

            // Implement graphics_debugger_interface
            virtual bool is_graphics_debugger_attached() const
            {
#if !defined(ELECTROSLAG_BUILD_SHIP)
                return (m_graphics_debugger_attached);
#else
                return (false);
#endif
            }

            // Implement context_interface
            virtual void initialize(graphics_initialize_params const* params);
            virtual void shutdown();

            virtual void unbind_all_objects();

            virtual frame_buffer_interface::ref& get_display_frame_buffer();
            virtual frame_buffer_interface::ref& get_frame_buffer();
            virtual void bind_frame_buffer(frame_buffer_interface::ref const& fbi);

            virtual primitive_stream_interface::ref& get_primitive_stream();
            virtual void bind_primitive_stream(primitive_stream_interface::ref const& prim_stream);

            virtual shader_program_interface::ref& get_shader_program();
            virtual void bind_shader_program(shader_program_interface::ref const& shader);

            virtual buffer_interface::ref& get_uniform_buffer(int binding);
            virtual void bind_uniform_buffer(buffer_interface::ref const& ubo, int binding);
            virtual void bind_uniform_buffer_range(buffer_interface::ref const& ubo, int binding, int start, int end = -1);

            virtual void set_sync_point(sync_interface::ref& sync);

            virtual void set_depth_test(depth_test_params const* depth_test);
            virtual void set_blending(blending_params const* blend);

            virtual void clear_color(float red, float green, float blue, float alpha);
            virtual void clear_depth_stencil(float depth, int stencil = 0);

            virtual void draw(
                int element_count = 0,
                int index_buffer_start_offset = 0,
                int index_value_offset = 0
                );

            virtual void swap();

#if !defined(ELECTROSLAG_BUILD_SHIP)
            virtual void push_debug_group(std::string const& name);
            virtual void pop_debug_group(std::string const& name);
#endif

            // These are all methods for use in enqueued commands.
            void check_render_thread() const;
            void check_not_render_thread() const;

#if !defined(ELECTROSLAG_BUILD_SHIP)
            static void APIENTRY debug_msg_callback(
                GLenum source,
                GLenum type,
                GLuint id,
                GLenum severity,
                GLsizei length,
                GLchar const* msg,
                void const* data
                );
#endif

        private:
            // The format vector is used in initialize to enumerate the formats.
            typedef std::vector<int> format_vector;

#if defined(_WIN32)
            template<
                int RedBitsOffset,
                int GreenBitsOffset,
                int BlueBitsOffset,
                int AlphaBitsOffset,
                int DepthBitsOffset,
                int StencilBitsOffset,
                int MsaaBoolOffset,
                int MsaaSamplesOffset,
                int SRGBBoolOffset
                >
            static void set_display_attribs(
                frame_buffer_attribs const* display_attribs,
                int* attribs
                );
#endif

            static GLenum make_gl_depth_func(depth_test_mode test_mode);
            static GLenum make_gl_blend_equation(blending_mode mode);
            static GLenum make_gl_blend_func(blending_operand op);

            // There is always a frame buffer associated with the display.
            frame_buffer_interface::ref m_display_frame_buffer;

            // Cached copies of GL driver limitations.
            int m_max_total_uniform_bindings;
            int m_max_stage_uniform_bindings[shader_stage_count];
            int m_min_ubo_offset_alignment;

#if !defined(ELECTROSLAG_BUILD_SHIP)
            // Set on context create. No support yet for "detaching" a
            // graphics debugger.
            bool m_graphics_debugger_attached;

            bool initialize_renderdoc();
            dynamic_library m_renderdoc_library;
#endif

            // These are the current objects.
            frame_buffer_interface::ref m_bound_frame_buffer;
            primitive_stream_interface::ref m_bound_primitive_stream;
            shader_program_interface::ref m_bound_shader_program;

            typedef std::vector<buffer_interface::ref> uniform_buffer_binding_vector;
            uniform_buffer_binding_vector m_bound_uniform_buffers;

            // Cached state copy
            blending_params m_blend_params;
            depth_test_params m_depth_test_params;

#if defined(_WIN32)
            class win32_dummy_context {
            public:
                win32_dummy_context();
                ~win32_dummy_context();

                HDC get_hdc() const
                {
                    return (m_hdc);
                }
            private:
                static char const* const window_class_name;

                HINSTANCE m_hinstance;
                HWND m_hwnd;
                HDC m_hdc;
                HGLRC m_hglrc;
            };

            void win32_initialize_context(graphics_initialize_params const* params);

            HDC m_hdc;
            HGLRC m_hglrc;
#endif

            // Disallowed operations:
            explicit context_opengl(context_opengl const&);
            context_opengl& operator =(context_opengl const&);
        };

#if defined(_WIN32)
        // static
        template<
            int RedBitsOffset,
            int GreenBitsOffset,
            int BlueBitsOffset,
            int AlphaBitsOffset,
            int DepthBitsOffset,
            int StencilBitsOffset,
            int MsaaBoolOffset,
            int MsaaSamplesOffset,
            int SRGBBoolOffset
            >
        void context_opengl::set_display_attribs(
            frame_buffer_attribs const* display_attribs,
            int* attribs
            )
        {
            ELECTROSLAG_CHECK(display_attribs);
            ELECTROSLAG_CHECK(attribs);

            switch (display_attribs->color_format) {
            case frame_buffer_color_format_r8g8b8a8:
            case frame_buffer_color_format_r8g8b8a8_srgb:
                attribs[RedBitsOffset] = 8;
                attribs[GreenBitsOffset] = 8;
                attribs[BlueBitsOffset] = 8;
                attribs[AlphaBitsOffset] = 8;
                break;

            default:
                throw parameter_failure("display_attribs->color_format");
            }

            if (display_attribs->color_format == frame_buffer_color_format_r8g8b8a8_srgb) {
                attribs[SRGBBoolOffset] = gl::TRUE_;
            }
            else {
                attribs[SRGBBoolOffset] = gl::FALSE_;
            }

            switch (display_attribs->depth_stencil_format) {
            case frame_buffer_depth_stencil_format_none:
                break;

            case frame_buffer_depth_stencil_format_d16:
                attribs[DepthBitsOffset] = 16;
                break;

            case frame_buffer_depth_stencil_format_d24:
                attribs[DepthBitsOffset] = 24;
                break;

            case frame_buffer_depth_stencil_format_d32:
                attribs[DepthBitsOffset] = 32;
                break;

            case frame_buffer_depth_stencil_format_d24s8:
                attribs[DepthBitsOffset] = 24;
                attribs[StencilBitsOffset] = 8;
                break;

            default:
                throw parameter_failure("display_attribs->depth_format");
            }

            switch (display_attribs->msaa) {
            case frame_buffer_msaa_none:
                attribs[MsaaBoolOffset] = gl::FALSE_;
                attribs[MsaaSamplesOffset] = 0;
                break;

            case frame_buffer_msaa_2:
                attribs[MsaaBoolOffset] = gl::TRUE_;
                attribs[MsaaSamplesOffset] = 2;
                break;

            case frame_buffer_msaa_4:
                attribs[MsaaBoolOffset] = gl::TRUE_;
                attribs[MsaaSamplesOffset] = 4;
                break;

            case frame_buffer_msaa_6:
                attribs[MsaaBoolOffset] = gl::TRUE_;
                attribs[MsaaSamplesOffset] = 6;
                break;

            case frame_buffer_msaa_8:
                attribs[MsaaBoolOffset] = gl::TRUE_;
                attribs[MsaaSamplesOffset] = 8;
                break;

            case frame_buffer_msaa_16:
                attribs[MsaaBoolOffset] = gl::TRUE_;
                attribs[MsaaSamplesOffset] = 16;
                break;

            default:
                throw parameter_failure("display_attribs->msaa");
            }
        }
#endif
    }
}
