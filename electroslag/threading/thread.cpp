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
#include "electroslag/threading/thread.hpp"

namespace electroslag {
    namespace threading {
        // static
        int thread::hardware_concurrency()
        {
            SYSTEM_INFO si = {0};
            GetSystemInfo(&si);
            return (static_cast<int>(si.dwNumberOfProcessors));
        }

        // static
        HANDLE const thread::not_a_thread_handle = 0;

        thread::thread()
            : m_handle(not_a_thread_handle)
            , m_thread_id(0)
            , m_wrapper_thread(false)
        {}

        thread::thread(unsigned int wrap_thread_id, HANDLE wrap_thread_handle)
            : m_handle(wrap_thread_handle)
            , m_thread_id(wrap_thread_id)
            , m_wrapper_thread(true)
        {}

        thread::~thread()
        {}

        void thread::spawn(
            thread_entry_point entry_point,
            void* argument
            )
        {
            lock_guard thread_mutex(&m_mutex);
            ELECTROSLAG_CHECK(!is_a_thread());

            std::unique_ptr<entry_point_wrapper_param> param(new entry_point_wrapper_param());
            param->entry_point = entry_point;
            param->argument = argument;
            param->this_thread = this;

            m_handle = reinterpret_cast<HANDLE>(_beginthreadex(
                0,
                0,
                entry_point_wrapper,
                reinterpret_cast<void*>(param.get()),
                0,
                &m_thread_id
                ));
            if (!m_handle) {
                throw win32_api_failure("_beginthreadex");
            }

            // The thread now owns the parameter block.
            param.release();
        }

        bool thread::is_a_thread() const
        {
            lock_guard thread_mutex(&m_mutex);
            return (m_handle != not_a_thread_handle);
        }

        bool thread::is_wrapper_thread() const
        {
            // Don't need the mutex: this field is not changed after object creation.
            return (m_wrapper_thread);
        }

        void thread::join()
        {
            lock_guard thread_mutex(&m_mutex);
            if (is_a_thread()) {
                WaitForSingleObject(m_handle, INFINITE);
                CloseHandle(m_handle);
                m_handle = not_a_thread_handle;
                m_thread_id = 0;
            }
        }

        thread_id thread::get_id() const
        {
            lock_guard thread_mutex(&m_mutex);
            if (is_a_thread()) {
                return (thread_id(m_thread_id));
            }
            else {
                return (thread_id());
            }
        }

        void thread::set_thread_name(unsigned long long name_hash)
        {
            set_hash(name_hash);
        }

        void thread::set_thread_name(std::string const& name)
        {
            set_name(name);
#if !defined(ELECTROSLAG_BUILD_SHIP)
            set_name_in_debugger(m_thread_id, name.c_str());
#endif
            m_mutex.set_name("m:" + name);
        }

        void thread::set_thread_name(std::string const& name, unsigned long long name_hash)
        {
            set_name_and_hash(name, name_hash);
#if !defined(ELECTROSLAG_BUILD_SHIP)
            set_name_in_debugger(m_thread_id, name);
#endif
            m_mutex.set_name("m:" + name);
        }

        // static
        unsigned int WINAPI thread::entry_point_wrapper(void* argument)
        {
            std::unique_ptr<entry_point_wrapper_param> param(reinterpret_cast<entry_point_wrapper_param*>(argument));
            this_thread::set(param->this_thread);
            param->entry_point(param->argument);
            this_thread::cleanup_on_exit();
            return (0);
        }

#if !defined(ELECTROSLAG_BUILD_SHIP)
        void thread::set_name_in_debugger(unsigned int id, std::string const& name) throw()
        {
            if (IsDebuggerPresent()) {
                THREADNAME_INFO info;
                info.dwType = 0x1000;
                info.szName = name.c_str();
                info.dwThreadID = id;
                info.dwFlags = 0;

                __try {
                    RaiseException(
                        visual_cpp_exception,
                        0,
                        sizeof(THREADNAME_INFO) / sizeof(ULONG_PTR),
                        reinterpret_cast<ULONG_PTR*>(&info)
                        );
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {}
            }
        }
#endif
    }
}
