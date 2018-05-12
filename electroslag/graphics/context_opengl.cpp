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
#include "electroslag/logger.hpp"
#include "electroslag/ui/ui_interface.hpp"
#include "electroslag/ui/ui_win32.hpp"
#include "electroslag/graphics/graphics_opengl.hpp"
#include "electroslag/graphics/context_opengl.hpp"
#include "electroslag/graphics/buffer_opengl.hpp"
#include "electroslag/graphics/frame_buffer_opengl.hpp"
#include "electroslag/graphics/primitive_stream_opengl.hpp"
#include "electroslag/graphics/shader_program_opengl.hpp"
#include "electroslag/graphics/sync_opengl.hpp"

namespace electroslag {
    namespace graphics {
        void context_opengl::initialize(graphics_initialize_params const* params)
        {
            check_render_thread();

            {
#if defined(_WIN32)
                // Need an OpenGL context to load extensions.
                win32_dummy_context dummy_context;

                wgl::sys::LoadFunctions(dummy_context.get_hdc());
#endif
                gl::sys::LoadFunctions();

                if (!gl::sys::IsVersionGEQ(4, 5)) {
                    throw std::runtime_error("OpenGL versions prior to 4.5 are not supported");
                }

                ELECTROSLAG_LOG_GFX("OpenGL Renderer: %s", gl::GetString(gl::RENDERER));
                ELECTROSLAG_LOG_GFX("OpenGL Extensions: %s", gl::GetString(gl::EXTENSIONS));
            }

#if defined(_WIN32)
            // Unlike other platforms, win32 must create it's "real" context after
            // loading extensions because it uses extensions to do so.
            win32_initialize_context(params);
#endif

#if !defined(ELECTROSLAG_BUILD_SHIP)
            // Listen to debug output from OpenGL
            gl::DebugMessageCallback(debug_msg_callback, this);
            gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE, gl::DONT_CARE, 0, 0, gl::TRUE_);
            gl::Disable(gl::DEBUG_OUTPUT_SYNCHRONOUS);
            context_opengl::check_opengl_error();

            m_graphics_debugger_attached = initialize_renderdoc();
#endif

            // All contexts have a frame buffer that represents rendering on-screen.
            ELECTROSLAG_CHECK(!(m_display_frame_buffer.is_valid()));
            m_display_frame_buffer = frame_buffer_opengl::create_display_fbo(
                &params->display_attribs
                ).cast<frame_buffer_interface>();
            bind_frame_buffer(m_display_frame_buffer);

            // Gather constraints from the OpenGL driver.
            gl::GetIntegerv(gl::MAX_UNIFORM_BUFFER_BINDINGS, &m_max_total_uniform_bindings);
            gl::GetIntegerv(gl::MAX_VERTEX_UNIFORM_BLOCKS, &m_max_stage_uniform_bindings[shader_stage_vertex]);
            gl::GetIntegerv(gl::MAX_TESS_CONTROL_UNIFORM_BLOCKS, &m_max_stage_uniform_bindings[shader_stage_tessellation_control]);
            gl::GetIntegerv(gl::MAX_TESS_EVALUATION_UNIFORM_BLOCKS, &m_max_stage_uniform_bindings[shader_stage_tessellation_evaluation]);
            gl::GetIntegerv(gl::MAX_GEOMETRY_UNIFORM_BLOCKS, &m_max_stage_uniform_bindings[shader_stage_geometry]);
            gl::GetIntegerv(gl::MAX_FRAGMENT_UNIFORM_BLOCKS, &m_max_stage_uniform_bindings[shader_stage_fragment]);
            gl::GetIntegerv(gl::MAX_COMPUTE_UNIFORM_BLOCKS, &m_max_stage_uniform_bindings[shader_stage_compute]);
            gl::GetIntegerv(gl::UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_min_ubo_offset_alignment);

            m_bound_uniform_buffers.resize(m_max_total_uniform_bindings);

            // Default to counter-clockwise winding as front facing
            gl::FrontFace(gl::CCW);
            check_opengl_error();

            gl::CullFace(gl::BACK);
            check_opengl_error();

            gl::Enable(gl::CULL_FACE);
            check_opengl_error();

            // Set up clip space; 0 to 1 rather than -1 to 1 to improve z buffer precision.
            gl::ClipControl(gl::LOWER_LEFT, gl::ZERO_TO_ONE);
        }

