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

#include "electroslag/precomp.hpp"
#include "electroslag/mesh/gltf2_importer.hpp"
#include "electroslag/threading/thread_pool.hpp"
#include "electroslag/file_stream.hpp"
#include "electroslag/graphics/buffer_descriptor.hpp"
#include "electroslag/serialize/base64.hpp"

namespace electroslag {
    namespace mesh {
        // Convert accessor_type enum value into the number of values implied by that type.
        // static
        int const gltf2_importer::async_mesh_loader::accessor_type_value_count[accessor_type_count] = {
            1,
            2,
            3,
            4,
            4,
            9,
            16
        };

        gltf2_importer::gltf2_importer(
            std::string const& file_name,
            std::string const& object_prefix,
            serialize::load_record::ref& record,
            glm::f32vec3 const& bake_in_scale,
            renderer::transform_descriptor::ref const& base_transform
            )
        {
            import(file_name, object_prefix, record, bake_in_scale, base_transform);
        }
        
        gltf2_importer::gltf2_importer(serialize::archive_reader_interface* ar)
        {
            std::string file_name;
            if (!ar->read_string("file_name", &file_name)) {
                throw load_object_failure("file_name");
            }

            std::filesystem::path dir(file_name);
            ELECTROSLAG_CHECK(dir.has_filename());

            // This method seems to have been renamed between VS 2013 and VS 2015.
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
            if (!dir.is_complete()) {
#else
            if (!dir.is_absolute()) {
#endif
                std::filesystem::path combined_dir(ar->get_base_directory());
                combined_dir /= dir;
                dir = std::filesystem::canonical(combined_dir);
            }

            std::string object_prefix("");
            ar->read_string("object_name_prefix", &object_prefix);

            glm::f32vec3 bake_in_scale(1.0f);
            ar->read_buffer("bake_in_scale", &bake_in_scale, sizeof(bake_in_scale));

            renderer::transform_descriptor::ref transform_desc;
            unsigned long long transform_hash = 0;
            if (ar->read_name_hash("instance_transform", &transform_hash)) {
                transform_desc = serialize::get_database()->find_object_ref<renderer::transform_descriptor>(transform_hash);
            }

            import(dir.string(), object_prefix, ar->get_load_record(), bake_in_scale, transform_desc);
        }

        void gltf2_importer::save_to_archive(serialize::archive_writer_interface*)
        {
            // This class just imports data; there is no logical way to save an importer.
        }

        void gltf2_importer::finish_importing()
        {
            if (m_async_loader.is_valid()) {
                m_async_loader->wait_for_done();
            }
        }

        void gltf2_importer::import(
            std::string const& file_name,
            std::string const& object_prefix,
            serialize::load_record::ref& record,
            glm::f32vec3 const& bake_in_scale,
            renderer::transform_descriptor::ref const& base_transform
            )
        {
            m_async_loader = threading::get_io_thread_pool()->enqueue_work_item<async_mesh_loader>(
                ref(this),
                file_name,
                object_prefix,
                record,
                bake_in_scale,
                base_transform
                ).cast<import_future>();
        }

        gltf2_importer::import_future::value_type gltf2_importer::async_mesh_loader::execute_for_value()
        {
            file_stream gltf2_stream;
            gltf2_stream.open(m_file_name, file_stream_access_mode_read);

            long long stream_size = gltf2_stream.get_size();
            if (stream_size <= 0) {
                throw load_object_failure("gltf2 0 byte stream");
            }

            // Read the gltf2 json into a buffer and set up to parse it.
            std::unique_ptr<char[]> stream_buffer(new char[stream_size + 1]);

            gltf2_stream.read(stream_buffer.get(), stream_size);
            stream_buffer[stream_size] = '\0';

            rapidjson::Document doc;
            doc.ParseInsitu<rapidjson::kParseInsituFlag | rapidjson::kParseCommentsFlag>(stream_buffer.get());

            // TODO: Deal with parse errors better
            ELECTROSLAG_CHECK(!doc.HasParseError());
            ELECTROSLAG_CHECK(doc.IsObject());

            // Parse all of the gltf2 objects.
            parse_asset(doc);
            parse_extensions(doc);
            parse_buffers(doc);
            parse_buffer_views(doc);
            parse_accessors(doc);
            parse_images(doc);
            parse_samplers(doc);
            parse_textures(doc);
            parse_materials(doc);
            parse_meshes(doc);
            parse_nodes(doc);
            parse_scenes(doc);

            // Create electroslag objects from the parsed gltf2.
            renderer::instance_descriptor::ref scene_desc(create_descriptors());

            // Resolve circular object dependencies.
            m_this_importer.reset();
            m_load_record.reset();
            return (scene_desc);
        }

        // static
        int gltf2_importer::async_mesh_loader::locate_base64_start(int uri_length, char const* uri)
        {
            // The GLTF2 allows for buffer data to be encoded inline with base64. This function
            // detects if the headers imply this, and returns the offset of where to expect the
            // start of the base64 data.
            int base64_start = -1;

            static struct base64_prefix {
                char const* prefix;
                int length;
            } const base64_prefixes[] = {
                { ELECTROSLAG_STRING_AND_LENGTH("data:text/plain;base64,") },
                { ELECTROSLAG_STRING_AND_LENGTH("data:; base64,") }
            };

            for (int p = 0; p < _countof(base64_prefixes); ++p) {
                if ((uri_length > base64_prefixes[p].length) &&
                    (std::strncmp(uri, base64_prefixes[p].prefix, base64_prefixes[p].length) == 0)) {

                    base64_start = base64_prefixes[p].length;
                    break;
                }
            }

            return (base64_start);
        }

        void gltf2_importer::async_mesh_loader::parse_asset(rapidjson::Document const& doc)
        {
            rapidjson::Value::ConstMemberIterator asset(doc.FindMember("asset"));
            if (asset == doc.MemberEnd()) {
                throw load_object_failure("gltf2 asset object missing");
            }

            // If there is a minimum version, we need to meet it; major and minor.
            unsigned int major = 0;
            unsigned int minor = 0;

            rapidjson::Value::ConstMemberIterator minVersion(asset->value.FindMember("minVersion"));
            if (minVersion != asset->value.MemberEnd()) {
                if (std::sscanf(minVersion->value.GetString(), "%u.%u", &major, &minor) != 1) {
                    if (major > 2 || minor > 0) {
                        throw load_object_failure("gltf2 minimum version mismatch");
                    }
                }
                else {
                    throw load_object_failure("gltf2 version string parse error");
                }
            }

            // Otherwise, assume we can do something useful with files of matching major version.
            major = 0;
            minor = 0;

            rapidjson::Value::ConstMemberIterator version(asset->value.FindMember("version"));
            if (version != asset->value.MemberEnd()) {
                if (std::sscanf(version->value.GetString(), "%u.%u", &major, &minor) != 1) {
                    if (major > 2) {
                        throw load_object_failure("gltf2 major version mismatch");
                    }
                }
                else {
                    throw load_object_failure("gltf2 version string parse error");
                }
            }
            else {
                throw load_object_failure("gltf2 file missing version");
            }
        }

        void gltf2_importer::async_mesh_loader::parse_extensions(rapidjson::Document const& /*doc*/)
        {}

        void gltf2_importer::async_mesh_loader::parse_buffers(rapidjson::Document const& doc)
        {
            rapidjson::Value::ConstMemberIterator buffers(doc.FindMember("buffers"));
            if (buffers == doc.MemberEnd()) {
                return;
            }
            ELECTROSLAG_CHECK(buffers->value.IsArray());

            for (rapidjson::Value::ConstValueIterator b(buffers->value.Begin());
                 b != buffers->value.End();
                 ++b) {

                buffer this_buffer;

                // How big is the buffer?
                rapidjson::Value::ConstMemberIterator bytes_member(b->FindMember("byteLength"));
                if (bytes_member == b->MemberEnd()) {
                    throw load_object_failure("gltf2 buffer missing byteLength");
                }

                int bytes = bytes_member->value.GetInt();;

                // Get the data.
                this_buffer.data = referenced_buffer_from_sizeof::create(bytes);
                {
                    referenced_buffer_interface::accessor buffer_acessor(this_buffer.data);

                    rapidjson::Value::ConstMemberIterator uri_member(b->FindMember("uri"));
                    if (uri_member != b->MemberEnd()) {
                        char const* uri = uri_member->value.GetString();
                        int uri_length = uri_member->value.GetStringLength();

                        // Figure out if the uri is a supported data-uri.
                        int base64_start = locate_base64_start(uri_length, uri);

                        // Decode the data-uri or load the file.
                        if (base64_start != -1) {
                            serialize::base64::decode(
                                uri + base64_start,
                                uri_length - base64_start,
                                buffer_acessor.get_pointer(),
                                buffer_acessor.get_sizeof()
                                );
                        }
                        else {
                            // All file URIs are relative to the gltf2 file path.
                            std::filesystem::path buffer_path(m_file_name);
                            buffer_path = buffer_path.remove_filename();
                            buffer_path /= uri;
                            buffer_path = std::filesystem::canonical(buffer_path);

                            file_stream buffer_stream;
                            buffer_stream.open(buffer_path.string(), file_stream_access_mode_read);

                            long long stream_size = buffer_stream.get_size();
                            if (stream_size > 0) {
                                buffer_stream.read(buffer_acessor.get_pointer(), buffer_acessor.get_sizeof());
                            }
                            else {
                                throw load_object_failure("gltf2 bin file is 0 bytes");
                            }
                        }
                    }
                    else {
                        // TODO: support GDB
                    }
                }

                m_buffers.push_back(this_buffer);
            }
        }

        void gltf2_importer::async_mesh_loader::parse_buffer_views(rapidjson::Document const& doc)
        {
            rapidjson::Value::ConstMemberIterator buffer_views(doc.FindMember("bufferViews"));
            if (buffer_views == doc.MemberEnd()) {
                return;
            }
            ELECTROSLAG_CHECK(buffer_views->value.IsArray());

            for (rapidjson::Value::ConstValueIterator bv(buffer_views->value.Begin());
                 bv != buffer_views->value.End();
                 ++bv) {

                buffer_view this_view;

                // Buffer is referenced by array index.
                rapidjson::Value::ConstMemberIterator buffer_member(bv->FindMember("buffer"));
                if (buffer_member == bv->MemberEnd()) {
                    throw load_object_failure("gltf2 buffer view missing buffer");
                }

                this_view.buffer = buffer_member->value.GetInt();
                if (this_view.buffer < 0 || this_view.buffer >= m_buffers.size()) {
                    throw load_object_failure("gltf2 buffer view invalid buffer");
                }

                // Offset in the buffer in bytes
                rapidjson::Value::ConstMemberIterator byte_offset_member(bv->FindMember("byteOffset"));
                if (byte_offset_member != bv->MemberEnd()) {
                    this_view.byte_offset = byte_offset_member->value.GetInt();
                }

                // Length in the buffer in bytes
                rapidjson::Value::ConstMemberIterator byte_length_member(bv->FindMember("byteLength"));
                if (byte_length_member == bv->MemberEnd()) {
                    throw load_object_failure("gltf2 buffer view missing byte_length");
                }

                this_view.byte_length = byte_length_member->value.GetInt();

                // Offset in the buffer in bytes
                rapidjson::Value::ConstMemberIterator stride_member(bv->FindMember("byteStride"));
                if (stride_member != bv->MemberEnd()) {
                    this_view.byte_stride = stride_member->value.GetInt();
                }

                // Target is a GL like enum
                rapidjson::Value::ConstMemberIterator target_member(bv->FindMember("target"));
                if (target_member != bv->MemberEnd()) {
                    this_view.target = static_cast<buffer_view_target>(target_member->value.GetInt());
                }

                m_buffer_views.emplace_back(this_view);
            }
        }

        void gltf2_importer::async_mesh_loader::parse_accessors(rapidjson::Document const& doc)
        {
            rapidjson::Value::ConstMemberIterator accessors(doc.FindMember("accessors"));
            if (accessors == doc.MemberEnd()) {
                return;
            }
            ELECTROSLAG_CHECK(accessors->value.IsArray());

            for (rapidjson::Value::ConstValueIterator a(accessors->value.Begin());
                 a != accessors->value.End();
                 ++a) {
                accessor this_accessor;

                // Buffer view is referenced by array index.
                rapidjson::Value::ConstMemberIterator buffer_view_member(a->FindMember("bufferView"));
                if (buffer_view_member != a->MemberEnd()) {
                    this_accessor.buffer_view = buffer_view_member->value.GetInt();
                    if (this_accessor.buffer_view < 0 || this_accessor.buffer_view >= m_buffer_views.size()) {
                        throw load_object_failure("gltf2 accessor invalid buffer_view");
                    }
                }
                else {
                    throw load_object_failure("gltf2 accessor buffer view is not found");
                }

                // Offset in the buffer view in bytes
                rapidjson::Value::ConstMemberIterator byte_offset_member(a->FindMember("byteOffset"));
                if (byte_offset_member != a->MemberEnd()) {
                    this_accessor.byte_offset = byte_offset_member->value.GetInt();
                }

                // Component type is a GL like enum
                rapidjson::Value::ConstMemberIterator component_type_member(a->FindMember("componentType"));
                if (component_type_member == a->MemberEnd()) {
                    throw load_object_failure("gltf2 accessor missing component_type");
                }

                this_accessor.component_type = static_cast<accessor_component_type>(component_type_member->value.GetInt());

                // Normalization
                rapidjson::Value::ConstMemberIterator normalized_member(a->FindMember("normalized"));
                if (normalized_member != a->MemberEnd()) {
                    this_accessor.normalized = normalized_member->value.GetBool();
                }

                // Attribute count
                rapidjson::Value::ConstMemberIterator count_member(a->FindMember("count"));
                if (count_member == a->MemberEnd()) {
                    throw load_object_failure("gltf2 accessor missing count");
                }

                this_accessor.count = count_member->value.GetInt();

                // Data type
                rapidjson::Value::ConstMemberIterator type_member(a->FindMember("type"));
                if (type_member == a->MemberEnd()) {
                    throw load_object_failure("gltf2 accessor missing type");
                }

                std::string type_string(type_member->value.GetString());
                if (type_string == "SCALAR") {
                    this_accessor.type = accessor_type_scalar;
                }
                else if (type_string == "VEC2") {
                    this_accessor.type = accessor_type_vec2;
                }
                else if (type_string == "VEC3") {
                    this_accessor.type = accessor_type_vec3;
                }
                else if (type_string == "VEC4") {
                    this_accessor.type = accessor_type_vec4;
                }
                else if (type_string == "MAT2") {
                    this_accessor.type = accessor_type_mat2;
                }
                else if (type_string == "MAT3") {
                    this_accessor.type = accessor_type_mat3;
                }
                else if (type_string == "MAT4") {
                    this_accessor.type = accessor_type_mat4;
                }
                else {
                    throw load_object_failure("gltf2 accessor type is not valid");
                }

                // Max values for each component
                rapidjson::Value::ConstMemberIterator max_array(a->FindMember("max"));
                if (max_array != a->MemberEnd()) {
                    ELECTROSLAG_CHECK(max_array->value.IsArray());

                    if (static_cast<int>(max_array->value.Size()) != accessor_type_value_count[this_accessor.type]) {
                        throw load_object_failure("gltf2 accessor max array incorrectly sized");
                    }

                    int max_array_index = 0;
                    for (rapidjson::Value::ConstValueIterator max_value(max_array->value.Begin());
                         max_value != max_array->value.End();
                         ++max_value, ++max_array_index) {

                        this_accessor.max[max_array_index] = max_value->GetFloat();
                    }
                }

                // Min values for each component
                rapidjson::Value::ConstMemberIterator min_array(a->FindMember("min"));
                if (min_array != a->MemberEnd()) {
                    ELECTROSLAG_CHECK(min_array->value.IsArray());

                    if (static_cast<int>(min_array->value.Size()) != accessor_type_value_count[this_accessor.type]) {
                        throw load_object_failure("gltf2 accessor min array incorrectly sized");
                    }

                    int min_array_index = 0;
                    for (rapidjson::Value::ConstValueIterator min_value(min_array->value.Begin());
                        min_value != min_array->value.End();
                        ++min_value, ++min_array_index) {

                        this_accessor.min[min_array_index] = min_value->GetFloat();
                    }
                }

                m_accessors.emplace_back(this_accessor);
            }
        }

        void gltf2_importer::async_mesh_loader::parse_images(rapidjson::Document const& doc)
        {
            rapidjson::Value::ConstMemberIterator images(doc.FindMember("images"));
            if (images == doc.MemberEnd()) {
                return;
            }
            ELECTROSLAG_CHECK(images->value.IsArray());

            for (rapidjson::Value::ConstValueIterator i(images->value.Begin());
                 i != images->value.End();
                 ++i) {

                rapidjson::Value::ConstMemberIterator uri_member(i->FindMember("uri"));
                if (uri_member != i->MemberEnd()) {
                    char const* uri = uri_member->value.GetString();
                    int uri_length = uri_member->value.GetStringLength();

                    // Figure out if the uri is a supported data-uri.
                    int base64_start = locate_base64_start(uri_length, uri);

                    // Decode the data-uri or load the file.
                    if (base64_start != -1) {
                        // TODO: How to support base64?
                    }
                    else {
                        // All file URIs are relative to the gltf2 file path.
                        std::filesystem::path image_path(m_file_name);
                        image_path = image_path.remove_filename();
                        image_path /= uri;
                        image_path = std::filesystem::canonical(image_path);

                        m_image_paths.emplace_back(image_path.generic_string());
                    }
                }

                // TODO: What to do with MIME type? Expecting all URIs to be decodable by gli.
                // TODO: How to support buffer view
            }
        }

        void gltf2_importer::async_mesh_loader::parse_samplers(rapidjson::Document const& doc)
        {
            rapidjson::Value::ConstMemberIterator samplers(doc.FindMember("samplers"));
            if (samplers == doc.MemberEnd()) {
                return;
            }
            ELECTROSLAG_CHECK(samplers->value.IsArray());

            for (rapidjson::Value::ConstValueIterator s(samplers->value.Begin());
                 s != samplers->value.End();
                 ++s) {

                graphics::sampler_params this_sampler;

                // Filter for when texels must be magnified
                rapidjson::Value::ConstMemberIterator mag_filter_member(s->FindMember("magFilter"));
                if (mag_filter_member != s->MemberEnd()) {
                    switch (mag_filter_member->value.GetInt()) {
                    case gl::NEAREST:
                        this_sampler.magnification_filter = graphics::texture_filter_point;
                        break;

                    case gl::LINEAR:
                        this_sampler.magnification_filter = graphics::texture_filter_linear;
                        break;

                    default:
                        throw load_object_failure("gltf2 sampler mag_filter is not valid");
                    }
                }

                // Filter for when texels must be minified
                rapidjson::Value::ConstMemberIterator min_filter_member(s->FindMember("minFilter"));
                if (min_filter_member != s->MemberEnd()) {
                    switch (min_filter_member->value.GetInt()) {
                    case gl::NEAREST:
                        this_sampler.minification_filter = graphics::texture_filter_point;
                        break;

                    case gl::LINEAR:
                        this_sampler.minification_filter = graphics::texture_filter_linear;
                        break;

                    case gl::NEAREST_MIPMAP_NEAREST:
                        this_sampler.minification_filter = graphics::texture_filter_point;
                        this_sampler.mip_filter = graphics::texture_filter_point;
                        break;

                    case gl::LINEAR_MIPMAP_NEAREST:
                        this_sampler.minification_filter = graphics::texture_filter_linear;
                        this_sampler.mip_filter = graphics::texture_filter_point;
                        break;

                    case gl::NEAREST_MIPMAP_LINEAR:
                        this_sampler.minification_filter = graphics::texture_filter_point;
                        this_sampler.mip_filter = graphics::texture_filter_linear;
                        break;

                    case gl::LINEAR_MIPMAP_LINEAR:
                        this_sampler.minification_filter = graphics::texture_filter_linear;
                        this_sampler.mip_filter = graphics::texture_filter_linear;
                        break;

                    default:
                        throw load_object_failure("gltf2 sampler min_filter is not valid");
                    }
                }

                // Wrapping mode for coordinates in the s-direction
                this_sampler.s_wrap_mode = graphics::texture_coord_wrap_repeat;
                rapidjson::Value::ConstMemberIterator s_wrap_member(s->FindMember("wrapS"));
                if (s_wrap_member != s->MemberEnd()) {
                    switch (s_wrap_member->value.GetInt()) {
                    case gl::CLAMP_TO_EDGE:
                        this_sampler.s_wrap_mode = graphics::texture_coord_wrap_clamp;
                        break;

                    case gl::MIRRORED_REPEAT:
                        this_sampler.s_wrap_mode = graphics::texture_coord_wrap_mirror;
                        break;

                    case gl::REPEAT:
                        this_sampler.s_wrap_mode = graphics::texture_coord_wrap_repeat;
                        break;

                    default:
                        throw load_object_failure("gltf2 sampler s_wrap is not valid");
                    }
                }

                // Wrapping mode for coordinates in the t-direction
                this_sampler.t_wrap_mode = graphics::texture_coord_wrap_repeat;
                rapidjson::Value::ConstMemberIterator t_wrap_member(s->FindMember("wrapT"));
                if (t_wrap_member != s->MemberEnd()) {
                    switch (t_wrap_member->value.GetInt()) {
                    case gl::CLAMP_TO_EDGE:
                        this_sampler.t_wrap_mode = graphics::texture_coord_wrap_clamp;
                        break;

                    case gl::MIRRORED_REPEAT:
                        this_sampler.t_wrap_mode = graphics::texture_coord_wrap_mirror;
                        break;

                    case gl::REPEAT:
                        this_sampler.t_wrap_mode = graphics::texture_coord_wrap_repeat;
                        break;

                    default:
                        throw load_object_failure("gltf2 sampler t_wrap is not valid");
                    }
                }

                m_samplers.emplace_back(this_sampler);
            }
        }

        void gltf2_importer::async_mesh_loader::parse_textures(rapidjson::Document const& doc)
        {
            rapidjson::Value::ConstMemberIterator textures(doc.FindMember("textures"));
            if (textures == doc.MemberEnd()) {
                return;
            }
            ELECTROSLAG_CHECK(textures->value.IsArray());

            std::string prefix;

            int texture_id = 0;
            for (rapidjson::Value::ConstValueIterator t(textures->value.Begin());
                 t != textures->value.End();
                 ++t, ++texture_id) {

                // Index of the image to use for texels in this texture.
                rapidjson::Value::ConstMemberIterator source_member(t->FindMember("source"));
                if (source_member == t->MemberEnd()) {
                    throw load_object_failure("gltf2 texture missing source");
                }

                int source_index = source_member->value.GetInt();

                if (source_index < 0 || source_index >= m_image_paths.size()) {
                    throw load_object_failure("gltf2 texture invalid source");
                }

                // Sampler settings to use in this texture.
                rapidjson::Value::ConstMemberIterator sampler_member(t->FindMember("sampler"));
                if (sampler_member == t->MemberEnd()) {
                    throw load_object_failure("gltf2 texture missing sampler");
                }

                int sampler_index = sampler_member->value.GetInt();

                if (sampler_index < 0 || sampler_index >= m_samplers.size()) {
                    throw load_object_failure("gltf2 texture invalid sampler");
                }

                // Start the image importer as early as possible to get the most parallelism.
                formatted_string_append(prefix, "%s::texture::%d", m_object_prefix.c_str(), texture_id);
                m_texture_importers.emplace_back(
                    texture::gli_importer::create(m_image_paths[source_index], prefix, m_load_record, &m_samplers[sampler_index])
                    );
            }
        }

        void gltf2_importer::async_mesh_loader::parse_meshes(rapidjson::Document const& doc)
        {
            rapidjson::Value::ConstMemberIterator meshes(doc.FindMember("meshes"));
            if (meshes == doc.MemberEnd()) {
                return;
            }
            ELECTROSLAG_CHECK(meshes->value.IsArray());

            for (rapidjson::Value::ConstValueIterator m(meshes->value.Begin());
                 m != meshes->value.End();
                 ++m) {

                mesh this_mesh;

                // Mesh is just an array of primitive objects
                rapidjson::Value::ConstMemberIterator primitives(m->FindMember("primitives"));
                ELECTROSLAG_CHECK(primitives->value.IsArray());

                for (rapidjson::Value::ConstValueIterator p(primitives->value.Begin());
                    p != primitives->value.End();
                    ++p) {

                    primitive this_primitive;

                    // IBO is an index to an accessor (which in turn references a buffer_view)
                    rapidjson::Value::ConstMemberIterator index_accessor_member(p->FindMember("indices"));
                    if (index_accessor_member != p->MemberEnd()) {
                        this_primitive.index_accessor = index_accessor_member->value.GetInt();

                        if (this_primitive.index_accessor < 0 || this_primitive.index_accessor >= m_accessors.size()) {
                            throw load_object_failure("gltf2 invalid accessor index");
                        }

                        accessor& a = m_accessors.at(this_primitive.index_accessor);
                        if ((a.type != accessor_type_scalar) ||
                            ((a.component_type != accessor_component_type_unsigned_byte) &&
                             (a.component_type != accessor_component_type_unsigned_short) &&
                             (a.component_type != accessor_component_type_unsigned_int))) {
                            throw load_object_failure("gltf2 invalid index buffer accessor");
                        }

                        buffer_view& bv = m_buffer_views.at(a.buffer_view);
                        if (bv.target != buffer_view_target_element_array_buffer) {
                            throw load_object_failure("gltf2 invalid index buffer buffer view");
                        }
                    }

                    // Single material per mesh, by index
                    rapidjson::Value::ConstMemberIterator material_member(p->FindMember("material"));
                    if (material_member == p->MemberEnd()) {
                        throw load_object_failure("gltf2 material member not present");
                    }

                    this_primitive.material = material_member->value.GetInt();

                    if (this_primitive.material < 0 || this_primitive.material >= m_materials.size()) {
                        throw load_object_failure("gltf2 invalid material index");
                    }

                    // Primitive mode (tri-list, and so on)
                    rapidjson::Value::ConstMemberIterator primitive_mode_member(p->FindMember("mode"));
                    if (primitive_mode_member != p->MemberEnd()) {
                        this_primitive.primitive_mode = static_cast<primitive_mode_type>(primitive_mode_member->value.GetInt());
                    }

                    // Vertex attributes are an array of accessor indexes
                    rapidjson::Value::ConstMemberIterator attributes_member(p->FindMember("attributes"));
                    if (attributes_member == p->MemberEnd()) {
                        throw load_object_failure("gltf2 attributes member not present");
                    }

                    // Vertex position
                    rapidjson::Value::ConstMemberIterator position_member(attributes_member->value.FindMember("POSITION"));
                    if (position_member != attributes_member->value.MemberEnd()) {
                        this_primitive.position_attrib_accessor = position_member->value.GetInt();

                        if (this_primitive.position_attrib_accessor < 0 || this_primitive.position_attrib_accessor >= m_accessors.size()) {
                            throw load_object_failure("gltf2 invalid accessor index");
                        }

                        accessor& a = m_accessors.at(this_primitive.position_attrib_accessor);
                        if ((a.type != accessor_type_vec3) ||
                            (a.component_type != accessor_component_type_float)) {
                            throw load_object_failure("gltf2 invalid position accessor");
                        }
                    }

                    // Vertex normal direction
                    rapidjson::Value::ConstMemberIterator normal_member(attributes_member->value.FindMember("NORMAL"));
                    if (normal_member != attributes_member->value.MemberEnd()) {
                        this_primitive.normal_attrib_accessor = normal_member->value.GetInt();

                        if (this_primitive.normal_attrib_accessor < 0 || this_primitive.normal_attrib_accessor >= m_accessors.size()) {
                            throw load_object_failure("gltf2 invalid accessor index");
                        }

                        accessor& a = m_accessors.at(this_primitive.normal_attrib_accessor);
                        if ((a.type != accessor_type_vec3) ||
                            (a.component_type != accessor_component_type_float)) {
                            throw load_object_failure("gltf2 invalid normal accessor");
                        }
                    }

                    // Vertex tangent space
                    rapidjson::Value::ConstMemberIterator tangent_member(attributes_member->value.FindMember("TANGENT"));
                    if (tangent_member != attributes_member->value.MemberEnd()) {
                        this_primitive.tangent_attrib_accessor = tangent_member->value.GetInt();

                        if (this_primitive.tangent_attrib_accessor < 0 || this_primitive.tangent_attrib_accessor >= m_accessors.size()) {
                            throw load_object_failure("gltf2 invalid accessor index");
                        }

                        accessor& a = m_accessors.at(this_primitive.tangent_attrib_accessor);
                        if ((a.type != accessor_type_vec4) ||
                            (a.component_type != accessor_component_type_float)) {
                            throw load_object_failure("gltf2 invalid tangent accessor");
                        }
                    }

                    // Currently allow two sets of texture coordinates per vertex
                    rapidjson::Value::ConstMemberIterator texcoord_0_member(attributes_member->value.FindMember("TEXCOORD_0"));
                    if (texcoord_0_member != attributes_member->value.MemberEnd()) {
                        this_primitive.texcoord_0_attrib_accessor = texcoord_0_member->value.GetInt();

                        if (this_primitive.texcoord_0_attrib_accessor < 0 || this_primitive.texcoord_0_attrib_accessor >= m_accessors.size()) {
                            throw load_object_failure("gltf2 invalid accessor index");
                        }

                        accessor& a = m_accessors.at(this_primitive.texcoord_0_attrib_accessor);
                        if ((a.type != accessor_type_vec2) ||
                            ((a.component_type != accessor_component_type_float) &&
                             (a.component_type != accessor_component_type_unsigned_byte) &&
                             (a.component_type != accessor_component_type_unsigned_short))) {
                            throw load_object_failure("gltf2 invalid texcoord_0 accessor");
                        }
                    }

                    // The other vertex texture coordinate set.
                    rapidjson::Value::ConstMemberIterator texcoord_1_member(attributes_member->value.FindMember("TEXCOORD_1"));
                    if (texcoord_1_member != attributes_member->value.MemberEnd()) {
                        this_primitive.texcoord_1_attrib_accessor = texcoord_1_member->value.GetInt();

                        if (this_primitive.texcoord_1_attrib_accessor < 0 || this_primitive.texcoord_1_attrib_accessor >= m_accessors.size()) {
                            throw load_object_failure("gltf2 invalid accessor index");
                        }

                        accessor& a = m_accessors.at(this_primitive.texcoord_1_attrib_accessor);
                        if ((a.type != accessor_type_vec2) ||
                            ((a.component_type != accessor_component_type_float) &&
                             (a.component_type != accessor_component_type_unsigned_byte) &&
                             (a.component_type != accessor_component_type_unsigned_short))) {
                            throw load_object_failure("gltf2 invalid texcoord_1 accessor");
                        }
                    }

                    // Allow a single per-vertex color as well.
                    rapidjson::Value::ConstMemberIterator color_0_member(attributes_member->value.FindMember("COLOR_0"));
                    if (color_0_member != attributes_member->value.MemberEnd()) {
                        this_primitive.color_0_attrib_accessor = color_0_member->value.GetInt();

                        if (this_primitive.color_0_attrib_accessor < 0 || this_primitive.color_0_attrib_accessor >= m_accessors.size()) {
                            throw load_object_failure("gltf2 invalid accessor index");
                        }

                        accessor& a = m_accessors.at(this_primitive.color_0_attrib_accessor);
                        if (((a.type != accessor_type_vec3) &&
                             (a.type != accessor_type_vec4)) ||
                            ((a.component_type != accessor_component_type_float) &&
                            (a.component_type != accessor_component_type_unsigned_byte) &&
                                (a.component_type != accessor_component_type_unsigned_short))) {
                            throw load_object_failure("gltf2 invalid color_0 accessor");
                        }
                    }

                    this_mesh.primitives.emplace_back(this_primitive);
                }

                m_meshes.emplace_back(this_mesh);
            }
        }

        void gltf2_importer::async_mesh_loader::parse_materials(rapidjson::Document const& doc)
        {
            rapidjson::Value::ConstMemberIterator materials(doc.FindMember("materials"));
            if (materials == doc.MemberEnd()) {
                return;
            }
            ELECTROSLAG_CHECK(materials->value.IsArray());

            for (rapidjson::Value::ConstValueIterator mat(materials->value.Begin());
                 mat != materials->value.End();
                 ++mat) {

                material this_material;

                // PBR describes color related material properties.
                rapidjson::Value::ConstMemberIterator pbr_member(mat->FindMember("pbrMetallicRoughness"));
                if (pbr_member == mat->MemberEnd()) {
                    throw load_object_failure("gltf2 material missing pbr");
                }

                // Base color also goes by albedo.
                rapidjson::Value::ConstMemberIterator base_color_map_member(pbr_member->value.FindMember("baseColorTexture"));
                if (base_color_map_member != pbr_member->value.MemberEnd()) {
                    parse_material_map(base_color_map_member, &this_material.base_color_map);
                }

                rapidjson::Value::ConstMemberIterator base_color_member(pbr_member->value.FindMember("baseColorFactor"));
                if (base_color_member != pbr_member->value.MemberEnd()) {
                    for (int i = 0; i < 4; ++i) {
                        this_material.base_color[i] = base_color_member->value.GetArray()[i].GetFloat();
                    }
                }

                rapidjson::Value::ConstMemberIterator metallic_member(pbr_member->value.FindMember("metallicFactor"));
                if (metallic_member != pbr_member->value.MemberEnd()) {
                    this_material.metallic = metallic_member->value.GetFloat();
                }

                rapidjson::Value::ConstMemberIterator roughness_member(pbr_member->value.FindMember("roughnessFactor"));
                if (roughness_member != pbr_member->value.MemberEnd()) {
                    this_material.roughness = roughness_member->value.GetFloat();
                }

                rapidjson::Value::ConstMemberIterator metallic_roughness_map_member(pbr_member->value.FindMember("metallicRoughnessTexture"));
                if (metallic_roughness_map_member != pbr_member->value.MemberEnd()) {
                    parse_material_map(metallic_roughness_map_member, &this_material.metallic_roughness_map);
                }

                rapidjson::Value::ConstMemberIterator normal_map_member(mat->FindMember("normalTexture"));
                if (normal_map_member != mat->MemberEnd()) {
                    parse_material_map(normal_map_member, &this_material.normal_map);

                    rapidjson::Value::ConstMemberIterator normal_map_scalar_member(normal_map_member->value.FindMember("scale"));
                    if (normal_map_scalar_member != normal_map_member->value.MemberEnd()) {
                        this_material.normal_scalar = normal_map_scalar_member->value.GetFloat();
                    }
                }

                rapidjson::Value::ConstMemberIterator occulsion_map_member(mat->FindMember("occulsionTexture"));
                if (occulsion_map_member != mat->MemberEnd()) {
                    parse_material_map(occulsion_map_member, &this_material.occulsion_map);

                    rapidjson::Value::ConstMemberIterator occulsion_map_strength_member(occulsion_map_member->value.FindMember("strength"));
                    if (occulsion_map_strength_member != occulsion_map_member->value.MemberEnd()) {
                        this_material.occulsion_strength = occulsion_map_strength_member->value.GetFloat();
                    }
                }

                rapidjson::Value::ConstMemberIterator emissive_map_member(mat->FindMember("emissiveTexture"));
                if (emissive_map_member != mat->MemberEnd()) {
                    parse_material_map(emissive_map_member, &this_material.emissive_map);
                }

                rapidjson::Value::ConstMemberIterator emissive_color_member(mat->FindMember("emissiveFactor"));
                if (emissive_color_member != mat->MemberEnd()) {
                    for (int i = 0; i < 4; ++i) {
                        this_material.emissive_color[i] = emissive_color_member->value.GetArray()[i].GetFloat();
                    }
                }

                rapidjson::Value::ConstMemberIterator alpha_mode_member(mat->FindMember("alphaMode"));
                if (alpha_mode_member != mat->MemberEnd()) {
                    std::string alpha_mode_string(alpha_mode_member->value.GetString());
                    if (alpha_mode_string == "OPAQUE") {
                        this_material.alpha_mode = alpha_mode_type_opaque;
                    }
                    else if (alpha_mode_string == "MASK") {
                        this_material.alpha_mode = alpha_mode_type_mask;
                    }
                    else if (alpha_mode_string == "BLEND") {
                        this_material.alpha_mode = alpha_mode_type_blend;
                    }
                    else {
                        throw load_object_failure("gltf2 material alpha mode is not valid");
                    }
                }
                else {
                    this_material.alpha_mode = alpha_mode_type_opaque;
                }

                rapidjson::Value::ConstMemberIterator alpha_cutoff_member(mat->FindMember("alphaCutoff"));
                if (alpha_cutoff_member != mat->MemberEnd()) {
                    if (this_material.alpha_mode != alpha_mode_type_mask) {
                        throw load_object_failure("gltf2 material alpha cutoff for not mask mode is not valid");
                    }

                    this_material.alpha_cutoff = alpha_cutoff_member->value.GetFloat();
                }

                rapidjson::Value::ConstMemberIterator double_sided_member(mat->FindMember("doubleSided"));
                if (double_sided_member != mat->MemberEnd()) {
                    this_material.double_sided = double_sided_member->value.GetBool();
                }

                m_materials.emplace_back(this_material);
            }
        }

        void gltf2_importer::async_mesh_loader::parse_material_map(
            rapidjson::Value::ConstMemberIterator const& map_member,
            material_map* out_map
            )
        {
            // Which texture map to reference
            rapidjson::Value::ConstMemberIterator index_member(map_member->value.FindMember("index"));
            if (index_member == map_member->value.MemberEnd()) {
                throw load_object_failure("gltf2 missing texture map index");
            }

            out_map->texture_index = index_member->value.GetInt();

            if (out_map->texture_index < 0 || out_map->texture_index >= m_texture_importers.size()) {
                throw load_object_failure("gltf2 invalid texture map index");
            }

            // Which set of texture coordinates to sample with
            rapidjson::Value::ConstMemberIterator coord_member(map_member->value.FindMember("texCoord"));
            if (coord_member != map_member->value.MemberEnd()) {

                out_map->texcoord_index = coord_member->value.GetInt();

                if (out_map->texcoord_index < 0 || out_map->texcoord_index > 1) {
                    throw load_object_failure("gltf2 invalid texture coordinate index");
                }
            }
        }

        void gltf2_importer::async_mesh_loader::parse_nodes(rapidjson::Document const& doc)
        {
            rapidjson::Value::ConstMemberIterator nodes(doc.FindMember("nodes"));
            if (nodes == doc.MemberEnd()) {
                return;
            }
            ELECTROSLAG_CHECK(nodes->value.IsArray());

            for (rapidjson::Value::ConstValueIterator n(nodes->value.Begin());
                 n != nodes->value.End();
                 ++n) {

                node this_node;

                // Each node is an "instance" of a mesh.
                rapidjson::Value::ConstMemberIterator mesh_member(n->FindMember("mesh"));
                if (mesh_member != n->MemberEnd()) {
                    this_node.mesh = mesh_member->value.GetInt();

                    if (this_node.mesh < 0 || this_node.mesh >= m_meshes.size()) {
                        throw load_object_failure("gltf2 invalid mesh index");
                    }
                }

                // Node transform is either by matrix or scale-rotate-translate.
                rapidjson::Value::ConstMemberIterator matrix_member(n->FindMember("matrix"));
                if (matrix_member != n->MemberEnd()) {
                    this_node.transform_type = node_transform_type_matrix;

                    for (int i = 0; i < 16; ++i) {
                        int mi = i / 4;
                        int mj = i % 4;
                        this_node.matrix[mi][mj] = matrix_member->value.GetArray()[i].GetFloat();
                    }
                }
                else {
                    rapidjson::Value::ConstMemberIterator scale_member(n->FindMember("scale"));
                    rapidjson::Value::ConstMemberIterator rotation_member(n->FindMember("rotation"));
                    rapidjson::Value::ConstMemberIterator translation_member(n->FindMember("translation"));

                    if (scale_member != n->MemberEnd() ||
                        rotation_member != n->MemberEnd() ||
                        translation_member != n->MemberEnd()) {
                        this_node.transform_type = node_transform_type_srt;

                        if (scale_member != n->MemberEnd()) {
                            for (int i = 0; i < 3; ++i) {
                                this_node.scale[i] = scale_member->value.GetArray()[i].GetFloat();
                            }
                        }
                        else {
                            this_node.scale = glm::f32vec3(1.0f, 1.0f, 1.0f);
                        }

                        if (rotation_member != n->MemberEnd()) {
                            for (int i = 0; i < 4; ++i) {
                                this_node.rotate[i] = rotation_member->value.GetArray()[i].GetFloat();
                            }
                        }
                        else {
                            this_node.rotate = glm::f32quat(0.0f, 0.0f, 0.0f, 1.0f);
                        }

                        if (translation_member != n->MemberEnd()) {
                            for (int i = 0; i < 3; ++i) {
                                this_node.translate[i] = translation_member->value.GetArray()[i].GetFloat();
                            }
                        }
                        else {
                            this_node.translate = glm::f32vec3(0.0f, 0.0f, 0.0f);
                        }
                    }
                }

                m_nodes.emplace_back(this_node);
            }
        }

        void gltf2_importer::async_mesh_loader::parse_scenes(rapidjson::Document const& doc)
        {
            rapidjson::Value::ConstMemberIterator scenes(doc.FindMember("scenes"));
            if (scenes == doc.MemberEnd()) {
                throw load_object_failure("gltf2 no scene array");
            }
            ELECTROSLAG_CHECK(scenes->value.IsArray());

            for (rapidjson::Value::ConstValueIterator s(scenes->value.Begin());
                 s != scenes->value.End();
                 ++s) {

                scene this_scene;

                // A scene is just an array of node indexes.
                rapidjson::Value::ConstMemberIterator nodes(s->FindMember("nodes"));
                ELECTROSLAG_CHECK(nodes->value.IsArray());

                for (rapidjson::Value::ConstValueIterator n(nodes->value.Begin());
                     n != nodes->value.End();
                     ++n) {

                    int node_index = n->GetInt();

                    if (node_index < 0 || node_index >= m_nodes.size()) {
                        throw load_object_failure("gltf2 invalid node index");
                    }

                    this_scene.nodes.emplace_back(node_index);
                }
                m_scenes.emplace_back(this_scene);
            }

            // The gltf2 allows for a single "default" scene to be selected.
            rapidjson::Value::ConstMemberIterator scene_member(doc.FindMember("scene"));
            if (scene_member != doc.MemberEnd()) {

                m_scene = scene_member->value.GetInt();

                if (m_scene < 0 || m_scene >= m_scenes.size()) {
                    throw load_object_failure("gltf2 invalid scene index");
                }
            }
        }

        renderer::instance_descriptor::ref gltf2_importer::async_mesh_loader::create_descriptors()
        {
            std::vector<scene>::iterator s(m_scenes.begin());
            int scene_id = 0;
            while (s != m_scenes.end()) {
                s->desc = renderer::instance_descriptor::create();

                std::string scene_desc_name;
                formatted_string_append(scene_desc_name, "%s::scene::%d", m_object_prefix.c_str(), scene_id);
                s->desc->set_name(scene_desc_name);

                std::vector<int>::const_iterator node_index_iter(s->nodes.begin());
                while (node_index_iter != s->nodes.end()) {
                    std::string node_name;
                    formatted_string_append(node_name, "%s::node::%d", m_object_prefix.c_str(), *node_index_iter);

                    s->desc->insert_child_instance()
                }

                m_load_record->insert_object(s->desc);
                ++s; ++scene_id;
            }

            // The top level instance has a single child pointing at the default scene.
            renderer::instance_descriptor::ref scene_desc;
            if (m_scene >= 0) {
                m_scene = 0;
            }

            scene_desc = renderer::instance_descriptor::create();
            scene_desc->set_name(m_object_prefix + "::scene");
            scene_desc->insert_child_instance(m_scenes[m_scene].desc);
            scene_desc->set_transform(m_base_transform);
            m_load_record->insert_object(scene_desc);

            return  (scene_desc);

            /*
            // Name the buffer descriptor.
            std::string buffer_desc_name;
            formatted_string_append(buffer_desc_name, "%s::buffer::%d", m_object_prefix.c_str(), buffer_id);

            // Make a buffer descriptor.
            graphics::buffer_descriptor::ref buffer(graphics::buffer_descriptor::create());
            buffer->set_name(buffer_desc_name);
            buffer->set_buffer_memory_map(graphics::buffer_memory_map_static);
            buffer->set_buffer_memory_caching(graphics::buffer_memory_caching_static);
            buffer->set_initialized_data(buffer_data);
            m_load_record->insert_object(buffer);

            if (r->transform.is_valid()) {
                transform_descriptor::ref composed_transform(transform_descriptor::create());
                composed_transform->set_hash(transform_desc->get_hash() ^ r->transform->get_hash());
                composed_transform->set_translate(transform_desc->get_translate() + r->transform->get_translate());
                composed_transform->set_scale(transform_desc->get_scale() * r->transform->get_scale());
                // Quaternion multiplication is not commutative. (Existing instance rotation before new instance rotation.)
                composed_transform->set_rotation(transform_desc->get_rotation() * r->transform->get_rotation());
                ar->get_load_record()->insert_object(composed_transform);

                m_composite_scene->insert_renderable(r->descriptor, composed_transform, r->name_hash);
            }
            */
        }
    }
}
