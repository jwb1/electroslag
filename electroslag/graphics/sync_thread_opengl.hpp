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
#include "electroslag/graphics/sync_interface.hpp"
#include "electroslag/graphics/sub_context_opengl.hpp"

namespace electroslag {
    namespace graphics {
        class sync_thread_opengl {
        public:
            sync_thread_opengl()
                : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:sync_thread"))
                , m_to_condition_var(ELECTROSLAG_STRING_AND_HASH("cv:to_sync_thread"))
                , m_from_condition_var(ELECTROSLAG_STRING_AND_HASH("cv:from_sync_thread"))
            {}

            ~sync_thread_opengl()
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

            bool is_started() const;
            void spawn();
            void wait_for_ready();

            void signal_exit();
            void wait_for_exit();
            void join();

            void wait_for_sync(sync_interface::ref& s);

        private:
            static void thread_stub(void* argument)
            {
                reinterpret_cast<sync_thread_opengl*>(argument)->thread_method();
            }

            void thread_method();
            bool wait_for_next_sync();

            threading::thread m_thread;
            threading::thread_id m_thread_id;

            mutable threading::mutex m_mutex;

            typedef std::deque<sync_interface::ref> sync_wait_deque;
            sync_wait_deque m_sync_wait;

            // State changes the sync thread listens to.
            threading::condition_variable m_to_condition_var;
            struct to_thread_state {
                to_thread_state()
                    : exit_thread(false)
                    , object_to_wait_on(false)
                {}

                bool exit_thread:1;
                bool object_to_wait_on:1;
            } m_to_state;
            ELECTROSLAG_STATIC_CHECK(sizeof(to_thread_state) == 1, "Bit packing check");

            // State changes precipitated by the sync thread
            threading::condition_variable m_from_condition_var;
            std::exception_ptr m_exception;

            struct from_thread_state {
                from_thread_state()
                    : ready(false)
                {}

                bool ready:1;
            } m_from_state;
            ELECTROSLAG_STATIC_CHECK(sizeof(from_thread_state) == 1, "Bit packing check");

            // Private state to the sync thread.
            sub_context_opengl m_sub_context;

            // Disallowed operations:
            explicit sync_thread_opengl(sync_thread_opengl const&);
            sync_thread_opengl& operator =(sync_thread_opengl const&);
        };
    }
}