        void context_opengl::shutdown()
        {
            m_display_frame_buffer.reset();

#if defined(_WIN32)
            HWND hwnd = ui::get_ui_win32()->get_win32_window()->get_hwnd();

            wglMakeCurrent(0, 0);
            wglDeleteContext(m_hglrc);
            m_hglrc = 0;

            ReleaseDC(hwnd, m_hdc);
            m_hdc = 0;
#endif
        }

        void context_opengl::unbind_all_objects()
        {
            m_bound_frame_buffer.reset();
            m_bound_primitive_stream.reset();
            m_bound_shader_program.reset();
            m_bound_uniform_buffers.clear();
        }

        void context_opengl::check_render_thread() const
        {
            get_graphics()->get_render_thread()->check();
        }

        void context_opengl::check_not_render_thread() const
        {
            get_graphics()->get_render_thread()->check_not();
        }

        void context_opengl::set_sync_point(sync_interface::ref& sync)
        {
            check_render_thread();
            sync.cast<sync_opengl>()->opengl_set();
        }

        frame_buffer_interface::ref& context_opengl::get_display_frame_buffer()
        {
            return (m_display_frame_buffer);
        }

        frame_buffer_interface::ref& context_opengl::get_frame_buffer()
        {
            return (m_bound_frame_buffer);
        }

        void context_opengl::bind_frame_buffer(frame_buffer_interface::ref const& fbi)
        {
            check_render_thread();
            if (fbi != m_bound_frame_buffer) {
                if (fbi.is_valid()) {
                    fbi.cast<frame_buffer_opengl>()->bind();
                }
                m_bound_frame_buffer = fbi;
            }
        }

        primitive_stream_interface::ref& context_opengl::get_primitive_stream()
        {
            return (m_bound_primitive_stream);
        }

        void context_opengl::bind_primitive_stream(primitive_stream_interface::ref const& prim_stream)
        {
            check_render_thread();
            if (prim_stream != m_bound_primitive_stream) {
                if (prim_stream.is_valid()) {
                    prim_stream.cast<primitive_stream_opengl>()->bind();
                }
                m_bound_primitive_stream = prim_stream;
            }
        }

        shader_program_interface::ref& context_opengl::get_shader_program()
        {
            return (m_bound_shader_program);
        }

        void context_opengl::bind_shader_program(shader_program_interface::ref const& shader)
        {
            check_render_thread();
            if (shader != m_bound_shader_program) {
                if (shader.is_valid()) {
                    shader.cast<shader_program_opengl>()->bind();
                }
                m_bound_shader_program = shader;
            }
        }

        buffer_interface::ref& context_opengl::get_uniform_buffer(int binding)
        {
            return (m_bound_uniform_buffers[binding]);
        }

        void context_opengl::bind_uniform_buffer(buffer_interface::ref const& ubo, int binding)
        {
            check_render_thread();
            if (ubo.is_valid()) {
                ubo.cast<buffer_opengl>()->bind_to_index(gl::UNIFORM_BUFFER, binding);
            }
            m_bound_uniform_buffers[binding] = ubo;
        }

        void context_opengl::bind_uniform_buffer_range(buffer_interface::ref const& ubo, int binding, int start, int end /*= -1*/)
        {
            check_render_thread();
            if (ubo.is_valid()) {
                ubo.cast<buffer_opengl>()->bind_range_to_index(gl::UNIFORM_BUFFER, binding, start, end);
            }
            m_bound_uniform_buffers[binding] = ubo;
        }

        void context_opengl::set_depth_test(depth_test_params const* depth_test)
        {
            check_render_thread();
            if (*depth_test != m_depth_test_params) {
                if (depth_test->test_enable) {
                    gl::Enable(gl::DEPTH_TEST);
                    gl::DepthFunc(make_gl_depth_func(depth_test->test_mode));
                }
                else {
                    gl::Disable(gl::DEPTH_TEST);
                }

                if (depth_test->write_enable) {
                    gl::DepthMask(gl::TRUE_);
                }
                else {
                    gl::DepthMask(gl::FALSE_);
                }
                check_opengl_error();
                m_depth_test_params = *depth_test;
            }
        }

        GLenum context_opengl::make_gl_depth_func(depth_test_mode test_mode)
        {
            static GLenum const gl_depth_funcs[depth_test_mode_count] = {
                gl::NEVER,
                gl::LESS,
                gl::EQUAL,
                gl::LEQUAL,
                gl::GREATER,
                gl::NOTEQUAL,
                gl::GEQUAL,
                gl::ALWAYS
            };

            return (gl_depth_funcs[test_mode]);
        }

