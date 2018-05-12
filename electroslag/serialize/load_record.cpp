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
#include "electroslag/serialize/load_record.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace serialize {
        load_record::load_record()
            : m_loaded_object_head(0)
            , m_loaded_object_tail(0)
        {
            std::string mutex_name;
            formatted_string_append(mutex_name, "m:load_record:%016p", this);
            m_mutex.set_name(mutex_name);
        }

        load_record::~load_record()
        {
            database* db = get_database();

            // It's only possible to get here after calling database::clear_objects,
            // so no need to manually remove from the db like load_record::remove_object
            serializable_object_interface* obj = m_loaded_object_head;
            serializable_object_interface* next_obj = 0;
            while (obj) {
                next_obj = obj->get_next_loaded_object();

                if (db->find_type(obj->get_type_hash())->is_referenced()) {
                    referenced_object* referenced_obj = dynamic_cast<referenced_object*>(obj);
                    referenced_obj->release();
                }
                else {
                    delete obj;
                }

                obj = next_obj;
            }
        }

        void load_record::insert_object(
            serializable_object_interface* obj,
            loaded_object_is_referenced is_referenced
            )
        {
            ELECTROSLAG_CHECK(!obj->is_cloned());

            // Figure out if we are keeping a reference.
            if (is_referenced == loaded_object_is_referenced_unknown) {
                is_referenced = (get_database()->find_type(
                    obj->get_type_hash()
                    )->is_referenced()) ? load_record::loaded_object_is_referenced_yes : load_record::loaded_object_is_referenced_no;
            }

            if (is_referenced == loaded_object_is_referenced_yes) {
                referenced_object* referenced_obj = dynamic_cast<referenced_object*>(obj);
                referenced_obj->add_ref();
            }

            // Add to the load record linked list.
            {
                threading::lock_guard load_record_lock(&m_mutex);

                if (m_loaded_object_tail) {
                    m_loaded_object_tail->set_next_loaded_object(obj);
                    m_loaded_object_tail = obj;
                }
                else {
                    m_loaded_object_head = m_loaded_object_tail = obj;
                }
            }

            // Add to the database.
            get_database()->import_object(obj);
        }

        void load_record::remove_object(serializable_object_interface* obj)
        {
            database* db = get_database();

            // Can't find this any more; about to be gone.
            get_database()->remove_object(obj);

            // Remove from the object link list.
            {
                threading::lock_guard load_record_lock(&m_mutex);

                serializable_object_interface* current_obj = m_loaded_object_head;
                serializable_object_interface* next_obj = obj->get_next_loaded_object();
                serializable_object_interface* prev_obj = 0;

                while (current_obj) {
                    if (current_obj == obj) {
                        break;
                    }

                    prev_obj = current_obj;
                    current_obj = current_obj->get_next_loaded_object();
                }
                ELECTROSLAG_CHECK(current_obj == obj);

                if (prev_obj) {
                    prev_obj->set_next_loaded_object(next_obj);
                }

                if (m_loaded_object_head == obj) {
                    ELECTROSLAG_CHECK(prev_obj == 0);
                    m_loaded_object_head = next_obj;
                }

                if (m_loaded_object_tail == obj) {
                    ELECTROSLAG_CHECK(next_obj == 0);
                    m_loaded_object_tail = prev_obj;
                }
            }

            // Blow away the object.
            if (db->find_type(obj->get_type_hash())->is_referenced()) {
                referenced_object* referenced_obj = dynamic_cast<referenced_object*>(obj);
                referenced_obj->release();
            }
            else {
                delete obj;
            }
        }

        bool load_record::has_object(serializable_object_interface const* obj) const
        {
            threading::lock_guard load_record_lock(&m_mutex);

            serializable_object_interface* search_obj = m_loaded_object_head;
            while (search_obj) {
                if (search_obj == obj) {
                    return (true);
                }
                else {
                    search_obj = search_obj->get_next_loaded_object();
                }
            }
            return (false);
        }
    }
}
