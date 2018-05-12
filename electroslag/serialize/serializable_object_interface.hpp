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
#include "electroslag/delegate.hpp"
#include "electroslag/named_object.hpp"

namespace electroslag {
    namespace serialize {
        class archive_writer_interface;
        class serializable_object_interface : public named_object {
        public:
            virtual ~serializable_object_interface()
            {}

            virtual unsigned long long get_type_hash() const = 0;

            virtual void save_to_archive(archive_writer_interface* ar) = 0;

            template <class T>
            bool is_type() const
            {
                return (get_type_hash() == T::get_type_registration()->get_type_hash());
            }

            // Objects from a load operation are stored together in a linked list, in
            // an order that ensures dependencies are resolved.
            serializable_object_interface const* get_next_loaded_object() const
            {
                ELECTROSLAG_CHECK(!is_cloned());
                return (m_next_loaded_object);
            }

            serializable_object_interface* get_next_loaded_object()
            {
                ELECTROSLAG_CHECK(!is_cloned());
                return (m_next_loaded_object);
            }

            void set_next_loaded_object(serializable_object_interface* next_loaded_obejct)
            {
                m_next_loaded_object = next_loaded_obejct;
            }

            // Cloned objects may not be added to the database, and thus never be a
            // "loaded object"
            bool is_cloned() const
            {
                return (m_next_loaded_object == cloned_object);
            }

        protected:
            serializable_object_interface()
                : m_next_loaded_object(0)
            {}

            explicit serializable_object_interface(unsigned long long name_hash)
                : named_object(name_hash)
                , m_next_loaded_object(0)
            {}

            explicit serializable_object_interface(std::string const& name)
                : named_object(name)
                , m_next_loaded_object(0)
            {}

            serializable_object_interface(std::string const& name, unsigned long long name_hash)
                : named_object(name, name_hash)
                , m_next_loaded_object(0)
            {}

            serializable_object_interface(serializable_object_interface const& copy_object)
                : named_object(copy_object)
                , m_next_loaded_object(cloned_object)
            {}

            serializable_object_interface& operator =(serializable_object_interface const& copy_object)
            {
                if (&copy_object != this) {
                    named_object::operator =(copy_object);
                    m_next_loaded_object = cloned_object;
                }
                return (*this);
            }

        private:
            // Cloned objects get a bogus "next" pointer; that is, equal to cloned_object, which
            // itself is a bogus pointer.
            static serializable_object_interface* const cloned_object;

            serializable_object_interface* m_next_loaded_object;
        };
    }
}
