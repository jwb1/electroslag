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
#include "electroslag/threading/this_thread.hpp"
#include "electroslag/threading/thread_local_map.hpp"
#include "electroslag/threading/thread.hpp"

namespace electroslag {
    namespace threading {
        // static
        thread* this_thread::get()
        {
            thread_local_map* tls_map = thread_local_map::get();
            if (tls_map->seen_this_thread()) {
                thread* current_thread = reinterpret_cast<thread*>(
                    tls_map->get_value(tls_map->get_reserved_key(thread_local_map::reserved_key_thread))
                    );
                if (current_thread) {
                    return (current_thread);
                }
            }

#if defined(_WIN32)
            thread* wrapper_thread = new thread(GetCurrentThreadId(), GetCurrentThread());
#else
#error How to create a thread_wrapper for this platform?
#endif
            set(wrapper_thread);
            return (wrapper_thread);
        }

        // static
        void this_thread::set(thread* t)
        {
            thread_local_map* tls_map = thread_local_map::get();
            unsigned int key = tls_map->get_reserved_key(thread_local_map::reserved_key_thread);

#if !defined(ELECTROSLAG_BUILD_SHIP)
            thread* current_thread = reinterpret_cast<thread*>(tls_map->get_value(key));
            if (current_thread) {
                throw std::logic_error("There should not be a TLS map for this_thread::set threads!");
            }
#endif

            tls_map->set_value(key, t);
        }

        // static
        void this_thread::cleanup_on_exit()
        {
            // The thread_local_map knows how to clean up any dangling values in reserved_key_thread
            thread_local_map::get()->on_thread_exit();
        }
    }
}
