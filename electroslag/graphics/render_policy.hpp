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
#include "electroslag/threading/mutex.hpp"
#include "electroslag/graphics/command_queue.hpp"
#include "electroslag/graphics/context_interface.hpp"
#include "electroslag/graphics/sync_interface.hpp"

namespace electroslag {
    namespace graphics {
        class render_policy {
        public:
            render_policy();
            ~render_policy();

            void initialize();
            void destroy_graphics_objects();
            void shutdown();

            threading::mutex* get_mutex() const
            {
                return (&m_mutex);
            }

            // Client objects access the command lists via these methods
            void insert_command_queue(
                command_queue_interface::ref const& q
                );
            
            void insert_command_queue(
                command_queue_interface::ref const& q,
                command_queue_interface::ref const& insert_after
                );

            void remove_command_queue(command_queue_interface::ref const& q);

            command_queue_interface::ref find_command_queue(unsigned long long name_hash) const;

            command_queue_interface::ref find_command_queue(std::string const& name) const
            {
                return (find_command_queue(hash_string_runtime(name)));
            }

            command_queue_interface::ref get_system_command_queue() const;

            sync_interface::ref get_system_sync() const;

            // Called by the render thread.
            void execute_command_queues(context_interface* context);
            void execute_system_command_queue(context_interface* context);

            // Called by the run thread.
            void swap();

        private:
            class set_sync_command : public command {
            public:
                explicit set_sync_command(sync_interface::ref const& system_sync)
                    : m_system_sync(system_sync)
                {}

                virtual void execute(context_interface* context);

            private:
                sync_interface::ref m_system_sync;

                // Disallowed operations:
                set_sync_command();
                explicit set_sync_command(set_sync_command const&);
                set_sync_command& operator =(set_sync_command const&);
            };

            // Protects this object's state, and the command list state as well.
            mutable threading::mutex m_mutex;

            command_queue_interface::ref m_system_command_queue;
            command_queue_interface::ref m_system_sync_queue;

            mutable sync_interface::ref m_system_sync;

            typedef std::vector<command_queue_interface::ref> frame_command_queues;

            frame_command_queues m_current_frame_queues;
            frame_command_queues m_executing_frame_queues;

            // Disallowed operations:
            explicit render_policy(render_policy const&);
            render_policy& operator =(render_policy const&);
        };
    }
}
