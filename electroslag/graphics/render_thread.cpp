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
#include "electroslag/graphics/render_thread.hpp"
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/graphics/context_opengl.hpp"

namespace electroslag {
    namespace graphics {
        void render_thread::spawn(graphics_initialize_params const* params)
        {
            if (!m_thread.is_a_thread()) {
                threading::lock_guard thread_lock(&m_mutex);
                m_params = *params;
                m_thread.spawn(&thread_stub, this);
            }
        }

        bool render_thread::is_started() const
        {
            bool thread_is_started = false;
            if (m_thread.is_a_thread()) {
                threading::lock_guard thread_lock(&m_mutex);
                if (m_exception) {
                    std::rethrow_exception(m_exception);
                }
                thread_is_started = m_from_state.ready_to_run;
            }
            return (thread_is_started);
        }

        void render_thread::wait_for_ready()
        {
            if (!m_thread.is_a_thread()) {
                throw std::logic_error("thread not spawned yet; will never be ready");
            }

            {
                threading::lock_guard thread_lock(&m_mutex);
                while (!m_from_state.ready_to_run) {
                    if (m_exception) {
                        std::rethrow_exception(m_exception);
                    }

                    m_from_condition_var.wait(&thread_lock);
                }
            }
        }

        void render_thread::wait_for_exit()
        {
            if (!m_thread.is_a_thread()) {
                throw std::logic_error("thread not spawned yet; will never exit");
            }

            // Wait for render thread to acknowledge it has exited.
            {
                threading::lock_guard thread_lock(&m_mutex);
                while (m_from_state.ready_to_run) {
                    if (m_exception) {
                        std::rethrow_exception(m_exception);
                    }

                    m_from_condition_var.wait(&thread_lock);
                }
            }
        }

        void render_thread::wait_for_ready_to_swap()
        {
            if (!m_thread.is_a_thread()) {
                throw std::logic_error("thread not spawned yet; will never be ready to swap");
            }

            // Wait for render thread to acknowledge it has completed pending work.
            {
                threading::lock_guard thread_lock(&m_mutex);
                while (!m_from_state.ready_to_swap) {
                    if (m_exception) {
                        std::rethrow_exception(m_exception);
                    }

                    m_from_condition_var.wait(&thread_lock);
                }
            }
        }

        void render_thread::signal_work()
        {
            threading::lock_guard thread_lock(&m_mutex);
            if (m_exception) {
                std::rethrow_exception(m_exception);
            }

            m_to_state.run_commands = true;
            m_from_state.ready_to_swap = false;
            m_to_condition_var.notify_all();
        }

        void render_thread::signal_exit()
        {
            threading::lock_guard thread_lock(&m_mutex);
            if (m_exception) {
                std::rethrow_exception(m_exception);
            }

            m_to_state.exit_thread = true;
            m_to_condition_var.notify_all();
        }

        void render_thread::join()
        {
            threading::lock_guard thread_lock(&m_mutex);
            m_thread.join();
            m_thread_id = threading::thread_id();
        }

        void render_thread::thread_method()
        {
            ELECTROSLAG_LOG_MESSAGE("render_thread::thread_method");
            try {
                m_thread.set_thread_name(ELECTROSLAG_STRING_AND_HASH("t:graphics"));

                render_policy* rp = get_graphics()->get_render_policy();

                m_thread_id = threading::this_thread::get_id();

                // TODO: Create the context through graphics_interface
                ELECTROSLAG_CHECK(!m_context);
                m_context = new context_opengl();
                m_context->initialize(&m_params);

                m_from_state.ready_to_run = true;
                m_from_state.ready_to_swap = true;

                // No more free ride with synchronization and the graphics object.
                // This unblocks the initializing thread.
                m_from_condition_var.notify_all();

                bool exit_thread = false;
                bool run_commands = false;

                do {
                    {
                        threading::lock_guard thread_lock(&m_mutex);
                        // If the previous pass through the loop resulted in commands being
                        // run, indicate to the calling thread that we are done.
                        if (run_commands) {
                            m_from_state.ready_to_swap = true;
                            m_from_condition_var.notify_all();
                        }

                        // Wait for the controlling thread to tell us to do something.
                        exit_thread = m_to_state.exit_thread;
                        run_commands = m_to_state.run_commands;

                        while (!exit_thread && !run_commands) {
                            // This will drop the state mutex and give other threads a
                            // chance to send us commands and possibly react to the
                            // "ready to swap" above.
                            m_to_condition_var.wait(&thread_lock);

                            exit_thread = m_to_state.exit_thread;
                            run_commands = m_to_state.run_commands;
                        }

                        // Update local state copies before dropping the state mutex.
                        if (run_commands) {
                            m_to_state.run_commands = false;
                        }

                        if (exit_thread) {
                            m_to_state.exit_thread = false;
                        }
                    }

                    if (run_commands) {
                        rp->execute_command_queues(m_context);
                    }
                } while (!exit_thread);

                ELECTROSLAG_LOG_MESSAGE("render_thread::thread_method - exiting");
                rp->execute_system_command_queue(m_context);
                m_context->unbind_all_objects();
            }
            catch (...) {
                m_exception = std::current_exception();
            }

            {
                threading::lock_guard thread_lock(&m_mutex);
                m_from_state.ready_to_run = false;
                m_from_state.ready_to_swap = false;
                m_from_condition_var.notify_all();
            }
            m_context->shutdown();
            delete m_context;
            m_context = 0;
        }
    }
}