        void context_opengl::set_blending(blending_params const* blend)
        {
            check_render_thread();
            if (*blend != m_blend_params) {
                if (blend->enable) {
                    gl::Enable(gl::BLEND);
                    gl::BlendEquationSeparate(
                        make_gl_blend_equation(blend->color_mode),
                        make_gl_blend_equation(blend->alpha_mode)
                        );
                    gl::BlendFuncSeparate(
                        make_gl_blend_func(blend->color_op1),
                        make_gl_blend_func(blend->color_op2),
                        make_gl_blend_func(blend->alpha_op1),
                        make_gl_blend_func(blend->alpha_op2)
                        );
                }
                else {
                    gl::Disable(gl::BLEND);
                }
                check_opengl_error();
                m_blend_params = *blend;
            }
        }

        // static
        GLenum context_opengl::make_gl_blend_equation(blending_mode mode)
        {
            static GLenum const gl_blend_equations[blending_mode_count] = {
                gl::FUNC_ADD,
                gl::FUNC_SUBTRACT,
                gl::FUNC_REVERSE_SUBTRACT,
                gl::MIN,
                gl::MAX
            };

            return (gl_blend_equations[mode]);
        }

        // static
        GLenum context_opengl::make_gl_blend_func(blending_operand op)
        {
            static GLenum const gl_blend_funcs[blending_operand_count] = {
                gl::ZERO,
                gl::ONE,
                gl::SRC_COLOR,
                gl::ONE_MINUS_SRC_COLOR,
                gl::DST_COLOR,
                gl::ONE_MINUS_DST_COLOR,
                gl::SRC_ALPHA,
                gl::ONE_MINUS_SRC_ALPHA,
                gl::DST_ALPHA,
                gl::ONE_MINUS_DST_ALPHA,
                gl::CONSTANT_COLOR,
                gl::ONE_MINUS_CONSTANT_COLOR,
                gl::CONSTANT_ALPHA,
                gl::ONE_MINUS_CONSTANT_ALPHA,
                gl::SRC_ALPHA_SATURATE,
                gl::SRC1_COLOR,
                gl::ONE_MINUS_SRC1_COLOR,
                gl::SRC1_ALPHA,
                gl::ONE_MINUS_SRC1_ALPHA
            };

            return (gl_blend_funcs[op]);
        }

        void context_opengl::clear_color(float red, float green, float blue, float alpha)
        {
            check_render_thread();
            gl::ClearColor(red, green, blue, alpha);
            gl::Clear(gl::COLOR_BUFFER_BIT);
            check_opengl_error();
        }

        void context_opengl::clear_depth_stencil(float depth, int stencil)
        {
            check_render_thread();
            gl::ClearDepth(depth);
            gl::ClearStencil(stencil);
            gl::Clear(gl::DEPTH_BUFFER_BIT | gl::STENCIL_BUFFER_BIT);
            check_opengl_error();
        }

        void context_opengl::draw(
            int element_count,
            int index_buffer_start_offset,
            int index_value_offset
            )
        {
            check_render_thread();
            ELECTROSLAG_CHECK(m_bound_frame_buffer.is_valid());
            ELECTROSLAG_CHECK(m_bound_shader_program.is_valid());
            ELECTROSLAG_CHECK(m_bound_primitive_stream.is_valid());
            ELECTROSLAG_CHECK(index_buffer_start_offset >= 0);

            primitive_type prim_type = m_bound_primitive_stream.cast<primitive_stream_opengl>()->get_primitive_type();
            int sizeof_index = m_bound_primitive_stream.cast<primitive_stream_opengl>()->get_sizeof_index();

            if (element_count <= 0) {
                element_count = primitive_type_util::get_element_count(
                    prim_type,
                    m_bound_primitive_stream.cast<primitive_stream_opengl>()->get_primitive_count()
                    );
            }

            intptr_t ibo_start_offset = index_buffer_start_offset;
            gl::DrawElementsBaseVertex(
                primitive_type_util::get_opengl_primitive_type(prim_type),
                element_count,
                primitive_type_util::get_opengl_index_size(sizeof_index),
                reinterpret_cast<void*>(ibo_start_offset),
                index_value_offset
                );
            check_opengl_error();
        }

