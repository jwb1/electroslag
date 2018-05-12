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
#include "electroslag/serialize/serializable_object.hpp"

namespace electroslag {
    namespace serialize {
        class serializable_name_table
            : public serializable_object<serializable_name_table> {
        public:
            // Hard coding the name for all instances of the type ensures only one
            // name table per archive. (But requires special handling at load time.)
            serializable_name_table()
                : serializable_object(ELECTROSLAG_STRING_AND_HASH("d:name_table"))
            {}

            // Implement serializable_object
            explicit serializable_name_table(archive_reader_interface* ar);
            virtual void save_to_archive(archive_writer_interface* ar);
        };
    }
}
