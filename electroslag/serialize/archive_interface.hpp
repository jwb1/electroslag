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
#include "electroslag/serialize/load_record.hpp"

namespace electroslag {
    namespace serialize {
        // Use this to generate value names for serializing data sets that enumeratable,
        // but the size is unknown at compile time.
        class enumeration_namer {
        public:
            explicit enumeration_namer(int count, char prefix)
                : m_count(count)
                , m_index(0)
                , m_prefix(prefix)
            {
                ELECTROSLAG_CHECK(count >= 0 && count < 10000);
            }

            bool used_all_names() const
            {
                return (m_index == m_count);
            }

            std::string get_next_name()
            {
                std::string next_name;
                formatted_string_append(next_name, "%c%.4d", m_prefix, m_index++);
                return (next_name);
            }

        private:
            int m_count;
            int m_index;
            char m_prefix;
        };

        class archive_reader_interface {
        public:
            virtual bool next_object(
                unsigned long long* out_type_hash,
                unsigned long long* out_name_hash
                ) = 0;

            // Each of these return false if the named value is not found or can not be read as
            // the desired type.
            virtual bool read_buffer(std::string const& name, void* buffer, int sizeof_buffer) = 0;
            virtual bool read_string(std::string const& name, std::string* out_value) = 0;

            virtual bool read_uint8(std::string const& name,  uint8_t* out_value) = 0;
            virtual bool read_uint16(std::string const& name, uint16_t* out_value) = 0;
            virtual bool read_uint32(std::string const& name, uint32_t* out_value) = 0;
            virtual bool read_uint64(std::string const& name, uint64_t* out_value) = 0;

            virtual bool read_int8(std::string const& name, int8_t* out_value) = 0;
            virtual bool read_int16(std::string const& name, int16_t* out_value) = 0;
            virtual bool read_int32(std::string const& name, int32_t* out_value) = 0;
            virtual bool read_int64(std::string const& name, int64_t* out_value) = 0;

            virtual bool read_float(std::string const& name, float* out_value) = 0;
            virtual bool read_double(std::string const& name, double* out_value) = 0;

            virtual bool read_name_hash(std::string const& name, unsigned long long* out_value) = 0;

            bool read_boolean(std::string const& name, bool* out_value)
            {
                uint32_t value = 0;
                if (read_uint32(name, &value)) {
                    if (value) {
                        *out_value = true;
                    }
                    else {
                        *out_value = false;
                    }
                    return (true);
                }

                std::string s;
                if (read_string(name, &s)) {
                    if (s.compare("true") == 0) {
                        *out_value = true;
                        return (true);
                    }
                    else if (s.compare("false") == 0) {
                        *out_value = false;
                        return (true);
                    }
                }

                return (false);
            }

            template<class Enum, int EnumCount>
            bool read_enumeration(
                std::string const& name,
                Enum* out_value,
                char const* const(&enum_strings)[EnumCount]
                )
            {
                // Try to read the enumeration value directly.
                ELECTROSLAG_STATIC_CHECK((std::is_enum<Enum>::value), "Not an enumeration.");
                ELECTROSLAG_STATIC_CHECK(sizeof(Enum) == sizeof(int32_t), "Enumeration is not integer sized.");
                int32_t value = 0;
                if (read_int32(name, &value)) {
                    if (value >= 0 && value < EnumCount) {
                        *out_value = static_cast<Enum>(value);
                        return (true);
                    }
                    else {
                        return (false);
                    }
                }

                // Try to match the enumeration value as a string.
                std::string s;
                if (!read_string(name, &s)) {
                    return (false);
                }

                for (int i = 0; i < EnumCount; ++i) {
                    if (s == enum_strings[i]) {
                        *out_value = static_cast<Enum>(i);
                        return (true);
                    }
                }
                return (false);
            }

