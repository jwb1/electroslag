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
#include "electroslag/systems.hpp"
#include "electroslag/animation/property_manager.hpp"

namespace electroslag {
    namespace animation {
        property_manager* get_property_manager_internal()
        {
            return (get_systems()->get_property_manager());
        }

        property_manager_interface* get_property_manager()
        {
            return (get_property_manager_internal());
        }

        property_manager::property_manager()
            : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:property_manager"))
        {}

        void property_manager::set_controller(animated_object::ref ao, unsigned long long name_hash, property_controller_interface::ref controller)
        {
            threading::lock_guard property_manager_lock(&m_mutex);
            m_pending_controller_changes.emplace_back(controller_change_action_set, ao, name_hash, controller);
        }

        void property_manager::clear_controller(animated_object::ref ao, unsigned long long name_hash)
        {
            threading::lock_guard property_manager_lock(&m_mutex);
            m_pending_controller_changes.emplace_back(controller_change_action_clear, ao, name_hash);
        }

        void property_manager::apply_controller_changes()
        {
            // Applying all controller updates for a frame together reduces mutex traffic.
            threading::lock_guard property_manager_lock(&m_mutex);
            pending_controller_change_vector::iterator i(m_pending_controller_changes.begin());
            while (i != m_pending_controller_changes.end()) {
                switch (i->action) {
                case controller_change_action_set:
                    i->ao->set_controller(i->name_hash, i->controller);
                    break;

                case controller_change_action_clear:
                    i->ao->clear_controller(i->name_hash);
                    break;

                default:
                    throw std::runtime_error("Invalid property controller action");
                }
                ++i;
            }
            m_pending_controller_changes.clear();
        }
    }
}
