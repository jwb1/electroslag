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
#include "electroslag/threading/condition_variable.hpp"

namespace electroslag {
    namespace threading {
        condition_variable::condition_variable()
        {
            InitializeConditionVariable(&m_cv);
        }

        condition_variable::condition_variable(unsigned long long name_hash)
            : named_object(name_hash)
        {
            InitializeConditionVariable(&m_cv);
        }

        condition_variable::condition_variable(std::string const& name)
            : named_object(name)
        {
            InitializeConditionVariable(&m_cv);
        }

        condition_variable::condition_variable(std::string const& name, unsigned long long name_hash)
            : named_object(name, name_hash)
        {
            InitializeConditionVariable(&m_cv);
        }

        void condition_variable::wait(mutex* m)
        {
            DWORD wait_milliseconds = 30 * 1000; // 30 seconds is a hang.
            // Extend thread timeouts while debugging.
            if (being_debugged()) {
                wait_milliseconds = 60 * 60 * 1000; // 1 hour
            }

            if (!SleepConditionVariableCS(&m_cv, &m->m_cs, wait_milliseconds)) {
                if (GetLastError() == ERROR_TIMEOUT) {
                    throw timeout_failure("condition_variable wait timed out");
                }
                else {
                    throw win32_api_failure("SleepConditionVariableCS");
                }
            }
        }

        void condition_variable::notify_one()
        {
            WakeConditionVariable(&m_cv);
        }

        void condition_variable::notify_all()
        {
            WakeAllConditionVariable(&m_cv);
        }
    }
}
