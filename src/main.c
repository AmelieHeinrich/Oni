/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-27 13:36:21
 */

#include "core/log.h"
#include "core/window.h"

int main(void)
{
    window_t window;

    log_init();
    window_open(&window, 1280, 720, "Oni | <VULKAN> | <WIN32> | <1.0>");
    while (!window_should_close(&window)) {
        event_t event;
        window_poll_event(&window, &event);
    }
    window_destroy(&window);
    log_exit();
    return 0;
}
