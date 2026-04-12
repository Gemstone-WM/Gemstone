#pragma once
#include "server.h"
#include <wlr/types/wlr_keyboard.h>

void on_new_keyboard(struct wl_listener *listener, void *data);