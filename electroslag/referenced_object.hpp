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
    class referenced_object {
    public:
        void add_ref()
        {
            m_reference_count.fetch_add(1, std::memory_order_relaxed);
        }

        void release()
        {
            int before_release_references = m_reference_count.fetch_sub(1, std::memory_order_release);
            ELECTROSLAG_CHECK(before_release_references > 0);
            if (before_release_references == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete this;
            }
        }

        int get_ref_count() const
        {
            return (m_reference_count.load());
        }

    protected:
        referenced_object()
            : m_reference_count(0)
        {}

        virtual ~referenced_object()
        {}

    private:
        mutable std::atomic<int> m_reference_count;

        // Disallowed operations:
        explicit referenced_object(referenced_object const&);
        referenced_object& operator =(referenced_object const&);
    };
}
