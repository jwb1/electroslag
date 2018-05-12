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
#include "electroslag/name_table.hpp"
#include "electroslag/logger.hpp"
#include "electroslag/threading/thread_local_map.hpp"
#include "electroslag/threading/thread_pool.hpp"
#include "electroslag/serialize/database.hpp"
#include "electroslag/ui/ui_win32.hpp"
#include "electroslag/graphics/graphics_opengl.hpp"
#include "electroslag/animation/property_manager.hpp"
#include "electroslag/renderer/renderer.hpp"

namespace electroslag {
    class name_table;
    class systems {
    public:
        systems()
            : m_name_table(0)
            , m_logger(0)
            , m_thread_local_map(0)
            , m_frame_thread_pool(0)
            , m_io_thread_pool(0)
            , m_database(0)
            , m_ui_win32(0)
            , m_graphics_opengl(0)
            , m_renderer(0)
            , m_property_manager(0)
            , m_destruction(false)
        {}
        ~systems();

        name_table* get_name_table()
        {
            if (!m_name_table) {
                ELECTROSLAG_CHECK(!m_destruction);
                m_name_table = reinterpret_cast<name_table*>(m_system_buffer + name_table_offset);
                new (m_name_table) name_table();
            }
            return (m_name_table);
        }

        logger* get_logger()
        {
            if (!m_logger) {
                ELECTROSLAG_CHECK(!m_destruction);
                m_logger = reinterpret_cast<logger*>(m_system_buffer + logger_offset);
                new (m_logger) logger();
            }
            return (m_logger);
        }

        threading::thread_local_map* get_thread_local_map()
        {
            if (!m_thread_local_map) {
                ELECTROSLAG_CHECK(!m_destruction);
                m_thread_local_map = reinterpret_cast<threading::thread_local_map*>(m_system_buffer + thread_local_map_offset);
                new (m_thread_local_map) threading::thread_local_map();
            }
            return (m_thread_local_map);
        }

        threading::thread_pool* get_frame_thread_pool()
        {
            if (!m_frame_thread_pool) {
                ELECTROSLAG_CHECK(!m_destruction);
                m_frame_thread_pool = reinterpret_cast<threading::thread_pool*>(m_system_buffer + frame_thread_pool_offset);
                new (m_frame_thread_pool) threading::thread_pool("frame");
            }
            return (m_frame_thread_pool);
        }

        threading::thread_pool* get_io_thread_pool()
        {
            if (!m_io_thread_pool) {
                ELECTROSLAG_CHECK(!m_destruction);
                m_io_thread_pool = reinterpret_cast<threading::thread_pool*>(m_system_buffer + io_thread_pool_offset);
                new (m_io_thread_pool) threading::thread_pool("io");
            }
            return (m_io_thread_pool);
        }

        serialize::database* get_database()
        {
            if (!m_database) {
                ELECTROSLAG_CHECK(!m_destruction);
                m_database = reinterpret_cast<serialize::database*>(m_system_buffer + database_offset);
                new (m_database) serialize::database();
            }
            return (m_database);
        }

        ui::ui_win32* get_ui_win32()
        {
            if (!m_ui_win32) {
                ELECTROSLAG_CHECK(!m_destruction);
                m_ui_win32 = reinterpret_cast<ui::ui_win32*>(m_system_buffer + ui_win32_offset);
                new (m_ui_win32) ui::ui_win32();
            }
            return (m_ui_win32);
        }

        graphics::graphics_opengl* get_graphics_opengl()
        {
            if (!m_graphics_opengl) {
                ELECTROSLAG_CHECK(!m_destruction);
                m_graphics_opengl = reinterpret_cast<graphics::graphics_opengl*>(m_system_buffer + graphics_opengl_offset);
                new (m_graphics_opengl) graphics::graphics_opengl();
            }
            return (m_graphics_opengl);
        }

        renderer::renderer* get_renderer()
        {
            if (!m_renderer) {
                ELECTROSLAG_CHECK(!m_destruction);
                m_renderer = reinterpret_cast<renderer::renderer*>(m_system_buffer + renderer_offset);
                new (m_renderer) renderer::renderer();
            }
            return (m_renderer);
        }

        animation::property_manager* get_property_manager()
        {
            if (!m_property_manager) {
                ELECTROSLAG_CHECK(!m_destruction);
                m_property_manager = reinterpret_cast<animation::property_manager*>(m_system_buffer + property_manager_offset);
                new (m_property_manager) animation::property_manager();
            }
            return (m_property_manager);
        }

    private:
        static unsigned int const name_table_offset        = 0;
        static unsigned int const logger_offset            = align_up(name_table_offset + sizeof(name_table), alignof(logger));
        static unsigned int const thread_local_map_offset  = align_up(logger_offset + sizeof(logger), alignof(threading::thread_local_map));
        static unsigned int const frame_thread_pool_offset = align_up(thread_local_map_offset + sizeof(threading::thread_local_map), alignof(threading::thread_pool));
        static unsigned int const io_thread_pool_offset    = align_up(frame_thread_pool_offset + sizeof(threading::thread_pool), alignof(threading::thread_pool));
        static unsigned int const database_offset          = align_up(io_thread_pool_offset + sizeof(threading::thread_pool), alignof(serialize::database));
        static unsigned int const ui_win32_offset          = align_up(database_offset + sizeof(serialize::database), alignof(ui::ui_win32));
        static unsigned int const graphics_opengl_offset   = align_up(ui_win32_offset + sizeof(ui::ui_win32), alignof(graphics::graphics_opengl));
        static unsigned int const property_manager_offset  = align_up(graphics_opengl_offset + sizeof(graphics::graphics_opengl), alignof(animation::property_manager));
        static unsigned int const renderer_offset          = align_up(property_manager_offset + sizeof(animation::property_manager), alignof(renderer::renderer));

        static unsigned int const system_buffer_size = renderer_offset + sizeof(renderer::renderer);

        name_table* m_name_table;
        logger* m_logger;
        threading::thread_local_map* m_thread_local_map;
        threading::thread_pool* m_frame_thread_pool;
        threading::thread_pool* m_io_thread_pool;
        serialize::database* m_database;
        ui::ui_win32* m_ui_win32;
        graphics::graphics_opengl* m_graphics_opengl;
        animation::property_manager* m_property_manager;
        renderer::renderer* m_renderer;

        alignas(alignof(name_table)) byte m_system_buffer[system_buffer_size];
        bool m_destruction;
    };

    systems* get_systems();
}
