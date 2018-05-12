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
#include "electroslag/named_object.hpp"
#include "electroslag/graphics/graphics_types.hpp"

namespace electroslag {
    namespace graphics {
        class render_policy;

        class command_queue_interface
            : public referenced_object
            , public named_object {
        public:
            typedef reference<command_queue_interface> ref;

            template<class T, class... Params>
            void enqueue_command(
                Params... params
                )
            {
                ELECTROSLAG_STATIC_CHECK(
                    (std::is_base_of<command, T>::value),
                    "Can only enqueue classes derived from command base class"
                    );
                new (get_command_memory(sizeof(T), alignof(T))) T(params...);
            }

        protected:
            explicit command_queue_interface(unsigned long long name_hash)
                : named_object(name_hash)
            {}

            explicit command_queue_interface(std::string const& name)
                : named_object(name)
            {}

            command_queue_interface(std::string const& name, unsigned long long name_hash)
                : named_object(name, name_hash)
            {}

            virtual void* get_command_memory(int bytes, int alignment) = 0;
            virtual void execute_commands(context_interface* context) = 0;
            virtual void swap() = 0;

        private:
            // Disallowed operations:
            command_queue_interface();
            explicit command_queue_interface(command_queue_interface const&);
            command_queue_interface& operator =(command_queue_interface const&);

            friend class render_policy;
        };
    }
}
