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
#include "electroslag/event.hpp"

namespace electroslag {
    namespace graphics {

        // A OpenGL context that shares objects with context_opengl; used to
        // enable worker threads to call OpenGL in limited scenarios.
        class sub_context_opengl {
        public:
#if defined(_WIN32)
            // Sub-contexts _must_create their context right after the main one
            // on Win32 in order to make wglShareLists work.
            typedef event<void, HGLRC> hglrc_created_event;
            typedef hglrc_created_event::bound_delegate hglrc_created_delegate;
#endif

            sub_context_opengl();
            virtual ~sub_context_opengl();

            void initialize();
            void shutdown();
        private:
#if defined(_WIN32)
            void win32_create_context(HGLRC main_context);
            void win32_initialize_context();

            void unbind_hglrc_created_event();

            hglrc_created_delegate* m_hglrc_created;

            HDC m_hdc;
            HGLRC m_hglrc;
#endif

            // Disallowed operations:
            explicit sub_context_opengl(sub_context_opengl const&);
            sub_context_opengl& operator =(sub_context_opengl const&);
        };
    }
}
