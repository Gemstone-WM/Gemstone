#pragma once
#include "server.h"
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_input_device.h>

void on_new_input(struct wl_listener *listener, void *data);
void on_cursor_motion(struct wl_listener *listener, void *data);