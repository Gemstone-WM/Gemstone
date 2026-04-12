#pragma once

#define WLR_USE_UNSTABLE
#define _POSIX_C_SOURCE 200112L

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_presentation_time.h>

struct Server {
    struct wl_display         *display;
    struct wlr_backend        *backend;
    struct wlr_renderer       *renderer;
    struct wlr_allocator      *allocator;
    struct wlr_scene          *scene;
    struct wlr_cursor         *cursor;
    struct wlr_xcursor_manager *cursor_mgr;
    struct wl_listener         new_output;
    struct wl_listener         new_input;
    struct wl_listener cursor_motion;
    struct wlr_output_layout *output_layout;
    struct wlr_scene_tree *cursor_scene;
};

void server_init(struct Server *server);
void server_run(struct Server *server);
void server_destroy(struct Server *server);