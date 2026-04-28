#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>

struct gemstone_server {
    struct wl_display *wl_display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;

    struct wl_listener new_output; // monitors
};

// monitor connected event
void server_new_output(struct wl_listener *listener, void *data);

#endif