            template<class Enum, int EnumCount>
            bool read_enumeration_flags(
                std::string const& name,
                Enum* out_value,
                char const* const(&enum_strings)[EnumCount],
                Enum const(&enum_bit_values)[EnumCount]
                )
            {
                // Try to read the enumeration value directly.
                ELECTROSLAG_STATIC_CHECK((std::is_enum<Enum>::value), "Not an enumeration.");
                ELECTROSLAG_STATIC_CHECK(sizeof(Enum) == sizeof(int32_t), "Enumeration is not integer sized.");
                uint32_t value = 0;
                if (read_uint32(name, &value)) {
                    // Could OR together all of the values from enum_bit_values, and check against that.
                    *out_value = static_cast<Enum>(value);
                    return (true);
                }

                int i = 0;
                std::string s;
                if (read_string(name, &s)) {
                    // Try to match the enumeration value as a string; this is a shortcut for setting one bit
                    for (; i < EnumCount; ++i) {
                        if (s == enum_strings[i]) {
                            *out_value = enum_bit_values[i];
                            return (true);
                        }
                    }

                    // Try to parse the string as a list of bit token strings. Look for whitespace delimiters.
                    value = 0;
                    int token_begin = 0;
                    int token_end = 0;
                    do {
                        while (iswspace(s[token_begin]) && token_begin < s.length()) {
                            ++token_begin;
                        }
                        if (token_begin == s.length()) {
                            break;
                        }

                        token_end = token_begin + 1;
                        while (!(iswspace(s[token_end]) && token_end < s.length())) {
                            ++token_end;
                        }

                        for (i = 0; i < EnumCount; ++i) {
                            if (s.compare(token_begin, token_end - token_begin, enum_strings[i]) == 0) {
                                value |= enum_bit_values[i];
                                break;
                            }
                        }
                        if (i == EnumCount) {
                            return (false);
                        }

                        token_begin = token_end + 1;
                    } while (token_begin < s.length());

                    *out_value = static_cast<Enum>(value);
                    return (true);
                }

                return (false);
            }

            std::string const& get_base_directory() const
            {
                return (m_base_directory);
            }
            void set_base_directory(std::string const& base_directory)
            {
                m_base_directory = base_directory;
            }

            void set_load_record(load_record::ref const& load)
            {
                ELECTROSLAG_CHECK(load.is_valid());
                m_load_record = load;
            }
            void clear_load_record()
            {
                m_load_record.reset();
            }
            load_record::ref& get_load_record()
            {
                ELECTROSLAG_CHECK(m_load_record.is_valid());
                return (m_load_record);
            }

        protected:
            archive_reader_interface()
            {}

        private:
            std::string m_base_directory;
            load_record::ref m_load_record;
        };

        class serializable_object_interface;
        class archive_writer_interface {
        public:
            typedef std::vector<unsigned long long> saved_object_vector;
            typedef saved_object_vector::const_iterator const_saved_object_iterator;
            typedef saved_object_vector::iterator saved_object_iterator;

            virtual ~archive_writer_interface()
            {}

            virtual void start_object(serializable_object_interface* obj) = 0;

            virtual void write_buffer(std::string const& name, void const* buffer, int sizeof_buffer) = 0;
            virtual void write_string(std::string const& name, std::string const& value) = 0;

            virtual void write_uint8(std::string const& name, uint8_t value) = 0;
            virtual void write_uint16(std::string const& name, uint16_t value) = 0;
            virtual void write_uint32(std::string const& name, uint32_t value) = 0;
            virtual void write_uint64(std::string const& name, uint64_t value) = 0;

            virtual void write_int8(std::string const& name, int8_t value) = 0;
            virtual void write_int16(std::string const& name, int16_t value) = 0;
            virtual void write_int32(std::string const& name, int32_t value) = 0;
            virtual void write_int64(std::string const& name, int64_t value) = 0;

            virtual void write_float(std::string const& name, float value) = 0;
            virtual void write_double(std::string const& name, double value) = 0;

            virtual void write_name_hash(std::string const& name, unsigned long long name_hash) = 0;

            virtual void write_boolean(std::string const& name, bool value) = 0;

            saved_object_vector const& get_saved_object_vector() const
            {
                return (m_saved_objects);
            }

            void set_load_record(load_record::ref const& load)
            {
                ELECTROSLAG_CHECK(load.is_valid());
                m_load_record = load;
            }
            void clear_load_record()
            {
                m_load_record.reset();
            }
            load_record::ref& get_load_record()
            {
                ELECTROSLAG_CHECK(m_load_record.is_valid());
                return (m_load_record);
            }

        protected:
            archive_writer_interface()
                : m_write_enable(false)
            {}

            saved_object_vector m_saved_objects;
            load_record::ref m_load_record;
            bool m_write_enable;
        };
    }
}
