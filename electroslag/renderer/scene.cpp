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
#include "electroslag/threading/thread_pool.hpp"
#include "electroslag/renderer/scene.hpp"
#include "electroslag/renderer/static_mesh.hpp"
#include "electroslag/renderer/camera.hpp"

namespace electroslag {
    namespace renderer {
        scene::scene(instance_descriptor::ref const& scene_desc)
            : m_visible(false)
        {
            load_instance(scene_desc, transform_desciptor::ref::null_ref);

            m_mesh_transforms.resize(m_meshes.size());
            m_camera_transforms.resize(m_cameras.size());
        }

        void scene::load_instance(
            instance_descriptor::ref const& instance_desc,
            transform_descriptor::ref const& parent_transform_desc
            )
        {
            int component_bits = r->descriptor->get_component_bits();
            if (static_mesh::supported_component_bits(component_bits)) {
                m_meshes.emplace_back(static_mesh::create(
                    r->descriptor,
                    r->transform,
                    r->name_hash
                    ).cast<mesh_interface>());
            }
            else if (camera::supported_component_bits(component_bits)) {
                m_cameras.emplace_back(camera::create(
                    r->descriptor,
                    r->transform,
                    r->name_hash
                    ));
            }
            else {
                throw load_object_failure("component bits");
            }
        }

        void scene::show()
        {
            m_visible.store(true);
        }

        void scene::hide()
        {
            m_visible.store(false);
        }

        void scene::make_transform_work_item(frame_details* this_frame_details)
        {
            m_camera_transforms.clear();
            camera_vector::iterator c(m_cameras.begin());
            while (c != m_cameras.end()) {
                m_camera_transforms.emplace_back((*c)->make_transform_work_item(
                    this_frame_details
                    ).cast<frame_work_item>());
                ++c;
            }

            m_mesh_transforms.clear();
            mesh_iterator m(m_meshes.begin());
            while (m != m_meshes.end()) {
                m_mesh_transforms.emplace_back((*m)->make_transform_work_item(
                    this_frame_details
                    ).cast<frame_work_item>());

                this_frame_details->total_meshes++;

                ++m;
            }
        }

        void scene::make_render_work_item(frame_details* this_frame_details)
        {
            if (m_visible.load()) {
                for (int m = 0; m < m_meshes.size(); ++m) {
                    m_meshes[m]->make_render_work_item(m_mesh_transforms[m], this_frame_details);
                }
            }
        }
    }
}
