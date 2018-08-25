//  Electroslag Interactive1 Graphics System
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
#include "electroslag/renderer/renderer_types.hpp"
#include "electroslag/renderer/instance_descriptor.hpp"
#include "electroslag/renderer/mesh_interface.hpp"
#include "electroslag/renderer/camera.hpp"

namespace electroslag {
    namespace renderer {
        class scene : public referenced_object {
        public:
            typedef reference<scene> ref;
        private:
            typedef std::vector<mesh_interface::ref> mesh_vector;
            typedef std::vector<camera::ref> camera_vector;
        public:
            typedef mesh_vector::const_iterator const_mesh_iterator;
            typedef mesh_vector::iterator mesh_iterator;

            typedef camera_vector::const_iterator const_camera_iterator;
            typedef camera_vector::iterator camera_iterator;

            virtual ~scene()
            {}

            // scene methods
            void show();
            void hide();

            // mesh access
            mesh_interface::ref const& find_mesh(unsigned long long name_hash) const
            {
                mesh_vector::const_iterator i(m_meshes.begin());
                while (i != m_meshes.end()) {
                    if ((*i)->get_hash() == name_hash) {
                        return (*i);
                    }
                    ++i;
                }
                throw object_not_found_failure("mesh_interface", name_hash);
            }

            mesh_interface::ref& find_mesh(unsigned long long name_hash)
            {
                mesh_vector::iterator i(m_meshes.begin());
                while (i != m_meshes.end()) {
                    if ((*i)->get_hash() == name_hash) {
                        return (*i);
                    }
                    ++i;
                }
                throw object_not_found_failure("mesh_interface", name_hash);
            }

            const_mesh_iterator begin_meshes() const
            {
                return (m_meshes.begin());
            }

            mesh_iterator begin_meshes()
            {
                return (m_meshes.begin());
            }

            const_mesh_iterator end_meshes() const
            {
                return (m_meshes.end());
            }

            mesh_iterator end_meshes()
            {
                return (m_meshes.end());
            }

            int get_mesh_count() const
            {
                return (static_cast<int>(m_meshes.size()));
            }

            // camera access
            camera::ref const& find_camera(unsigned long long name_hash) const
            {
                camera_vector::const_iterator i(m_cameras.begin());
                while (i != m_cameras.end()) {
                    if ((*i)->get_hash() == name_hash) {
                        return (*i);
                    }
                    ++i;
                }
                throw object_not_found_failure("camera", name_hash);
            }

            camera::ref& find_camera(unsigned long long name_hash)
            {
                camera_vector::iterator i(m_cameras.begin());
                while (i != m_cameras.end()) {
                    if ((*i)->get_hash() == name_hash) {
                        return (*i);
                    }
                    ++i;
                }
                throw object_not_found_failure("camera", name_hash);
            }

            const_camera_iterator begin_cameras() const
            {
                return (m_cameras.begin());
            }

            camera_iterator begin_cameras()
            {
                return (m_cameras.begin());
            }

            const_camera_iterator end_cameras() const
            {
                return (m_cameras.end());
            }

            camera_iterator end_cameras()
            {
                return (m_cameras.end());
            }

            int get_camera_count() const
            {
                return (static_cast<int>(m_cameras.size()));
            }

        private:
            static ref create(instance_descriptor::ref const& scene_desc)
            {
                return (ref(new scene(scene_desc)));
            }

            explicit scene(instance_descriptor::ref const& scene_desc);

            void load_instance(
                instance_descriptor::ref const& instance_desc,
                transform_descriptor::ref const& parent_transform_desc
                );

            void make_transform_work_item(frame_details* this_frame_details);
            void make_render_work_item(frame_details* this_frame_details);

            mesh_vector m_meshes;
            camera_vector m_cameras;

            frame_work_item_vector m_mesh_transforms;
            frame_work_item_vector m_camera_transforms;

            std::atomic<bool> m_visible;

            // Disallowed operations:
            scene();
            explicit scene(scene const&);
            scene& operator =(scene const&);

            friend class renderer;
        };
    }
}
