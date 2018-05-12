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

namespace electroslag {
    namespace threading {
        thread_pool* get_frame_thread_pool()
        {
            return (get_systems()->get_frame_thread_pool());
        }

        thread_pool* get_io_thread_pool()
        {
            return (get_systems()->get_io_thread_pool());
        }

        thread_pool::thread_pool(std::string const& base_name, int total_threads)
            : m_next_worker(0)
        {
            if (total_threads <= 0) {
                total_threads = thread::hardware_concurrency() * 2;
                if (total_threads <= 0) {
                    throw std::runtime_error("invalid thread count");
                }
            }

            m_workers.set_entries(total_threads);
            for (int i = 0; i < total_threads; ++i) {
                std::string worker_base_name;
                formatted_string_append(worker_base_name, "%s:worker%d", base_name.c_str(), i);
                m_workers.emplace(i , worker_base_name);
            }
        }
    }
}
