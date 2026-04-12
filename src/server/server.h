#pragma once

#define WLR_USE_UNSTABLE 

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>

enum CursorMode {
    CURSOR_PASSTHROUGH,
    CURSOR_MOVE,
    CURSOR_RESIZE,
};

struct View;

struct Server {
    struct wl_display         *display;
    struct wlr_backend        *backend;
    struct wlr_renderer       *renderer;
    struct wlr_allocator      *allocator;
    struct wlr_scene          *scene;

    struct wlr_cursor          *cursor;
    struct wlr_xcursor_manager *cursor_mgr;
    struct wl_listener          cursor_motion;
    struct wl_listener          cursor_motion_absolute;
    struct wl_listener          cursor_button;
    struct wl_listener          request_set_cursor;   // NEW: client cursor shape requests
    struct wlr_scene_tree      *cursor_scene;

    struct wl_listener          new_output;
    struct wl_listener          new_input;
    struct wlr_output_layout   *output_layout;

    struct wlr_xdg_shell       *xdg_shell;
    struct wl_listener          new_xdg_toplevel;
    struct wl_listener          new_xdg_popup;

    struct wlr_seat            *seat;

    struct wl_list              views;

    enum CursorMode             cursor_mode;
    struct View                *grabbed_view;
    double                      grab_x, grab_y;
    uint32_t                    resize_edges;
    struct wlr_box              grab_geobox;
};

void server_init(struct Server *server);
void server_run(struct Server *server);
void server_destroy(struct Server *server);