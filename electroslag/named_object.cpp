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
#include "electroslag/named_object.hpp"
#include "electroslag/name_table.hpp"

namespace electroslag {
    // Implementations of methods that depend on the name_table.
    named_object::named_object(named_object const& copy_named_object)
        : m_hash(copy_named_object.m_hash)
    {
        get_name_table()->duplicate(m_hash);
    }

    named_object& named_object::operator=(named_object const& copy_named_object)
    {
        if (&copy_named_object != this) {
            clear_name();

            m_hash = copy_named_object.m_hash;
            get_name_table()->duplicate(m_hash);
        }
        return (*this);
    }

    named_object::~named_object()
    {
        clear_name();
    }

    std::string named_object::get_name() const
    {
        return (get_name_table()->lookup(m_hash));
    }

    void named_object::set_name(std::string const& name)
    {
        // Really should only use this in the case of the default constructor.
        ELECTROSLAG_CHECK(m_hash == 0);
        m_hash = hash_string_runtime(name.c_str());
        get_name_table()->insert(name, m_hash);
    }

    void named_object::set_name_and_hash(std::string const& name, unsigned long long name_hash)
    {
        // Really should only use this in the case of the default constructor.
        ELECTROSLAG_CHECK(name_hash != 0);
        ELECTROSLAG_CHECK(m_hash == 0);
        m_hash = name_hash;
        get_name_table()->insert(name, m_hash);
    }

    bool named_object::has_name_string() const
    {
        return (get_name_table()->contains(m_hash));
    }

    void named_object::clear_name()
    {
        if (m_hash != 0) {
            get_name_table()->remove(m_hash);
            m_hash = 0;
        }
    }
}
