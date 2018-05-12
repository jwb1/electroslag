//  Electroslag Interactive Graphics System
//  Copyright 2015 Joshua Buckman
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

#if defined(ELECTROSLAG_BUILD_SHIP)
#error texture file importer not to be included in SHIP build!
#endif

#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/importer_interface.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/serialize/load_record.hpp"
#include "electroslag/graphics/graphics_types.hpp"
#include "electroslag/graphics/texture_descriptor.hpp"
#include "electroslag/threading/future_interface.hpp"

namespace electroslag {
    namespace texture {
        class gli_importer
            : public serialize::importer_interface
            , public serialize::serializable_object<gli_importer> {
        public:
            typedef reference<gli_importer> ref;

            static ref create(
                std::string const& file_name,
                std::string const& object_name,
                serialize::load_record::ref& record,
                graphics::sampler_params const* sampler,
                graphics::texture_level_generate generate_mip_levels = graphics::texture_level_generate_off
                )
            {
                return (ref(new gli_importer(file_name, object_name, record, sampler, generate_mip_levels)));
            }

            virtual ~gli_importer()
            {}

            // Implement serializable_object
            explicit gli_importer(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            // Implement importer_interface
            virtual void finish_importing();

            // gli_importer methods.
            typedef threading::future_interface<graphics::texture_descriptor::ref> import_future;

            import_future::ref const& get_future() const
            {
                return (m_async_loader);
            }

        private:
            class async_texture_loader : public import_future {
            public:
                typedef reference<async_texture_loader> ref;

                async_texture_loader(
                    gli_importer::ref const& importer,
                    std::string const& file_name,
                    std::string const& object_name,
                    serialize::load_record::ref& record,
                    graphics::sampler_params const* sampler,
                    graphics::texture_level_generate generate_mip_levels
                    )
                    : m_this_importer(importer)
                    , m_file_name(file_name)
                    , m_object_name(object_name)
                    , m_load_record(record)
                    , m_sampler(*sampler)
                    , m_generate_mip_levels(generate_mip_levels)
                {}
                virtual ~async_texture_loader()
                {}

                virtual import_future::value_type execute_for_value();

            private:
                gli::texture convert(gli::texture const& gli_texture, gli::format dest_format);

                gli_importer::ref m_this_importer;
                std::string m_file_name;
                std::string m_object_name;
                serialize::load_record::ref m_load_record;
                graphics::sampler_params m_sampler;
                graphics::texture_level_generate m_generate_mip_levels;
            };

            gli_importer(
                std::string const& file_name,
                std::string const& object_name,
                serialize::load_record::ref& record,
                graphics::sampler_params const* sampler,
                graphics::texture_level_generate generate_mip_levels
                );

            void import(
                std::string const& file_name,
                std::string const& object_name,
                serialize::load_record::ref& record,
                graphics::sampler_params const* sampler,
                graphics::texture_level_generate generate_mip_levels
                );

            import_future::ref m_async_loader;

            // Disallowed operations:
            gli_importer();
            explicit gli_importer(gli_importer const&);
            gli_importer& operator =(gli_importer const&);
        };
    }
}
