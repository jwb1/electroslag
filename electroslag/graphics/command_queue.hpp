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
#include "electroslag/threading/thread_local_ptr.hpp"
#include "electroslag/graphics/command_queue_interface.hpp"
#include "electroslag/graphics/double_buffer_queue.hpp"
#include "electroslag/graphics/context_interface.hpp"

namespace electroslag {
    namespace graphics {
        class command_queue : public command_queue_interface {
        public:
            static ref create(unsigned long long name_hash)
            {
                return (ref(new command_queue(name_hash)));
            }

            static ref create(std::string const& name)
            {
                return (ref(new command_queue(name)));
            }

            static ref create(std::string const& name, unsigned long long name_hash)
            {
                return (ref(new command_queue(name, name_hash)));
            }

        protected:
            explicit command_queue(unsigned long long name_hash);
            explicit command_queue(std::string const& name);
            command_queue(std::string const& name, unsigned long long name_hash);
            virtual ~command_queue();

            // Implement queue_interface
            virtual void* get_command_memory(int bytes, int alignment);
            virtual void execute_commands(context_interface* context);
            virtual void swap();

        private:
            // Producer threads each have their own structure.
            threading::thread_local_ptr<double_buffer_queue> m_producer_data;

            // Consumer thread reads from all queues.
            typedef std::vector<double_buffer_queue*> queue_vector;
            queue_vector m_queue_vector;

            // Updates to the queue vector are made during swap to synchronize
            // access to the queue vector without needing an extra mutex.
            queue_vector m_update_vector;

            // Disallowed operations:
            command_queue();
            explicit command_queue(command_queue const&);
            command_queue& operator =(command_queue const&);
        };
    }
}
