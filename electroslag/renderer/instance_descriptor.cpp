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
#include "electroslag/renderer/instance_descriptor.hpp"
#include "electroslag/serialize/database.hpp"
#include "electroslag/serialize/serializable_object.hpp"
#if !defined(ELECTROSLAG_BUILD_SHIP)
#include "electroslag/mesh/gltf2_importer.hpp"
#endif

namespace electroslag {
    namespace renderer {
        instance_descriptor::instance_descriptor(serialize::archive_reader_interface* ar)
        {
            // Transform. Treated as identity if none is specified.
            unsigned long long transform_hash = 0;
            if (!ar->read_name_hash("transform", &transform_hash)) {
                throw load_object_failure("transform");
            }

            if (transform_hash) {
                m_instance_transform = serialize::get_database()->find_object_ref<transform_descriptor>(transform_hash);
            }

            // Renderable to draw for this instance.
            unsigned long long renderable_hash = 0;
            if (!ar->read_name_hash("renderable", &renderable_hash)) {
                throw load_object_failure("renderable");
            }

            if (renderable_hash) {
                m_instance_renderable = serialize::get_database()->find_object_ref<renderable_descriptor>(renderable_hash);
            }

            // Children instances
            int32_t child_count = 0;
            if (!ar->read_int32("child_count", &child_count) || child_count <= 0) {
                throw load_object_failure("child_count");
            }
            m_child_instances.reserve(child_count);

            std::string child_name;
            serialize::enumeration_namer namer(child_count, 'c');
            while (!namer.used_all_names()) {
                child_name = namer.get_next_name();

                instance_descriptor::ref child_instance_desc;
#if !defined(ELECTROSLAG_BUILD_SHIP)
                unsigned long long importer_hash = 0;
                if (ar->read_name_hash(child_name + "_importer", &importer_hash)) {
                    // TODO: It would be nice to be able to get the importer_interface base class here.
                    mesh::gltf2_importer::ref importer(
                        serialize::get_database()->find_object_ref<mesh::gltf2_importer>(importer_hash)
                        );
                    child_instance_desc = importer->get_future()->get_wait().cast<instance_descriptor>();
                }
#endif
                unsigned long long instance_hash = 0;
                if (ar->read_name_hash(child_name + "_instance", &instance_hash)) {
                    child_instance_desc = serialize::get_database()->find_object_ref<instance_descriptor>(instance_hash);
                }

                insert_child_instance(child_instance_desc);
            }
        }

        void instance_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            // Write in all of the instance renderable/transform and all children.
            if (m_instance_transform.is_valid()) {
                m_instance_transform->save_to_archive(ar);
            }

            if (m_instance_renderable.is_valid()) {
                m_instance_renderable->save_to_archive(ar);
            }

            instance_vector::iterator i(m_child_instances.begin());
            while (i != m_child_instances.end()) {
                (*i)->save_to_archive(ar);
                ++i;
            }

            // Write in the container description
            serializable_object::save_to_archive(ar);

            if (m_instance_transform.is_valid()) {
                ar->write_name_hash("transform", m_instance_transform->get_hash());
            }
            else {
                ar->write_name_hash("transform", 0);
            }

            if (m_instance_renderable.is_valid()) {
                ar->write_name_hash("renderable", m_instance_renderable->get_hash());
            }
            else {
                ar->write_name_hash("renderable", 0);
            }

            int child_count = static_cast<int>(m_child_instances.size());
            ar->write_int32("child_count", child_count);

            std::string child_name;
            serialize::enumeration_namer namer(child_count, 'c');
            i = begin_child_instances();
            while (i != end_child_instances()) {
                child_name = namer.get_next_name();

                ar->write_name_hash(child_name + "_instance", (*i)->get_hash());

                ++i;
            }
        }
    }
}
