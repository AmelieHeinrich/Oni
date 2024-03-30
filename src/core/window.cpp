/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 17:43:08
 */

#include "window.hpp"

#include "log.hpp"

#include <ImGui/imgui_impl_win32.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
    if (_hwnd) {
        DestroyWindow(_hwnd);
    }
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
    _open = false;
    DestroyWindow(_hwnd);
}

void Window::GetSize(uint32_t& width, uint32_t& height)
{
    RECT ClientRect;
    GetClientRect(_hwnd, &ClientRect);
    width = ClientRect.right - ClientRect.left;
    height = ClientRect.bottom - ClientRect.top;
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    Window *window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return 1;

    switch (msg) {
        case WM_CLOSE: {
            window->_open = false;
            break;
        }
        case WM_SIZE: {
            int width = LOWORD(lparam);
            int height = HIWORD(lparam);
            if (window->_resize) {
                window->_resize(width, height);
            }
        }
        default: {
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
    }

    return 0;
}
