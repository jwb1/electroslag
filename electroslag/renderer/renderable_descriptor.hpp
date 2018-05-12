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
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/renderer/renderer_types.hpp"
#include "electroslag/renderer/transform_descriptor.hpp"
#include "electroslag/renderer/geometry_descriptor.hpp"
#include "electroslag/renderer/pipeline_descriptor.hpp"
#include "electroslag/renderer/camera_descriptor.hpp"

namespace electroslag {
    namespace renderer {
        class renderable_descriptor
            : public referenced_object
            , public serialize::serializable_object<renderable_descriptor> {
        public:
            typedef reference<renderable_descriptor> ref;

            static ref create()
            {
                return (ref(new renderable_descriptor()));
            }

            // Implement serializable_object
            explicit renderable_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            renderable_descriptor_component_bits get_component_bits() const
            {
                return (static_cast<renderable_descriptor_component_bits>(m_component_bits));
            }

            transform_descriptor::ref const& get_transform_component() const
            {
                return (m_transform);
            }

            transform_descriptor::ref get_transform_component()
            {
                return (m_transform);
            }

            void set_transform_component(transform_descriptor::ref const& t);

            geometry_descriptor::ref const& get_geometry_component() const
            {
                return (m_geometry);
            }

            geometry_descriptor::ref get_geometry_component()
            {
                return (m_geometry);
            }

            void set_geometry_component(geometry_descriptor::ref const& g);

            geometry_descriptor::ref const& get_shape_component() const
            {
                return (m_shape_only);
            }

            geometry_descriptor::ref get_shape_component()
            {
                return (m_shape_only);
            }

            void set_shape_component(geometry_descriptor::ref const& s);

            camera_descriptor::ref const& get_camera_component() const
            {
                return (m_camera);
            }

            camera_descriptor::ref get_camera_component()
            {
                return (m_camera);
            }

            void set_camera_component(camera_descriptor::ref const& g);

            pipeline_descriptor::ref const& get_pipeline_component(pipeline_type type) const;

            pipeline_descriptor::ref get_pipeline_component(pipeline_type type);

            void set_pipeline_component(pipeline_descriptor::ref const& m);

        private:
            renderable_descriptor()
                : m_component_bits(0)
            {}

            uint32_t m_component_bits;

            // Components
            transform_descriptor::ref m_transform;
            geometry_descriptor::ref m_geometry;
            geometry_descriptor::ref m_shape_only;
            camera_descriptor::ref m_camera;

            typedef std::vector<pipeline_descriptor::ref> pipeline_vector;
            pipeline_vector m_pipelines;

            // Disallowed operations:
            explicit renderable_descriptor(renderable_descriptor const&);
            renderable_descriptor& operator =(renderable_descriptor const&);
        };
    }
}
