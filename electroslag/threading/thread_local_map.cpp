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
#include "electroslag/systems.hpp"
#include "electroslag/threading/this_thread.hpp"
#include "electroslag/threading/thread.hpp"

namespace electroslag {
    namespace threading {
        // static
        thread_local_map* thread_local_map::get()
        {
            return (get_systems()->get_thread_local_map());
        }

        thread_local_map::thread_local_map()
            : m_next_key(0)
            , m_mutex(ELECTROSLAG_STRING_AND_HASH("m:thread_local_map"))
        {
            for (int i = 0; i < reserved_key_count; ++i) {
                m_reserved_keys[i] = generate_key();
            }

#if defined(_WIN32)
            m_tls_index = TlsAlloc();
            if (m_tls_index == TLS_OUT_OF_INDEXES) {
                throw win32_api_failure("TlsAlloc");
            }
#endif
        }

        thread_local_map::~thread_local_map()
        {
            threading::lock_guard thread_local_lock(&m_mutex);
            this_thread::cleanup_on_exit();

            // Clean up any remaining per-thread data blocks. This can happen if there
            // are callbacks from system worker threads, for example.
            thread_vectors::iterator i(m_thread_vectors.begin());
            while (i != m_thread_vectors.end()) {
                cleanup_per_thread_vector(*i);
                ++i;
            }

#if defined(_WIN32)
            if (m_tls_index != TLS_OUT_OF_INDEXES) {
                TlsFree(m_tls_index);
                m_tls_index = TLS_OUT_OF_INDEXES;
            }
#endif
        }

        unsigned int thread_local_map::get_reserved_key(reserved_key key) const
        {
            if (key > reserved_key_unknown && key < reserved_key_count) {
                return (m_reserved_keys[key]);
            }
            else {
                throw parameter_failure("key");
            }
        }

        unsigned int thread_local_map::generate_key()
        {
            return (m_next_key.fetch_add(1));
        }

        void thread_local_map::retire_key(unsigned int /*key*/)
        {
            // Nothing for now...
        }

        void* thread_local_map::get_value(unsigned int key)
        {
            per_thread_vector* v = get_per_thread_vector();
            return (v->at(key));
        }

        void thread_local_map::set_value(unsigned int key, void* value)
        {
            per_thread_vector* v = get_per_thread_vector();
            v->at(key) = value;
        }

        bool thread_local_map::seen_this_thread()
        {
            if (m_tls_index != TLS_OUT_OF_INDEXES) {
                if (TlsGetValue(m_tls_index)) {
                    return (true);
                }
                else {
                    return (false);
                }
            }
            else {
                return (false);
            }
        }

        void thread_local_map::on_thread_exit()
        {
            per_thread_vector* this_vector = 0;

#if defined(_WIN32)
            if (m_tls_index != TLS_OUT_OF_INDEXES) {
                this_vector = reinterpret_cast<per_thread_vector*>(TlsGetValue(m_tls_index));
            }
#endif

            if (this_vector) {
                cleanup_per_thread_vector(this_vector);

                {
                    threading::lock_guard thread_local_lock(&m_mutex);
                    m_thread_vectors.erase(std::remove(m_thread_vectors.begin(), m_thread_vectors.end(), this_vector), m_thread_vectors.end());
                }

#if defined(_WIN32)
                TlsSetValue(m_tls_index, 0);
#endif
            }
        }

        thread_local_map::per_thread_vector* thread_local_map::get_per_thread_vector()
        {
            unsigned int min_size = m_next_key.load();
#if defined(_WIN32)
            ELECTROSLAG_CHECK(m_tls_index != TLS_OUT_OF_INDEXES);
            per_thread_vector* this_vector = reinterpret_cast<per_thread_vector*>(TlsGetValue(m_tls_index));
#endif
            if (!this_vector) {
                threading::lock_guard thread_local_lock(&m_mutex);
                this_vector = new per_thread_vector(min_size, 0);
                m_thread_vectors.emplace_back(this_vector);
#if defined(_WIN32)
                TlsSetValue(m_tls_index, this_vector);
#endif
            }

            if (this_vector->size() < min_size) {
                this_vector->resize(min_size, 0);
            }

            return (this_vector);
        }

        void thread_local_map::cleanup_per_thread_vector(per_thread_vector* this_vector)
        {
            unsigned int key = get_reserved_key(thread_local_map::reserved_key_thread);

            thread* current_thread = reinterpret_cast<thread*>(this_vector->at(key));
            if (current_thread && current_thread->is_wrapper_thread()) {
                delete current_thread;
            }
            // TODO: What about the other remaining pointers still in the vector!?

            delete this_vector;
        }

    }
}
