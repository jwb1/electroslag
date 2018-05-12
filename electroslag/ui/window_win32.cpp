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
#include "electroslag/resource_id.hpp"
#include "electroslag/threading/this_thread.hpp"
#include "electroslag/ui/ui_win32.hpp"
#include "electroslag/ui/window_win32.hpp"

namespace electroslag {
    namespace ui {
        // static
        char const* const window_win32::window_class_name = "electroslag_window";
        // static
        char const* const window_win32::default_title = "electroslag";

        // static
        window_dimensions const window_win32::default_dimensions(CW_USEDEFAULT, CW_USEDEFAULT, 800, 600);

        window_win32::window_win32()
            : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:win32_window"))
            , m_state(window_state_unknown)
            , m_hwnd(0)
        {
            memset(&m_window_placement, 0, sizeof(m_window_placement));
        }

        void window_win32::initialize(
            window_initialize_params const* params
            )
        {
            if (!params) {
                throw parameter_failure("params");
            }

            threading::lock_guard window_lock(&m_mutex);

            // Save the initialization parameters.
            m_state = window_state_unknown;
            m_dimensions = params->dimensions;
            if (!params->title.empty()) {
                m_title = params->title;
            }
            else {
                m_title = default_title;
            }

            // Register the window class.
            HINSTANCE hinstance = ui::get_ui_win32()->get_hinstance();
            WNDCLASSEX wcex = { 0 };
            wcex.cbSize = sizeof(wcex);
            wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            wcex.lpfnWndProc = window_proc;
            wcex.hInstance = hinstance;
            wcex.lpszClassName = window_class_name;
            wcex.hCursor = LoadCursor(0, IDC_ARROW);

            // Try to load the standard size icon. OK if this fails, NULL corresponds to
            // a stock icon.
            wcex.hIcon = static_cast<HICON>(LoadImage(
                hinstance,
                MAKEINTRESOURCE(IDI_APP_WINDOW),
                IMAGE_ICON,
                GetSystemMetrics(SM_CXICON),
                GetSystemMetrics(SM_CYICON),
                LR_DEFAULTCOLOR
                ));

            // Load the small sized icon. Again, OK if this is set to NULL: the
            // standard size icon will be resized.
            wcex.hIconSm = static_cast<HICON>(LoadImage(
                hinstance,
                MAKEINTRESOURCE(IDI_APP_WINDOW),
                IMAGE_ICON,
                GetSystemMetrics(SM_CXSMICON),
                GetSystemMetrics(SM_CYSMICON),
                LR_DEFAULTCOLOR
                ));

            if (RegisterClassEx(&wcex)) {
                m_flags.registered_class = true;
            }
            else {
                throw win32_api_failure("RegisterClassEx");
            }

            // Validate the window dimensions. Use default dimensions if something fails.
            if (m_dimensions.width <= 0 || m_dimensions.height <= 0) {
                m_dimensions = default_dimensions;
            }

            // Get the window rectangle that expresses our desired client area plus the appropriate
            // non-client area.
            RECT window_rect = dimensions_to_window_rect(&m_dimensions);

            // Create the window, initially in the window_state_normal state.
            m_hwnd = CreateWindowEx(
                0, // extended style
                window_class_name,
                m_title.c_str(),
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                window_rect.left,
                window_rect.top,
                window_rect.right - window_rect.left,
                window_rect.bottom - window_rect.top,
                0, // parent handle
                0, // menu handle
                hinstance,
                0  // create lparam: could be 'this'
                );
            if (!m_hwnd) {
                throw win32_api_failure("CreateWindowEx");
            }

            // Store our 'this' pointer in extra window memory so we can get back from window_proc.
            SetLastError(0);
            if (!SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this)) &&
                GetLastError()) {
                throw win32_api_failure("SetWindowLongPtr");
            }

