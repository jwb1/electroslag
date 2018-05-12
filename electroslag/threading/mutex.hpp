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

namespace electroslag {
    namespace threading {
        class mutex : public named_object {
        public:
            mutex();
            explicit mutex(unsigned long long name_hash);
            explicit mutex(std::string const& name);
            mutex(std::string const& name, unsigned long long name_hash);
            ~mutex(); // Not intended to be inherited from

            void check_holding() const;
            void check_not_holding() const;

            void lock();
            void unlock();

        private:
            CRITICAL_SECTION m_cs;

#if !defined(ELECTROSLAG_BUILD_SHIP)
            thread_id m_mutex_holder;
            int m_recursion_count;
#endif
            // Disallowed operations:
            explicit mutex(mutex const&);
            mutex& operator =(mutex const&);

            // The condition variable has to access the underlying critical section.
            friend class condition_variable;
        };

        class lock_guard {
        public:
            explicit lock_guard(mutex* m)
                : m_mutex(m)
            {
                m_mutex->lock();
            }

            ~lock_guard()
            {
                m_mutex->unlock();
            }

        private:
            mutex* m_mutex;

            // Disallowed operations:
            lock_guard();
            explicit lock_guard(lock_guard const&);
            lock_guard& operator =(lock_guard const&);

            // The condition variable has to do manual mutex lock/unlock which normally the
            // lock_guard does not allow.
            friend class condition_variable;
        };
    }
}
