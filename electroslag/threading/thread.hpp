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
#include "electroslag/named_object.hpp"
#include "electroslag/threading/this_thread.hpp"
#include "electroslag/threading/mutex.hpp"

namespace electroslag {
    namespace threading {
        class thread : private named_object {
        public:
            typedef void(*thread_entry_point)(void*);

            struct entry_point_wrapper_param {
                entry_point_wrapper_param()
                    : entry_point(0)
                    , argument(0)
                    , this_thread(0)
                {}

                thread_entry_point entry_point;
                void* argument;
                thread* this_thread;
            };

            static int hardware_concurrency();

            thread(); // Constructor to create "not-a-thread"
            thread(unsigned int wrap_thread_id, HANDLE wrap_thread_handle);
            ~thread(); // Not intended to be inherited from

            bool is_a_thread() const;
            bool is_wrapper_thread() const;
            thread_id get_id() const;

            void spawn(thread_entry_point entry_point, void* argument);
            void join();

            bool has_thread_name() const
            {
                return (has_name_string());
            }

            void set_thread_name(unsigned long long name_hash);
            void set_thread_name(std::string const& name);
            void set_thread_name(std::string const& name, unsigned long long name_hash);

            std::string get_thread_name() const
            {
                return (get_name());
            }

        private:
            mutable mutex m_mutex;

            static unsigned int WINAPI entry_point_wrapper(void* argument);

            static HANDLE const not_a_thread_handle;
            HANDLE m_handle;
            unsigned int m_thread_id;
            bool m_wrapper_thread;

            // Set the thread name in the debugger.
            // This "feature" seems to be only supported by Microsoft's
            // debugger, so it probably should only be attempted when using
            // Microsoft's compilers.

            // Logic from here:
            // http://msdn.microsoft.com/en-us/library/xcb2z8hs(VS.90).aspx

#if !defined(ELECTROSLAG_BUILD_SHIP)
            const static DWORD visual_cpp_exception = 0x406D1388;

#pragma pack(push,8)
            struct THREADNAME_INFO {
                DWORD dwType; // Must be 0x1000.
                LPCSTR szName; // Pointer to name (in user addr space).
                DWORD dwThreadID; // Thread ID (-1=caller thread).
                DWORD dwFlags; // Reserved for future use, must be zero.
            };
#pragma pack(pop)

            static void set_name_in_debugger(
                unsigned int id,
                std::string const& name
                ) throw();
#endif

            // Disallowed operations:
            explicit thread(thread const&);
            thread& operator =(thread const&);
        };
    }
}
