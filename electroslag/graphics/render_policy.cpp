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
#include "electroslag/graphics/render_policy.hpp"
#include "electroslag/graphics/graphics_interface.hpp"

namespace electroslag {
    namespace graphics {
        render_policy::render_policy()
            : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:render_policy"))
        {}

        render_policy::~render_policy()
        {
            shutdown();
        }

        void render_policy::initialize()
        {
            threading::lock_guard policy_lock(&m_mutex);

            // The system command list should always be first.
            m_system_command_queue = command_queue::create(ELECTROSLAG_STRING_AND_HASH("cq:system"));
            m_current_frame_queues.emplace_back(m_system_command_queue);

            // A special queue that only ever contains a sync object
            m_system_sync_queue = command_queue::create(ELECTROSLAG_STRING_AND_HASH("cq:system-sync"));
            m_current_frame_queues.emplace_back(m_system_sync_queue);
        }

        void render_policy::destroy_graphics_objects()
        {
            threading::lock_guard policy_lock(&m_mutex);
            m_system_sync.reset();
        }

        void render_policy::shutdown()
        {
            threading::lock_guard policy_lock(&m_mutex);
            m_current_frame_queues.clear();
            m_executing_frame_queues.clear();
            m_system_command_queue.reset();
            m_system_sync_queue.reset();
        }

        void render_policy::insert_command_queue(
            command_queue_interface::ref const& q
            )
        {
            get_graphics()->get_render_thread()->check_not();
            threading::lock_guard policy_lock(&m_mutex);

            if (q.is_valid()) {
                frame_command_queues::iterator i(std::find(
                    m_current_frame_queues.begin(),
                    m_current_frame_queues.end(),
                    q
                    ));
                if (i != m_current_frame_queues.end()) {
                    throw std::runtime_error("cannot add duplicate queue to policy");
                }

                m_current_frame_queues.emplace_back(q);
            }
            else {
                throw parameter_failure("q");
            }
        }

        void render_policy::insert_command_queue(
            command_queue_interface::ref const& q,
            command_queue_interface::ref const& insert_after
            )
        {
            get_graphics()->get_render_thread()->check_not();
            threading::lock_guard policy_lock(&m_mutex);

            if (q.is_valid()) {
                frame_command_queues::iterator i(m_current_frame_queues.begin());
                frame_command_queues::iterator insert_point(m_current_frame_queues.end());
                while (i != m_current_frame_queues.end()) {
                    if ((*i)->is_equal(q)) {
                        throw std::runtime_error("cannot add duplicate queue to policy");
                    }
                    else if ((*i)->is_equal(insert_after)) {
                        insert_point = i;
                        ++insert_point; //insert_point must be one past the insert_after.
                    }
                    ++i;
                }

                m_current_frame_queues.insert(insert_point, q);
            }
            else {
                throw parameter_failure("q");
            }
        }

        void render_policy::remove_command_queue(
            command_queue_interface::ref const& q
            )
        {
            get_graphics()->get_render_thread()->check_not();
            threading::lock_guard policy_lock(&m_mutex);

            // Disallow removal of the system command queue.
            if (q == m_system_command_queue || q == m_system_sync_queue) {
                throw std::runtime_error("cannot remove system command queue");
            }

            m_current_frame_queues.erase(
                std::remove(m_current_frame_queues.begin(), m_current_frame_queues.end(), q),
                m_current_frame_queues.end()
                );
        }

        command_queue_interface::ref render_policy::find_command_queue(
            unsigned long long name_hash
            ) const
        {
            get_graphics()->get_render_thread()->check_not();
            threading::lock_guard policy_lock(&m_mutex);

            frame_command_queues::const_iterator i(m_current_frame_queues.begin());
            while (i != m_current_frame_queues.end()) {
                if ((*i)->is_equal(name_hash)) {
                    return (*i);
                }
                ++i;
            }
            throw object_not_found_failure("command_queue_interface", name_hash);
        }

        command_queue_interface::ref render_policy::get_system_command_queue() const
        {
            graphics::graphics_interface* g = get_graphics();
            g->get_render_thread()->check_not();
            threading::lock_guard policy_lock(&m_mutex);

            if (!m_system_sync.is_valid()) {
                m_system_sync = g->create_sync();
            }

            return (m_system_command_queue);
        }

        sync_interface::ref render_policy::get_system_sync() const
        {
            threading::lock_guard policy_lock(&m_mutex);
            return (m_system_sync);
        }

        void render_policy::execute_command_queues(context_interface* context)
        {
            // No mutex here. The executing refs vector is only touched in swap,
            // which happens while the render thread is blocked.
            get_graphics()->get_render_thread()->check();

            frame_command_queues::iterator i(m_executing_frame_queues.begin());
            while (i != m_executing_frame_queues.end()) {
                (*i)->execute_commands(context);
                ++i;
            }
        }

        void render_policy::execute_system_command_queue(context_interface* context)
        {
            // No mutex here. The executing refs vector is only touched in swap,
            // which happens while the render thread is blocked.
            get_graphics()->get_render_thread()->check();
            m_executing_frame_queues.at(0)->execute_commands(context);
        }

        void render_policy::swap()
        {
            get_graphics()->get_render_thread()->check_not();
            threading::lock_guard policy_lock(&m_mutex);

            if (m_system_sync.is_valid()) {
                // Set the sync object that always is set after system queue work is done.
                m_system_sync_queue->enqueue_command<set_sync_command>(m_system_sync);
                m_system_sync.reset();
            }

            // Save a reference to all of the queues we are going to execute.
            m_executing_frame_queues.assign(m_current_frame_queues.begin(), m_current_frame_queues.end());

            // Swap all queues.
            frame_command_queues::iterator i(m_executing_frame_queues.begin());
            while (i != m_executing_frame_queues.end()) {
                (*i)->swap();
                ++i;
            }
        }

        void render_policy::set_sync_command::execute(context_interface* context)
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            context->set_sync_point(m_system_sync);
        }
    }
}
