#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include "types.h"

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>

struct gemstone_windows {
    struct wlr_xdg_surface *xdg_surface;
    struct Vector2 position;
    struct wl_list link; // linked list
};

struct gemstone_server {
    struct wl_display *wl_display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;

    struct wlr_xdg_shell *xdg_shell; // for windows and stuff
    struct wl_listener new_xdg_surface; // new window event

    struct wl_listener new_output; // monitors

    struct wl_list windows; // list of windows
};

// monitor connected event
void server_new_output(struct wl_listener *listener, void *data);
// new window event
void server_new_xdg_surface(struct wl_listener *listener, void *data);

#endif