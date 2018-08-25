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
#include "electroslag/file_stream.hpp"
#include "electroslag/systems.hpp"
#include "electroslag/logger.hpp"
#include "electroslag/resource.hpp"
#include "electroslag/resource_id.hpp"
#include "electroslag/buffer_stream.hpp"
#include "electroslag/serialize/serializable_type_registration.hpp"
#include "electroslag/serialize/serializable_name_table.hpp"
#include "electroslag/serialize/binary_archive.hpp"
#if !defined(ELECTROSLAG_BUILD_SHIP)
#include "electroslag/serialize/json_archive.hpp"
#endif
#include "electroslag/serialize/importer_interface.hpp"

namespace electroslag {
    namespace serialize {
        database* get_database()
        {
            return (get_systems()->get_database());
        }

        database::database()
            : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:database"))
        {}

        database::~database()
        {}

        void database::register_serializable_type(
            unsigned long long name_hash,
            serializable_type_registration* const type
            )
        {
            threading::lock_guard object_db_lock(&m_mutex);
            m_serializable_type_table.insert(std::make_pair(name_hash, type));
        }

        load_record::ref database::create_load_record()
        {
            // Create an empty load record to be populated and then used / saved.
            load_record::ref load = load_record::create();
            {
                threading::lock_guard object_db_lock(&m_mutex);
                m_load_records.emplace_back(load);
            }
            return (load);
        }

        load_record::ref database::load_objects(std::string const& file_name)
        {
            ELECTROSLAG_LOG_SERIALIZE("Loading objects from binary %s.", file_name.c_str());
            file_stream db;
            db.open(file_name, file_stream_access_mode_read);

            std::filesystem::path dir(file_name);
            dir = std::filesystem::canonical(dir);
            dir = dir.remove_filename();

            // Use the directory containing the archive file as the base directory.
            return (load_objects(&db, dir.string()));
        }

        load_record::ref database::load_objects(std::string const& file_name, std::string const& base_directory)
        {
            ELECTROSLAG_LOG_SERIALIZE("Loading objects from binary %s.", file_name.c_str());
            file_stream db;
            db.open(file_name, file_stream_access_mode_read);
            return (load_objects(&db, base_directory));
        }

        load_record::ref database::load_objects(stream_interface* s, std::string const& base_directory)
        {
            binary_archive_reader ar(s);
            ar.set_base_directory(base_directory);
            return (load_from_archive(&ar));
        }

        load_record::ref database::load_objects(
            referenced_buffer_interface::ref const& buffer_ref,
            std::string const& base_directory
            )
        {
            buffer_stream b(buffer_ref);
            return (load_objects(&b, base_directory));
        }

#if !defined(ELECTROSLAG_BUILD_SHIP)
        load_record::ref database::load_objects_json(std::string const& file_name)
        {
            ELECTROSLAG_LOG_SERIALIZE("Loading objects from json %s.", file_name.c_str());
            file_stream db;
            db.open(file_name, file_stream_access_mode_read);

            std::filesystem::path dir(file_name);
            dir = std::filesystem::canonical(dir);
            dir = dir.remove_filename();

            // Use the directory containing the archive file as the base directory.
            return (load_objects_json(&db, dir.string()));
        }

        load_record::ref database::load_objects_json(
            std::string const& file_name,
            std::string const& base_directory
            )
        {
            ELECTROSLAG_LOG_SERIALIZE("Loading objects from json %s.", file_name.c_str());
            file_stream db;
            db.open(file_name, file_stream_access_mode_read);
            return (load_objects_json(&db, base_directory));
        }

        load_record::ref database::load_objects_json(
            stream_interface* s,
            std::string const& base_directory
            )
        {
            json_archive_reader ar(s);
            ar.set_base_directory(base_directory);
            return (load_from_archive(&ar));
        }

        load_record::ref database::load_objects_json(
            referenced_buffer_interface::ref const& buffer_ref,
            std::string const& base_directory
            )
        {
            buffer_stream b(buffer_ref);
            return (load_objects_json(&b, base_directory));
        }

        void database::save_objects(std::string const& file_name, load_record::ref& record)
        {
            ELECTROSLAG_LOG_SERIALIZE("Saving objects to binary %s.", file_name.c_str());
            file_stream db;
            db.create_new(file_name, file_stream_access_mode_write);
            save_objects(&db, record);
        }

        void database::save_objects(stream_interface* s, load_record::ref& record)
        {
            binary_archive_writer ar(s);
            save_to_archive(&ar, record);
        }

        void database::save_objects(
            referenced_buffer_interface::ref const& buffer_ref,
            load_record::ref& record
            )
        {
            buffer_stream b(buffer_ref);
            return (save_objects(&b, record));
        }

