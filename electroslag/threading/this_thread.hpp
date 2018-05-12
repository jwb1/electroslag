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

namespace electroslag {
    namespace threading {
        class thread_id {
        public:
            thread_id()  // Id of "not-a-thread"
#if defined(_WIN32)
                : m_id(not_a_thread_id)
#else
                : m_is_a_thread(false)
#endif
            {}

#if defined(_WIN32)
            explicit thread_id(unsigned int id)
                : m_id(id)
            {}
#else
            explicit thread_id(pthread_t handle)
                thread_id::thread_id(pthread_t handle)
                : m_handle(handle)
                , m_is_a_thread(true)
            {}
#endif
            thread_id(thread_id const& copy_id)
#if defined(_WIN32)
                : m_id(copy_id.m_id)
#else
                : m_handle(copy_id.m_handle)
                , m_is_a_thread(copy_id.m_is_a_thread)
#endif
            {}

            thread_id& operator =(thread_id const& copy_id)
            {
                if (&copy_id != this) {
#if defined(_WIN32)
                    m_id = copy_id.m_id;
#else
                    m_handle = copy_id.m_handle;
                    m_is_a_thread = copy_id.m_is_a_thread;
#endif
                }
                return (*this);
            }

            bool operator ==(thread_id const& compare_with) const
            {
#if defined(_WIN32)
                return (m_id == compare_with.m_id);
#else
                if (m_is_a_thread && compare_with.m_is_a_thread) {
                    return (pthread_equal(m_handle, compare_with.m_handle) ? true : false);
                }
                else if (!m_is_a_thread && !compare_with.m_is_a_thread) {
                    return (true);
                }
                else {
                    return (false);
                }
#endif
            }

            bool operator !=(thread_id const& compare_with) const
            {
                return (!((*this) == compare_with));
            }

        private:
#if defined(_WIN32)
            static unsigned int const not_a_thread_id = 0;
            unsigned int m_id;
#else
            pthread_t m_handle;
            bool m_is_a_thread;
#endif
        };

        class thread;
        class this_thread {
        public:
            static thread* get();
            static void set(thread* t);
            static void cleanup_on_exit();

            static thread_id get_id()
            {
#if defined(_WIN32)
                return (thread_id(GetCurrentThreadId()));
#else
                return (thread_id(pthread_self()));
#endif
            }

            static void sleep_for(int milliseconds)
            {
#if defined(_WIN32)
                Sleep(milliseconds);
#else
                usleep(milliseconds * 1000);
#endif
            }

        private:
            // Disallowed operations:
            this_thread();
            ~this_thread();
            explicit this_thread(this_thread const&);
            this_thread& operator =(this_thread const&);
        };
    }
}
