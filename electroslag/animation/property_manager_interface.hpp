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
#include "electroslag/animation/animated_object.hpp"
#include "electroslag/animation/property_controller_interface.hpp"

namespace electroslag {
    namespace animation {
        class property_manager_interface {
        public:
            virtual ~property_manager_interface()
            {}

            virtual void set_controller(animated_object::ref ao, unsigned long long name_hash, property_controller_interface::ref controller) = 0;
            virtual void clear_controller(animated_object::ref ao, unsigned long long name_hash) = 0;

        protected:
            property_manager_interface()
            {}

        private:
            // Disallowed operations:
            explicit property_manager_interface(property_manager_interface const&);
            property_manager_interface& operator =(property_manager_interface const&);
        };

        property_manager_interface* get_property_manager();
    }
}
