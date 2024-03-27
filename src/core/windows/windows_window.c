/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-27 18:09:02
 */

#include <core/window.h>
#include <core/log.h>

#include <Windows.h>

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    window_t *window = (window_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CLOSE: {
            window->should_close = true;            
            break;
        };
        default: {
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
    }

    return 0;
}

void window_open(window_t *window, int width, int height, const char *title)
{
    WNDCLASSA window_class = {0};
    window_class.hInstance = GetModuleHandle(NULL);
    window_class.lpfnWndProc = window_proc;
    window_class.hCursor = (HCURSOR)LoadCursor(window_class.hInstance, IDC_ARROW);
    window_class.hIcon = (HICON)LoadIcon(window_class.hInstance, IDI_WINLOGO);
    window_class.lpszClassName = "oni_window_class";
    RegisterClassA(&window_class);

    HWND hwnd = CreateWindowA(window_class.lpszClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, window_class.hInstance, NULL);
    if (!hwnd) {
        log_error("Failed to create window!");
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    ShowWindow(hwnd, SW_SHOW);

    window->width = width;
    window->height = height;
    window->handle = hwnd;
    window->should_close = false;
}

void window_close(window_t *window)
{
    DestroyWindow((HWND)window->handle);
}

void window_destroy(window_t *window)
{
    CloseWindow((HWND)window->handle);
}

int window_poll_event(window_t *window, event_t *event)
{
    MSG msg;
    while (PeekMessageA(&msg, (HWND)window->handle, 0, 0, PM_REMOVE)) {
        // TODO(ahi): Convert win32 event to oni event
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

b8 window_should_close(window_t *window)
{
    return window->should_close;
}
