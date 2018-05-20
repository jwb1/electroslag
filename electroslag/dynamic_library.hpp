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

namespace electroslag {
    class dynamic_library {
    public:
        dynamic_library()
            : m_is_open(false)
#if defined(_WIN32)
            , m_handle(0)
            , m_close_handle(false)
#endif
        {}

        ~dynamic_library()
        {
            if (is_open()) {
                close();
            }
        }

        bool open(std::string const& library_name)
        {
            ELECTROSLAG_CHECK(!is_open());

#if defined(_WIN32)
            std::string windows_library_name(library_name);
            windows_library_name.append(".dll");

            m_handle = LoadLibraryA(windows_library_name.c_str());
            if (!m_handle) {
                return (false);
            }

            m_close_handle = true;
#endif

            m_is_open = true;
            return (true);
        }

        bool load(std::string const& library_name)
        {
            ELECTROSLAG_CHECK(!is_open());

#if defined(_WIN32)
            std::string windows_library_name(library_name);
            windows_library_name.append(".dll");

            HANDLE process = GetCurrentProcess();
            int sizeof_module_array = 0;
            if (!EnumProcessModulesEx(
                process,
                0,
                0,
                reinterpret_cast<LPDWORD>(&sizeof_module_array),
                LIST_MODULES_ALL
                )) {
                throw win32_api_failure("EnumProcessModulesEx");
            }

            HMODULE* module_array = static_cast<HMODULE*>(alloca(sizeof_module_array));
            if (!EnumProcessModulesEx(
                process,
                module_array,
                sizeof_module_array,
                reinterpret_cast<LPDWORD>(&sizeof_module_array),
                LIST_MODULES_ALL
                )) {
                throw win32_api_failure("EnumProcessModulesEx");
            }

            m_handle = 0;
            int countof_module_array = sizeof_module_array / sizeof(HMODULE);
            char module_name[_MAX_FNAME];
            for (int i = 0; i < countof_module_array; ++i) {
                // Module names should be unique. Find the matching name, if any.
                module_name[0] = '\0';
                if (GetModuleBaseNameA(
                    process,
                    module_array[i],
                    module_name,
                    sizeof(module_name)
                    ) <= 0) {
                    throw win32_api_failure("GetModuleBaseNameA");
                }

                if (windows_library_name == module_name) {
                    m_handle = module_array[i];
                    break;
                }
            }

            if (!m_handle) {
                return (false);
            }

            m_close_handle = false;
#endif

            m_is_open = true;
            return (true);
        }

        void close()
        {
            m_is_open = false;

#if defined(_WIN32)
            if (m_handle && m_close_handle) {
                FreeLibrary(m_handle);
            }
            m_handle = 0;
            m_close_handle = false;
#endif
        }

        bool is_open() const
        {
            return (m_is_open);
        }

        void* find_entry_point(std::string const& entry_point_name)
        {
            ELECTROSLAG_CHECK(is_open());

#if defined(_WIN32)
            void* entry_point = reinterpret_cast<void*>(GetProcAddress(m_handle, entry_point_name.c_str()));
            if (!entry_point) {
                throw win32_api_failure("GetProcAddress");
            }
            return (entry_point);
#endif
        }

    protected:
#if defined(_WIN32)
        HMODULE m_handle;
        bool m_close_handle;
#endif

        bool m_is_open;

        // Disallowed operations:
        explicit dynamic_library(dynamic_library const&);
        dynamic_library& operator =(dynamic_library const&);
    };
}
