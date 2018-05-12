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
#include "electroslag/threading/worker_thread.hpp"

namespace electroslag {
    namespace threading {
        worker_thread::worker_thread(std::string const& base_name)
            : m_worker(0)
            , m_state(worker_state_initializing)
        {
            std::string name;

            name.assign("m:");
            name.append(base_name);
            m_mutex.set_name(name);

            name.assign("cv:");
            name.append(base_name);
            name.append(":work_ready");
            m_work_ready.set_name(name);

            name.assign("cv:");
            name.append(base_name);
            name.append(":work_done");
            m_work_done.set_name(name);

            thread_initializer* initializer = 0;
            initializer = new thread_initializer();
            initializer->worker = this;
            initializer->base_name = base_name;
            m_worker = new thread();
            m_worker->spawn(&thread_stub, initializer);
        }

        worker_thread::~worker_thread()
        {
            {
                lock_guard worker_thread_lock(&m_mutex);

                // Signal all threads to exit.
                m_state = worker_state_exiting;
                m_work_ready.notify_all();

                m_work_done.wait(&worker_thread_lock);
            }

            m_worker->join();
            delete m_worker;
        }

        void worker_thread::wait_for_work_done(work_item_interface const* work)
        {
            lock_guard worker_thread_lock(&m_mutex);
            while (!work->is_done()) {
                m_work_done.wait(&worker_thread_lock);
            }
        }

        void worker_thread::thread_method(thread_initializer* initializer)
        {
            work_item_interface::ref work;

            try {
                // Finish thread initialization.
                {
                    lock_guard worker_thread_lock(&m_mutex);

                    // Set the worker thread's name; has to be done on thread.
                    if (!initializer->base_name.empty()) {
                        std::string worker_thread_name("t:");
                        worker_thread_name.append(initializer->base_name);
                        m_worker->set_thread_name(worker_thread_name);
                    }

                    delete initializer;
                    m_state = worker_state_ready;
                    m_work_ready.notify_all();
                }

                // Work loop.
                bool exit_thread = false;
                do {
                    {
                        lock_guard worker_thread_lock(&m_mutex);

                        // Deal with the bookkeeping for a just executed work item.
                        if (work.is_valid()) {
                            work->set_done();
                            m_work_done.notify_all();

                            work.reset();
                        }

                        // Wait for something to do.
                        do {
                            if (!m_work_queue.empty()) {
                                work = m_work_queue.front();
                                m_work_queue.pop_front();
                                break;
                            }

                            if (m_state == worker_state_exiting) {
                                exit_thread = true;
                                break;
                            }

                            m_work_ready.wait(&worker_thread_lock);
                        } while (!exit_thread);
                    }

                    if (work.is_valid()) {
                        work->execute();
                    }
                } while (!exit_thread);

                // We're done.
                {
                    lock_guard worker_thread_lock(&m_mutex);
                    m_state = worker_state_exited;
                    m_work_done.notify_all();
                }
            }
            catch (...) {
                lock_guard worker_thread_lock(&m_mutex);

                if (work.is_valid()) {
                    work->set_done();
                    work.reset();
                }

                m_state = worker_state_exception;
                m_exception = std::current_exception();
                m_work_done.notify_all();
            }
        }
    }
}
