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
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/serialize/serializable_object.hpp"

namespace electroslag {
    namespace serialize {
        class serializable_map
            : public referenced_object
            , public serializable_object<serializable_map> {
        public:
            typedef reference<serializable_map> ref;

            static ref create()
            {
                return (ref(new serializable_map()));
            }

            // Implement serializable_object
            explicit serializable_map(archive_reader_interface* ar);
            virtual void save_to_archive(archive_writer_interface* ar);

            bool has_value(unsigned long long key) const
            {
                return (m_table.find(key) != m_table.end());
            }

            unsigned long long find_value(unsigned long long key) const
            {
                return (m_table.at(key));
            }

            bool locate_value(unsigned long long key, unsigned long long* out_value) const
            {
                table::const_iterator v(m_table.find(key));
                if (v != m_table.end()) {
                    *out_value = v->second;
                    return (true);
                }
                else {
                    return (false);
                }
            }

            void set_value(unsigned long long key, unsigned long long value);

        private:
            serializable_map()
            {}

            typedef std::unordered_map<
                unsigned long long,
                unsigned long long,
                prehashed_key<unsigned long long>,
                std::equal_to<unsigned long long>
            > table;
            table m_table;

            // Disallowed operations:
            explicit serializable_map(serializable_map const&);
            serializable_map& operator =(serializable_map const&);
        };
    }
}
