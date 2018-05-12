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
#include "electroslag/event.hpp"
#include "electroslag/renderer/renderer_types.hpp"
#include "electroslag/renderer/instance_descriptor.hpp"
#include "electroslag/renderer/scene.hpp"

namespace electroslag {
    namespace renderer {
        class renderer_interface {
        public:
            virtual ~renderer_interface()
            {}

            virtual void initialize() = 0;
            virtual void shutdown() = 0;

            virtual bool is_initialized() const = 0;

            void check_not_initialized() const
            {
                ELECTROSLAG_CHECK(!is_initialized());
            }

            void check_initialized() const
            {
                ELECTROSLAG_CHECK(is_initialized());
            }

            virtual scene::ref create_instances(instance_descriptor::ref const& desc) = 0;

            typedef event<void> destroyed_event;
            typedef destroyed_event::bound_delegate destroyed_delegate;
            mutable destroyed_event destroyed;

        protected:
            renderer_interface()
            {}

        private:
            // Disallowed operations:
            explicit renderer_interface(renderer_interface const&);
            renderer_interface& operator =(renderer_interface const&);
        };

        renderer_interface* get_renderer();
    }
}
