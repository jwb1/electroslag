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
#include "electroslag/threading/work_item_interface.hpp"

namespace electroslag {
    namespace threading {
        template<class T>
        class future_interface : public work_item_interface {
        public:
            typedef T value_type;
            typedef reference<future_interface> ref;

            virtual ~future_interface()
            {}

            T get() const
            {
                return (m_return_value);
            }

            T get_wait() const
            {
                wait_for_done();
                return (m_return_value);
            }

            virtual T execute_for_value() = 0;

        protected:
            future_interface()
            {}

            template<class T>
            explicit future_interface(
                reference<T>& dependency
                )
                : work_item_interface(dependency)
            {}

            explicit future_interface(
                work_item_vector& dependencies
                )
                : work_item_interface(dependencies)
            {}

            virtual void execute()
            {
                m_return_value = execute_for_value();
            }

        private:
            T m_return_value;

            // Disallowed operations:
            explicit future_interface(future_interface const&);
            future_interface& operator =(future_interface const&);
        };
    }
}
