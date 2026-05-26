#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include "types.h"

#include <wayland-server-core.h>

#include <wlr/types/wlr_seat.h>

#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_scene.h>

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>

enum gemstone_cursor_mode {
    GEMSTONE_CURSOR_PASSTHROUGH,
    GEMSTONE_CURSOR_MOVE,
};

struct gemstone_keyboard {
    struct wl_list link;
    struct gemstone_server *server;
    struct wlr_keyboard *wlr_keyboard;
    struct wl_listener modifiers;
    struct wl_listener key;
};

struct gemstone_windows {
    struct wlr_xdg_toplevel *xdg_toplevel;
    struct wlr_scene_tree *scene_tree;
    struct Vector2 position;
    struct wl_list link;
    struct wl_listener commit; // waiting for window updates

    struct gemstone_server *server; // back reference to server
    struct wl_listener map; // waiting for window being visible
};
struct gemstone_output {
    struct wlr_output *wlr_output;
    struct gemstone_server *server;
    struct wl_listener frame; // monitor main loop
    struct wl_list link;
};
struct gemstone_server {
    struct wl_display *wl_display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;

    struct wlr_xdg_shell *xdg_shell; // for windows and stuff
    struct wl_listener new_xdg_toplevel; // new toplevel window event

    struct wl_listener new_output; // monitors

    struct wl_list windows; // list of windows

    struct wlr_scene *scene;
    struct wl_list outputs; // list of monitors

    // input handling
    struct wlr_seat *seat;
    struct wl_listener new_input;
    struct wl_list keyboards;
    struct wl_listener cursor_button;

    // mouse handling
    struct wlr_output_layout *output_layout;
    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;
    struct wl_listener cursor_motion;

    enum gemstone_cursor_mode cursor_mode;
    struct gemstone_windows *grabbed_window;
    double grab_x, grab_y; // offset coords
};

// monitor connected event
void server_new_output(struct wl_listener *listener, void *data);
// new toplevel window event
void server_new_xdg_toplevel(struct wl_listener *listener, void *data);
// new input event
void server_new_input(struct wl_listener *listener, void *data);
// cursor event
void server_cursor_motion(struct wl_listener *listener, void *data);
// mouse button
void server_cursor_button(struct wl_listener *listener, void *data);
#endif