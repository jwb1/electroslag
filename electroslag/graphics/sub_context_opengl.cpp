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
#include "electroslag/ui/ui_interface.hpp"
#include "electroslag/ui/ui_win32.hpp"
#include "electroslag/graphics/graphics_opengl.hpp"
#include "electroslag/graphics/sub_context_opengl.hpp"
#include "electroslag/graphics/context_opengl.hpp"

namespace electroslag {
    namespace graphics {
        sub_context_opengl::sub_context_opengl()
#if defined(_WIN32)
            : m_hglrc_created(0)
            , m_hdc(0)
            , m_hglrc(0)
#endif
        {
#if defined(_WIN32)
            graphics_opengl* gfx = static_cast<graphics_opengl*>(get_graphics());

            m_hglrc_created = hglrc_created_delegate::create_from_method<sub_context_opengl, &sub_context_opengl::win32_create_context>(this);
            gfx->hglrc_created.bind(m_hglrc_created, event_bind_mode_reference_listener);
#endif
        }

        sub_context_opengl::~sub_context_opengl()
        {
#if defined(_WIN32)
            unbind_hglrc_created_event();
#endif
        }

        void sub_context_opengl::initialize()
        {
#if defined(_WIN32)
            win32_initialize_context();
#endif

#if !defined(ELECTROSLAG_BUILD_SHIP)
            // Listen to debug output from OpenGL
            gl::DebugMessageCallback(context_opengl::debug_msg_callback, this);
            gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE, gl::DONT_CARE, 0, 0, gl::TRUE_);
            gl::Disable(gl::DEBUG_OUTPUT_SYNCHRONOUS);
            context_opengl::check_opengl_error();
#endif
        }

        void sub_context_opengl::shutdown()
        {
#if defined(_WIN32)
            HWND hwnd = ui::get_ui_win32()->get_win32_window()->get_hwnd();

            wglMakeCurrent(0, 0);
            wglDeleteContext(m_hglrc);
            m_hglrc = 0;

            ReleaseDC(hwnd, m_hdc);
            m_hdc = 0;
#endif
        }

#if defined(_WIN32)
        void sub_context_opengl::win32_create_context(HGLRC main_context)
        {
            HWND hwnd = ui::get_ui_win32()->get_win32_window()->get_hwnd();

            // Assumes the window created for us uses CS_OWNDC in it's window class!
            ELECTROSLAG_CHECK(!m_hdc);
            m_hdc = GetDC(hwnd);
            if (!m_hdc) {
                throw win32_api_failure("GetDC");
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

            if (!wglShareLists(m_hglrc, main_context)) {
                throw win32_api_failure("wglShareLists");
            }
        }

        void sub_context_opengl::win32_initialize_context()
        {
            ELECTROSLAG_CHECK(m_hglrc);
            unbind_hglrc_created_event();

            if (!wglMakeCurrent(m_hdc, m_hglrc)) {
                throw win32_api_failure("wglMakeCurrent");
            }
        }

        void sub_context_opengl::unbind_hglrc_created_event()
        {
            if (m_hglrc_created) {
                graphics_opengl* gfx = static_cast<graphics_opengl*>(get_graphics());
                gfx->hglrc_created.unbind(m_hglrc_created);
                delete m_hglrc_created;
                m_hglrc_created = 0;
            }
        }
#endif
    }
}
