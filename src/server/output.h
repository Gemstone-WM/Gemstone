#pragma once
#include "server.h"
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>

struct Output {
    struct wlr_output       *wlr_output;
    struct wlr_scene_output *scene_output;
    struct wl_listener       frame;
};

void on_new_output(struct wl_listener *listener, void *data);