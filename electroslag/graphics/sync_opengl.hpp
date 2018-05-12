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
#include "electroslag/graphics/sync_interface.hpp"
#include "electroslag/graphics/context_interface.hpp"

namespace electroslag {
    namespace graphics {
        class sync_opengl : public sync_interface {
        public:
            typedef reference<sync_opengl> ref;

            static ref create()
            {
                return (ref(new sync_opengl()));
            }

            // Implement sync_interface
            virtual bool is_clear() const
            {
                return (m_state.load(std::memory_order_acquire) == sync_state_clear);
            }
            virtual bool is_set() const
            {
                return (m_state.load(std::memory_order_acquire) == sync_state_set);
            }
            virtual bool is_signaled() const
            {
                return (m_state.load(std::memory_order_acquire) == sync_state_signaled);
            }
            virtual void clear()
            {
                // TODO: What if this is being waited on?
                m_state.store(sync_state_clear, std::memory_order_release);
            }

            void opengl_set();
            void opengl_wait();

        private:
            enum sync_state {
                sync_state_unknown = 1,
                sync_state_clear,
                sync_state_set,
                sync_state_signaled
            };

            sync_opengl()
                : m_state(sync_state_clear)
                , m_sync(0)
            {}
            virtual ~sync_opengl()
            {}

            std::atomic<sync_state> m_state;
            GLsync m_sync;

            // Disallowed operations:
            explicit sync_opengl(sync_opengl const&);
            sync_opengl& operator =(sync_opengl const&);
        };
    }
}
