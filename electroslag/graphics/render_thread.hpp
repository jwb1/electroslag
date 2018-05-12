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
#include "electroslag/threading/this_thread.hpp"
#include "electroslag/threading/thread.hpp"
#include "electroslag/threading/mutex.hpp"
#include "electroslag/threading/condition_variable.hpp"
#include "electroslag/graphics/context_interface.hpp"
#include "electroslag/graphics/graphics_debugger_interface.hpp"

namespace electroslag {
    namespace graphics {
        class render_thread {
        public:
            render_thread()
                : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:render_thread"))
                , m_to_condition_var(ELECTROSLAG_STRING_AND_HASH("cv:to_render_thread"))
                , m_from_condition_var(ELECTROSLAG_STRING_AND_HASH("cv:from_render_thread"))
                , m_context(0)
            {}

            ~render_thread()
            {
                join();
            }

            bool is_running() const
            {
                return (threading::this_thread::get_id() == m_thread_id);
            }

            void check() const
            {
                ELECTROSLAG_CHECK(threading::this_thread::get_id() == m_thread_id);
            }

            void check_not() const
            {
                ELECTROSLAG_CHECK(threading::this_thread::get_id() != m_thread_id);
            }

            void spawn(graphics_initialize_params const* params);

            bool is_started() const;

            void wait_for_ready();
            void wait_for_exit();
            void wait_for_ready_to_swap();

            void signal_work();
            void signal_exit();

            void join();

            context_capability_interface* get_context_capability()
            {
                return (m_context);
            }

            graphics_debugger_interface* get_graphics_debugger()
            {
                return (m_context);
            }

        private:
            static void thread_stub(void* argument)
            {
                reinterpret_cast<render_thread*>(argument)->thread_method();
            }

            void thread_method();

            graphics_initialize_params m_params;

            threading::thread m_thread;
            threading::thread_id m_thread_id;

            mutable threading::mutex m_mutex;

            // State changes the render thread listens to.
            threading::condition_variable m_to_condition_var;
            struct to_thread_state {
                to_thread_state()
                    : exit_thread(false)
                    , run_commands(false)
                {}

                bool exit_thread:1;
                bool run_commands:1;
            } m_to_state;
            ELECTROSLAG_STATIC_CHECK(sizeof(to_thread_state) == 1, "Bit packing check");

            // State changes precipitated by the render thread
            threading::condition_variable m_from_condition_var;
            std::exception_ptr m_exception;

            struct from_thread_state {
                from_thread_state()
                    : ready_to_run(false)
                    , ready_to_swap(false)
                {}

                bool ready_to_run:1;
                bool ready_to_swap:1;
            } m_from_state;
            ELECTROSLAG_STATIC_CHECK(sizeof(from_thread_state) == 1, "Bit packing check");

            // Private state to the render thread.
            context_interface* m_context;

            // Disallowed operations:
            explicit render_thread(render_thread const&);
            render_thread& operator =(render_thread const&);
        };
    }
}
