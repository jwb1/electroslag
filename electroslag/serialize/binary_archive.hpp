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
#include "electroslag/stream_interface.hpp"
#include "electroslag/serialize/archive_interface.hpp"

namespace electroslag {
    namespace serialize {
        class binary_archive {
        public:
            explicit binary_archive(stream_interface* s)
                : m_stream(s)
            {}

        protected:
            static uint32_t const magic_number = 'S' | 'l' << 8 | 'a' << 16 | 'g' << 24;
            static uint32_t const binary_version_number = 1;

            struct header {
                uint32_t magic_number;
                uint32_t version_number;
                int32_t object_count;
            };

            stream_interface* m_stream;
        };

        class binary_archive_reader
            : public binary_archive
            , public archive_reader_interface {
        public:
            explicit binary_archive_reader(stream_interface* s);

            // Implement archive_reader_interface
            virtual bool next_object(
                unsigned long long* out_type_hash,
                unsigned long long* out_name_hash
                );

            virtual bool read_buffer(std::string const& name, void* buffer, int sizeof_buffer);
            virtual bool read_string(std::string const& name, std::string* out_value);

            virtual bool read_uint8(std::string const&, uint8_t* out_value)
            {
                m_stream->read(out_value, sizeof(uint8_t));
                return (true);
            }
            virtual bool read_uint16(std::string const&, uint16_t* out_value)
            {
                m_stream->read(out_value, sizeof(uint16_t));
                return (true);
            }
            virtual bool read_uint32(std::string const&, uint32_t* out_value)
            {
                m_stream->read(out_value, sizeof(uint32_t));
                return (true);
            }
            virtual bool read_uint64(std::string const&, uint64_t* out_value)
            {
                m_stream->read(out_value, sizeof(uint64_t));
                return (true);
            }

            virtual bool read_int8(std::string const&, int8_t* out_value)
            {
                m_stream->read(out_value, sizeof(int8_t));
                return (true);
            }
            virtual bool read_int16(std::string const&, int16_t* out_value)
            {
                m_stream->read(out_value, sizeof(int16_t));
                return (true);
            }
            virtual bool read_int32(std::string const&, int32_t* out_value)
            {
                m_stream->read(out_value, sizeof(int32_t));
                return (true);
            }
            virtual bool read_int64(std::string const&, int64_t* out_value)
            {
                m_stream->read(out_value, sizeof(int64_t));
                return (true);
            }

            virtual bool read_float(std::string const&, float* out_value)
            {
                m_stream->read(out_value, sizeof(float));
                return (true);
            }
            virtual bool read_double(std::string const&, double* out_value)
            {
                m_stream->read(out_value, sizeof(double));
                return (true);
            }

            virtual bool read_name_hash(std::string const&, unsigned long long* out_value)
            {
                return (read_uint64(std::string(), out_value));
            }

        private:
            int m_current_object;
            int m_object_count;

            // Disallowed operations:
            binary_archive_reader();
            explicit binary_archive_reader(binary_archive_reader const&);
            binary_archive_reader& operator =(binary_archive_reader const&);
        };

        class binary_archive_writer
            : public binary_archive
            , public archive_writer_interface {
        public:
            explicit binary_archive_writer(stream_interface* s);
            virtual ~binary_archive_writer();

            // Implement archive_writer_interface
            virtual void start_object(serializable_object_interface* obj);

            virtual void write_buffer(std::string const&, void const* buffer, int sizeof_buffer);
            virtual void write_string(std::string const&, std::string const& value);

            virtual void write_uint8(std::string const&, uint8_t value)
            {
                if (m_write_enable) {
                    m_stream->write(&value, sizeof(value));
                }
            }
            virtual void write_uint16(std::string const&, uint16_t value)
            {
                if (m_write_enable) {
                    m_stream->write(&value, sizeof(value));
                }
            }
            virtual void write_uint32(std::string const&, uint32_t value)
            {
                if (m_write_enable) {
                    m_stream->write(&value, sizeof(value));
                }
            }
            virtual void write_uint64(std::string const&, uint64_t value)
            {
                if (m_write_enable) {
                    m_stream->write(&value, sizeof(value));
                }
            }

            virtual void write_int8(std::string const&, int8_t value)
            {
                if (m_write_enable) {
                    m_stream->write(&value, sizeof(value));
                }
            }
            virtual void write_int16(std::string const&, int16_t value)
            {
                if (m_write_enable) {
                    m_stream->write(&value, sizeof(value));
                }
            }
            virtual void write_int32(std::string const&, int32_t value)
            {
                if (m_write_enable) {
                    m_stream->write(&value, sizeof(value));
                }
            }
            virtual void write_int64(std::string const&, int64_t value)
            {
                if (m_write_enable) {
                    m_stream->write(&value, sizeof(value));
                }
            }

            virtual void write_float(std::string const&, float value)
            {
                if (m_write_enable) {
                    m_stream->write(&value, sizeof(value));
                }
            }
            virtual void write_double(std::string const&, double value)
            {
                if (m_write_enable) {
                    m_stream->write(&value, sizeof(value));
                }
            }

            virtual void write_name_hash(std::string const&, unsigned long long name_hash)
            {
                write_uint64(std::string(), name_hash);
            }

            virtual void write_boolean(std::string const& name, bool value)
            {
                if (value) {
                    write_int32(name, 1);
                }
                else {
                    write_int32(name, 0);
                }
            }

        private:

            // Disallowed operations:
            binary_archive_writer();
            explicit binary_archive_writer(binary_archive_writer const&);
            binary_archive_writer& operator =(binary_archive_writer const&);
        };
    }
}
