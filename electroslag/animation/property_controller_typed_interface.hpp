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
        enum property_value_state {
            property_value_state_unknown = -1,
            property_value_state_no_change,
            property_value_state_changed
        };

        template<class T>
        class property_controller_typed_interface : public property_controller_interface {
        public:
            typedef reference<property_controller_typed_interface> ref;
            typedef T value_type;

            virtual ~property_controller_typed_interface()
            {}

            virtual property_value_state update_value(int millisec_elapsed, value_type* in_out_value) = 0;

        protected:
            property_controller_typed_interface()
            {}

        private:
            // Disallowed operations:
            explicit property_controller_typed_interface(property_controller_typed_interface const&);
            property_controller_typed_interface& operator =(property_controller_typed_interface const&);
        };
    }
}