        void database::save_objects_json(std::string const& file_name, load_record::ref& record)
        {
            ELECTROSLAG_LOG_SERIALIZE("Saving objects to json %s.", file_name.c_str());
            file_stream db;
            db.create_new(file_name, file_stream_access_mode_write);
            save_objects_json(&db, record);
        }

        void database::save_objects_json(stream_interface* s, load_record::ref& record)
        {
            json_archive_writer ar(s);
            save_to_archive(&ar, record);
        }

        void database::save_objects_json(
            referenced_buffer_interface::ref const& buffer_ref,
            load_record::ref& record
            )
        {
            buffer_stream b(buffer_ref);
            return (save_objects_json(&b, record));
        }
#endif

        serializable_object_interface* database::find_object(unsigned long long name_hash) const
        {
            threading::lock_guard object_db_lock(&m_mutex);
            return (m_object_table.at(name_hash));
        }

        serializable_object_interface* database::locate_object(unsigned long long name_hash) const
        {
            threading::lock_guard object_db_lock(&m_mutex);
            object_table::const_iterator found_object = m_object_table.find(name_hash);
            if (found_object != m_object_table.end()) {
                return (found_object->second);
            }
            else {
                return (0);
            }
        }

        void database::clear_objects(load_record::ref& record)
        {
            threading::lock_guard object_db_lock(&m_mutex);

            if (record.is_valid()) {
                serializable_object_interface* obj = record->get_loaded_object_head();
                while (obj != 0) {
                    m_object_table.erase(obj->get_hash());
                    obj = obj->get_next_loaded_object();
                }

                m_load_records.erase(std::remove(m_load_records.begin(), m_load_records.end(), record), m_load_records.end());
                record.reset();
            }
            else {
                m_object_table.clear();
                m_load_records.clear();
            }
        }

        void database::iterate_objects(iterate_object_delegate* iterator)
        {
            threading::lock_guard object_db_lock(&m_mutex);
            object_table::iterator o(m_object_table.begin());
            while (o != m_object_table.end()) {
                if (!iterator->invoke(o->second)) {
                    break;
                }
                ++o;
            }
        }

        load_record::ref database::load_from_archive(archive_reader_interface* ar)
        {
            load_record::ref load = load_record::create();

            // Prepare the archive reader.
            ELECTROSLAG_CHECK(!ar->get_base_directory().empty());
            ar->set_load_record(load);

            // Read the archive; create objects into the load record.
            unsigned long long type_hash = 0;
            unsigned long long name_hash = 0;
            while (ar->next_object(&type_hash, &name_hash)) {
                serializable_type_registration const* type = find_type(type_hash);
                ELECTROSLAG_CHECK(type);

                ELECTROSLAG_LOG_SERIALIZE("Loading [0x%016llX], Type %s [0x%016llX]",
                    name_hash,
                    type->get_name().c_str(),
                    type_hash
                    );

                serializable_object_interface* new_obj = type->get_create_delegate()->invoke(ar);

                if (type != serializable_name_table::get_type_registration()) {
                    new_obj->set_hash(name_hash);
                    load->insert_object(
                        new_obj,
                        type->is_referenced() ? load_record::loaded_object_is_referenced_yes : load_record::loaded_object_is_referenced_no
                        );
                }
                else {
                    // The name table object is special; we only care about the persistent
                    // side-effects of creation; not the object itself.
                    delete new_obj;
                }
            }

            ar->clear_load_record();

            // Save a reference to the load_record.
            {
                threading::lock_guard object_db_lock(&m_mutex);
                m_load_records.emplace_back(load);
            }

            return (load);
        }

#if !defined(ELECTROSLAG_BUILD_SHIP)
        void database::save_to_archive(archive_writer_interface* ar, load_record::ref& record)
        {
            // Locate all importers in the objects to be saved. These must all finish
            // before saving can occur.
            typedef std::vector<importer_interface::ref> importer_pointer_vector;
            importer_pointer_vector importers;
            {
                threading::lock_guard object_db_lock(&m_mutex);
                if (record.is_valid()) {
                    serializable_object_interface* obj = record->get_loaded_object_head();
                    while (obj != 0) {
                        if (is_importer(obj)) {
                            importers.emplace_back(dynamic_cast<importer_interface*>(obj));
                        }
                        obj = obj->get_next_loaded_object();
                    }

                }
                else {
                    object_table::iterator o(m_object_table.begin());
                    while (o != m_object_table.end()) {
                        if (is_importer(o->second)) {
                            importers.emplace_back(dynamic_cast<importer_interface*>(o->second));
                        }
                        ++o;
                    }
                }
            }

            // The db lock is now dropped to allow importers to finish.
            importer_pointer_vector::iterator i(importers.begin());
            while (i != importers.end()) {
                (*i)->finish_importing();
                ++i;
            }
            importers.clear();

            // Prepare the archive reader.
            ar->set_load_record(record);

            // Reacquire the database lock to save all of the objects.
            {
                threading::lock_guard object_db_lock(&m_mutex);
                if (record.is_valid()) {
                    serializable_object_interface* obj = record->get_loaded_object_head();
                    while (obj != 0) {
                        save_object(obj, ar);
                        obj = obj->get_next_loaded_object();
                    }

                }
                else {
                    object_table::iterator o(m_object_table.begin());
                    while (o != m_object_table.end()) {
                        save_object(o->second, ar);
                        ++o;
                    }
                }

                // Now, write the string form of the names of the objects.
                serializable_name_table* name_table = new serializable_name_table();
                ELECTROSLAG_CHECK(name_table);

                // Temporarily insert the name table into the load record to save.
                record->insert_object(name_table);
                save_object(name_table, ar);
                record->remove_object(name_table); // deletes the object too.
            }

            // Cleanup to finish.
            ar->clear_load_record();
        }

