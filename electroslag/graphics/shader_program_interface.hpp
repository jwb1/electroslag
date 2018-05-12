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
#include "electroslag/graphics/shader_program_descriptor.hpp"

namespace electroslag {
    namespace graphics {
        class shader_program_interface : public referenced_object {
        public:
            typedef reference<shader_program_interface> ref;

            // Are async operations related to shader creation done?
            virtual bool is_finished() const = 0;

            // The shader program keeps a copy of it's descriptor, including extra
            // information that is discovered at run time.
            virtual shader_program_descriptor::ref const& get_descriptor() const = 0;
        };
    }
}
