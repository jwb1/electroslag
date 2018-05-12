//  Electroslag Interactive Graphics System
//  Copyright 2015 Joshua Buckman
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

#if defined(ELECTROSLAG_BUILD_SHIP)
#error mesh file importer not to be included in SHIP build!
#endif

#include "electroslag/referenced_buffer.hpp"
#include "electroslag/threading/future_interface.hpp"
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/importer_interface.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/serialize/load_record.hpp"
#include "electroslag/graphics/graphics_types.hpp"
#include "electroslag/renderer/instance_descriptor.hpp"
#include "electroslag/renderer/transform_descriptor.hpp"
#include "electroslag/texture/gli_importer.hpp"

namespace electroslag {
    namespace mesh {
        class gltf2_importer
            : public serialize::importer_interface
            , public serialize::serializable_object<gltf2_importer> {
        public:
            typedef reference<gltf2_importer> ref;

            static ref create(
                std::string const& file_name,
                std::string const& object_prefix,
                serialize::load_record::ref& record,
                glm::f32vec3 const& bake_in_scale = glm::f32vec3(),
                renderer::transform_descriptor::ref const& base_transform = renderer::transform_descriptor::ref::null_ref
                )
            {
                return (ref(new gltf2_importer(file_name, object_prefix, record, bake_in_scale, base_transform)));
            }

            virtual ~gltf2_importer()
            {}

            // Implement serializable_object
            explicit gltf2_importer(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            // Implement importer_interface
            virtual void finish_importing();

            // gltf2_importer methods.
            typedef threading::future_interface<renderer::instance_descriptor::ref> import_future;

            import_future::ref const& get_future() const
            {
                return (m_async_loader);
            }

        private:
            class async_mesh_loader : public import_future {
            public:
                typedef reference<async_mesh_loader> ref;

                async_mesh_loader(
                    gltf2_importer::ref const& importer,
                    std::string const& file_name,
                    std::string const& object_prefix,
                    serialize::load_record::ref& record,
                    glm::f32vec3 const& bake_in_scale,
                    renderer::transform_descriptor::ref const& base_transform
                    )
                    : m_this_importer(importer)
                    , m_file_name(file_name)
                    , m_object_prefix(object_prefix)
                    , m_load_record(record)
                    , m_bake_in_scale(bake_in_scale)
                    , m_base_transform(base_transform)
                    , m_scene(-1)
                {}
                virtual ~async_mesh_loader()
                {}

                virtual import_future::value_type execute_for_value();

            private:
                struct buffer {
                    referenced_buffer_interface::ref data;
                };

                enum buffer_view_target {
                    buffer_view_target_unknown = -1,
                    buffer_view_target_array_buffer = gl::ARRAY_BUFFER,
                    buffer_view_target_element_array_buffer = gl::ELEMENT_ARRAY_BUFFER
                };

                struct buffer_view {
                    buffer_view()
                        : buffer(-1)
                        , byte_offset(0)
                        , byte_length(-1)
                        , byte_stride(1)
                        , target(buffer_view_target_unknown)
                    {}

                    int buffer;
                    int byte_offset;
                    int byte_length;
                    int byte_stride;
                    buffer_view_target target;
                };

                enum accessor_component_type {
                    accessor_component_type_unknown = -1,
                    accessor_component_type_byte = gl::BYTE,
                    accessor_component_type_unsigned_byte = gl::UNSIGNED_BYTE,
                    accessor_component_type_short = gl::SHORT,
                    accessor_component_type_unsigned_short = gl::UNSIGNED_SHORT,
                    accessor_component_type_unsigned_int = gl::UNSIGNED_INT,
                    accessor_component_type_float = gl::FLOAT
                };

                enum accessor_type {
                    accessor_type_unknown = -1,
                    accessor_type_scalar,
                    accessor_type_vec2,
                    accessor_type_vec3,
                    accessor_type_vec4,
                    accessor_type_mat2,
                    accessor_type_mat3,
                    accessor_type_mat4,
                    accessor_type_count // Ensure this is the last enum entry
                };

                static int const accessor_type_value_count[accessor_type_count];

                struct accessor {
                    accessor()
                        : buffer_view(-1)
                        , byte_offset(0)
                        , component_type(accessor_component_type_unknown)
                        , normalized(false)
                        , count(0)
                        , type(accessor_type_unknown)
                    {
                        memset(max, 0, sizeof(max));
                        memset(min, 0, sizeof(min));
                    }

                    int buffer_view;
                    int byte_offset;
                    accessor_component_type component_type;
                    bool normalized;
                    int count;
                    accessor_type type;
                    float max[16];
                    float min[16];
                    // TODO sparse
                };

                enum alpha_mode_type {
                    alpha_mode_type_unknown = -1,
                    alpha_mode_type_opaque,
                    alpha_mode_type_mask,
                    alpha_mode_type_blend
                };

                struct material_map {
                    material_map()
                        : texture_index(-1)
                        , texcoord_index(0)
                    {}

                    int texture_index;
                    int texcoord_index;
                };

                struct material {
                    material()
                        : base_color(1.0f, 1.0f, 1.0f, 1.0f)
                        , metallic(1.0f)
                        , roughness(1.0f)
                        , normal_scalar(1.0f)
                        , occulsion_strength(1.0f)
                        , emissive_color(0.0f, 0.0f, 0.0f)
                        , alpha_mode(alpha_mode_type_unknown)
                        , alpha_cutoff(0.5f)
                        , double_sided(false)
                    {}

                    glm::f32vec4 base_color;
                    material_map base_color_map;
                    float metallic;
                    float roughness;
                    material_map metallic_roughness_map;

                    material_map normal_map;
                    float normal_scalar;

                    material_map occulsion_map;
                    float occulsion_strength;

                    material_map emissive_map;
                    glm::f32vec3 emissive_color;

                    alpha_mode_type alpha_mode;
                    float alpha_cutoff;

                    bool double_sided;
                };

                enum primitive_mode_type {
                    primitive_mode_type_unknown = -1,
                    primitive_mode_type_points = gl::POINTS,
                    primitive_mode_type_lines = gl::LINES,
                    primitive_mode_type_line_loop = gl::LINE_LOOP,
                    primitive_mode_type_line_strip = gl::LINE_STRIP,
                    primitive_mode_type_triangles = gl::TRIANGLES,
                    primitive_mode_type_triangle_strip = gl::TRIANGLE_STRIP,
                    primitive_mode_type_triangle_fan = gl::TRIANGLE_FAN
                };

                struct primitive {
                    primitive()
                        : index_accessor(-1)
                        , material(-1)
                        , primitive_mode(primitive_mode_type_unknown)
                        , position_attrib_accessor(-1)
                        , normal_attrib_accessor(-1)
                        , tangent_attrib_accessor(-1)
                        , texcoord_0_attrib_accessor(-1)
                        , texcoord_1_attrib_accessor(-1)
                        , color_0_attrib_accessor(-1)
                    {}

                    int index_accessor;
                    int material;
                    primitive_mode_type primitive_mode;

                    int position_attrib_accessor;
                    int normal_attrib_accessor;
                    int tangent_attrib_accessor;
                    int texcoord_0_attrib_accessor;
                    int texcoord_1_attrib_accessor;
                    int color_0_attrib_accessor;
                };

                struct mesh {
                    std::vector<primitive> primitives;
                };

                enum node_transform_type {
                    node_transform_type_unknown = -1,
                    node_transform_type_none = 0,
                    node_transform_type_srt = 1,
                    node_transform_type_matrix = 2
                };

                struct node {
                    node()
                        : mesh(-1)
                        , transform_type(node_transform_type_none)
                    {}

                    int mesh;
                    node_transform_type transform_type;
                    glm::f32vec3 translate;
                    glm::f32vec3 scale;
                    glm::f32quat rotate;
                    glm::f32mat4 matrix;
                };

                struct scene {
                    std::vector<int> nodes;
                    renderer::instance_descriptor::ref desc;
                };

                static int locate_base64_start(int uri_length, char const* uri);

                void parse_asset(rapidjson::Document const& doc);
                void parse_extensions(rapidjson::Document const& doc);
                void parse_buffers(rapidjson::Document const& doc);
                void parse_buffer_views(rapidjson::Document const& doc);
                void parse_accessors(rapidjson::Document const& doc);
                void parse_images(rapidjson::Document const& doc);
                void parse_samplers(rapidjson::Document const& doc);
                void parse_textures(rapidjson::Document const& doc);
                void parse_meshes(rapidjson::Document const& doc);
                void parse_materials(rapidjson::Document const& doc);
                void parse_nodes(rapidjson::Document const& doc);
                void parse_scenes(rapidjson::Document const& doc);

                void parse_material_map(rapidjson::Value::ConstMemberIterator const& map_member, material_map* out_map);

                renderer::instance_descriptor::ref create_descriptors();

                // Input parameters.
                gltf2_importer::ref m_this_importer;
                std::string m_file_name;
                std::string m_object_prefix;
                serialize::load_record::ref m_load_record;
                glm::f32vec3 m_bake_in_scale;
                renderer::transform_descriptor::ref m_base_transform;

                // Parse state.
                std::vector<buffer> m_buffers;
                std::vector<buffer_view> m_buffer_views;
                std::vector<accessor> m_accessors;
                std::vector<std::string> m_image_paths;
                std::vector<graphics::sampler_params> m_samplers;
                std::vector<texture::gli_importer::ref> m_texture_importers;
                std::vector<material> m_materials;
                std::vector<mesh> m_meshes;
                std::vector<node> m_nodes;
                std::vector<scene> m_scenes;
                int m_scene;
            };

            gltf2_importer(
                std::string const& file_name,
                std::string const& object_prefix,
                serialize::load_record::ref& record,
                glm::f32vec3 const& bake_in_scale,
                renderer::transform_descriptor::ref const& base_transform
                );

            void import(
                std::string const& file_name,
                std::string const& object_prefix,
                serialize::load_record::ref& record,
                glm::f32vec3 const& bake_in_scale,
                renderer::transform_descriptor::ref const& base_transform
                );

            import_future::ref m_async_loader;

            // Disallowed operations:
            gltf2_importer();
            explicit gltf2_importer(gltf2_importer const&);
            gltf2_importer& operator =(gltf2_importer const&);
        };
    }
}
