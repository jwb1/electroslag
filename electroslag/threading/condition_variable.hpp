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
#include "electroslag/threading/mutex.hpp"

namespace electroslag {
    namespace threading {
        class condition_variable : public named_object {
        public:
            condition_variable();
            explicit condition_variable(unsigned long long name_hash);
            explicit condition_variable(std::string const& name);
            condition_variable(std::string const& name, unsigned long long name_hash);
            virtual ~condition_variable()
            {}

            void wait(mutex* m = 0);
            void wait(lock_guard* lock)
            {
                wait(lock->m_mutex);
            }

            void notify_one();
            void notify_all();

        private:
            CONDITION_VARIABLE m_cv;

            // Disallowed operations:
            explicit condition_variable(condition_variable const&);
            condition_variable& operator =(condition_variable const&);
        };
    }
}