        bool database::is_importer(serializable_object_interface const* obj) const
        {
            unsigned long long type_hash = obj->get_type_hash();
            serializable_type_registration const* type = find_type(type_hash);
            ELECTROSLAG_CHECK(type);

            return (type->is_importer());
        }

        void database::save_object(serializable_object_interface* obj, archive_writer_interface* ar) const
        {
            unsigned long long type_hash = obj->get_type_hash();
            serializable_type_registration const* type = find_type(type_hash);
            ELECTROSLAG_CHECK(type);

            ELECTROSLAG_LOG_SERIALIZE("Saving %s [0x%016llX], Type %s [0x%016llX]",
                (obj->has_name_string() ? obj->get_name().c_str() : "<unnamed>"),
                obj->get_hash(),
                type->get_name().c_str(),
                type_hash
                );
            obj->save_to_archive(ar);
        }
#endif

        void database::import_object(serializable_object_interface* obj)
        {
            threading::lock_guard object_db_lock(&m_mutex);
            m_object_table.insert_or_assign(obj->get_hash(), obj);
        }

        void database::remove_object(serializable_object_interface* obj)
        {
            threading::lock_guard object_db_lock(&m_mutex);
            m_object_table.erase(obj->get_hash());
        }

#if !defined(ELECTROSLAG_BUILD_SHIP)
        void database::dump_types() const
        {
            threading::lock_guard object_db_lock(&m_mutex);
            ELECTROSLAG_LOG_MESSAGE("Serializable Type Table Dump");
            ELECTROSLAG_LOG_MESSAGE("Type Hash          | Ref  | Name");
            ELECTROSLAG_LOG_MESSAGE("-------------------|------|-----------------------------------------------");
            serializable_type_table::const_iterator i(m_serializable_type_table.begin());
            while (i != m_serializable_type_table.end()) {
                ELECTROSLAG_LOG_MESSAGE(
                    "0x%016llX | %4s | %s",
                    i->first,
                    i->second->is_referenced() ? "yes" : "no",
                    i->second->get_name().c_str()
                    );
                ++i;
            }
        }

        void database::dump_objects(load_record::ref const& record) const
        {
            threading::lock_guard object_db_lock(&m_mutex);
            ELECTROSLAG_LOG_MESSAGE("Database Object Table Dump");
            ELECTROSLAG_LOG_MESSAGE("Object Hash        | Type Hash          | RefCount | Name");
            ELECTROSLAG_LOG_MESSAGE("-------------------|--------------------|----------|-----------------------------------------------");
            if (record.is_valid()) {
                serializable_object_interface const* obj = record->get_loaded_object_head();
                while (obj != 0) {
                    dump_object(obj);
                    obj = obj->get_next_loaded_object();
                }

            }
            else {
                object_table::const_iterator o(m_object_table.begin());
                while (o != m_object_table.end()) {
                    dump_object(o->second);
                    ++o;
                }
            }
        }

        void database::dump_object(serializable_object_interface const* obj) const
        {
            int ref_count = 0;

            unsigned long long type_hash = obj->get_type_hash();
            serializable_type_registration const* type = find_type(type_hash);
            if (type) {
                if (type->is_referenced()) {
                    ref_count = dynamic_cast<referenced_object const*>(obj)->get_ref_count();
                }
            }

            std::string object_name;
            if (obj->has_name_string()) {
                object_name = obj->get_name();
            }
            else {
                object_name = "<unnamed>";
            }

            ELECTROSLAG_LOG_MESSAGE(
                "0x%016llX | 0x%016llX | %8d | %s",
                obj->get_hash(),
                type_hash,
                ref_count,
                object_name.c_str()
                );
        }
#endif
    }
}
