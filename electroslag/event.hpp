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
#include "electroslag/delegate.hpp"

namespace electroslag {
    enum event_bind_mode {
        event_bind_mode_unknown = -1,
        event_bind_mode_reference_listener,
        event_bind_mode_own_listener
    };

    template<class ReturnType, class... Params>
    class event {
    public:
        typedef delegate<ReturnType, Params...> bound_delegate;

        event() {}
        ~event() { clear(); }

        void signal(
            Params... params
            ) const
        {
            // Indices instead of iterators and reverse iteration to allow
            // unbind on invoke.
            int i = static_cast<int>(m_listeners.size());
            while (i > 0) {
                m_listeners.at(--i).listener->invoke(params...);
            }
        }

        void clear()
        {
            typename listener_vector::iterator i(m_listeners.begin()); 
            while (i != m_listeners.end()) {
                if (i->bind_mode == event_bind_mode_own_listener) {
                    delete i->listener;
                }
                ++i;
            }
            m_listeners.clear();
        }

        bool has_bound_delegate() const
        {
            return (!m_listeners.empty());
        }

        void bind(
            bound_delegate* listener,
            event_bind_mode bind_mode = event_bind_mode_reference_listener
            )
        {
            if (!listener) {
                throw parameter_failure("listener");
            }

            if (bind_mode < event_bind_mode_reference_listener ||
                bind_mode > event_bind_mode_own_listener) {
                throw parameter_failure("bind_mode");
            }

            m_listeners.emplace_back(listener, bind_mode);
        }

        void unbind(
            bound_delegate* listener
            )
        {
            listener_record search_record(listener, event_bind_mode_unknown);
            typename listener_vector::iterator found_listener = std::find(
                m_listeners.begin(),
                m_listeners.end(),
                search_record
                );
            if (found_listener != m_listeners.end()) {
                if (found_listener->bind_mode == event_bind_mode_own_listener) {
                    delete found_listener->listener;
                }
                m_listeners.erase(found_listener);
            }
        }

    private:
        struct listener_record {
            listener_record(
                bound_delegate* new_listener,
                event_bind_mode new_bind_mode
                )
                : listener(new_listener)
                , bind_mode(new_bind_mode)
            {}

            bool operator ==(listener_record const& compare_with) const
            {
                return (listener == compare_with.listener);
            }

            bound_delegate* listener;
            event_bind_mode bind_mode;
        };
        typedef std::vector<listener_record> listener_vector;
        listener_vector m_listeners;

        // Disallowed operations:
        explicit event(event const&);
        event& operator =(event const&);
    };
}
