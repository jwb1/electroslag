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
#include "electroslag/graphics/sync_thread_opengl.hpp"
#include "electroslag/graphics/sync_opengl.hpp"

namespace electroslag {
    namespace graphics {
        bool sync_thread_opengl::is_started() const
        {
            bool thread_is_started = false;
            if (m_thread.is_a_thread()) {
                threading::lock_guard thread_lock(&m_mutex);
                if (m_exception) {
                    std::rethrow_exception(m_exception);
                }
                thread_is_started = m_from_state.ready;
            }
            return (thread_is_started);
        }

        void sync_thread_opengl::spawn()
        {
            if (!m_thread.is_a_thread()) {
                m_thread.spawn(&thread_stub, this);
            }
        }

        void sync_thread_opengl::wait_for_ready()
        {
            if (!m_thread.is_a_thread()) {
                throw std::logic_error("thread not spawned yet; will never be ready");
            }

            {
                threading::lock_guard thread_lock(&m_mutex);
                while (!m_from_state.ready) {
                    if (m_exception) {
                        std::rethrow_exception(m_exception);
                    }

                    m_from_condition_var.wait(&thread_lock);
                }
            }
        }

        void sync_thread_opengl::signal_exit()
        {
            threading::lock_guard thread_lock(&m_mutex);
            if (m_exception) {
                std::rethrow_exception(m_exception);
            }

            m_to_state.exit_thread = true;
            m_to_condition_var.notify_all();
        }

        void sync_thread_opengl::wait_for_exit()
        {
            if (!m_thread.is_a_thread()) {
                throw std::logic_error("thread not spawned yet; will never exit");
            }

            // Wait for sync thread to acknowledge it has exited.
            {
                threading::lock_guard thread_lock(&m_mutex);
                while (m_from_state.ready) {
                    if (m_exception) {
                        std::rethrow_exception(m_exception);
                    }

                    m_from_condition_var.wait(&thread_lock);
                }
            }
        }

        void sync_thread_opengl::join()
        {
            threading::lock_guard thread_lock(&m_mutex);
            m_thread.join();
            m_thread_id = threading::thread_id();
        }

        void sync_thread_opengl::wait_for_sync(sync_interface::ref& s)
        {
            threading::lock_guard thread_lock(&m_mutex);
            if (m_exception) {
                std::rethrow_exception(m_exception);
            }

            m_sync_wait.emplace_back(s);

            m_to_state.object_to_wait_on = true;
            m_to_condition_var.notify_all();
        }

        void sync_thread_opengl::thread_method()
        {
            ELECTROSLAG_LOG_MESSAGE("sync_thread_opengl::thread_method");
            try {
                threading::lock_guard thread_lock(&m_mutex);
                m_thread.set_thread_name(ELECTROSLAG_STRING_AND_HASH("t:graphics-sync"));

                m_thread_id = threading::this_thread::get_id();

                m_sub_context.initialize();

                m_to_state.exit_thread = false;
                m_to_state.object_to_wait_on = false;

                m_from_state.ready = true;
                m_from_condition_var.notify_all();

                do {
                    // Wait for the render thread to pass an object.
                    while (!m_to_state.exit_thread && !m_to_state.object_to_wait_on) {
                        m_to_condition_var.wait(&thread_lock);
                    }

                    // Wait on it.
                    if (m_to_state.object_to_wait_on) {
                        m_to_state.object_to_wait_on = wait_for_next_sync();
                    }
                } while (!m_to_state.exit_thread);
            }
            catch (...) {
                m_exception = std::current_exception();
            }

            {
                threading::lock_guard thread_lock(&m_mutex);
                m_from_state.ready = false;
                m_from_condition_var.notify_all();

                m_sub_context.shutdown();
            }
        }

        bool sync_thread_opengl::wait_for_next_sync()
        {
            // Assume the m_mutex is being held.
            sync_interface::ref waiter(m_sync_wait.front());
            m_sync_wait.pop_front();

            waiter.cast<sync_opengl>()->opengl_wait();

            return (m_sync_wait.size() > 0);
        }
    }
}
