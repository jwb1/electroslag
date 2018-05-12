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
#include "electroslag/graphics/context_interface.hpp"
#include "electroslag/graphics/shader_program_interface.hpp"
#include "electroslag/renderer/renderer_types.hpp"
#include "electroslag/renderer/field_source_interface.hpp"

namespace electroslag {
    namespace renderer {
        class pipeline_interface
            : public referenced_object
            , public field_source_interface {
        public:
            typedef reference<pipeline_interface> ref;

            virtual ~pipeline_interface()
            {}

            virtual void initialize_step() = 0;

            virtual bool ready() const = 0;

            virtual bool is_transparent() const = 0;

            virtual int get_dynamic_ubo_size() const = 0;

            virtual graphics::shader_program_interface::ref const& get_shader() const = 0;

            virtual void bind_pipeline(
                graphics::context_interface* context,
                frame_details* this_frame_details,
                int dynamic_ubo_base_offset
                ) const = 0;

        protected:
            pipeline_interface()
            {}

        private:
            // Disallowed operations:
            explicit pipeline_interface(pipeline_interface const&);
            pipeline_interface& operator =(pipeline_interface const&);
        };
    }
}
