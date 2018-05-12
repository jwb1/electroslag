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
    namespace ui {
        ui_win32* get_ui_win32()
        {
            return (get_systems()->get_ui_win32());
        }

        ui_internal_interface* get_ui_internal()
        {
            return (get_ui_win32());
        }

        ui_interface* get_ui()
        {
            return (get_ui_win32());
        }

        ui_win32::ui_win32()
            : m_initialization_mutex(ELECTROSLAG_STRING_AND_HASH("m:ui_init"))
            , m_initialized(false)
            , m_hinstance(0)
        {}

        void ui_win32::initialize(
            window_initialize_params const* window_params
            )
        {
            if (!window_params) {
                throw parameter_failure("window_params");
            }

            // Only allow a single thread to initialize the library in case more than one tries
            // at once.
            threading::lock_guard library_initialization_lock(&m_initialization_mutex);

            if (m_initialized) {
                return;
            }

            check_operating_system();
            check_processor();
            platform_initialize();

            // Now, do per-subsystem initialization.
            m_window.initialize(window_params);
            m_timer.initialize(&m_window);

            m_initialized = true;
        }

        void ui_win32::shutdown()
        {
            threading::lock_guard library_initialization_lock(&m_initialization_mutex);
            if (m_initialized) {
                // Shutdown is done in two stages; explicit shutdown methods are called in reverse
                // dependency order to clean up anything that might have a dependency, then
                // destructors can run in any order.
                m_timer.shutdown();
                m_window.shutdown();
                m_initialized = false;
            }
        }

        bool ui_win32::is_initialized() const
        {
            threading::lock_guard library_initialization_lock(&m_initialization_mutex);
            return (m_initialized);
        }

        std::string ui_win32::get_program_path() const
        {
            return (std::string(_pgmptr));
        }

        // static
        void ui_win32::check_operating_system()
        {
            // On Desktop Windows, we currently need at least Windows Vista SP2
            ELECTROSLAG_STATIC_CHECK(
                WINVER == 0x0600,
                "check_operating_system() is checking for Windows Vista, but that's not the SDK version we are compiling with"
                );

            if (!IsWindowsVistaSP2OrGreater()) {
                throw win32_api_failure("Current OS version is unsupported");
            }
        }

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#if defined(_MSC_VER)
#define ELECTROSLAG_CPUID(cpu_info, level) \
    __cpuid(cpu_info, level)
#elif defined(__MINGW32__) || defined(__MINGW64__)
#define ELECTROSLAG_CPUID(cpu_info, level) \
    __get_cpuid( \
        level, \
        (unsigned int*)(&cpu_info[0]), \
        (unsigned int*)(&cpu_info[1]), \
        (unsigned int*)(&cpu_info[2]), \
        (unsigned int*)(&cpu_info[3]) \
        )
#endif
#endif

        // static
        void ui_win32::check_processor()
        {
            bool have_sse3 = false;
            bool have_avx = false;

#if defined(ELECTROSLAG_CPUID)
            int cpu_info[4];
            ELECTROSLAG_CPUID(cpu_info, 0);
            int max_cpuid_support = cpu_info[0];

            if (max_cpuid_support >= 1) {
                ELECTROSLAG_CPUID(cpu_info, 1);
                have_sse3 = (cpu_info[2] & (1 << 0)) != 0;
                have_avx = (cpu_info[2] & (1 << 28)) != 0;
            }
#elif defined(__GNUC__)
            have_sse3 = __builtin_cpu_supports("sse3");
            have_avx = __builtin_cpu_supports("avx");
#else
#error Need compiler support for CPU feature detection.
#endif

            // GLM is configured for AVX instruction intrinsics, so check for at least AVX.
            if (!have_avx) {
                throw std::runtime_error("AVX instructions not supported");
            }

            // Set the FPU to round denormalized numbers to zero.
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            if (have_sse3) {
                _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
            }
        }

#if defined(ELECTROSLAG_CPUID)
#undef ELECTROSLAG_CPUID
#endif

        typedef BOOL(WINAPI *pSetProcessUserModeExceptionPolicy)(DWORD dwFlags);
        typedef BOOL(WINAPI *pGetProcessUserModeExceptionPolicy)(DWORD* dwFlags);

        void ui_win32::platform_initialize()
        {
            m_hinstance = GetModuleHandle(0);

            std::srand(static_cast<unsigned int>(std::time(0)));

            // Clearing this bit ensures that exceptions throw from window procs are not immediatly swallowed
            // on certain version of Windows (32bit apps on x64 Windows 7, notably.)
            static DWORD const process_callback_filter_enabled = 0x1;
            HMODULE kernel32_handle = LoadLibrary("kernel32.dll");
            if (kernel32_handle) {
                pSetProcessUserModeExceptionPolicy set_policy = reinterpret_cast<pSetProcessUserModeExceptionPolicy>(GetProcAddress(
                    kernel32_handle,
                    "SetProcessUserModeExceptionPolicy"
                    ));
                pGetProcessUserModeExceptionPolicy get_policy = reinterpret_cast<pGetProcessUserModeExceptionPolicy>(GetProcAddress(
                    kernel32_handle,
                    "GetProcessUserModeExceptionPolicy"
                    ));
                if (set_policy && get_policy) {
                    DWORD exception_policy_flags = 0;
                    if (get_policy(&exception_policy_flags)) {
                        if (!set_policy(exception_policy_flags & ~process_callback_filter_enabled)) {
                            throw win32_api_failure("SetProcessUserModeExceptionPolicy");
                        }
                    }
                    else {
                        throw win32_api_failure("GetProcessUserModeExceptionPolicy");
                    }
                }

                FreeLibrary(kernel32_handle);
            }
            else {
                throw win32_api_failure("LoadLibrary");
            }
        }
    }
}
