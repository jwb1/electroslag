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
#include "electroslag/logger.hpp"
#include "electroslag/name_table.hpp"
#include "electroslag/serialize/serializable_name_table.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace serialize {
        serializable_name_table::serializable_name_table(archive_reader_interface* ar)
        {
            // Now read the array of strings.
            int name_count = 0;
            if (!ar->read_int32("name_count", &name_count)) {
                throw load_object_failure("name_count");
            }

            enumeration_namer field_namer(name_count, 'n');
            std::string name;
            while (!field_namer.used_all_names()) {
                if (ar->read_string(field_namer.get_next_name(), &name)) {
                    ELECTROSLAG_LOG_SERIALIZE("Name [0x%016llX] = %s",
                        hash_string_runtime(name),
                        name.c_str()
                        );

                    get_name_table()->insert(name);
                }
                else {
                    throw load_object_failure("name");
                }
            }
        }

        void serializable_name_table::save_to_archive(archive_writer_interface* ar)
        {
            serializable_object::save_to_archive(ar);

            name_table* nt = get_name_table();

            // Count the valid names.
            int name_count = 0;
            archive_writer_interface::const_saved_object_iterator i(ar->get_saved_object_vector().begin());
            while (i != ar->get_saved_object_vector().end()) {
                if (nt->contains(*i)) {
                    name_count++;
                }
                ++i;
            }

            ar->write_int32("name_count", name_count);
            if (name_count > 0) {
                // Re-iterate and write the names.
                i = ar->get_saved_object_vector().begin();
                enumeration_namer namer(name_count, 'n');
                while (i != ar->get_saved_object_vector().end()) {
                    if (nt->contains(*i)) {
                        ar->write_string(namer.get_next_name(), nt->lookup(*i));
                    }
                    ++i;
                }
            }
        }
    }
}
