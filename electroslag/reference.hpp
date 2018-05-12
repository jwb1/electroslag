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
    template<class T>
    class reference {
    public:
        static reference null_ref;

        reference()
            : m_pointer(0)
        {}

        reference(reference const& copy_reference)
            : m_pointer(copy_reference.m_pointer)
        {
            if (m_pointer) {
                m_pointer->add_ref();
            }
        }

        reference(T* pointer)
            : m_pointer(pointer)
        {
            if (m_pointer) {
                m_pointer->add_ref();
            }
        }

        ~reference()
        {
            if (m_pointer) {
                m_pointer->release();
            }
        }

        reference& operator =(reference const& copy_reference)
        {
            if (m_pointer != copy_reference.m_pointer) {
                if (m_pointer) {
                    m_pointer->release();
                }
                m_pointer = copy_reference.m_pointer;
                if (m_pointer) {
                    m_pointer->add_ref();
                }
            }
            return (*this);
        }

        T const* operator ->() const
        {
            return (m_pointer);
        }

        T* operator ->()
        {
            return (m_pointer);
        }

        T const* get_pointer() const
        {
            return (m_pointer);
        }

        T* get_pointer()
        {
            return (m_pointer);
        }

        bool operator ==(reference const& compare_with) const
        {
            return (m_pointer == compare_with.m_pointer);
        }

        bool operator !=(reference const& compare_with) const
        {
            return (m_pointer != compare_with.m_pointer);
        }

        void reset()
        {
            if (m_pointer) {
                m_pointer->release();
                m_pointer = 0;
            }
        }

        bool is_valid() const
        {
            return (m_pointer != 0);
        }

        template<class U>
        U const* cast() const
        {
            return (dynamic_cast<U const*>(m_pointer));
        }

        template<class U>
        U* cast()
        {
            return (dynamic_cast<U*>(m_pointer));
        }

    private:
        T* m_pointer;
    };

    // static
    template<class T>
    reference<T> reference<T>::null_ref;
}
