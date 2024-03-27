/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-27 18:06:37
 */

#ifndef WINDOW_H_
#define WINDOW_H_

#include "core.h"

typedef struct event_t {
    int placeholder;
} event_t;

typedef struct window_t {
    int width;
    int height;
    b8 should_close;
    void *handle;
} window_t;

void window_open(window_t *window, int width, int height, const char *title);
void window_close(window_t *window);
void window_destroy(window_t *window);
int window_poll_event(window_t *window, event_t *event);
b8 window_should_close(window_t *window);

// TODO(ahi): Window events

#endif
