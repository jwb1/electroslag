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
#include "electroslag/threading/condition_variable.hpp"

namespace electroslag {
    namespace graphics {
        class sync_interface
            : public threading::condition_variable
            , public referenced_object {
        public:
            typedef reference<sync_interface> ref;

            virtual ~sync_interface()
            {}

            virtual bool is_clear() const = 0;
            virtual bool is_set() const = 0;
            virtual bool is_signaled() const = 0;
            virtual void clear() = 0;

        protected:
            sync_interface()
            {}

        private:
            // Disallowed operations:
            explicit sync_interface(sync_interface const&);
            sync_interface& operator =(sync_interface const&);
        };
    }
}
