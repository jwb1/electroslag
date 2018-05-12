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
#include "electroslag/referenced_buffer.hpp"
#include "electroslag/serialize/serializable_object.hpp"

namespace electroslag {
    namespace serialize {
        class serializable_buffer
            : public referenced_buffer
            , public serializable_object<serializable_buffer> {
        public:
            typedef reference<serializable_buffer> ref;

            static ref create(unsigned long long name_hash, int buffer_sizeof)
            {
                return (ref(new serializable_buffer(name_hash, buffer_sizeof)));
            }

            static ref clone(ref copy_ref)
            {
                return (ref(new serializable_buffer(*copy_ref.get_pointer())));
            }

            // Implement serializable_object
            explicit serializable_buffer(archive_reader_interface* ar)
            {
                // Read the buffer size, then the data itself.
                if (!ar->read_int32("sizeof", &m_sizeof)) {
                    throw load_object_failure("sizeof");
                }

                m_buffer = static_cast<byte*>(std::malloc(m_sizeof));
                if (!m_buffer) {
                    throw std::bad_alloc();
                }
                if (!ar->read_buffer("buffer", m_buffer, m_sizeof)) {
                    throw load_object_failure("buffer");
                }
            }

            virtual void save_to_archive(archive_writer_interface* ar)
            {
                serializable_object::save_to_archive(ar);

                ar->write_int32("sizeof", m_sizeof);
                ar->write_buffer("buffer", m_buffer, m_sizeof);
            }

        protected:
            serializable_buffer(unsigned long long name_hash, int buffer_sizeof)
                : serializable_object(name_hash)
            {
                if (buffer_sizeof <= 0) {
                    throw parameter_failure("buffer_sizeof");
                }

                m_sizeof = buffer_sizeof;
                m_buffer = static_cast<byte*>(std::malloc(m_sizeof));
                if (!m_buffer) {
                    throw std::bad_alloc();
                }
            }

            explicit serializable_buffer(serializable_buffer const& copy_object)
                : serializable_object(copy_object)
            {
                m_sizeof = copy_object.m_sizeof;
                m_buffer = static_cast<byte*>(std::malloc(m_sizeof));
                if (!m_buffer) {
                    throw std::bad_alloc();
                }

                memcpy(m_buffer, copy_object.m_buffer, m_sizeof);
            }

        private:
            virtual ~serializable_buffer()
            {
                if (m_buffer) {
                    std::free(m_buffer);
                    m_buffer = 0;
                    m_sizeof = 0;
                }
            }

            // Disallowed operations:
            serializable_buffer();
            serializable_buffer& operator =(serializable_buffer const&);
        };
    }
}