        void context_opengl::swap()
        {
            check_render_thread();

#if defined(_WIN32)
            if (!SwapBuffers(m_hdc)) {
                throw win32_api_failure("SwapBuffers");
            }
#endif
        }

#if !defined(ELECTROSLAG_BUILD_SHIP)
        void context_opengl::push_debug_group(std::string const& name)
        {
            check_render_thread();
            gl::PushDebugGroup(gl::DEBUG_SOURCE_APPLICATION, 1, static_cast<GLsizei>(name.length()), name.c_str());
            check_opengl_error();
        }

        void context_opengl::pop_debug_group(std::string const&)
        {
            check_render_thread();
            gl::PopDebugGroup();
            check_opengl_error();
        }
#endif

#if defined(_WIN32)
        // static
        char const* const context_opengl::win32_dummy_context::window_class_name = "electroslag_dummy_context_window";

        context_opengl::win32_dummy_context::win32_dummy_context()
        {
            m_hinstance = ui::get_ui_win32()->get_hinstance();

            WNDCLASSEX wcex = { 0 };
            wcex.cbSize = sizeof(wcex);
            wcex.lpfnWndProc = DefWindowProc;
            wcex.hInstance = m_hinstance;
            wcex.lpszClassName = window_class_name;

            if (!RegisterClassEx(&wcex)) {
                throw win32_api_failure("RegisterClassEx");
            }

            m_hwnd = CreateWindowEx(
                0, // extended style
                window_class_name,
                0, // title
                0, // window style
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0, // parent handle
                0, // menu handle
                m_hinstance,
                0  // create lparam
                );
            if (!m_hwnd) {
                throw win32_api_failure("CreateWindowEx");
            }

            m_hdc = GetDC(m_hwnd);
            if (!m_hdc) {
                throw win32_api_failure("GetDC");
            }

            PIXELFORMATDESCRIPTOR pfd = {0};
            pfd.nSize = sizeof(pfd);

            // 1 is always the first hardware accelerated pixel format.
            if (!SetPixelFormat(m_hdc, 1, &pfd)) {
                throw win32_api_failure("SetPixelFormat");
            }

            m_hglrc = wglCreateContext(m_hdc);
            if (!m_hglrc) {
                throw win32_api_failure("wglCreateContext");
            }

            if (!wglMakeCurrent(m_hdc, m_hglrc)) {
                throw win32_api_failure("wglMakeCurrent");
            }
        }

        context_opengl::win32_dummy_context::~win32_dummy_context()
        {
            wglMakeCurrent(0, 0);
            wglDeleteContext(m_hglrc);
            m_hglrc = 0;

            ReleaseDC(m_hwnd, m_hdc);
            m_hdc = 0;

            DestroyWindow(m_hwnd);
            m_hwnd = 0;

            UnregisterClass(window_class_name, m_hinstance);
            m_hinstance = 0;
        }

