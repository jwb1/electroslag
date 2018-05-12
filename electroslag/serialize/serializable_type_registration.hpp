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
#include "electroslag/serialize/serializable_object_interface.hpp"
#include "electroslag/serialize/archive_interface.hpp"

namespace electroslag {
    namespace serialize {
        class serializable_type_registration {
        public:
            typedef delegate<serializable_object_interface*, archive_reader_interface*> create_from_archive_delegate;

            serializable_type_registration(
                std::type_info const& info,
                create_from_archive_delegate* new_delegate,
                bool referenced,
                bool importer
                );

            ~serializable_type_registration()
            {
                delete m_create_delegate;
            }

            create_from_archive_delegate const* get_create_delegate() const
            {
                return (m_create_delegate);
            }

            bool is_referenced() const
            {
                return (m_referenced);
            }

            bool is_importer() const
            {
                return (m_importer);
            }

            std::string const& get_name() const
            {
                return (m_name);
            }

            unsigned long long get_type_hash() const
            {
                return (m_hash);
            }

        private:
            create_from_archive_delegate* m_create_delegate;
            std::string m_name;
            unsigned long long m_hash;
            bool m_referenced;
            bool m_importer;

            // Disallowed operations:
            serializable_type_registration();
            explicit serializable_type_registration(serializable_type_registration const&);
            serializable_type_registration& operator =(serializable_type_registration const&);
        };
    }
}
