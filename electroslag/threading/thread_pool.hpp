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
#include "electroslag/named_object.hpp"
#include "electroslag/dynamic_array.hpp"
#include "electroslag/threading/mutex.hpp"
#include "electroslag/threading/condition_variable.hpp"
#include "electroslag/threading/work_item_interface.hpp"
#include "electroslag/threading/worker_thread.hpp"

namespace electroslag {
    namespace threading {
        class thread_pool : public named_object {
        public:
            explicit thread_pool(std::string const& base_name = 0, int total_threads = 0);
            virtual ~thread_pool()
            {}

            template<class T, class... Params>
            reference<T> enqueue_work_item(Params... params)
            {
                ELECTROSLAG_STATIC_CHECK(
                    (std::is_base_of<work_item_interface, T>::value),
                    "Can only enqueue classes derived from work_item_interface base class"
                    );

                // Schedule a worker to do the work
                unsigned int worker_index = m_next_worker++; // This is an atomic r-m-w
                worker_index = worker_index % m_workers.get_entries();

                return (m_workers[worker_index].enqueue_work_item<T>(params...));
            }

        private:
            std::atomic<unsigned int> m_next_worker;

            // This vector is created in the constructor and is constant until the destructor.
            typedef dynamic_array<worker_thread> worker_vector;
            worker_vector m_workers;

            // Disallowed operations:
            explicit thread_pool(thread_pool const&);
            thread_pool& operator =(thread_pool const&);
        };

        thread_pool* get_frame_thread_pool();
        thread_pool* get_io_thread_pool();
    }
}
