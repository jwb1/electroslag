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
#include "electroslag/serialize/serializable_map.hpp"

namespace electroslag {
    namespace serialize {
        serializable_map::serializable_map(archive_reader_interface* ar)
        {
            int table_count = 0;
            if (!ar->read_int32("table_count", &table_count)) {
                throw load_object_failure("table_count");
            }

            m_table.reserve(table_count);

            enumeration_namer namer(table_count, 'm');
            std::string name;
            while (!namer.used_all_names()) {
                name = namer.get_next_name();

                unsigned long long key = 0;
                if (!ar->read_name_hash(name + "_key", &key)) {
                    throw load_object_failure("key");
                }

                unsigned long long value = 0;
                if (!ar->read_name_hash(name + "_value", &value)) {
                    throw load_object_failure("value");
                }

                set_value(key, value);
            }
        }

        void serializable_map::save_to_archive(archive_writer_interface* ar)
        {
            serializable_object::save_to_archive(ar);

            int table_count = static_cast<int>(m_table.size());
            ar->write_int32("table_count", table_count);

            enumeration_namer namer(table_count, 'm');
            std::string name;
            table::const_iterator t(m_table.begin());
            while (t != m_table.end()) {
                name = namer.get_next_name();

                ar->write_name_hash(name + "_key", t->first);
                ar->write_name_hash(name + "_value", t->second);
                ++t;
            }
        }

        void serializable_map::set_value(unsigned long long key, unsigned long long value)
        {
            ELECTROSLAG_CHECK(!has_value(key));
            m_table.insert(std::make_pair(key, value));
        }
    }
}
