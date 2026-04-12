#include "xdg_shell.h"
#include <stdlib.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>

void focus_view(struct View *view, struct wlr_surface *surface) {
    if (view == NULL || view->scene_tree == NULL) return;
    wlr_scene_node_raise_to_top(&view->scene_tree->node);
    wlr_xdg_toplevel_set_activated(view->xdg_toplevel, true);
    
    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(view->server->seat);
    if (keyboard != NULL) {
        wlr_seat_keyboard_notify_enter(view->server->seat, surface, 
            keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
    }
}

// wlroots 0.20: map/unmap no longer exist as signals on xdg_surface->events.
// Instead, we listen to surface commits and check the initial_commit flag to
// do our first configure, then track mapped state via surface->mapped.
static void xdg_toplevel_surface_commit(struct wl_listener *listener, void *data) {
    struct View *view = wl_container_of(listener, view, surface_commit);

    if (view->xdg_toplevel->base->initial_commit) {
        // First commit: send the configure handshake so the client can draw.
        wlr_xdg_surface_schedule_configure(view->xdg_toplevel->base);
        return;
    }

    // Once the surface has a buffer and is mapped, create the scene node.
    if (view->xdg_toplevel->base->surface->mapped && view->scene_tree == NULL) {
        view->scene_tree = wlr_scene_xdg_surface_create(
            &view->server->scene->tree, view->xdg_toplevel->base);
        view->scene_tree->node.data = view;
        view->xdg_toplevel->base->data = view->scene_tree;

        wlr_log(WLR_INFO, "💎 WIN! The window successfully mapped!");
        focus_view(view, view->xdg_toplevel->base->surface);
    }

    // If the surface was unmapped (e.g. minimized/hidden), clean up our reference.
    if (!view->xdg_toplevel->base->surface->mapped && view->scene_tree != NULL) {
        view->scene_tree = NULL;
    }
}

static void xdg_toplevel_destroy(struct wl_listener *listener, void *data) {
    struct View *view = wl_container_of(listener, view, destroy);
    wl_list_remove(&view->surface_commit.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->request_move.link);
    wl_list_remove(&view->request_resize.link);
    wl_list_remove(&view->request_maximize.link);
    wl_list_remove(&view->request_minimize.link);
    free(view);
}

static void xdg_toplevel_request_move(struct wl_listener *listener, void *data) {
    struct View *view = wl_container_of(listener, view, request_move);
    struct Server *server = view->server;

    server->grabbed_view = view;
    server->cursor_mode  = CURSOR_MOVE;
    server->grab_x = server->cursor->x - view->scene_tree->node.x;
    server->grab_y = server->cursor->y - view->scene_tree->node.y;
}

static void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) {
    struct View *view = wl_container_of(listener, view, request_resize);
    struct wlr_xdg_toplevel_resize_event *event = data;
    struct Server *server = view->server;

    server->grabbed_view  = view;
    server->cursor_mode   = CURSOR_RESIZE;
    server->resize_edges  = event->edges;

    // wlroots 0.20: use current.geometry directly
    struct wlr_box geo = view->xdg_toplevel->base->current.geometry;
    server->grab_geobox.x      = view->scene_tree->node.x + geo.x;
    server->grab_geobox.y      = view->scene_tree->node.y + geo.y;
    server->grab_geobox.width  = geo.width;
    server->grab_geobox.height = geo.height;

    server->grab_x = server->cursor->x;
    server->grab_y = server->cursor->y;
}

static void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data) {
    struct View *view = wl_container_of(listener, view, request_maximize);
    wlr_xdg_surface_schedule_configure(view->xdg_toplevel->base);
}

static void xdg_toplevel_request_minimize(struct wl_listener *listener, void *data) {
    struct View *view = wl_container_of(listener, view, request_minimize);
    wlr_xdg_surface_schedule_configure(view->xdg_toplevel->base);
}

static void server_new_xdg_toplevel(struct wl_listener *listener, void *data) {
    struct Server *server = wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *toplevel = data;

    struct View *view = calloc(1, sizeof(struct View));
    view->server = server;
    view->xdg_toplevel = toplevel;

    // wlroots 0.20: map/unmap signals are gone. Use surface commit + initial_commit.
    view->surface_commit.notify = xdg_toplevel_surface_commit;
    wl_signal_add(&toplevel->base->surface->events.commit, &view->surface_commit);

    view->destroy.notify = xdg_toplevel_destroy;
    wl_signal_add(&toplevel->events.destroy, &view->destroy);

    view->request_move.notify = xdg_toplevel_request_move;
    wl_signal_add(&toplevel->events.request_move, &view->request_move);

    view->request_resize.notify = xdg_toplevel_request_resize;
    wl_signal_add(&toplevel->events.request_resize, &view->request_resize);

    view->request_maximize.notify = xdg_toplevel_request_maximize;
    wl_signal_add(&toplevel->events.request_maximize, &view->request_maximize);

    view->request_minimize.notify = xdg_toplevel_request_minimize;
    wl_signal_add(&toplevel->events.request_minimize, &view->request_minimize);

    // NOTE: Do NOT call wlr_xdg_surface_schedule_configure here.
    // wlroots 0.20 requires waiting for initial_commit before configuring.
}

// Small wrapper since popups don't have a View
struct Popup {
    struct wlr_xdg_popup    *popup;
    struct wl_listener       surface_commit;
    struct wl_listener       destroy;
};

static void popup_handle_surface_commit(struct wl_listener *listener, void *data) {
    struct Popup *p = wl_container_of(listener, p, surface_commit);
    if (p->popup->base->initial_commit) {
        wlr_xdg_surface_schedule_configure(p->popup->base);
    }
}

static void popup_handle_destroy(struct wl_listener *listener, void *data) {
    struct Popup *p = wl_container_of(listener, p, destroy);
    wl_list_remove(&p->surface_commit.link);
    wl_list_remove(&p->destroy.link);
    free(p);
}

static void server_new_xdg_popup(struct wl_listener *listener, void *data) {
    struct wlr_xdg_popup *popup = data;

    // Parent the popup to its parent surface's scene tree so it renders
    // at the correct position relative to the parent window.
    struct wlr_xdg_surface *parent_xdg =
        wlr_xdg_surface_try_from_wlr_surface(popup->parent);
    if (parent_xdg == NULL || parent_xdg->data == NULL) return;

    struct wlr_scene_tree *parent_tree = parent_xdg->data;
    wlr_scene_xdg_surface_create(parent_tree, popup->base);

    // wlroots 0.20: must wait for initial_commit before calling schedule_configure
    struct Popup *p = calloc(1, sizeof(struct Popup));
    p->popup = popup;

    p->surface_commit.notify = popup_handle_surface_commit;
    wl_signal_add(&popup->base->surface->events.commit, &p->surface_commit);

    p->destroy.notify = popup_handle_destroy;
    wl_signal_add(&popup->base->events.destroy, &p->destroy);
}

void init_xdg_shell(struct Server *server) {
    server->xdg_shell = wlr_xdg_shell_create(server->display, 6);
    
    server->new_xdg_toplevel.notify = server_new_xdg_toplevel;
    wl_signal_add(&server->xdg_shell->events.new_toplevel, &server->new_xdg_toplevel);

    server->new_xdg_popup.notify = server_new_xdg_popup;
    wl_signal_add(&server->xdg_shell->events.new_popup, &server->new_xdg_popup);
}