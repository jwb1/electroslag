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
#include "electroslag/threading/mutex.hpp"

namespace electroslag {
    class name_table {
    public:
        // An ugly call sequence: this constructor results in the mutex being
        // locked before it's fully constructed and the string map inserted
        // before the name_table is fully constructed. It works, but might be
        // brittle due to reliance on member initialization order.
        name_table()
            : m_string_map()
            , m_mutex(ELECTROSLAG_STRING_AND_HASH("m:name_table"))
        {}

        ~name_table()
        {
            // The same ugly call sequence above means that the mutex might
            // be entered after it's destroyed. Clear the name to make sure that
            // doesn't happen.
            m_mutex.clear_name();
        }

        void insert(std::string const& name)
        {
            insert(name, hash_string_runtime(name.c_str()));
        }

        void insert(std::string const& name, unsigned long long name_hash)
        {
            threading::lock_guard name_table_lock(&m_mutex);

            name_iterator in_map(m_string_map.find(name_hash));
            if (in_map == m_string_map.end()) {
                name_entry n;
                n.str = name;
                n.count = 1;
                m_string_map.insert(std::make_pair(name_hash, n));
            }
            else {
                in_map->second.count += 1;
            }
        }

        void remove(unsigned long long name_hash)
        {
            threading::lock_guard name_table_lock(&m_mutex);
            name_iterator in_map(m_string_map.find(name_hash));
            if (in_map != m_string_map.end()) {
                if (in_map->second.count == 1) {
                    m_string_map.erase(in_map);
                }
                else {
                    in_map->second.count -= 1;
                }
            }
        }

        bool contains(unsigned long long name_hash)
        {
            threading::lock_guard name_table_lock(&m_mutex);
            return (m_string_map.count(name_hash) == 1);
        }

        std::string lookup(unsigned long long name_hash)
        {
            threading::lock_guard name_table_lock(&m_mutex);
            return (m_string_map.at(name_hash).str);
        }

        void duplicate(unsigned long long name_hash)
        {
            threading::lock_guard name_table_lock(&m_mutex);
            name_iterator in_map(m_string_map.find(name_hash));
            if (in_map != m_string_map.end()) {
                in_map->second.count += 1;
            }
        }

#if !defined(ELECTROSLAG_BUILD_SHIP)
        void dump();
#endif

    private:
        struct name_entry {
            std::string str;
            int count;
        };

        typedef std::unordered_map<
            unsigned long long,
            name_entry,
            prehashed_key<unsigned long long>,
            std::equal_to<unsigned long long>
        > string_map;
        typedef string_map::const_iterator const_name_iterator;
        typedef string_map::iterator name_iterator;

        string_map m_string_map;

        mutable threading::mutex m_mutex;
    };

    name_table* get_name_table();
}
