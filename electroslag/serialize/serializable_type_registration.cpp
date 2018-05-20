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
#include "electroslag/serialize/serializable_type_registration.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace serialize {
        serializable_type_registration::serializable_type_registration(
            std::type_info const& info,
            create_from_archive_delegate* new_delegate,
            bool referenced,
            bool importer
            )
            : m_create_delegate(new_delegate)
            , m_referenced(referenced)
            , m_importer(importer)
        {
#if defined(ELECTROSLAG_COMPILER_MSVC)
            // Visual C++ RTTI uses "class <fully qualified name>"
            // We want just the fully qualified name, so drop the first 6 characters
            m_name = info.name() + 6;
#else
#error Need RTTI name scheme for this compiler
#endif
            m_hash = hash_string_runtime(m_name);
            get_database()->register_serializable_type(m_hash, this);
        }

    }
}
