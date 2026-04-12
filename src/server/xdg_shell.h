#pragma once
#include "server.h"
#include <wlr/types/wlr_xdg_shell.h>

struct View {
    struct wl_list link;
    struct Server *server;
    struct wlr_xdg_toplevel *xdg_toplevel;
    struct wlr_scene_tree *scene_tree;

    // wlroots 0.20: map/unmap signals removed; we use surface_commit + initial_commit instead
    struct wl_listener surface_commit;
    struct wl_listener destroy;

    struct wl_listener request_move;
    struct wl_listener request_maximize;
    struct wl_listener request_minimize;
    struct wl_listener request_resize;
};

void init_xdg_shell(struct Server *server);
void focus_view(struct View *view, struct wlr_surface *surface);