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
#include "electroslag/systems.hpp"

namespace electroslag {
    systems* get_systems()
    {
        static systems sys;
        return (&sys);
    }

    systems::~systems()
    {
        m_destruction = true;
        // The order of destruction is important!

        if (m_renderer) {
            m_renderer->~renderer();
            m_renderer = 0;
        }

        if (m_property_manager) {
            m_property_manager->~property_manager();
            m_property_manager = 0;
        }

        if (m_graphics_opengl) {
            m_graphics_opengl->~graphics_opengl();
            m_graphics_opengl = 0;
        }

        if (m_ui_win32) {
            m_ui_win32->~ui_win32();
            m_ui_win32 = 0;
        }

        if (m_database) {
            m_database->~database();
            m_database = 0;
        }

        if (m_frame_thread_pool) {
            m_frame_thread_pool->~thread_pool();
            m_frame_thread_pool = 0;
        }

        if (m_io_thread_pool) {
            m_io_thread_pool->~thread_pool();
            m_io_thread_pool = 0;
        }

        if (m_logger) {
            m_logger->~logger();
            m_logger = 0;
        }

        if (m_thread_local_map) {
            m_thread_local_map->~thread_local_map();
            m_thread_local_map = 0;
        }

        if (m_name_table) {
            m_name_table->~name_table();
            m_name_table = 0;
        }
    }
}
