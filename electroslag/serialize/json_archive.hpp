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

#if defined(ELECTROSLAG_BUILD_SHIP)
#error JSON archiver not to be included in SHIP build!
#endif

#include "electroslag/stream_interface.hpp"
#include "electroslag/serialize/archive_interface.hpp"

namespace electroslag {
    namespace serialize {
        class json_archive {
        public:
            explicit json_archive(stream_interface* s)
                : m_stream(s)
            {}

        protected:
            static uint32_t const json_version_number = 1;

            stream_interface* m_stream;
            rapidjson::Document m_document;
        };

        class json_archive_reader
            : public json_archive
            , public archive_reader_interface {
        public:
            explicit json_archive_reader(stream_interface* s);
            virtual ~json_archive_reader()
            {}

            // Implement archive_reader_interface
            virtual bool next_object(
                unsigned long long* out_type_hash,
                unsigned long long* out_name_hash
                );

            virtual bool read_buffer(std::string const& name, void* buffer, int sizeof_buffer);
            virtual bool read_string(std::string const& name, std::string* out_value);

            virtual bool read_uint8(std::string const& name, uint8_t* out_value);
            virtual bool read_uint16(std::string const& name, uint16_t* out_value);
            virtual bool read_uint32(std::string const& name, uint32_t* out_value);
            virtual bool read_uint64(std::string const& name, uint64_t* out_value);

            virtual bool read_int8(std::string const& name, int8_t* out_value);
            virtual bool read_int16(std::string const& name, int16_t* out_value);
            virtual bool read_int32(std::string const& name, int32_t* out_value);
            virtual bool read_int64(std::string const& name, int64_t* out_value);

            virtual bool read_float(std::string const& name, float* out_value);
            virtual bool read_double(std::string const& name, double* out_value);

            virtual bool read_name_hash(std::string const& name, unsigned long long* out_value);

        private:
            void get_object_type(unsigned long long* out_type_hash);
            void get_object_name(unsigned long long* out_name_hash);

            bool read_float_array_to_buffer(int element_sizeof, void* buffer, rapidjson::Value::ConstMemberIterator& val);
            bool read_uint_array_to_buffer(int element_sizeof, void* buffer, rapidjson::Value::ConstMemberIterator& val);
            bool read_int_array_to_buffer(int element_sizeof, void* buffer, rapidjson::Value::ConstMemberIterator& val);

            rapidjson::Value::ConstMemberIterator m_current_object;
            std::unique_ptr<char[]> m_stream_buffer;
            bool m_iterating;

            // Disallowed operations:
            json_archive_reader();
            explicit json_archive_reader(json_archive_reader const&);
            json_archive_reader& operator =(json_archive_reader const&);
        };

        class json_archive_writer
            : public json_archive
            , public archive_writer_interface {
        public:
            explicit json_archive_writer(stream_interface* s);
            virtual ~json_archive_writer();

            // Implement archive_writer_interface
            virtual void start_object(serializable_object_interface* obj);

            virtual void write_buffer(std::string const& name, void const* buffer, int sizeof_buffer);
            virtual void write_string(std::string const& name, std::string const& value);

            virtual void write_uint8(std::string const& name, uint8_t value);
            virtual void write_uint16(std::string const& name, uint16_t value);
            virtual void write_uint32(std::string const& name, uint32_t value);
            virtual void write_uint64(std::string const& name, uint64_t value);

            virtual void write_int8(std::string const& name, int8_t value);
            virtual void write_int16(std::string const& name, int16_t value);
            virtual void write_int32(std::string const& name, int32_t value);
            virtual void write_int64(std::string const& name, int64_t value);

            virtual void write_float(std::string const& name, float value);
            virtual void write_double(std::string const& name, double value);

            virtual void write_name_hash(std::string const& name, unsigned long long name_hash);

            virtual void write_boolean(std::string const& name, bool value)
            {
                if (value) {
                    write_string(name, "true");
                }
                else {
                    write_string(name, "false");
                }
            }

        private:
            rapidjson::Value::MemberIterator m_current_object;

            // Disallowed operations:
            json_archive_writer();
            explicit json_archive_writer(json_archive_writer const&);
            json_archive_writer& operator =(json_archive_writer const&);
        };
    }
}