            set_state(params->state);
            // A resulting windows message will cause the internal pause state to go to false.
            // (Which is not paused...)
            if (params->state != window_state_minimized) {
                if (!SetFocus(m_hwnd)) {
                    throw win32_api_failure("SetFocus");
                }
            }
        }

        void window_win32::shutdown()
        {
            threading::lock_guard window_lock(&m_mutex);

            // Destroy the window if necessary.
            if (m_hwnd) {
                DestroyWindow(m_hwnd);
            }

            // Unregister the window class, and free any associated resources.
            if (m_flags.registered_class) {
                HINSTANCE hinstance = ui::get_ui_win32()->get_hinstance();

                WNDCLASSEX wcex = { 0 };
                wcex.cbSize = sizeof(wcex);
                if (GetClassInfoEx(hinstance, window_class_name, &wcex)) {
                    if (wcex.hIcon) {
                        DestroyIcon(wcex.hIcon);
                    }

                    if (wcex.hIconSm) {
                        DestroyIcon(wcex.hIconSm);
                    }
                }

                UnregisterClass(window_class_name, hinstance);
                m_flags.registered_class = false;
            }
        }

        int window_win32::run()
        {
            int return_value = 0;
            bool run_loop = true;
            bool paused = get_overall_paused();

            while (run_loop) {
                if (!paused) {
                    return_value = run_not_paused(&run_loop, &paused);
                }
                else {
                    return_value = run_paused(&run_loop, &paused);
                }
            }

            return (return_value);
        }

        int window_win32::run_paused(
            bool* out_run_loop,
            bool* out_paused
            )
        {
            MSG msg = { 0 };
            bool run_loop = *out_run_loop;
            bool paused = *out_paused;

            while (run_loop && paused) {
                int get_msg_return = GetMessage(&msg, 0, 0, 0);
                if (get_msg_return != -1) {
                    if (msg.message != WM_QUIT) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);

                        if (msg.message == WM_ELECTROSLAG_PAUSED_CHANGE) {
                            paused = paused_change_lparam(msg.lParam).flags.new_overall_paused;
                        }
                    }
                    else {
                        run_loop = false;
                    }

                    // Windows silently swallows C++ exceptions when dispatching
                    // messages, so we need to check here to propagate.
                    if (m_exception) {
                        std::rethrow_exception(m_exception);
                    }
                }
                else {
                    throw win32_api_failure("GetMessage");
                }
            }

            *out_run_loop = run_loop;
            *out_paused = paused;
            return (static_cast<int>(msg.wParam));
        }

        int window_win32::run_not_paused(
            bool* out_run_loop,
            bool* out_paused
            )
        {
            timer_interface* timer = ui::get_ui()->get_timer();
            MSG msg = { 0 };

            unsigned int frame_time = 0;
            unsigned int prev_frame_time = timer->read_milliseconds();

            int sleep_time = 0;

            bool run_loop = *out_run_loop;
            bool paused = *out_paused;

            bool do_frame = true;
            bool do_sleep = true;

            while (run_loop && !paused) {
                {
                    if (do_frame) {
                        frame_time = timer->read_milliseconds();
                        handle_frame(frame_time - prev_frame_time);
                        prev_frame_time = frame_time;

                        do_frame = false;
                    }

                    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);

                        // Windows silently swallows C++ exceptions when dispatching
                        // messages, so we need to check here to propagate.
                        if (m_exception) {
                            std::rethrow_exception(m_exception);
                        }

                        if (msg.message == WM_ELECTROSLAG_PAUSED_CHANGE) {
                            paused = paused_change_lparam(msg.lParam).flags.new_overall_paused;
                        }
                        else if (msg.message == WM_QUIT) {
                            run_loop = false;
                            break;
                        }
                    }
                }

                do_sleep = (run_loop && !paused);
                if (do_sleep) {
                    // Figure out how much time to sleep off, if any
                    sleep_time = total_frame_time - static_cast<int>(timer->read_milliseconds() - frame_time);
                    if (sleep_time <= pad_frame_time) {
                        do_sleep = false;
                        do_frame = true;
                    }
                }

                if (do_sleep) {
                    switch (MsgWaitForMultipleObjects(
                        0, 0, // No kernel object array.
                        FALSE,
                        sleep_time,
                        QS_ALLEVENTS
                        )) {
                    case WAIT_OBJECT_0:
                        break;

                    case WAIT_TIMEOUT:
                        do_frame = true;
                        break;

                    default:
                    case WAIT_FAILED:
                        throw win32_api_failure("MsgWaitForMultipleObjects");
                    }
                }
            }

            *out_run_loop = run_loop;
            *out_paused = paused;
            return (static_cast<int>(msg.wParam));
        }

        window_state window_win32::get_state() const
        {
            threading::lock_guard window_lock(&m_mutex);
            return (m_state);
        }

        window_dimensions const* window_win32::get_dimensions() const
        {
            threading::lock_guard window_lock(&m_mutex);
            return (&m_dimensions);
        }

        std::string window_win32::get_title() const
        {
            threading::lock_guard window_lock(&m_mutex);
            return (m_title);
        }

        bool window_win32::get_paused() const
        {
            threading::lock_guard window_lock(&m_mutex);
            return (m_flags.paused);
        }

        bool window_win32::get_internal_paused() const
        {
            threading::lock_guard window_lock(&m_mutex);
            return (m_flags.internal_paused);
        }

        bool window_win32::get_overall_paused() const
        {
            threading::lock_guard window_lock(&m_mutex);
            return (m_flags.paused || m_flags.internal_paused);
        }

        int window_win32::get_frame_time() const
        {
            // Currently a constant. Mutex if it ever becomes a variable.
            return (total_frame_time);
        }

        void window_win32::set_state(
            window_state new_state
            )
        {
            if (new_state <= window_state_unknown || new_state > window_state_full_screen) {
                throw parameter_failure("new_state");
            }

            threading::lock_guard window_lock(&m_mutex);
            ELECTROSLAG_CHECK(m_hwnd);

            // Each of these cases result in a WM_SIZE message being sent to the handler, which will
            // in turn call handle_state_change.
            if (new_state != m_state) {
                if (m_state == window_state_full_screen) {
                    set_state_exit_full_screen(new_state);
                }
                else if (new_state == window_state_full_screen) {
                    set_state_enter_full_screen();
                }
                else {
                    int cmd_show = state_to_cmd_show(new_state);
                    ShowWindow(m_hwnd, cmd_show);
                }
            }
        }

        void window_win32::set_dimensions(
            window_dimensions const* new_dimensions
            )
        {
            if (!new_dimensions) {
                throw parameter_failure("new_dimensions");
            }

            threading::lock_guard window_lock(&m_mutex);
            ELECTROSLAG_CHECK(m_hwnd);

            if (*new_dimensions != m_dimensions) {
                RECT window_rect = dimensions_to_window_rect(new_dimensions);

                // window_rect is in "virtual" screen coordinates. WINDOWPLACEMENT calls for workspace
                // coordinates and it's not clear of they need to be converted.

                if (m_state != window_state_full_screen) {
                    WINDOWPLACEMENT window_placement = { 0 };
                    window_placement.length = sizeof(window_placement);
                    if (!GetWindowPlacement(m_hwnd, &window_placement)) {
                        throw win32_api_failure("GetWindowPlacement");
                    }

                    window_placement.rcNormalPosition = window_rect;

                    if (!SetWindowPlacement(m_hwnd, &window_placement)) {
                        throw win32_api_failure("SetWindowPlacement");
                    }
                }
                else {
                    // Placement is cached while we are full screen.
                    m_window_placement.rcNormalPosition = window_rect;
                }
            }
        }

        void window_win32::set_title(
            std::string const& new_title
            )
        {
            if (new_title.length() == 0) {
                throw parameter_failure("new_title");
            }

            threading::lock_guard window_lock(&m_mutex);
            ELECTROSLAG_CHECK(m_hwnd);

            if (new_title != m_title) {
                if (!SetWindowText(m_hwnd, new_title.c_str())) {
                    throw win32_api_failure("SetWindowText");
                }
            }
        }

        void window_win32::set_paused(
            bool new_paused
            )
        {
            threading::lock_guard window_lock(&m_mutex);
            ELECTROSLAG_CHECK(m_hwnd);

            if (new_paused != m_flags.paused) {
                handle_paused(new_paused, m_flags.internal_paused);

                paused_change_lparam lparam;
                lparam.flags.paused_changed = true;
                lparam.flags.new_paused = new_paused;
                lparam.flags.new_overall_paused = get_overall_paused();

                if (!PostMessage(m_hwnd, WM_ELECTROSLAG_PAUSED_CHANGE, 0, lparam.lparam)) {
                    throw win32_api_failure("PostMessage");
                }
            }
        }

        void window_win32::set_internal_paused(
            bool new_internal_paused
            )
        {
            threading::lock_guard window_lock(&m_mutex);
            ELECTROSLAG_CHECK(m_hwnd);

            if (new_internal_paused != m_flags.internal_paused) {
                handle_paused(m_flags.paused, new_internal_paused);

                paused_change_lparam lparam;
                lparam.flags.internal_paused_changed = true;
                lparam.flags.new_internal_paused = new_internal_paused;
                lparam.flags.new_overall_paused = get_overall_paused();

                if (!PostMessage(m_hwnd, WM_ELECTROSLAG_PAUSED_CHANGE, 0, lparam.lparam)) {
                    throw win32_api_failure("PostMessage");
                }
            }
        }

        HWND window_win32::get_hwnd() const
        {
            return (m_hwnd);
        }

        // static
        LRESULT CALLBACK window_win32::window_proc(
            HWND hwnd,
            unsigned int message,
            WPARAM wparam,
            LPARAM lparam
            )
        {
            window_win32* this_window = reinterpret_cast<window_win32*>(GetWindowLongPtr(
                hwnd,
                GWLP_USERDATA
                ));
            if (this_window) {
                return (this_window->handle_message(message, wparam, lparam));
            }
            else {
                return (DefWindowProc(hwnd, message, wparam, lparam));
            }
        }

        LRESULT window_win32::handle_message(
            unsigned int message,
            WPARAM wparam,
            LPARAM lparam
            )
        {
            LRESULT return_value = 1;
            bool found_handler = false;

            try {
                found_handler = get_ui_win32()->get_win32_input()->handle_message(message, wparam, lparam, return_value);

                if (!found_handler) {
                    switch (message) {
                    case WM_DESTROY:
                        return_value = on_msg_destroy(wparam, lparam);
                        found_handler = true;
                        break;

                    case WM_SIZE:
                        return_value = on_msg_size(wparam, lparam);
                        found_handler = true;
                        break;

                    case WM_MOVE:
                        return_value = on_msg_move(wparam, lparam);
                        found_handler = true;
                        break;

                    case WM_SETTEXT:
                        return_value = on_msg_settext(wparam, lparam);
                        found_handler = true;
                        break;

                    case WM_ACTIVATE:
                        return_value = on_msg_activate(wparam, lparam);
                        found_handler = true;
                        break;

                    case WM_ENTERSIZEMOVE:
                        return_value = on_msg_enter_size_move(wparam, lparam);
                        found_handler = true;
                        break;

                    case WM_EXITSIZEMOVE:
                        return_value = on_msg_exit_size_move(wparam, lparam);
                        found_handler = true;
                        break;

                    case WM_PAINT:
                        return_value = on_msg_paint(wparam, lparam);
                        found_handler = true;
                        break;

                    case WM_ERASEBKGND:
                        return_value = on_msg_erase_background(wparam, lparam);
                        found_handler = true;
                        break;
                    }
                }
            }
            catch (...) {
                m_exception = std::current_exception();
                return_value = 1;
            }

            if (found_handler) {
                return (return_value);
            }
            else {
                return (DefWindowProc(m_hwnd, message, wparam, lparam));
            }
        }

        LRESULT window_win32::on_msg_destroy(WPARAM, LPARAM)
        {
            threading::lock_guard window_lock(&m_mutex);
            destroyed.signal();

            PostQuitMessage(EXIT_SUCCESS);
            m_hwnd = 0;
            return (0);
        }

        LRESULT window_win32::on_msg_size(WPARAM wparam, LPARAM lparam)
        {
            threading::lock_guard window_lock(&m_mutex);

            unsigned int size_flag = static_cast<unsigned int>(wparam);
            int new_width = LOWORD(lparam);
            int new_height = HIWORD(lparam);

            if (m_flags.exiting_full_screen || m_flags.entering_full_screen) {
                // Entering or exiting full screen might result in several WM_SIZE messages, so rely
                // on the full screen code to signal handlers.
                m_full_screen_change_dimensions.width = new_width;
                m_full_screen_change_dimensions.height = new_height;
            }
            else if (m_flags.restore_full_screen) {
                // Ugly case: restoring to full screen requires us to re-set all of the full screen
                // parameters.
                ELECTROSLAG_CHECK(size_flag != SIZE_MINIMIZED);
                set_state(window_state_full_screen);
                m_flags.restore_full_screen = false;
            }
            else {
                window_state new_state = window_state_unknown;

                switch (size_flag) {
                case SIZE_RESTORED:
                    new_state = window_state_normal;
                    break;
                case SIZE_MINIMIZED:
                    new_state = window_state_minimized;
                    break;
                case SIZE_MAXIMIZED:
                    new_state = window_state_maximized;
                    break;
                }

                if (new_state != window_state_unknown && m_state != new_state) {
                    handle_state_change(new_state);

                    if (new_state == window_state_minimized) {
                        // Minimize doesn't send a WM_MOVE, but we do want the position to be (0,0)
                        // so set that manually.
                        handle_reposition(0, 0);
                    }
                }

                if (new_width != m_dimensions.width || new_height != m_dimensions.height) {
                    handle_resize(new_width, new_height);
                }
            }

            return (0);
        }

        LRESULT window_win32::on_msg_move(WPARAM, LPARAM lparam)
        {
            threading::lock_guard window_lock(&m_mutex);

            int new_x = static_cast<short>(lparam & 0x0000FFFF);
            int new_y = static_cast<short>((lparam & 0xFFFF0000) >> 16);

            // The (-32000,-32000) move coordinates appears to be a special move message that
            // sometimes proceeds minimization. It's unreliable, so rather we'll skip the position
            // change here and do it in the size message handler.
            if ((new_x != m_dimensions.x || new_y != m_dimensions.y) &&
                (new_x != -32000 && new_y != -32000)) {
                // Full screen transitions generate sometimes multiple WM_MOVE messages. So rely on the
                // special case code to signal the reposition.
                if (m_flags.entering_full_screen || m_flags.exiting_full_screen) {
                    m_full_screen_change_dimensions.x = new_x;
                    m_full_screen_change_dimensions.y = new_y;
                }
                else if (!m_flags.restore_full_screen) {
                    handle_reposition(new_x, new_y);
                }
            }

            return (0);
        }

        LRESULT window_win32::on_msg_settext(WPARAM wparam, LPARAM lparam)
        {
            threading::lock_guard window_lock(&m_mutex);

            char* new_title = reinterpret_cast<char*>(lparam);
            m_title = new_title;
            return (DefWindowProc(m_hwnd, WM_SETTEXT, wparam, lparam));
        }

        LRESULT window_win32::on_msg_activate(WPARAM wparam, LPARAM)
        {
            set_internal_paused((LOWORD(wparam) == WA_INACTIVE));
            return (0);
        }

        LRESULT window_win32::on_msg_enter_size_move(WPARAM, LPARAM)
        {
            // Make sure we don't try to push frames while the window is being resized or moved.
            set_internal_paused(true);
            return (0);
        }

        LRESULT window_win32::on_msg_exit_size_move(WPARAM, LPARAM)
        {
            set_internal_paused(false);
            return (0);
        }

        LRESULT window_win32::on_msg_paint(WPARAM, LPARAM)
        {
            ELECTROSLAG_CHECK(m_hwnd);

            // Clear the WM_PAINT message by using BeginPaint/EndPaint.
            PAINTSTRUCT ps = { 0 };
            BeginPaint(m_hwnd, &ps);
            EndPaint(m_hwnd, &ps);

            // While we are paused, we need to draw the frames here.
            if (get_overall_paused()) {
                handle_frame(0); // Time does not elapse while paused, hence 0
            }

            return (0);
        }

        LRESULT window_win32::on_msg_erase_background(WPARAM, LPARAM)
        {
            // Nothing to do; but do not remove this method; the default implementation
            // of the message handler does unwanted painting.
            return (1);
        }

        void window_win32::handle_state_change(
            window_state new_state
            )
        {
            m_state = new_state;
            state_changed.signal(new_state);
        }

        void window_win32::handle_resize(
            int new_width,
            int new_height
            )
        {
            m_dimensions.width = new_width;
            m_dimensions.height = new_height;

            size_changed.signal(&m_dimensions);
        }

        void window_win32::handle_reposition(
            int new_x,
            int new_y
            )
        {
            m_dimensions.x = new_x;
            m_dimensions.y = new_y;

            position_changed.signal(&m_dimensions);
        }

        void window_win32::handle_paused(
            bool new_paused,
            bool new_internal_paused
            )
        {
            bool old_overall = get_overall_paused();

            m_flags.paused = new_paused;
            m_flags.internal_paused = new_internal_paused;

            bool new_overall = get_overall_paused();

            if (new_overall != old_overall) {
                paused_changed.signal(new_overall);
            }
        }

        void window_win32::handle_frame(
            int milliseconds_elapsed
            )
        {
            frame.signal(milliseconds_elapsed);
        }

        int window_win32::state_to_cmd_show(
            window_state state
            )
        {
            int cmd_show = 0;
            switch (state) {
            case window_state_normal:
            case window_state_full_screen:
                if (m_state == window_state_maximized || m_state == window_state_minimized) {
                    cmd_show = SW_RESTORE;
                }
                else {
                    cmd_show = SW_SHOWNORMAL;
                }
                break;
            case window_state_maximized:
                cmd_show = SW_MAXIMIZE;
                break;
            case window_state_minimized:
                cmd_show = SW_MINIMIZE;
                break;
            default:
                throw parameter_failure("state");
            }
            return (cmd_show);
        }

        RECT window_win32::dimensions_to_window_rect(
            window_dimensions const* dimensions
            )
        {
            RECT window_rect = { 0 };
            window_rect.left = dimensions->x;
            window_rect.top = dimensions->y;
            window_rect.right = dimensions->width + dimensions->x;
            window_rect.bottom = dimensions->height + dimensions->y;

            unsigned long window_style = GetWindowLong(m_hwnd, GWL_STYLE);
            unsigned long window_ex_style = GetWindowLong(m_hwnd, GWL_EXSTYLE);

            if (!AdjustWindowRectEx(
                &window_rect,
                window_style,
                FALSE, // No menu
                window_ex_style
                )) {
                throw win32_api_failure("AdjustWindowRectEx");
            }

            return (window_rect);
        }

        void window_win32::set_state_enter_full_screen()
        {
            // Any message processing can check this flag for special case handling
            m_flags.entering_full_screen = true;

            // Caching window placement saves dimensions and current minimization/maximization
            // state.
            m_window_placement.length = sizeof(m_window_placement);
            if (!GetWindowPlacement(m_hwnd, &m_window_placement)) {
                throw win32_api_failure("GetWindowPlacement");
            }

            // Turn off window borders and title bar
            unsigned long new_style = GetWindowLong(m_hwnd, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW;
            if (!SetWindowLong(m_hwnd, GWL_STYLE, new_style)) {
                throw win32_api_failure("SetWindowLongPtr");
            }

            // Get the details for the window's monitor.
            MONITORINFO monitor_info = { 0 };
            monitor_info.cbSize = sizeof(monitor_info);
            if (!GetMonitorInfo(
                MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY),
                &monitor_info
                )) {
                throw win32_api_failure("GetMonitorInfo");
            }

            // Reposition, bring to the top, and flush the border change
            if (!SetWindowPos(
                m_hwnd,
                HWND_TOPMOST,
                monitor_info.rcMonitor.left,
                monitor_info.rcMonitor.top,
                monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_NOOWNERZORDER
                )) {
                throw win32_api_failure("SetWindowPos");
            }

            m_flags.entering_full_screen = false;

            signal_full_screen_change(window_state_full_screen);
        }

        void window_win32::set_state_exit_full_screen(
            window_state new_state
            )
        {
            // Any message processing can check this flag for special case handling
            m_flags.exiting_full_screen = true;

            // Turn on window borders and title bar
            unsigned long new_style = GetWindowLong(m_hwnd, GWL_STYLE) | WS_OVERLAPPEDWINDOW;
            if (!SetWindowLong(m_hwnd, GWL_STYLE, new_style)) {
                throw win32_api_failure("SetWindowLong");
            }

            // Flush the border change and no longer on top
            if (!SetWindowPos(
                m_hwnd,
                HWND_NOTOPMOST,
                0, 0,
                0, 0,
                SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_FRAMECHANGED |
                SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE
                )) {
                throw win32_api_failure("SetWindowPos");
            }

            // Restore the cached positioning information; with updated show command from the
            // new state.
            m_window_placement.length = sizeof(m_window_placement);
            m_window_placement.showCmd = state_to_cmd_show(new_state);
            if (!SetWindowPlacement(m_hwnd, &m_window_placement)) {
                throw win32_api_failure("SetWindowPlacement");
            }

            m_flags.exiting_full_screen = false;

            // We need to be able to distinguish, at restore time, if we should go back to full
            // screen or not.
            if (new_state == window_state_minimized) {
                m_flags.restore_full_screen = true;
            }

            signal_full_screen_change(new_state);
        }

        void window_win32::signal_full_screen_change(
            window_state new_state
            )
        {
            handle_state_change(new_state);

            handle_reposition(
                m_full_screen_change_dimensions.x,
                m_full_screen_change_dimensions.y
                );

            handle_resize(
                m_full_screen_change_dimensions.width,
                m_full_screen_change_dimensions.height
                );
        }
    }
}
