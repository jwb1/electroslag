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
#include "electroslag/threading/thread_local_map.hpp"

namespace electroslag {
    namespace threading {
        template<class T>
        class thread_local_ptr {
            ELECTROSLAG_STATIC_CHECK((sizeof(T*) == sizeof(void*)), "thread_local_ptr pointer size mismatch");
        public:
            thread_local_ptr()
            {
                m_key = thread_local_map::get()->generate_key();
            }

            explicit thread_local_ptr(T* initial_value)
            {
                m_key = thread_local_map::get()->generate_key();
                reset(initial_value);
            }

            ~thread_local_ptr() // Not virtual; not intended to be inherited from.
            {
                thread_local_map::get()->retire_key(m_key);
            }

            T const* get() const
            {
                return (reinterpret_cast<T const*>(thread_local_map::get()->get_value(m_key)));
            }

            T* get()
            {
                return (reinterpret_cast<T*>(thread_local_map::get()->get_value(m_key)));
            }

            void reset(T* new_value)
            {
                thread_local_map::get()->set_value(m_key, new_value);
            }

        private:
            unsigned int m_key;

            // Disallowed operations:
            explicit thread_local_ptr(thread_local_ptr const&);
            thread_local_ptr& operator =(thread_local_ptr const&);
        };
    }
}
