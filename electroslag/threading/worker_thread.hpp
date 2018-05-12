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
#include "electroslag/reference.hpp"
#include "electroslag/threading/mutex.hpp"
#include "electroslag/threading/condition_variable.hpp"
#include "electroslag/threading/thread.hpp"
#include "electroslag/threading/work_item_interface.hpp"

namespace electroslag {
    namespace threading {
        class worker_thread {
        public:
            explicit worker_thread(std::string const& base_name = 0);
            virtual ~worker_thread();

            template<class T, class... Params>
            reference<T> enqueue_work_item(Params... params)
            {
                ELECTROSLAG_STATIC_CHECK(
                    (std::is_base_of<work_item_interface, T>::value),
                    "Can only enqueue classes derived from work_item_interface base class"
                    );

                reference<T> new_work_item;
                {
                    lock_guard thread_pool_lock(&m_mutex);

                    // Create and enqueue the work item.
                    new_work_item = new T(params...);
                    new_work_item->set_worker_thread(this);

                    m_work_queue.emplace_back(new_work_item.cast<work_item_interface>());
                }

                // Make sure the thread knows to do the new work item.
                m_work_ready.notify_one();

                return (new_work_item);
            }

        private:
            enum worker_state {
                worker_state_unknown = -1,
                worker_state_initializing = 0,
                worker_state_ready = 1,
                worker_state_exiting = 2,
                worker_state_exited = 3,
                worker_state_exception = 4
            };

            struct thread_initializer {
                worker_thread* worker;
                std::string base_name;
            };

            static void thread_stub(void* argument)
            {
                thread_initializer* initializer = static_cast<thread_initializer*>(argument);
                initializer->worker->thread_method(initializer);
            }

            void thread_method(thread_initializer* initializer);

            void wait_for_work_done(work_item_interface const* work);

            // Initialized data.
            thread* m_worker;

            // State protected by the pool mutex.
            worker_state m_state;
            std::exception_ptr m_exception;

            // The current work queue.
            typedef std::deque<work_item_interface::ref> work_queue;
            work_queue m_work_queue;

            // Synchronization for the availability and completion of work.
            mutex m_mutex;
            condition_variable m_work_ready;
            condition_variable m_work_done;

            // Disallowed operations:
            explicit worker_thread(worker_thread const&);
            worker_thread& operator =(worker_thread const&);

            // So the work items can use the unified wait method.
            friend class work_item_interface;
        };
    }
}
