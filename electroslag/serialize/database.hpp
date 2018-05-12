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
#include "electroslag/stream_interface.hpp"
#include "electroslag/referenced_buffer.hpp"
#include "electroslag/delegate.hpp"
#include "electroslag/threading/mutex.hpp"
#include "electroslag/serialize/serializable_type_registration.hpp"
#include "electroslag/serialize/serializable_object_interface.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/serialize/load_record.hpp"

namespace electroslag {
    namespace serialize {
        class serializable_type_registration;

        class database {
        public:
            database();
            ~database();

            // Loadable types are registered by C++ global object constructors.
            // (This method is called exclusively before entering main.)
            void register_serializable_type(
                unsigned long long name_hash,
                serializable_type_registration* const type
                );

            serializable_type_registration const* find_type(
                unsigned long long name_hash
                ) const
            {
                // The serialized type table is immutable after global object constructors.
                serializable_type_table::const_iterator t(m_serializable_type_table.find(name_hash));
                if (t != m_serializable_type_table.end()) {
                    return (t->second);
                }
                else {
                    return (0);
                }
            }

            // Generate an empty load record.
            load_record::ref create_load_record();

            // Load serialized objects in to the database.
            load_record::ref load_objects(
                std::string const& file_name
                );

            load_record::ref load_objects(
                std::string const& file_name,
                std::string const& base_directory
                );

            load_record::ref load_objects(
                referenced_buffer_interface::ref const& buffer_ref,
                std::string const& base_directory
                );

            load_record::ref load_objects(
                stream_interface* s,
                std::string const& base_directory
                );

#if !defined(ELECTROSLAG_BUILD_SHIP)
            // JSON and save support are stripped from ELECTROSLAG_BUILD_SHIP.
            load_record::ref load_objects_json(
                std::string const& file_name
                );

            load_record::ref load_objects_json(
                std::string const& file_name,
                std::string const& base_directory
                );

            load_record::ref load_objects_json(
                referenced_buffer_interface::ref const& buffer_ref,
                std::string const& base_directory
                );

            load_record::ref load_objects_json(
                stream_interface* s,
                std::string const& base_directory
                );

            // Save the current database to file.
            void save_objects(
                std::string const& file_name,
                load_record::ref& record = load_record::ref::null_ref
                );

            void save_objects(
                referenced_buffer_interface::ref const& buffer_ref,
                load_record::ref& record = load_record::ref::null_ref
                );

            void save_objects(
                stream_interface* s,
                load_record::ref& record = load_record::ref::null_ref
                );

            void save_objects_json(
                std::string const& file_name,
                load_record::ref& record = load_record::ref::null_ref
                );

            void save_objects_json(
                referenced_buffer_interface::ref const& buffer_ref,
                load_record::ref& record = load_record::ref::null_ref
                );

            void save_objects_json(
                stream_interface* s,
                load_record::ref& record = load_record::ref::null_ref
                );
#endif

            // Look for objects in the database.
            // find_ throws exceptions when objects are not found.
            template<class T>
            reference<T> find_object_ref(unsigned long long name_hash) const
            {
                ELECTROSLAG_STATIC_CHECK(
                    (std::is_base_of<serializable_object_interface, T>::value),
                    "Can't insert non-serializable type"
                    );

                return (reference<T>(dynamic_cast<T*>(find_object(name_hash))));
            }

            serializable_object_interface* find_object(unsigned long long name_hash) const;

            // locate_ returns null when objects are not found.
            template<class T>
            reference<T> locate_object_ref(unsigned long long name_hash) const
            {
                ELECTROSLAG_STATIC_CHECK(
                    (std::is_base_of<serializable_object_interface, T>::value),
                    "Can't insert non-serializable type"
                    );

                return (reference<T>(dynamic_cast<T*>(locate_object(name_hash))));
            }

            serializable_object_interface* locate_object(unsigned long long name_hash) const;

            // Destroy objects from the database.
            void clear_objects(load_record::ref& record = load_record::ref::null_ref);

            // Iterate all objects.
            typedef delegate<bool, serializable_object_interface*> iterate_object_delegate;

            void iterate_objects(iterate_object_delegate* iterator);

#if !defined(ELECTROSLAG_BUILD_SHIP)
            void dump_types() const;
            void dump_objects(load_record::ref const& record = load_record::ref::null_ref) const;
#endif
        private:
            load_record::ref load_from_archive(archive_reader_interface* ar);

#if !defined(ELECTROSLAG_BUILD_SHIP)
            void save_to_archive(archive_writer_interface* ar, load_record::ref& record = load_record::ref::null_ref);
            bool is_importer(serializable_object_interface const* obj) const;
            void save_object(serializable_object_interface* obj, archive_writer_interface* ar) const;

            void dump_object(serializable_object_interface const* obj) const;
#endif
            // Called from load_record to support object import
            void import_object(serializable_object_interface* obj);
            void remove_object(serializable_object_interface* obj);

            mutable threading::mutex m_mutex;

            // Types registered to be stored in the database.
            typedef std::unordered_map<
                unsigned long long,
                serializable_type_registration const*,
                prehashed_key<unsigned long long>,
                std::equal_to<unsigned long long>
            > serializable_type_table;
            serializable_type_table m_serializable_type_table;

            // The objects in the database.
            typedef std::unordered_map<
                unsigned long long,
                serializable_object_interface*,
                prehashed_key<unsigned long long>,
                std::equal_to<unsigned long long>
            > object_table;
            object_table m_object_table;

            // Load operations mark all of the object loaded as part of a record.
            typedef std::vector<load_record::ref> load_records;
            load_records m_load_records;

            // Disallowed operations:
            explicit database(database const&);
            database& operator =(database const&);

            friend class load_record;
        };

        database* get_database();
    }
}
