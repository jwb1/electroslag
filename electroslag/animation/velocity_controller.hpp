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
#include "electroslag/animation/property_controller_typed_interface.hpp"

namespace electroslag {
    namespace animation {
        template<class T>
        class velocity_controller : public property_controller_typed_interface<T> {
        public:
            static property_controller_interface::ref create(value_type const& velocity)
            {
                return (property_controller_interface::ref(new velocity_controller(velocity)));
            }

            virtual ~velocity_controller()
            {}

            virtual property_value_state update_value(int millisec_elapsed, value_type* in_out_value)
            {
                if (millisec_elapsed > 0) {
                    *in_out_value = *in_out_value + (m_velocity * millisec_elapsed);
                    return (property_value_state_changed);
                }
                else {
                    return (property_value_state_no_change);
                }
            }

        protected:
            explicit velocity_controller(value_type const& velocity)
                : m_velocity(velocity)
            {}

        private:
            value_type m_velocity;

            // Disallowed operations:
            velocity_controller();
            explicit velocity_controller(velocity_controller const&);
            velocity_controller& operator =(velocity_controller const&);
        };
    }
}
