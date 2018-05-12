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
    // This is a specialized container for holding collections of objects that:
    // 1. Do not have a copy constructor / assignment operator
    //    AND/OR
    // 2. Do not have a default constructor.
    // There are two phases to it's lifetime; a construction phase where the entry
    // count is set and objects are created via emplace and a second phase where the
    // objects may be accessed. Sparse is not supported. All entries must be emplaced
    // before safe access.
    template<class T>
    class dynamic_array {
    public:
        typedef T* iterator;
        typedef T const* const_iterator;

        dynamic_array()
            : m_pointer(0)
            , m_entries(0)
        {}

        explicit dynamic_array(int entries)
            : m_pointer(0)
            , m_entries(0)
        {
            set_entries(entries);
        }

        ~dynamic_array()
        {
            if (m_pointer) {
                for (int e = 0; e < m_entries; ++e) {
                    T* entry_pointer = m_pointer + e;
                    entry_pointer->~T();
                }

                std::free(m_pointer);
            }
        }

        int get_entries() const
        {
            return (m_entries);
        }

        void set_entries(int entries)
        {
            ELECTROSLAG_CHECK(!m_pointer);
            ELECTROSLAG_CHECK(m_entries == 0);
            m_entries = entries;
            m_pointer = static_cast<T*>(std::malloc(sizeof(T) * entries));
            if (!m_pointer) {
                throw std::bad_alloc();
            }
        }

        template<typename... Args>
        T* emplace(int index, Args&&... args)
        {
            ELECTROSLAG_CHECK(index >= 0 && index < m_entries);
            return (new (m_pointer + index) T(args...));
        }

        T const& operator [](int index) const
        {
            ELECTROSLAG_CHECK(index >= 0 && index < m_entries);
            return (*(m_pointer + index));
        }

        T& operator [](int index)
        {
            ELECTROSLAG_CHECK(index >= 0 && index < m_entries);
            return (*(m_pointer + index));
        }

        T const& at(int index) const
        {
            ELECTROSLAG_CHECK(index >= 0 && index < m_entries);
            return (*(m_pointer + index));
        }

        T& at(int index)
        {
            ELECTROSLAG_CHECK(index >= 0 && index < m_entries);
            return (*(m_pointer + index));
        }

        T const* begin() const
        {
            return (m_pointer);
        }

        T const* end() const
        {
            return (m_pointer + m_entries);
        }

        T* begin()
        {
            return (m_pointer);
        }

        T* end()
        {
            return (m_pointer + m_entries);
        }

    private:
        T* m_pointer;
        int m_entries;

        // Disallowed operations:
        explicit dynamic_array(dynamic_array const&);
        dynamic_array& operator =(dynamic_array const&);
    };
}
