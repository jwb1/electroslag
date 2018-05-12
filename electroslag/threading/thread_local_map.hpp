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

namespace electroslag {
    namespace threading {
        class thread_local_map {
        public:
            enum reserved_key {
                reserved_key_unknown = -1,
                reserved_key_thread,
                reserved_key_count // Ensure this is the last enum entry
            };

            static thread_local_map* get();

            thread_local_map();
            ~thread_local_map();

            bool seen_this_thread();
            void on_thread_exit();

            unsigned int get_reserved_key(reserved_key key) const;

            unsigned int generate_key();
            void retire_key(unsigned int key);

            void* get_value(unsigned int key);
            void set_value(unsigned int key, void* value);

        private:
            typedef std::vector<void*> per_thread_vector;

            per_thread_vector* get_per_thread_vector();
            void cleanup_per_thread_vector(per_thread_vector* this_vector);

            unsigned int m_reserved_keys[reserved_key_count];
            std::atomic<unsigned int> m_next_key;
#if defined(_WIN32)
            unsigned int m_tls_index;
#endif

            mutable mutex m_mutex;

            typedef std::vector<per_thread_vector*> thread_vectors;
            thread_vectors m_thread_vectors;

            // Disallowed operations:
            explicit thread_local_map(thread_local_map const&);
            thread_local_map& operator =(thread_local_map const&);
        };
    }
}
