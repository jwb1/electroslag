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

namespace electroslag {
    namespace threading {
        class worker_thread;
        class work_item_interface : public referenced_object {
        public:
            typedef reference<work_item_interface> ref;
            typedef std::vector<ref> work_item_vector;

            virtual ~work_item_interface()
            {}

            bool is_done() const
            {
                return (m_done.load(std::memory_order_acquire));
            }

            void wait_for_done() const;

        protected:
            // Work items are constructed by declaring the items that they depend on.
            // But they are stored in terms of the items that depend on them.
            work_item_interface()
                : m_worker(0)
                , m_done(false)
            {}

        private:
            // Methods called by the worker_thread
            void set_worker_thread(worker_thread* worker)
            {
                ELECTROSLAG_CHECK(worker);
                m_worker = worker;
            }

            virtual void execute() = 0;

            void set_done()
            {
                m_done.store(true, std::memory_order_release);
            }

            // The worker thread this work item was assigned to; Expected to be set
            // during the process of work item creation; constant after.
            worker_thread* m_worker;

            // Set to true when this item itself is done.
            std::atomic<bool> m_done;

            // Disallowed operations:
            explicit work_item_interface(work_item_interface const&);
            work_item_interface& operator =(work_item_interface const&);

            friend class worker_thread;
        };
    }
}
