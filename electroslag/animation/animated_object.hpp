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
#include "electroslag/animation/property_controller_interface.hpp"

namespace electroslag {
    namespace animation {
        class animated_object : public referenced_object {
        public:
            typedef reference<animated_object> ref;

            virtual ~animated_object()
            {}

            virtual void set_controller(unsigned long long name_hash, property_controller_interface::ref& controller) = 0;
            virtual void clear_controller(unsigned long long name_hash) = 0;

        protected:
            animated_object()
            {}

        private:
            // Disallowed operations:
            explicit animated_object(animated_object const&);
            animated_object& operator =(animated_object const&);
        };
    }
}
