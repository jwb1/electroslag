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

namespace electroslag {
    class named_object {
    public:
        virtual ~named_object();

        std::string get_name() const;

        void set_name(std::string const& name);

        unsigned long long get_hash() const
        {
            return (m_hash);
        }

        void set_hash(unsigned long long name_hash)
        {
            // Really should only use this in the case of the default constructor.
            ELECTROSLAG_CHECK(name_hash != 0);
            ELECTROSLAG_CHECK(m_hash == 0);
            m_hash = name_hash;
        }

        void set_name_and_hash(std::string const& name, unsigned long long name_hash);

        bool has_name_string() const;

        void clear_name();

        bool is_equal(unsigned long long compare_with_hash) const
        {
            return (get_hash() == compare_with_hash);
        }

        bool is_equal(named_object const* obj) const
        {
            return (get_hash() == obj->get_hash());
        }

        template<class T>
        bool is_equal(reference<T> r) const
        {
            return (get_hash() == r->get_hash());
        }

    protected:
        named_object()
            : m_hash(0) // This is technically a valid hash value, but a very unlikely one.
        {}

        explicit named_object(unsigned long long name_hash)
            : m_hash(name_hash)
        {}

        explicit named_object(std::string const& name)
            : m_hash(0)
        {
            set_name(name);
        }

        named_object(std::string const& name, unsigned long long name_hash)
            : m_hash(0)
        {
            set_name_and_hash(name, name_hash);
        }

        named_object(named_object const& copy_named_object);
        named_object& operator =(named_object const& copy_named_object);

    private:
        unsigned long long m_hash;
    };
}
