#include "input.h"
#include "keyboard.h"
#include "xdg_shell.h"
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>

// Helper: find the wlr_surface and View (if any) at the cursor position.
static struct wlr_surface *surface_at(struct Server *server,
        double *sx, double *sy, struct View **view_out) {
    if (view_out) *view_out = NULL;

    struct wlr_scene_node *node = wlr_scene_node_at(
        &server->scene->tree.node, server->cursor->x, server->cursor->y, sx, sy);

    if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER) return NULL;

    struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *scene_surface =
        wlr_scene_surface_try_from_buffer(scene_buffer);
    if (scene_surface == NULL) return NULL;

    // Walk up the tree to find a View, if one exists.
    if (view_out) {
        struct wlr_scene_tree *tree = node->parent;
        while (tree != NULL && tree->node.data == NULL) {
            tree = tree->node.parent;
        }
        if (tree != NULL) *view_out = tree->node.data;
    }

    return scene_surface->surface;
}

static void process_cursor_motion(struct Server *server, uint32_t time) {
    if (server->cursor_mode == CURSOR_MOVE) {
        wlr_scene_node_set_position(&server->grabbed_view->scene_tree->node,
            server->cursor->x - server->grab_x,
            server->cursor->y - server->grab_y);
        return;
    }

    if (server->cursor_mode == CURSOR_RESIZE) {
        struct View *view = server->grabbed_view;
        double dx = server->cursor->x - server->grab_x;
        double dy = server->cursor->y - server->grab_y;
        double x = view->scene_tree->node.x;
        double y = view->scene_tree->node.y;
        int width  = server->grab_geobox.width;
        int height = server->grab_geobox.height;

        if (server->resize_edges & WLR_EDGE_TOP) {
            y = server->grab_geobox.y + dy;
            height -= dy;
        } else if (server->resize_edges & WLR_EDGE_BOTTOM) {
            height += dy;
        }
        if (server->resize_edges & WLR_EDGE_LEFT) {
            x = server->grab_geobox.x + dx;
            width -= dx;
        } else if (server->resize_edges & WLR_EDGE_RIGHT) {
            width += dx;
        }

        wlr_scene_node_set_position(&view->scene_tree->node, x, y);
        wlr_xdg_toplevel_set_size(view->xdg_toplevel, width, height);
        return;
    }

    // PASSTHROUGH: find whatever surface is under the cursor.
    double sx, sy;
    struct wlr_surface *surface = surface_at(server, &sx, &sy, NULL);
    struct wlr_seat *seat = server->seat;

    if (surface != NULL) {
        // Only call notify_enter when the surface changes — calling it every
        // motion resets the cursor shape the client just set.
        if (seat->pointer_state.focused_surface != surface) {
            wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
        }
        wlr_seat_pointer_notify_motion(seat, time, sx, sy);
    } else {
        // Nothing under cursor — reset to default arrow and clear pointer focus.
        wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "left_ptr");
        wlr_seat_pointer_clear_focus(seat);
    }

    wlr_seat_pointer_notify_frame(seat);
}

// FIX: Handle cursor shape requests from clients (e.g. resize arrows, text beam).
static void on_request_set_cursor(struct wl_listener *listener, void *data) {
    struct Server *server = wl_container_of(listener, server, request_set_cursor);
    struct wlr_seat_pointer_request_set_cursor_event *event = data;

    // Only honour the request if it comes from the focused client.
    if (event->seat_client != server->seat->pointer_state.focused_client) return;

    wlr_cursor_set_surface(server->cursor, event->surface,
        event->hotspot_x, event->hotspot_y);
}

void on_new_input(struct wl_listener *listener, void *data) {
    struct Server *server = wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;

    if (device->type == WLR_INPUT_DEVICE_POINTER) {
        wlr_cursor_attach_input_device(server->cursor, device);
    }
    if (device->type == WLR_INPUT_DEVICE_KEYBOARD) {
        on_new_keyboard(&server->new_input, data);
    }
}

void on_cursor_motion(struct wl_listener *listener, void *data) {
    struct Server *server = wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;
    wlr_cursor_move(server->cursor, &event->pointer->base,
        event->delta_x, event->delta_y);
    process_cursor_motion(server, event->time_msec);
}

void on_cursor_motion_absolute(struct wl_listener *listener, void *data) {
    struct Server *server = wl_container_of(listener, server, cursor_motion_absolute);
    struct wlr_pointer_motion_absolute_event *event = data;
    wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, event->x, event->y);
    process_cursor_motion(server, event->time_msec);
}

void on_cursor_button(struct wl_listener *listener, void *data) {
    struct Server *server = wl_container_of(listener, server, cursor_button);
    struct wlr_pointer_button_event *event = data;

    if (event->state == WL_POINTER_BUTTON_STATE_PRESSED) {
        double sx, sy;
        struct View *view = NULL;
        struct wlr_surface *surface = surface_at(server, &sx, &sy, &view);

        // Focus the parent window if we found one (won't be set for popups).
        if (view != NULL) {
            focus_view(view, surface);
        }
        // FIX: Always forward the click to whatever surface is under the cursor,
        // including popup surfaces which have no associated View.
    }

    wlr_seat_pointer_notify_button(server->seat, event->time_msec,
        event->button, event->state);
    wlr_seat_pointer_notify_frame(server->seat);

    if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
        server->cursor_mode = CURSOR_PASSTHROUGH;
    }
}

void on_request_set_cursor_init(struct Server *server) {
    server->request_set_cursor.notify = on_request_set_cursor;
    wl_signal_add(&server->seat->events.request_set_cursor,
        &server->request_set_cursor);
}