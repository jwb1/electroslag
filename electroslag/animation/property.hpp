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
        template<class T, unsigned long long NameHash>
        class property {
        public:
            typedef T value_type;
            typedef property_controller_typed_interface<T> controller_type;
            typedef typename property_controller_typed_interface<T>::ref controller_ref;

            property()
            {}

            explicit property(T const& initial_value)
                : m_value(initial_value)
            {}

            static unsigned long long get_name_hash()
            {
                return (NameHash);
            }

            void const* get_read_pointer() const
            {
                return (&m_value);
            }

            T const& get_value() const
            {
                return (m_value);
            }

            T& get_value()
            {
                return (m_value);
            }

            void reset_value(T const& reset_value)
            {
                m_value = reset_value;
                clear_controller();
            }

            property_value_state update(int millisec_elapsed)
            {
                if (m_controller.is_valid()) {
                    return (m_controller->update_value(millisec_elapsed, &m_value));
                }
                else {
                    return (property_value_state_no_change);
                }
            }

            void clear_controller()
            {
                m_controller.reset();
            }

            void set_controller(property_controller_interface::ref& controller)
            {
                m_controller = controller.cast<controller_type>();
            }

        private:
            value_type m_value;
            controller_ref m_controller;

            // Disallowed operations:
            explicit property(property const&);
            property& operator =(property const&);
        };

        template<unsigned long long NameHash>
        class float_property : public property<float, NameHash> {
        public:
            float_property()
            {}

            explicit float_property(float initial_value)
                : property(initial_value)
            {}
        };

        template<unsigned long long NameHash>
        class vec2_property : public property<glm::f32vec2, NameHash> {
        public:
            vec2_property()
            {}

            explicit vec2_property(glm::f32vec2 const& initial_value)
                : property(initial_value)
            {}
        };

        template<unsigned long long NameHash>
        class vec3_property : public property<glm::f32vec3, NameHash> {
        public:
            vec3_property()
            {}

            explicit vec3_property(glm::f32vec3 const& initial_value)
                : property(initial_value)
            {}
        };

        template<unsigned long long NameHash>
        class vec4_property : public property<glm::f32vec4, NameHash> {
        public:
            vec4_property()
            {}

            explicit vec4_property(glm::f32vec4 const& initial_value)
                : property(initial_value)
            {}
        };

        template<unsigned long long NameHash>
        class quat_property : public property<glm::f32quat, NameHash> {
        public:
            quat_property()
            {}

            explicit quat_property(glm::f32quat const& initial_value)
                : property(initial_value)
            {}
        };

        template<unsigned long long NameHash>
        class mat4_property : public property<glm::f32mat4, NameHash> {
        public:
            mat4_property()
            {}

            explicit mat4_property(glm::f32mat4 const& initial_value)
                : property(initial_value)
            {}
        };
    }
}
