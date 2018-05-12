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
#include "electroslag/event.hpp"
#include "electroslag/file_stream.hpp"
#include "electroslag/threading/mutex.hpp"

#if defined(ELECTROSLAG_BUILD_SHIP) | defined(RC_INVOKED)
#define ELECTROSLAG_LOG_ERROR(...)
#define ELECTROSLAG_LOG_WARN(...)
#define ELECTROSLAG_LOG_MESSAGE(...)
#define ELECTROSLAG_LOG_GFX(...)
#define ELECTROSLAG_LOG_SERIALIZE(...)
#else
#define ELECTROSLAG_LOG_ERROR(...) \
    ::electroslag::get_logger()->log_error(__VA_ARGS__)
#define ELECTROSLAG_LOG_WARN(...) \
    ::electroslag::get_logger()->log_warning(__VA_ARGS__)
#define ELECTROSLAG_LOG_MESSAGE(...) \
    ::electroslag::get_logger()->log_message(__VA_ARGS__)
#define ELECTROSLAG_LOG_GFX(...) \
    ::electroslag::get_logger()->log_gfx(__VA_ARGS__)
#define ELECTROSLAG_LOG_SERIALIZE(...) \
    ::electroslag::get_logger()->log_serialize(__VA_ARGS__)
#endif

namespace electroslag {
    enum log_enable_bit {
        log_enable_bit_invalid = -1,
        log_enable_bit_none = 0,
        log_enable_bit_error = 1,
        log_enable_bit_warn = 2,
        log_enable_bit_message = 4,
        log_enable_bit_graphics = 8,
        log_enable_bit_serialize = 16,

        log_enable_bit_end // Keep at the end of the enum
    };

    enum log_output_bit {
        log_output_bit_invalid = -1,
        log_output_bit_none = 0,
        log_output_bit_console = 1,
        log_output_bit_debugger = 2,
        log_output_bit_file = 4,

        log_output_bit_end // Keep at the end of the enum
    };

    class logger {
    public:
        logger();

        void initialize();
        void shutdown();

        virtual int get_log_enable() const;
        virtual void set_log_enable(int enable_bits);

        virtual int get_log_output() const;
        virtual void set_log_output(int output_bits);
        virtual void set_log_output_file(std::string const& output_file);

        virtual void log_error(char const* format_string, ...)
        {
            threading::lock_guard logger_lock(&m_mutex);
            if (m_log_enable_bits & log_enable_bit_error) {
                va_list arg_list;
                va_start(arg_list, format_string);
                log_print('E', format_string, arg_list);
                va_end(arg_list);
            }
        }

        virtual void log_warning(char const* format_string, ...)
        {
            threading::lock_guard logger_lock(&m_mutex);
            if (m_log_enable_bits & log_enable_bit_warn) {
                va_list arg_list;
                va_start(arg_list, format_string);
                log_print('W', format_string, arg_list);
                va_end(arg_list);
            }
        }

        virtual void log_message(char const* format_string, ...)
        {
            threading::lock_guard logger_lock(&m_mutex);
            if (m_log_enable_bits & log_enable_bit_message) {
                va_list arg_list;
                va_start(arg_list, format_string);
                log_print('M', format_string, arg_list);
                va_end(arg_list);
            }
        }

        virtual void log_gfx(char const* format_string, ...)
        {
            threading::lock_guard logger_lock(&m_mutex);
            if (m_log_enable_bits & log_enable_bit_graphics) {
                va_list arg_list;
                va_start(arg_list, format_string);
                log_print('G', format_string, arg_list);
                va_end(arg_list);
            }
        }

        virtual void log_serialize(char const* format_string, ...)
        {
            threading::lock_guard logger_lock(&m_mutex);
            if (m_log_enable_bits & log_enable_bit_serialize) {
                va_list arg_list;
                va_start(arg_list, format_string);
                log_print('S', format_string, arg_list);
                va_end(arg_list);
            }
        }

    private:
        static int const log_enable_bit_valid = (1 << (log_enable_bit_end - 1)) - 1;
        static int const log_output_bit_valid = (1 << (log_output_bit_end - 1)) - 1;

#if defined(ELECTROSLAG_BUILD_SHIP)
        static int const default_log_enable_bits = log_enable_bit_none;
        static int const default_log_output_bits = log_output_bit_none;
#else
        static int const default_log_enable_bits =
            log_enable_bit_error | log_enable_bit_warn;
        static int const default_log_output_bits =
            log_output_bit_console | log_output_bit_debugger;
#endif

        virtual void log_print(char level_code, char const* format_string, va_list arg_list);

        mutable threading::mutex m_mutex;
        file_stream m_output_file;

        int m_log_enable_bits;
        int m_log_output_bits;

#if defined(_MSC_VER) && defined(ELECTROSLAG_BUILD_DEBUG)
        bool m_report_hook_set;

        static int crt_dbg_report_logger(int report_type, char* message, int* return_value);
#endif

        // Disallowed operations:
        explicit logger(logger const&);
        logger& operator =(logger const&);

    public:
        // Ensure the event is constructed after the mutex by declaring it below in the class.
        typedef event<void, char, unsigned int, char const*, char const*> log_output_event;
        typedef log_output_event::bound_delegate log_output_delegate;
        mutable log_output_event log_output;
    };

    logger* get_logger();
}