        void context_opengl::win32_initialize_context(graphics_initialize_params const* params)
        {
            HWND hwnd = ui::get_ui_win32()->get_win32_window()->get_hwnd();

            // Assumes the window created for us uses CS_OWNDC in it's window class!
            ELECTROSLAG_CHECK(!m_hdc);
            m_hdc = GetDC(hwnd);
            if (!m_hdc) {
                throw win32_api_failure("GetDC");
            }

            static int const query_attribs[] = { wgl::NUMBER_PIXEL_FORMATS_ARB };
            int query_results[1] = { 0 };
            if (!wgl::GetPixelFormatAttribivARB(
                m_hdc,
                1,
                0,
                1,
                query_attribs,
                query_results
                )) {
                throw win32_api_failure("wglGetPixelFormatAttribivARB");
            }

            int selected_format = 0;
            {
                unsigned int max_formats = query_results[0];

                static int pixel_format_attribs[] = {
                    // Basic format stuff; yes we want OpenGL, rendering into a window, accelerated, and
                    // double buffered.
                    wgl::SUPPORT_OPENGL_ARB, gl::TRUE_,
                    wgl::DRAW_TO_WINDOW_ARB, gl::TRUE_,
                    wgl::ACCELERATION_ARB, wgl::FULL_ACCELERATION_ARB,
                    wgl::PIXEL_TYPE_ARB, wgl::TYPE_RGBA_ARB,
                    wgl::DOUBLE_BUFFER_ARB, gl::TRUE_,
                    wgl::RED_BITS_ARB, 0,
                    wgl::GREEN_BITS_ARB, 0,
                    wgl::BLUE_BITS_ARB, 0,
                    wgl::ALPHA_BITS_ARB, 0,
                    wgl::DEPTH_BITS_ARB, 0,
                    wgl::STENCIL_BITS_ARB, 0,
                    wgl::SAMPLE_BUFFERS_ARB, gl::FALSE_,
                    wgl::SAMPLES_ARB, 0,
                    wgl::FRAMEBUFFER_SRGB_CAPABLE_ARB, gl::FALSE_,
                    0 // "NULL" termination
                };

                set_display_attribs<
                    11, // red_bits_offset
                    13, // green_bits_offset
                    15, // blue_bits_offset
                    17, // alpha_bits_offset
                    19, // depth_bits_offset
                    21, // stencil_bits_offset
                    23, // msaa_bool_offset
                    25, // msaa_samples_offset,
                    27  // srgb_bool_offset
                >(
                &params->display_attribs,
                pixel_format_attribs
                );

                // Some graphics cards have LOTS of pixel formats, so avoid alloca and stack explosion.
                unsigned int matching_formats = 0;
                format_vector formats(max_formats, 0);

                if (!wgl::ChoosePixelFormatARB(
                    m_hdc,
                    pixel_format_attribs,
                    0,
                    max_formats,
                    &formats.front(),
                    &matching_formats
                    )) {
                    throw win32_api_failure("wglChoosePixelFormatARB");
                }

                // For now use the "best fit" format.
                selected_format = formats[0];
            }

            PIXELFORMATDESCRIPTOR pfd = { 0 };
            pfd.nSize = sizeof(pfd);

            if (!SetPixelFormat(m_hdc, selected_format, &pfd)) {
                throw win32_api_failure("SetPixelFormat");
            }

            static int const context_attribs[] = {
                wgl::CONTEXT_MAJOR_VERSION_ARB, 4,
                wgl::CONTEXT_MINOR_VERSION_ARB, 5,
                wgl::CONTEXT_PROFILE_MASK_ARB, wgl::CONTEXT_CORE_PROFILE_BIT_ARB,
#if !defined(ELECTROSLAG_BUILD_SHIP)
                wgl::CONTEXT_FLAGS_ARB, wgl::CONTEXT_DEBUG_BIT_ARB,
#endif
                0 // "NULL" termination
            };

            ELECTROSLAG_CHECK(!m_hglrc);
            m_hglrc = wgl::CreateContextAttribsARB(m_hdc, 0, context_attribs);
            if (!m_hglrc) {
                throw win32_api_failure("wglCreateContextAttribsARB");
            }

            // Signal any sub-contexts that they can now create their contexts as well.
            graphics_opengl* gfx = static_cast<graphics_opengl*>(get_graphics());
            gfx->hglrc_created.signal(m_hglrc);

            // Return to the main context. The render thread will always keep it current.
            if (!wglMakeCurrent(m_hdc, m_hglrc)) {
                throw win32_api_failure("wglMakeCurrent");
            }

            // Set up the swap interval.
            if (params->swap_interval >= 0) {
                int swap_interval = params->swap_interval;

                // This extension extends the existing WGL_EXT_swap_control extension
                // by allowing a negative interval parameter to wglSwapIntervalEXT.
                // The negative interval allows late swaps to occur without
                // synchronization to the video frame. This reduces the visual stutter
                // on late frames and reduces the stall on subsequent frames.
                if (wgl::exts::var_EXT_swap_control_tear) {
                    swap_interval = -swap_interval;
                }

                if (!wgl::SwapIntervalEXT(swap_interval)) {
                    throw win32_api_failure("wglSwapIntervalEXT");
                }
            }
        }
#endif

#if !defined(ELECTROSLAG_BUILD_SHIP)
        bool context_opengl::initialize_renderdoc()
        {
            bool has_renderdoc = m_renderdoc_library.load("renderdoc");
            if (has_renderdoc) {
                ELECTROSLAG_LOG_GFX("context_opengl::initialize_renderdoc - RenderDoc integration enabled.");
            }
            return (has_renderdoc);
        }

        // static
        void APIENTRY context_opengl::debug_msg_callback(
            GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            GLchar const* msg,
            void const* //data (Set to context_opengl this pointer)
            )
        {
            ELECTROSLAG_LOG_GFX(
                "context_opengl::debug_msg_callback -"
                " [source:%d] [type:%d] [id:%d] [severity:%d] [message:%*s]",
                source, type, id, severity, length, msg
                );
        }
#endif
    }
}
