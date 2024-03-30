/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 17:39:07
 */

#pragma once

#include <Windows.h>

#include <string>
#include <functional>

class Window
{
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    void Update();
    bool IsOpen();
    void Close();
    void OnResize(std::function<void(uint32_t, uint32_t)> function) { _resize = function; }

    void GetSize(uint32_t& width, uint32_t &height);

    HWND GetHandle() { return _hwnd; }
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    std::function<void(uint32_t, uint32_t)> _resize;
    HWND _hwnd;
    bool _open;
};
