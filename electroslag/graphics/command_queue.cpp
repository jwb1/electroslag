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
#include "electroslag/threading/mutex.hpp"
#include "electroslag/graphics/command_queue.hpp"
#include "electroslag/graphics/graphics_interface.hpp"

namespace electroslag {
    namespace graphics {
        command_queue::command_queue(unsigned long long name_hash)
            : command_queue_interface(name_hash)
        {
            get_graphics()->get_render_thread()->check_not();
        }

        command_queue::command_queue(std::string const& name)
            : command_queue_interface(name)
        {
            get_graphics()->get_render_thread()->check_not();
        }

        command_queue::command_queue(std::string const& name, unsigned long long name_hash)
            : command_queue_interface(name, name_hash)
        {
            get_graphics()->get_render_thread()->check_not();
        }

        command_queue::~command_queue()
        {
            get_graphics()->get_render_thread()->check_not();
            threading::lock_guard policy_lock(get_graphics()->get_render_policy()->get_mutex());

            // Destroy all of the per thread data.
            queue_vector::iterator queue_iter(m_queue_vector.begin());
            while (queue_iter != m_queue_vector.end()) {
                delete *queue_iter;
                ++queue_iter;
            }
            m_queue_vector.clear();

            // Nuke any pending updates that will no longer happen.
            queue_vector::iterator update_iter(m_update_vector.begin());
            while (update_iter != m_update_vector.end()) {
                delete *update_iter;
                ++update_iter;
            }
            m_update_vector.clear();
        }

        void* command_queue::get_command_memory(int bytes, int alignment)
        {
            double_buffer_queue* q = m_producer_data.get();

            if (!q) {
                q = new double_buffer_queue();

                // Update the producer and consumer structures.
                m_producer_data.reset(q);
                {
                    threading::lock_guard policy_lock(get_graphics()->get_render_policy()->get_mutex());
                    m_update_vector.emplace_back(q);
                }
            }

            return (q->enqueue(bytes, alignment));
        }

        void command_queue::execute_commands(context_interface* context)
        {
            get_graphics()->get_render_thread()->check();

#if !defined(ELECTROSLAG_BUILD_SHIP)
            if (has_name_string()) {
                context->push_debug_group(get_name());
            }
#endif

            // Iterate over each thread that may have enqueued commands.
            queue_vector::const_iterator i(m_queue_vector.begin());
            while (i != m_queue_vector.end()) {
                double_buffer_queue* one_threads_queue = (*i);

                // Iterate over all commands, executing each.
                command* current_command = reinterpret_cast<command*>(one_threads_queue->dequeue());
                while (current_command) {
                    current_command->execute(context);
                    current_command->~command();
                    current_command = reinterpret_cast<command*>(one_threads_queue->dequeue());
                }
                ++i;
            }

#if !defined(ELECTROSLAG_BUILD_SHIP)
            if (has_name_string()) {
                context->pop_debug_group(get_name());
            }
#endif
        }

        void command_queue::swap()
        {
            // The render thread should be parked at this point; this
            // is important because we are going to access the m_queue_vector,
            // which doesn't get protected any other way.
            get_graphics()->get_render_thread()->check_not();

            // Handle any updates to the queue vector that were waiting.
            m_queue_vector.insert(
                m_queue_vector.end(),
                m_update_vector.begin(),
                m_update_vector.end()
                );
            m_update_vector.clear();

            queue_vector::const_iterator i(m_queue_vector.begin());
            while (i != m_queue_vector.end()) {
                (*i)->swap();
                ++i;
            }
        }
    }
}
