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
#include "electroslag/serialize/serializable_object_interface.hpp"
#include "electroslag/serialize/archive_interface.hpp"

namespace electroslag {
    namespace serialize {
        void serializable_object_interface::save_to_archive(archive_writer_interface* ar)
        {
            ELECTROSLAG_CHECK(!is_cloned());
            ar->start_object(this);
        }

        // static
        serializable_object_interface* const serializable_object_interface::cloned_object = reinterpret_cast<serializable_object_interface* const>(-1);
    }
}
