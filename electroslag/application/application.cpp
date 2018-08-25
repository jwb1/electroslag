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
#include "electroslag/application/application.hpp"
#include "electroslag/logger.hpp"
#include "electroslag/threading/this_thread.hpp"
#include "electroslag/threading/thread_pool.hpp"
#include "electroslag/serialize/database.hpp"
#include "electroslag/ui/ui_interface.hpp"
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/renderer/renderer_interface.hpp"

namespace electroslag {
    namespace application {
        application::application(int argc, char** argv)
            : m_renderer_ready_changed(ELECTROSLAG_STRING_AND_HASH("cv:renderer_ready"))
            , m_init_mutex(ELECTROSLAG_STRING_AND_HASH("m:app_init_mutex"))
            , m_run_content(true)
#if !defined(ELECTROSLAG_BUILD_SHIP)
            , m_dump_content(false)
            , m_optimize_content(false)
#endif
            , m_renderer_ready(false)
        {
            // Initialization that has to occur after global constructors.
            logger* l = get_logger();
            l->initialize();

            threading::this_thread::get()->set_thread_name(ELECTROSLAG_STRING_AND_HASH("t:main"));

            // Get configuration data and apply it.
            parse_command_line_options(argc, argv);

            if (!m_log_file_path.empty()) {
                l->set_log_output_file(m_log_file_path);
                l->set_log_output(l->get_log_output() | log_output_bit_file);
            }

#if !defined(ELECTROSLAG_BUILD_SHIP)
            int current_log_enable = l->get_log_enable();
            if (m_dump_content) {
                current_log_enable |= log_enable_bit_message;
            }
            if (m_optimize_content) {
                current_log_enable |= log_enable_bit_serialize;
            }
            l->set_log_enable(current_log_enable);

            if (m_dump_content) {
                serialize::get_database()->dump_types();
            }

            // Loading screen content is loaded synchronously.
            m_loading_screen.load(m_dump_content);
#else 
            m_loading_screen.load();
#endif
            // Start the asynchronous load of everything else.
            async_load_content();

            // Start the graphics subsystem, displaying the load screen.
            if (m_run_content) {
                start_graphics();
                // Remaining initialization happens on content load.
            }
        }

        application::~application()
        {
            // It might be impossible to get here with the content load unfinished;
            // there is a similar wait in on_renderer_destroyed; so this is just for
            // safety.
            if (m_async_content_loader.is_valid()) {
                m_async_content_loader->wait_for_done();
            }

            m_loading_screen.shutdown();

            if (m_run_content) {
                // The renderer and graphics systems shutdown as part of the window being destroyed.
                ui::get_ui()->shutdown();
            }

            // Other shutdown tasks that can't be done in system destructors.
            threading::this_thread::cleanup_on_exit();
            get_logger()->shutdown();
        }

        int application::run()
        {
            if (m_run_content) {
                return (ui::get_ui()->get_window()->run());
            }
#if !defined(ELECTROSLAG_BUILD_SHIP)
            else if (m_optimize_content) {
                m_loading_screen.save("loading_screen.bin");

                // TODO: Default timeout is 30s. Might need to wait more than that here.
                serialize::load_record::ref content_load_record(m_async_content_loader->get_wait());
                serialize::get_database()->save_objects("content.bin", content_load_record);
            }
#endif

            return (EXIT_SUCCESS);
        }

        // static
        std::string application::parse_option_value(std::string const& option, int option_offset, int& a, int argc, char** argv)
        {
            // Extract the value string from option[= ]value on the command line; Supports:
            // "option=value" in a single arg
            // "option value" in a single arg
            // "option" "value" in successive args

            std::string value;

            if ((option.length() == option_offset) && (a < argc - 1)) {
                a++;
                value = argv[a];
            }
            else if (option.length() > option_offset + 1) {
                if (option[option_offset] == '=') {
                    value = option.substr(option_offset + 1);
                }
                else if (iswspace(option[option_offset])) {
                    do {
                        option_offset++;
                    } while ((option_offset < option.length()) && iswspace(option[option_offset]));

                    if (option_offset < option.length()) {
                        value = option.substr(option_offset);
                    }
                }
            }
            else {
                std::printf("Ignoring unknown or invalid option \"%s\".\n", option.c_str());
            }

            return (value);
        }

