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
#include "electroslag/threading/mutex.hpp"

namespace electroslag {
    namespace threading {
        mutex::mutex()
#if !defined(ELECTROSLAG_BUILD_SHIP)
            : m_recursion_count(0)
#endif
        {
            InitializeCriticalSection(&m_cs);
        }

        mutex::mutex(unsigned long long name_hash)
#if !defined(ELECTROSLAG_BUILD_SHIP)
            : m_recursion_count(0)
#endif
        {
            InitializeCriticalSection(&m_cs);

            // Doing this here as opposed to with the named_object constructor ensures
            // that the mutex in name_table can be entered.
            set_hash(name_hash);
        }

        mutex::mutex(std::string const& name)
#if !defined(ELECTROSLAG_BUILD_SHIP)
            : m_recursion_count(0)
#endif
        {
            InitializeCriticalSection(&m_cs);

            // Doing this here as opposed to with the named_object constructor ensures
            // that the mutex in name_table can be entered.
            set_name(name);
        }

        mutex::mutex(std::string const& name, unsigned long long name_hash)
#if !defined(ELECTROSLAG_BUILD_SHIP)
            : m_recursion_count(0)
#endif
        {
            InitializeCriticalSection(&m_cs);

            // Doing this here as opposed to with the named_object constructor ensures
            // that the mutex in name_table can be entered.
            set_name_and_hash(name, name_hash);
        }

        mutex::~mutex() // Not intended to be inherited from
        {
            DeleteCriticalSection(&m_cs);
        }

        void mutex::check_holding() const
        {
            ELECTROSLAG_CHECK(this_thread::get_id() == m_mutex_holder);
        }

        void mutex::check_not_holding() const
        {
            ELECTROSLAG_CHECK(this_thread::get_id() != m_mutex_holder);
        }

        void mutex::lock()
        {
            EnterCriticalSection(&m_cs);

#if !defined(ELECTROSLAG_BUILD_SHIP)
            if (++m_recursion_count == 1) {
                m_mutex_holder = this_thread::get_id();
            }
#endif
        }

        void mutex::unlock()
        {
#if !defined(ELECTROSLAG_BUILD_SHIP)
            if (m_recursion_count-- == 1) {
                m_mutex_holder = this_thread::get_id();
            }
#endif

            LeaveCriticalSection(&m_cs);
        }
    }
}
