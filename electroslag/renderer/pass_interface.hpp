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
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/renderer/field_source_interface.hpp"
#include "electroslag/renderer/mesh_interface.hpp"
#include "electroslag/renderer/scene.hpp"
#include "electroslag/renderer/camera.hpp"
#include "electroslag/renderer/pipeline_descriptor.hpp"

namespace electroslag {
    namespace renderer {
        class renderer;
        class pass_interface
            : public referenced_object
            , public field_source_interface {
        public:
            typedef reference<pass_interface> ref;

            virtual ~pass_interface()
            {}

            virtual pass_type get_pass_type() const = 0;
            virtual pipeline_type get_pipeline_type() const = 0;

            // Each pass need to derive from this for a return value for it's enqueue_render_pass override.
            class pass_render_work_item : public frame_work_item {
            public:
                typedef reference<pass_render_work_item> ref;

                pass_render_work_item(
                    pass_interface* this_pass,
                    frame_details* this_frame_details
                    )
                    : m_this_pass(this_pass)
                    , m_this_frame_details(this_frame_details)
                {}

                virtual void execute() = 0;

            protected:
                pass_interface* m_this_pass;
                frame_details* m_this_frame_details;

            private:
                // Disallowed operations:
                pass_render_work_item();
                explicit pass_render_work_item(pass_render_work_item const&);
                pass_render_work_item& operator =(pass_render_work_item const&);
            };

            // Place a work item in the thread pool for generating draw commands that
            // are not done on a per-mesh basis.
            virtual pass_render_work_item::ref make_render_work_item(
                frame_details* this_frame_details
                ) = 0;

            // Called by each mesh from a thread pool work item to render themselves
            // in this pass.
            virtual void render_mesh_in_pass(mesh_interface::ref& mesh, frame_details* this_frame_details) = 0;

        protected:
            pass_interface()
            {}

        private:
            // Disallowed operations:
            explicit pass_interface(pass_interface const&);
            pass_interface& operator =(pass_interface const&);
        };
    }
}
