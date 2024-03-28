/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 17:39:07
 */

#pragma once

#include <Windows.h>
#include <string>

class Window
{
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    void Update();
    bool IsOpen();
    void Close();

    HWND GetHandle() { return _hwnd; }
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    HWND _hwnd;
    bool _open;
};
