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
#include "electroslag/serialize/serializable_object_interface.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/serialize/importer_interface.hpp"
#include "electroslag/serialize/serializable_type_registration.hpp"

namespace electroslag {
    namespace serialize {
        template<class T>
        class serializable_object : public serializable_object_interface {
        public:
            static serializable_object_interface* create_from_archive(archive_reader_interface* ar)
            {
                return (new T(ar));
            }

            static serializable_type_registration* get_type_registration()
            {
                return (&registration);
            }

            virtual unsigned long long get_type_hash() const
            {
                return (registration.get_type_hash());
            }

        protected:
            /* TODO: Is there a better way to ensure the type registration is instantiated?
               (void)&registration; is a C++ hack.
            */

            serializable_object()
            {
                (void)&registration;
            }

            explicit serializable_object(unsigned long long name_hash)
                : serializable_object_interface(name_hash)
            {
                (void)&registration;
            }

            explicit serializable_object(std::string const& name)
                : serializable_object_interface(name)
            {
                (void)&registration;
            }

            serializable_object(std::string const& name, unsigned long long name_hash)
                : serializable_object_interface(name, name_hash)
            {
                (void)&registration;
            }

            serializable_object(serializable_object const& copy_object)
                : serializable_object_interface(copy_object)
            {
                (void)&registration;
            }

            virtual ~serializable_object()
            {}

            serializable_object& operator =(serializable_object const& copy_object)
            {
                serializable_object_interface::operator =(copy_object);
                return (*this);
            }

        private:
            static serializable_type_registration registration;
        };

        // static
        template<class T>
        serializable_type_registration serializable_object<T>::registration(
            typeid(T),
            serializable_type_registration::create_from_archive_delegate::create_from_function<&serializable_object::create_from_archive>(),
            std::is_base_of<referenced_object, T>::value,
            std::is_base_of<importer_interface, T>::value
            );
    }
}
