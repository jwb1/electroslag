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
#include "electroslag/graphics/graphics_opengl.hpp"
#include "electroslag/graphics/context_opengl.hpp"
#include "electroslag/graphics/sync_opengl.hpp"

namespace electroslag {
    namespace graphics {
        void sync_opengl::opengl_set()
        {
            // Trying to set the sync object again before it's clear.
            if (!is_clear()) {
                throw std::logic_error("sync_opengl object is not cleared");
            }

            m_state.store(sync_state_set, std::memory_order_release);

            m_sync = gl::FenceSync(gl::SYNC_GPU_COMMANDS_COMPLETE, 0);
            context_opengl::check_opengl_error();

            // Pass the now set sync object back to the main graphics object where a worker
            // thread can wait on it.
            sync_interface::ref this_sync(this);
            get_graphics()->finish_setting_sync(this_sync);
        }

        void sync_opengl::opengl_wait()
        {
            get_graphics()->get_render_thread()->check_not();

            GLuint64 wait_milliseconds = 30 * 1000; // 30 seconds is a hang.
            // Extend thread timeouts while debugging.
            if (being_debugged()) {
                wait_milliseconds = 60 * 60 * 1000; // 1 hour
            }

            GLenum wait_return_value = gl::ClientWaitSync(
                m_sync,
                gl::SYNC_FLUSH_COMMANDS_BIT,
                wait_milliseconds
                );
            gl::DeleteSync(m_sync);
            m_sync = 0;

            if (wait_return_value == gl::ALREADY_SIGNALED || wait_return_value == gl::CONDITION_SATISFIED) {
                m_state.store(sync_state_signaled, std::memory_order_release);
                notify_all();
            }
            else if (wait_return_value == gl::TIMEOUT_EXPIRED) {
                throw std::runtime_error("Timeout waiting on OpenGL fence sync object");
            }
            else if (wait_return_value == gl::WAIT_FAILED_) {
                context_opengl::check_opengl_error();
            }
        }
    }
}
