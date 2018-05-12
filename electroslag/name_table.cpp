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
#include "electroslag/systems.hpp"
#if !defined(ELECTROSLAG_BUILD_SHIP)
#include "electroslag/logger.hpp"
#endif

namespace electroslag {
    name_table* get_name_table()
    {
        return (get_systems()->get_name_table());
    }

#if !defined(ELECTROSLAG_BUILD_SHIP)
    void name_table::dump()
    {
        threading::lock_guard name_table_lock(&m_mutex);
        ELECTROSLAG_LOG_MESSAGE("Name Table Dump");
        ELECTROSLAG_LOG_MESSAGE("Hash               | Count  | String");
        ELECTROSLAG_LOG_MESSAGE("-------------------|--------|-----------------------------------------------");
        const_name_iterator i(m_string_map.begin());
        while (i != m_string_map.end()) {
            ELECTROSLAG_LOG_MESSAGE(
                "0x%016llX |%8d| %s",
                i->first,
                i->second.count,
                i->second.str.c_str()
                );
            ++i;
        }
    }
#endif
}