        void application::parse_command_line_options(int argc, char** argv)
        {
            // Parse command line arguments.
            for (int a = 1; a < argc; ++a) {
                std::string option(argv[a]);

                if (option.compare(0, 9, "--logfile") == 0) {
                    m_log_file_path = parse_option_value(option, 9, a, argc, argv);
                }
                else if (option.compare(0, 2, "-l") == 0) {
                    m_log_file_path = parse_option_value(option, 2, a, argc, argv);
                }
#if !defined(ELECTROSLAG_BUILD_SHIP)
                else if ((option.compare(0, 6, "--dump") == 0) || (option.compare(0, 2, "-d") == 0)) {
                    m_dump_content = true;
                    m_run_content = false;
                }
                else if ((option.compare(0, 5, "--opt") == 0) || (option.compare(0, 2, "-o") == 0)) {
                    m_optimize_content = true;
                    m_run_content = false;
                }
#endif
                else {
                    std::printf("Ignoring unknown or invalid option \"%s\".\n", option.c_str());
                }
            }
        }

        void application::async_load_content()
        {
            std::string const bin_path("content.bin");
#if !defined(ELECTROSLAG_BUILD_SHIP)
            std::string const json_path("..\\..\\..\\electroslag\\resources\\content\\content.json");
#endif

            bool dump_content = false;
            std::string content_file;
            if (std::filesystem::exists(bin_path)) {
                content_file = bin_path;
            }
#if !defined(ELECTROSLAG_BUILD_SHIP)
            else if (std::filesystem::exists(json_path)) {
                content_file = json_path;
            }

            dump_content = m_dump_content;
#endif

            m_async_content_loader = threading::get_io_thread_pool()->enqueue_work_item<async_content_loader>(
                this,
                content_file,
                dump_content
                );
        }

        application::async_content_loader::value_type application::async_content_loader::execute_for_value()
        {
            serialize::database* d = serialize::get_database();
            serialize::load_record::ref loaded_objects;

            if (m_file_name.length() > 4 && m_file_name.compare(m_file_name.length() - 4, 4, ".bin") == 0) {
                loaded_objects = d->load_objects(m_file_name);
            }
#if !defined(ELECTROSLAG_BUILD_SHIP)
            else if (m_file_name.length() > 5 && m_file_name.compare(m_file_name.length() - 5, 5, ".json") == 0) {
                loaded_objects = d->load_objects_json(m_file_name);
            }

            ELECTROSLAG_CHECK(loaded_objects.is_valid());

            // Support dumping content for debugging.
            if (m_dump_content) {
                d->dump_objects(loaded_objects);
            }
#endif

            if (m_app->m_run_content) {
                // Need the renderer to proceed.
                {
                    threading::lock_guard init_lock(&(m_app->m_init_mutex));
                    while (!m_app->m_renderer_ready) {
                        m_app->m_renderer_ready_changed.wait(&init_lock);
                    }
                }

                // Create an instance of the loaded scene.
                renderer::instance_descriptor::ref scene_desc(d->find_object_ref<renderer::instance_descriptor>(
                    hash_string("content::scene")
                    ));

                m_app->m_scene = renderer::get_renderer()->create_instances(scene_desc);

                // Switch the loading screen with the newly loaded scene.
                m_app->m_loading_screen.hide();
                m_app->m_scene->show();
                m_app->m_loading_screen.shutdown();
            }

            return (loaded_objects);
        }

        void application::start_graphics()
        {
            // Initialize the OS user-interface abstraction system.
            ui::window_initialize_params window_params;
            window_params.state = ui::window_state_normal;
            window_params.dimensions.x = 75;
            window_params.dimensions.y = 75;
            window_params.dimensions.width = 800;
            window_params.dimensions.height = 600;
            window_params.title = "electroslag";

            ui::get_ui()->initialize(&window_params);

            // Initialize the OS graphics API abstraction system.
            graphics::graphics_initialize_params graphics_params;
            graphics_params.swap_interval = 0;
            graphics_params.display_attribs.color_format = graphics::frame_buffer_color_format_r8g8b8a8_srgb;
            graphics_params.display_attribs.depth_stencil_format = graphics::frame_buffer_depth_stencil_format_d24s8;

            graphics::get_graphics()->initialize(&graphics_params);

            // Now we can put up the loading screen.
            m_loading_screen.show();

            // Initialize the renderer layer.
            renderer::get_renderer()->initialize();

            renderer::get_renderer()->destroyed.bind(
                renderer::renderer_interface::destroyed_delegate::create_from_method<application, &application::on_renderer_destroyed>(this),
                event_bind_mode_own_listener
                );

            {
                threading::lock_guard init_lock(&m_init_mutex);
                m_renderer_ready = true;
            }
            m_renderer_ready_changed.notify_all();
        }

        void application::on_renderer_destroyed()
        {
            // Destroying renderer objects needs to happen on this event.
            // It is signaled before the graphics layer is shut down.
            m_async_content_loader->wait_for_done();
            m_scene.reset();
        }
    }
}
