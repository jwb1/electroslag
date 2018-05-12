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
#include "electroslag/name_table.hpp"
#include "electroslag/serialize/json_archive.hpp"
#include "electroslag/serialize/serializable_object_interface.hpp"
#include "electroslag/serialize/base64.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace serialize {
        json_archive_reader::json_archive_reader(stream_interface* s)
            : json_archive(s)
            , m_iterating(false)
        {
            long long stream_size = s->get_size();
            if (stream_size > 0) {
                m_stream_buffer.reset(new char[stream_size + 1]);

                s->read(m_stream_buffer.get(), stream_size);
                m_stream_buffer[stream_size] = '\0';
                m_document.ParseInsitu<rapidjson::kParseInsituFlag | rapidjson::kParseCommentsFlag>(m_stream_buffer.get());

                // Sanity checking.
                /* Waiting new version of RapidJSON
                m_document.ParseInsitu<rapidjson::kParseInsituFlag | rapidjson::kParseCommentsFlag>(m_stream_buffer.get());
                if (m_document.HasParseError()) {
                    ELECTROSLAG_LOG_SERIALIZE(
                        "JSON parse error: offset %u, line %u, col %u): %s",
                        (unsigned)m_document.GetErrorOffset(),
                        (unsigned)m_document.GetErrorLine(),
                        (unsigned)m_document.GetErrorColumn(),
                        rapidjson::GetParseError_En(m_document.GetParseError())
                        );
                    throw check_failure("JSON Parse error");
                }
                */

                if (m_document.HasParseError()) {
                    ELECTROSLAG_LOG_SERIALIZE(
                        "JSON parse error: %s (offset = %u)",
                        rapidjson::GetParseError_En(m_document.GetParseError()),
                        m_document.GetErrorOffset()
                        );
                    
                }
                ELECTROSLAG_CHECK(m_document.IsObject());
                ELECTROSLAG_CHECK(m_document.HasMember("d:version"));
                ELECTROSLAG_CHECK(m_document["d:version"].IsUint());
                ELECTROSLAG_CHECK(m_document["d:version"].GetUint() == json_version_number);
            }
        }

        bool json_archive_reader::next_object(
            unsigned long long* out_type_hash,
            unsigned long long* out_name_hash
            )
        {
            // The first call to next_object needs to point the reader at the -first- object; MemberBegin.
            if (m_iterating) {
                ++m_current_object;
            }
            else {
                m_current_object = m_document.MemberBegin();
                m_iterating = true;
            }

            // There are value members directly in the document for control purposes. Only
            // enumerate objects.
            while ((m_current_object != m_document.MemberEnd()) && (!m_current_object->value.IsObject())) {
                ++m_current_object;
            }

            if (m_current_object != m_document.MemberEnd()) {
                get_object_type(out_type_hash);
                get_object_name(out_name_hash);

                return (true);
            }
            else {
                return (false);
            }
        }

        void json_archive_reader::get_object_type(unsigned long long* out_type_hash)
        {
            // Type may be specified by hash or string name in json.
            rapidjson::Value::ConstMemberIterator type_val(m_current_object->value.FindMember("type_hash"));
            if (type_val != m_current_object->value.MemberEnd()) {
                ELECTROSLAG_CHECK(type_val->value.IsUint64());
                *out_type_hash = type_val->value.GetUint64();
            }
            else {
                type_val = m_current_object->value.FindMember("type_name");
                ELECTROSLAG_CHECK(type_val != m_current_object->value.MemberEnd());
                ELECTROSLAG_CHECK(type_val->value.IsString());

                unsigned long long type_hash = hash_string_runtime(type_val->value.GetString());

                ELECTROSLAG_CHECK(get_database()->find_type(type_hash));
                *out_type_hash = type_hash;
            }
        }

        void json_archive_reader::get_object_name(unsigned long long* out_name_hash)
        {
            // Name may be specified by hash or object name in json.
            // If the name_hash is specified, then the object name is ignored!
            rapidjson::Value::ConstMemberIterator name_val(m_current_object->value.FindMember("name_hash"));
            if (name_val != m_current_object->value.MemberEnd()) {
                ELECTROSLAG_CHECK(name_val->value.IsUint64());
                *out_name_hash = name_val->value.GetUint64();
            }
            else {
                ELECTROSLAG_CHECK(m_current_object->name.IsString());

                unsigned long long name_hash = hash_string_runtime(m_current_object->name.GetString());

                get_name_table()->insert(m_current_object->name.GetString(), name_hash);
                *out_name_hash = name_hash;
            }
        }

        bool json_archive_reader::read_buffer(std::string const& name, void* buffer, int sizeof_buffer)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }
            
            if (val->value.IsString()) {
                base64::decode(
                    val->value.GetString(),
                    val->value.GetStringLength(),
                    buffer,
                    sizeof_buffer
                    );
                return (true);
            }
            else if (val->value.IsArray()) {
                ELECTROSLAG_CHECK((sizeof_buffer % val->value.Capacity()) == 0);
                int element_sizeof = sizeof_buffer / val->value.Capacity();

                // This code can only handle arrays of a single type, so try to work out
                // which type we should use.
                int element_float = 0;
                int element_int = 0;
                int element_uint = 0;
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                    array_itr != val->value.End();
                    ++array_itr) {

                    if (array_itr->IsUint()) {
                        ++element_uint;
                    }
                    if (array_itr->IsInt()) {
                        ++element_int;
                    }
                    if (array_itr->IsDouble()) {
                        ++element_float;
                    }
                }

                // Copy the values, converted to the right type, in to the output buffer.
                int array_capacity = static_cast<int>(val->value.Capacity());
                if (element_uint == array_capacity) {
                    return (read_uint_array_to_buffer(element_sizeof, buffer, val));
                }
                else if (element_int == array_capacity) {
                    return (read_int_array_to_buffer(element_sizeof, buffer, val));
                }
                else if (element_float == array_capacity) {
                    return (read_float_array_to_buffer(element_sizeof, buffer, val));
                }
            }

            return (false);
        }

        bool json_archive_reader::read_string(std::string const& name, std::string* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsString()) {
                out_value->assign(val->value.GetString(), val->value.GetStringLength());
                return (true);
            }
            else if (val->value.IsArray()) {
                out_value->clear();
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                    array_itr != val->value.End();
                    ++array_itr) {

                    ELECTROSLAG_CHECK(array_itr->IsString());
                    out_value->append(array_itr->GetString(), array_itr->GetStringLength());
                    out_value->append("\r\n");
                }
                return (true);
            }

            return (false);
        }

        bool json_archive_reader::read_uint8(std::string const& name, uint8_t* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsUint()) {
                unsigned int value = val->value.GetUint();
                ELECTROSLAG_CHECK(value <= UINT8_MAX);
                *out_value = static_cast<uint8_t>(value);
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_uint16(std::string const& name, uint16_t* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsUint()) {
                unsigned int value = val->value.GetUint();
                ELECTROSLAG_CHECK(value <= UINT16_MAX);
                *out_value = static_cast<uint16_t>(value);
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_uint32(std::string const& name, uint32_t* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsUint()) {
                *out_value = val->value.GetUint();
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_uint64(std::string const& name, uint64_t* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsUint64()) {
                *out_value = val->value.GetUint64();
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_int8(std::string const& name, int8_t* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsInt()) {
                int value = val->value.GetInt();
                ELECTROSLAG_CHECK(value <= INT8_MAX && value >= INT8_MIN);
                *out_value = static_cast<int8_t>(value);
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_int16(std::string const& name, int16_t* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsInt()) {
                int value = val->value.GetInt();
                ELECTROSLAG_CHECK(value <= INT16_MAX && value >= INT16_MIN);
                *out_value = static_cast<int16_t>(value);
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_int32(std::string const& name, int32_t* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsInt()) {
                *out_value = val->value.GetInt();
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_int64(std::string const& name, int64_t* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsInt64()) {
                *out_value = val->value.GetInt64();
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_float(std::string const& name, float* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsNumber()) {
                *out_value = val->value.GetFloat();
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_double(std::string const& name, double* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsNumber()) {
                *out_value = val->value.GetDouble();
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_name_hash(std::string const& name, unsigned long long* out_value)
        {
            rapidjson::Value::ConstMemberIterator val(m_current_object->value.FindMember(name));
            if (val == m_current_object->value.MemberEnd()) {
                return (false);
            }

            if (val->value.IsUint64()) {
                *out_value = val->value.GetUint64();
                return (true);
            }
            else if (val->value.IsString()) {
                *out_value = hash_string_runtime(val->value.GetString());
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_float_array_to_buffer(int element_sizeof, void* buffer, rapidjson::Value::ConstMemberIterator& val)
        {
            if (element_sizeof == sizeof(float)) {
                float* float_buffer = static_cast<float*>(buffer);
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                     array_itr != val->value.End();
                     ++array_itr) {
                    *float_buffer++ = static_cast<float>(array_itr->GetDouble());
                }
                return (true);
            }
            else if (element_sizeof == sizeof(double)) {
                double* double_buffer = static_cast<double*>(buffer);
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                     array_itr != val->value.End();
                     ++array_itr) {
                    *double_buffer++ = array_itr->GetDouble();
                }
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_uint_array_to_buffer(int element_sizeof, void* buffer, rapidjson::Value::ConstMemberIterator& val)
        {
            if (element_sizeof == sizeof(uint8_t)) {
                uint8_t* uint_buffer = static_cast<uint8_t*>(buffer);
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                    array_itr != val->value.End();
                    ++array_itr) {
                    unsigned int u = array_itr->GetUint();
                    ELECTROSLAG_CHECK(u <= UINT8_MAX);
                    *uint_buffer++ = static_cast<uint8_t>(u);
                }
                return (true);
            }
            else if (element_sizeof == sizeof(uint16_t)) {
                uint16_t* uint_buffer = static_cast<uint16_t*>(buffer);
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                    array_itr != val->value.End();
                    ++array_itr) {
                    unsigned int u = array_itr->GetUint();
                    ELECTROSLAG_CHECK(u <= UINT16_MAX);
                    *uint_buffer++ = static_cast<uint16_t>(u);
                }
                return (true);
            }
            else if (element_sizeof == sizeof(uint32_t)) {
                uint32_t* uint_buffer = static_cast<uint32_t*>(buffer);
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                    array_itr != val->value.End();
                    ++array_itr) {
                    *uint_buffer++ = array_itr->GetUint();
                }
                return (true);
            }
            else if (element_sizeof == sizeof(uint64_t)) {
                uint64_t* uint_buffer = static_cast<uint64_t*>(buffer);
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                    array_itr != val->value.End();
                    ++array_itr) {
                    *uint_buffer++ = array_itr->GetUint64();
                }
                return (true);
            }
            else {
                return (false);
            }
        }

        bool json_archive_reader::read_int_array_to_buffer(int element_sizeof, void* buffer, rapidjson::Value::ConstMemberIterator& val)
        {
            if (element_sizeof == sizeof(int8_t)) {
                int8_t* int_buffer = static_cast<int8_t*>(buffer);
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                    array_itr != val->value.End();
                    ++array_itr) {
                    int i = array_itr->GetInt();
                    ELECTROSLAG_CHECK(i <= INT8_MAX && i >= INT8_MIN);
                    *int_buffer++ = static_cast<int8_t>(i);
                }
                return (true);
            }
            else if (element_sizeof == sizeof(int16_t)) {
                int16_t* int_buffer = static_cast<int16_t*>(buffer);
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                    array_itr != val->value.End();
                    ++array_itr) {
                    int i = array_itr->GetInt();
                    ELECTROSLAG_CHECK(i <= INT16_MAX && i >= INT16_MIN);
                    *int_buffer++ = static_cast<int16_t>(i);
                }
                return (true);
            }
            else if (element_sizeof == sizeof(int32_t)) {
                int32_t* int_buffer = static_cast<int32_t*>(buffer);
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                    array_itr != val->value.End();
                    ++array_itr) {
                    *int_buffer++ = array_itr->GetInt();
                }
                return (true);
            }
            else if (element_sizeof == sizeof(int64_t)) {
                int64_t* int_buffer = static_cast<int64_t*>(buffer);
                for (rapidjson::Value::ConstValueIterator array_itr = val->value.Begin();
                    array_itr != val->value.End();
                    ++array_itr) {
                    *int_buffer++ = array_itr->GetInt64();
                }
                return (true);
            }
            else {
                return (false);
            }
        }

        json_archive_writer::json_archive_writer(stream_interface* s)
            : json_archive(s)
        {
            // Parse empty document to get started.
            char empty_json[] = "{ }";
            m_document.Parse(empty_json);

            ELECTROSLAG_CHECK(!m_document.HasParseError());
        }

        json_archive_writer::~json_archive_writer()
        {
            rapidjson::Value version(rapidjson::kNumberType);
            version.SetUint(json_version_number);
            m_document.AddMember("d:version", version, m_document.GetAllocator());

            // Write the DOM back to the stream.
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            m_document.Accept(writer);
            long long size = buffer.GetSize();

            m_stream->set_size(size);
            m_stream->seek(0, stream_seek_mode_from_start);
            m_stream->write(buffer.GetString(), size);
        }

        void json_archive_writer::start_object(serializable_object_interface* obj)
        {
            unsigned long long obj_hash = obj->get_hash();
            ELECTROSLAG_CHECK(obj_hash != 0); // I guess a valid hash of 0 is possible. Seems _highly_ unlikely.
            unsigned long long type_hash = obj->get_type_hash();
            ELECTROSLAG_CHECK(type_hash != 0); 

            // Check for duplication; sub-objects might be referred to multiple times.
            saved_object_vector::const_iterator i(m_saved_objects.begin());
            while (i != m_saved_objects.end()) {
                if (*i == obj_hash) {
                    // This object has already been saved; assume this is a dupe!
                    m_write_enable = false;
                    return;
                }
                ++i;
            }

            // Check to see if the object should be filtered due to load key.
            if (m_load_record.is_valid()) {
                if (!m_load_record->has_object(obj)) {
                    m_write_enable = false;
                    return;
                }
            }

            m_write_enable = true;
            m_saved_objects.emplace_back(obj_hash);

            std::string o_name;
            formatted_string_append(o_name, "0x%016llX", obj_hash);
            rapidjson::Value o_name_val(rapidjson::kStringType);
            o_name_val.SetString(o_name, m_document.GetAllocator());

            rapidjson::Value o(rapidjson::kObjectType);

            m_document.AddMember(o_name_val, o, m_document.GetAllocator());
            m_current_object = m_document.FindMember(o_name);

            rapidjson::Value type_hash_val(rapidjson::kNumberType);
            type_hash_val.SetUint64(type_hash);
            m_current_object->value.AddMember("type_hash", type_hash_val, m_document.GetAllocator());

            rapidjson::Value obj_hash_val(rapidjson::kNumberType);
            obj_hash_val.SetUint64(obj_hash);
            m_current_object->value.AddMember("name_hash", obj_hash_val, m_document.GetAllocator());
        }

        void json_archive_writer::write_buffer(std::string const& name, void const* buffer, int sizeof_buffer)
        {
            if (m_write_enable) {
                // The encoded size is unknown in advance. Go at it 4k increments.
                std::string encoded_buffer;
                base64::encode(
                    buffer,
                    sizeof_buffer,
                    encoded_buffer
                    );

                write_string(name, encoded_buffer);
            }
        }

        void json_archive_writer::write_string(std::string const& name, std::string const& value)
        {
            if (m_write_enable) {
                rapidjson::Value name_val(rapidjson::kStringType);
                name_val.SetString(name, m_document.GetAllocator());

                rapidjson::Value val(rapidjson::kStringType);
                val.SetString(value, m_document.GetAllocator());

                m_current_object->value.AddMember(name_val, val, m_document.GetAllocator());
            }
        }

        void json_archive_writer::write_int8(std::string const& name, int8_t value)
        {
            if (m_write_enable) {
                rapidjson::Value name_val(rapidjson::kStringType);
                name_val.SetString(name, m_document.GetAllocator());

                rapidjson::Value val(rapidjson::kNumberType);
                val.SetInt(value);

                m_current_object->value.AddMember(name_val, val, m_document.GetAllocator());
            }
        }

        void json_archive_writer::write_int16(std::string const& name, int16_t value)
        {
            if (m_write_enable) {
                rapidjson::Value name_val(rapidjson::kStringType);
                name_val.SetString(name, m_document.GetAllocator());

                rapidjson::Value val(rapidjson::kNumberType);
                val.SetInt(value);

                m_current_object->value.AddMember(name_val, val, m_document.GetAllocator());
            }
        }

        void json_archive_writer::write_int32(std::string const& name, int32_t value)
        {
            if (m_write_enable) {
                rapidjson::Value name_val(rapidjson::kStringType);
                name_val.SetString(name, m_document.GetAllocator());

                rapidjson::Value val(rapidjson::kNumberType);
                val.SetInt(value);

                m_current_object->value.AddMember(name_val, val, m_document.GetAllocator());
            }
        }

        void json_archive_writer::write_int64(std::string const& name, int64_t value)
        {
            if (m_write_enable) {
                rapidjson::Value name_val(rapidjson::kStringType);
                name_val.SetString(name, m_document.GetAllocator());

                rapidjson::Value val(rapidjson::kNumberType);
                val.SetInt64(value);

                m_current_object->value.AddMember(name_val, val, m_document.GetAllocator());
            }
        }

        void json_archive_writer::write_uint8(std::string const& name, uint8_t value)
        {
            if (m_write_enable) {
                rapidjson::Value name_val(rapidjson::kStringType);
                name_val.SetString(name, m_document.GetAllocator());

                rapidjson::Value val(rapidjson::kNumberType);
                val.SetUint(value);

                m_current_object->value.AddMember(name_val, val, m_document.GetAllocator());
            }
        }

        void json_archive_writer::write_uint16(std::string const& name, uint16_t value)
        {
            if (m_write_enable) {
                rapidjson::Value name_val(rapidjson::kStringType);
                name_val.SetString(name, m_document.GetAllocator());

                rapidjson::Value val(rapidjson::kNumberType);
                val.SetUint(value);

                m_current_object->value.AddMember(name_val, val, m_document.GetAllocator());
            }
        }

        void json_archive_writer::write_uint32(std::string const& name, uint32_t value)
        {
            if (m_write_enable) {
                rapidjson::Value name_val(rapidjson::kStringType);
                name_val.SetString(name, m_document.GetAllocator());

                rapidjson::Value val(rapidjson::kNumberType);
                val.SetUint(value);

                m_current_object->value.AddMember(name_val, val, m_document.GetAllocator());
            }
        }

        void json_archive_writer::write_uint64(std::string const& name, uint64_t value)
        {
            if (m_write_enable) {
                rapidjson::Value name_val(rapidjson::kStringType);
                name_val.SetString(name, m_document.GetAllocator());

                rapidjson::Value val(rapidjson::kNumberType);
                val.SetUint64(value);

                m_current_object->value.AddMember(name_val, val, m_document.GetAllocator());
            }
        }

        void json_archive_writer::write_float(std::string const& name, float value)
        {
            if (m_write_enable) {
                rapidjson::Value name_val(rapidjson::kStringType);
                name_val.SetString(name, m_document.GetAllocator());

                rapidjson::Value val(rapidjson::kNumberType);
                val.SetDouble(value);

                m_current_object->value.AddMember(name_val, val, m_document.GetAllocator());
            }
        }

        void json_archive_writer::write_double(std::string const& name, double value)
        {
            if (m_write_enable) {
                rapidjson::Value name_val(rapidjson::kStringType);
                name_val.SetString(name, m_document.GetAllocator());

                rapidjson::Value val(rapidjson::kNumberType);
                val.SetDouble(value);

                m_current_object->value.AddMember(name_val, val, m_document.GetAllocator());
            }
        }

        void json_archive_writer::write_name_hash(std::string const& name, unsigned long long name_hash)
        {
            write_uint64(name, name_hash);
        }
    }
}
