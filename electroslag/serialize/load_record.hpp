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
#include "electroslag/threading/mutex.hpp"
#include "electroslag/serialize/serializable_object_interface.hpp"

namespace electroslag {
    namespace serialize {
        class load_record : public referenced_object {
        public:
            typedef reference<load_record> ref;

            // We often have this info when inserting/importing objects, so it can
            // save us a type lookup.
            enum loaded_object_is_referenced {
                loaded_object_is_referenced_unknown = -1,
                loaded_object_is_referenced_no = 0,
                loaded_object_is_referenced_yes
            };

            // Import adds to both the load record and the underlying database.
            template<class T>
            void insert_object(reference<T>& obj)
            {
                ELECTROSLAG_STATIC_CHECK(
                    (std::is_base_of<serializable_object_interface, T>::value),
                    "Can't import non-serializable type"
                );

                insert_object(obj.get_pointer(), loaded_object_is_referenced_yes);
            }

            void insert_object(
                serializable_object_interface* obj,
                loaded_object_is_referenced is_referenced = loaded_object_is_referenced_unknown
                );

            // Remove an object from this load record
            template<class T>
            void remove_object(reference<T>& obj)
            {
                ELECTROSLAG_STATIC_CHECK(
                    (std::is_base_of<serializable_object_interface, T>::value),
                    "Can't insert non-serializable type"
                    );

                remove_object(obj.get_pointer());
            }

            void remove_object(serializable_object_interface* obj);

            // Determine if an object is in this load record.
            bool has_object(serializable_object_interface const* obj) const;

            // Allow for iteration.
            serializable_object_interface const* get_loaded_object_head() const
            {
                return (m_loaded_object_head);
            }

            serializable_object_interface* get_loaded_object_head()
            {
                return (m_loaded_object_head);
            }

            serializable_object_interface const* get_loaded_object_tail() const
            {
                return (m_loaded_object_tail);
            }

            serializable_object_interface* get_loaded_object_tail()
            {
                return (m_loaded_object_tail);
            }

        private:
            static ref create()
            {
                return (ref(new load_record()));
            }

            load_record();
            virtual ~load_record();

            mutable threading::mutex m_mutex;

            // The objects generated by a load operation, stored in a list in the
            // load order.
            serializable_object_interface* m_loaded_object_head;
            serializable_object_interface* m_loaded_object_tail;

            // Disallowed operations:
            explicit load_record(load_record const&);
            load_record& operator =(load_record const&);

            // load_records should only be created by database load operations.
            friend class database;
        };
    }
}
