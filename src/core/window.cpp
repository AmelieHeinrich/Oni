/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 17:43:08
 */

#include "window.hpp"

#include "log.hpp"

Window::Window(int width, int height, const std::string& title)
{
    WNDCLASSA windowClass = {};
    windowClass.lpszClassName = "OniWindowClass";
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = GetModuleHandle(nullptr);
    RegisterClassA(&windowClass);

    _hwnd = CreateWindowA(windowClass.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, windowClass.hInstance, nullptr);
    if (!_hwnd) {
        Logger::Error("Failed to create window!");
        return;
    }

    SetWindowLongPtr(_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    _open = true;
    ShowWindow(_hwnd, SW_SHOW);

    Logger::Info("Created window '%s' with dimensions (%d, %d)", title.c_str(), width, height);
}

Window::~Window()
{
    DestroyWindow(_hwnd);
}

void Window::Update()
{
    MSG msg;
    while (PeekMessage(&msg, _hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool Window::IsOpen()
{
    return _open;
}

void Window::Close()
{
    CloseWindow(_hwnd);
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    Window *window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CLOSE: {
            window->_open = false;
            break;
        }
        default: {
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
    }

    return 0;
}
