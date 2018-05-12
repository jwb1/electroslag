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
#include "electroslag/threading/future_interface.hpp"
#include "electroslag/threading/condition_variable.hpp"
#include "electroslag/threading/mutex.hpp"
#include "electroslag/serialize/load_record.hpp"
#include "electroslag/renderer/scene.hpp"
#include "electroslag/application/loading_screen.hpp"

namespace electroslag {
    namespace application {
        class application {
        public:
            application(int argc, char** argv);
            ~application();

            int run();

        private:
            static std::string parse_option_value(std::string const& option, int option_offset, int& a, int argc, char** argv);

            void parse_command_line_options(int argc, char** argv);

            void async_load_content();

            void start_graphics();

            void on_renderer_destroyed();

            // Initialization sequence
            threading::condition_variable m_renderer_ready_changed;
            threading::mutex m_init_mutex;

            // Loading screen
            loading_screen m_loading_screen;

            // Async content load
            typedef threading::future_interface<serialize::load_record::ref> content_future;

            class async_content_loader : public content_future {
            public:
                typedef reference<async_content_loader> ref;

                async_content_loader(
                    application* app,
                    std::string const& file_name,
                    bool dump_content
                    )
                    : m_app(app)
                    , m_file_name(file_name)
                    , m_dump_content(dump_content)
                {}
                virtual ~async_content_loader()
                {}

                virtual content_future::value_type execute_for_value();

            private:
                // Input parameters.
                application* m_app;
                std::string m_file_name;
                bool m_dump_content;
            };

            async_content_loader::ref m_async_content_loader;

            // Content
            renderer::scene::ref m_scene;

            // Command line arguments
            std::string m_log_file_path;
            bool m_run_content;
#if !defined(ELECTROSLAG_BUILD_SHIP)
            bool m_dump_content;
            bool m_optimize_content;
#endif
            bool m_renderer_ready;
        };
    }
}
