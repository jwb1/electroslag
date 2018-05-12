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
#include "electroslag/renderer/renderable_descriptor.hpp"
#include "electroslag/renderer/renderer_types.hpp"

namespace electroslag {
    namespace renderer {
        renderable_descriptor::renderable_descriptor(serialize::archive_reader_interface* ar)
        {
            renderable_descriptor_component_bits component_bits = static_cast<renderable_descriptor_component_bits>(0);
            if (!ar->read_enumeration_flags(
                "components",
                &component_bits,
                renderable_descriptor_component_strings,
                renderable_descriptor_component_bits_values
                )) {
                throw load_object_failure("components");
            }
            m_component_bits = component_bits;

            unsigned long long name_hash = 0;
            if (m_component_bits & renderable_descriptor_component_bits_transform) {
                if (!ar->read_name_hash("transform", &name_hash)) {
                    throw load_object_failure("transform");
                }
                m_transform = serialize::get_database()->find_object_ref<transform_descriptor>(name_hash);
            }

            if (m_component_bits & renderable_descriptor_component_bits_geometry) {
                if (!ar->read_name_hash("geometry", &name_hash)) {
                    throw load_object_failure("geometry");
                }
                m_geometry = serialize::get_database()->find_object_ref<geometry_descriptor>(name_hash);
            }

            if (m_component_bits & renderable_descriptor_component_bits_geometry_shape_only) {
                if (!ar->read_name_hash("shape_only", &name_hash)) {
                    throw load_object_failure("shape_only");
                }
                m_shape_only = serialize::get_database()->find_object_ref<geometry_descriptor>(name_hash);
            }

            if (m_component_bits & renderable_descriptor_component_bits_camera) {
                if (!ar->read_name_hash("camera", &name_hash)) {
                    throw load_object_failure("camera");
                }
                m_camera = serialize::get_database()->find_object_ref<camera_descriptor>(name_hash);
            }

            if (m_component_bits & renderable_descriptor_component_bits_pipeline) {
                int32_t pipeline_count = 0;
                if (!ar->read_int32("pipeline_count", &pipeline_count) || pipeline_count <= 0) {
                    throw load_object_failure("pipeline_count");
                }
                m_pipelines.reserve(pipeline_count);

                serialize::enumeration_namer namer(pipeline_count, 'p');
                while (!namer.used_all_names()) {
                    unsigned long long pipeline_hash = 0;
                    if (!ar->read_name_hash(namer.get_next_name(), &pipeline_hash)) {
                        throw load_object_failure("pipeline");
                    }

                    m_pipelines.emplace_back(
                        serialize::get_database()->find_object_ref<pipeline_descriptor>(pipeline_hash)
                        );
                }
            }
        }

        void renderable_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            uint32_t component_bits = 0;
            if (m_transform.is_valid()) {
                m_transform->save_to_archive(ar);
                component_bits |= renderable_descriptor_component_bits_transform;
            }

            if (m_geometry.is_valid()) {
                m_geometry->save_to_archive(ar);
                component_bits |= renderable_descriptor_component_bits_geometry;
            }

            if (m_camera.is_valid()) {
                m_camera->save_to_archive(ar);
                component_bits |= renderable_descriptor_component_bits_camera;
            }

            if (m_shape_only.is_valid()) {
                m_shape_only->save_to_archive(ar);
                component_bits |= renderable_descriptor_component_bits_geometry_shape_only;
            }

            int pipeline_count = static_cast<int>(m_pipelines.size());
            if (pipeline_count > 0) {
                pipeline_vector::iterator i(m_pipelines.begin());
                while (i != m_pipelines.end()) {
                    (*i)->save_to_archive(ar);
                    ++i;
                }
                component_bits |= renderable_descriptor_component_bits_pipeline;
            }

            if (component_bits != static_cast<uint32_t>(m_component_bits)) {
                throw std::runtime_error("invalid component bits");
            }

            // Next write the shader_program itself.
            serializable_object::save_to_archive(ar);

            ar->write_uint32("components", component_bits);

            if (m_transform.is_valid()) {
                ar->write_name_hash("transform", m_transform->get_hash());
            }

            if (m_geometry.is_valid()) {
                ar->write_name_hash("geometry", m_geometry->get_hash());
            }

            if (m_camera.is_valid()) {
                ar->write_name_hash("camera", m_camera->get_hash());
            }

            if (m_shape_only.is_valid()) {
                ar->write_name_hash("shape_only", m_shape_only->get_hash());
            }

            if (pipeline_count > 0) {
                ar->write_int32("pipeline_count", pipeline_count);

                serialize::enumeration_namer namer(pipeline_count, 'p');
                pipeline_vector::iterator i(m_pipelines.begin());
                while (i != m_pipelines.end()) {
                    ar->write_name_hash(namer.get_next_name(), (*i)->get_hash());
                    ++i;
                }
            }
        }

        void renderable_descriptor::set_transform_component(transform_descriptor::ref const& t)
        {
            if (t.is_valid()) {
                m_component_bits |= renderable_descriptor_component_bits_transform;
            }
            else {
                m_component_bits &= ~renderable_descriptor_component_bits_transform;
            }
            m_transform = t;
        }

        void renderable_descriptor::set_geometry_component(geometry_descriptor::ref const& g)
        {
            if (g.is_valid()) {
                m_component_bits |= renderable_descriptor_component_bits_geometry;
            }
            else {
                m_component_bits &= ~renderable_descriptor_component_bits_geometry;
            }
            m_geometry = g;
        }

        void renderable_descriptor::set_shape_component(geometry_descriptor::ref const& s)
        {
            if (s.is_valid()) {
                m_component_bits |= renderable_descriptor_component_bits_geometry_shape_only;
            }
            else {
                m_component_bits &= ~renderable_descriptor_component_bits_geometry_shape_only;
            }
            m_shape_only = s;
        }

        void renderable_descriptor::set_camera_component(camera_descriptor::ref const& c)
        {
            if (c.is_valid()) {
                m_component_bits |= renderable_descriptor_component_bits_camera;
            }
            else {
                m_component_bits &= ~renderable_descriptor_component_bits_camera;
            }
            m_camera = c;
        }

        pipeline_descriptor::ref const& renderable_descriptor::get_pipeline_component(pipeline_type type) const
        {
            pipeline_vector::const_iterator i(m_pipelines.begin());
            while (i != m_pipelines.end()) {
                if ((*i)->get_pipeline_type() == type) {
                    return (*i);
                }

                ++i;
            }
            throw std::logic_error("Renderable does not support mode.");
        }

        pipeline_descriptor::ref renderable_descriptor::get_pipeline_component(pipeline_type type)
        {
            pipeline_vector::iterator i(m_pipelines.begin());
            while (i != m_pipelines.end()) {
                if ((*i)->get_pipeline_type() == type) {
                    return (*i);
                }

                ++i;
            }
            throw std::logic_error("Renderable does not support mode.");
        }

        void renderable_descriptor::set_pipeline_component(pipeline_descriptor::ref const& m)
        {
            if (m.is_valid()) {
                m_component_bits |= renderable_descriptor_component_bits_pipeline;
            }
            else {
                m_component_bits &= ~renderable_descriptor_component_bits_pipeline;
            }

            // Replace an existing pipeline for the given pass type.
            for (int i = 0; i < m_pipelines.size(); ++i) {
                if (m_pipelines.at(i)->get_pipeline_type() == m->get_pipeline_type()) {
                    m_pipelines.erase(m_pipelines.begin() + i);
                }
            }

            m_pipelines.emplace_back(m);
        }
    }
}
