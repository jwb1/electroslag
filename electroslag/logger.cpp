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
#include "electroslag/threading/thread.hpp"
#include "electroslag/threading/this_thread.hpp"
#include "electroslag/ui/ui_interface.hpp"

namespace electroslag {
    logger* get_logger()
    {
        return (get_systems()->get_logger());
    }

    logger::logger()
        : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:logger"))
        , m_log_enable_bits(default_log_enable_bits)
        , m_log_output_bits(default_log_output_bits)
#if defined(_MSC_VER) && defined(ELECTROSLAG_BUILD_DEBUG)
        , m_report_hook_set(false)
#endif 
    {}

    void logger::initialize()
    {
#if defined(_MSC_VER) && defined(ELECTROSLAG_BUILD_DEBUG)
        threading::lock_guard logger_lock(&m_mutex);
        if (!m_report_hook_set) {
            if (_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, &crt_dbg_report_logger) >= 0) {
                m_report_hook_set = true;
            }
        }
#endif
    }

    void logger::shutdown()
    {
#if defined(_MSC_VER) && defined(ELECTROSLAG_BUILD_DEBUG)
        threading::lock_guard logger_lock(&m_mutex);
        if (m_report_hook_set) {
            _CrtSetReportHook2(_CRT_RPTHOOK_REMOVE, &crt_dbg_report_logger);
            m_report_hook_set = false;
        }
#endif
        fflush(stdout);
    }

    int logger::get_log_enable() const
    {
        threading::lock_guard logger_lock(&m_mutex);
        return (m_log_enable_bits);
    }

    void logger::set_log_enable(int enable_bits)
    {
        threading::lock_guard logger_lock(&m_mutex);
        if ((enable_bits & ~log_enable_bit_valid) == 0) {
            m_log_enable_bits = enable_bits;
        }
        else {
            throw parameter_failure("enable_bits");
        }
    }

    int logger::get_log_output() const
    {
        threading::lock_guard logger_lock(&m_mutex);
        return (m_log_output_bits);
    }

    void logger::set_log_output(int output_bits)
    {
        threading::lock_guard logger_lock(&m_mutex);
        if ((output_bits & ~log_output_bit_valid) == 0) {
            m_log_output_bits = output_bits;
        }
        else {
            throw parameter_failure("output_bits");
        }
    }

    void logger::set_log_output_file(std::string const& output_file)
    {
        threading::lock_guard logger_lock(&m_mutex);
        if (m_output_file.is_open()) {
            m_output_file.close();
        }

        if (!output_file.empty()) {
            m_output_file.create_new(output_file, file_stream_access_mode_write);
        }
        else {
            m_log_output_bits &= ~log_output_bit_file;
        }
    }

    void logger::log_print(
        char level_code,
        char const* format_string,
        va_list arg_list
        )
    {
        // Don't bother to format if there will be no output.
        if (!m_log_output_bits) {
            return;
        }

        // Timestamp
        unsigned int timestamp = ui::get_ui()->get_timer()->read_milliseconds();

        // Figure out how long the message is.
        int formatted_length = std::vsnprintf(0, 0, format_string, arg_list);
        if (formatted_length == -1) {
            throw std::runtime_error("failed to format log parameters.");
        }
        else if (!formatted_length) {
            return;
        }

        threading::thread* this_thread = threading::this_thread::get();
        std::string this_thread_name;
        if (this_thread->has_thread_name()) {
            this_thread_name = this_thread->get_thread_name();
        }
        else {
            this_thread_name = "n/a";
        }
        int thread_name_length = static_cast<int>(this_thread_name.length());

        // Allocate stack space for the output string
        int expected_prefix_length =
            2 + 1 +  // '[' + level_code + ']'
            2 + 10 + // '[' + timestamp + ']'
            2 + thread_name_length + // '[' + thread name + ']'
            1;       // space
        int output_length = 
            formatted_length +
            expected_prefix_length +
            2;      // +1 for the NUL and might need newline, so +1 again
        char* output_string = static_cast<char*>(alloca(output_length));
        output_string[0] = '\0';

        // Write in the prefix.
        int prefix_length = std::snprintf(output_string, output_length, "[%c][%10u][%-*s] ",
            level_code,
            timestamp,
            thread_name_length,
            this_thread_name.c_str()
            );
        if (prefix_length != expected_prefix_length) {
            throw std::runtime_error("failed to format log parameters.");
        }

        // Write the message.
        int return_length = std::vsnprintf(
            output_string + prefix_length,
            output_length - prefix_length,
            format_string,
            arg_list
            );
        if (return_length == -1) {
            throw std::runtime_error("failed to format log parameters.");
        }

        // Append a newline if one isn't there already, and always NUL terminate.
        int write_length = 0;
        if (output_string[output_length - 3] != '\n') {
            output_string[output_length - 2] = '\n';
            output_string[output_length - 1] = '\0';
            write_length = output_length - 1;
        }
        else {
            output_string[output_length - 2] = '\0';
            write_length = output_length - 2;
        }

        // Output
        if (m_log_output_bits & log_output_bit_console) {
            std::fputs(output_string, stdout);
        }

#if defined(_WIN32)
        if ((m_log_output_bits & log_output_bit_debugger) && being_debugged()) {
            OutputDebugString(output_string);
        }
#endif

        if (m_log_output_bits & log_output_bit_file) {
            m_output_file.write(output_string, write_length);
        }

        log_output.signal(
            level_code,
            timestamp,
            this_thread_name.c_str(),
            output_string + prefix_length
            );
    }

#if defined(_MSC_VER) && defined(ELECTROSLAG_BUILD_DEBUG)
    // static
    int logger::crt_dbg_report_logger(
        int report_type,
        char* message,
        int* return_value
        )
    {
        int debug_break = 0;
        logger* l = get_logger();
        switch (report_type) {
        case _CRT_WARN:
            l->log_warning("CrtDbg: %s", message);
            break;
        case _CRT_ERROR:
            l->log_error("CrtDbg: %s", message);
            break;
        case _CRT_ASSERT:
            l->log_error("CrtDbg: %s", message);
            debug_break = 1;
            break;
        }

        if (return_value) {
            *return_value = debug_break;
        }
        return (1); // No more reporting necessary
    }
#endif
}
