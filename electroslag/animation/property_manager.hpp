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
#include "electroslag/threading/mutex.hpp"
#include "electroslag/animation/property_manager_interface.hpp"

namespace electroslag {
    namespace animation {
        class property_manager : public property_manager_interface {
        public:
            property_manager();
            virtual ~property_manager()
            {}

            // Implement property_manager_intrerface
            virtual void set_controller(animated_object::ref ao, unsigned long long name_hash, property_controller_interface::ref controller);
            virtual void clear_controller(animated_object::ref ao, unsigned long long name_hash);

            void apply_controller_changes();

        private:
            mutable threading::mutex m_mutex;

            enum controller_change_action {
                controller_change_action_unknown = -1,
                controller_change_action_set,
                controller_change_action_clear
            };

            struct pending_controller_change {
                pending_controller_change(
                    controller_change_action new_action,
                    animated_object::ref const& new_ao,
                    unsigned long long new_name_hash,
                    property_controller_interface::ref const& new_controller
                    )
                    : action(new_action)
                    , ao(new_ao)
                    , name_hash(new_name_hash)
                    , controller(new_controller)
                {}

                pending_controller_change(
                    controller_change_action new_action,
                    animated_object::ref const& new_ao,
                    unsigned long long new_name_hash
                    )
                    : action(new_action)
                    , ao(new_ao)
                    , name_hash(new_name_hash)
                {}

                controller_change_action action;
                animated_object::ref ao;
                unsigned long long name_hash;
                property_controller_interface::ref controller;
            };
            typedef std::vector<pending_controller_change> pending_controller_change_vector;
            pending_controller_change_vector m_pending_controller_changes;

            // Disallowed operations:
            explicit property_manager(property_manager const&);
            property_manager& operator =(property_manager const&);
        };

        property_manager* get_property_manager_internal();
    }
}
