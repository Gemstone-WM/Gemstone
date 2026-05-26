#include "compositor.h"
#include <time.h>
#include <stdlib.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>

#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <xkbcommon/xkbcommon.h> 

//finding window below cursor for input focus etc
struct gemstone_windows *desktop_window_at(struct gemstone_server *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy) {
    struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
    if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER) return NULL;
    
    struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
    if (!scene_surface) return NULL;
    
    *surface = scene_surface->surface;
    
    // looking for the node with the window data
    struct wlr_scene_tree *tree = node->parent;
    while (tree != NULL && tree->node.data == NULL) {
        tree = tree->node.parent;
    }
    return tree ? tree->node.data : NULL;
}

void focus_window(struct gemstone_windows *window, struct wlr_surface *surface) {
    if (window == NULL) return;
    struct gemstone_server *server = window->server;
    struct wlr_seat *seat = server->seat;
    struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
    
    if (prev_surface == surface) return; // already focused
    
    if (prev_surface) {
        struct wlr_xdg_toplevel *prev_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
        if (prev_toplevel) wlr_xdg_toplevel_set_activated(prev_toplevel, false);
    }
    
    wlr_xdg_toplevel_set_activated(window->xdg_toplevel, true);
    
    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
    if (keyboard) {
        wlr_seat_keyboard_notify_enter(seat, window->xdg_toplevel->base->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
    }
}

void output_frame(struct wl_listener *listener, void *data) {
    struct gemstone_output *output = wl_container_of(listener, output, frame);
    struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(output->server->scene, output->wlr_output);
    
    // pixel calculations
    wlr_scene_output_commit(scene_output, NULL);

    // frame pacing time thingy
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    // returning the frame
    wlr_scene_output_send_frame_done(scene_output, &now);
}

void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
    struct gemstone_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat, &keyboard->wlr_keyboard->modifiers);
}

void keyboard_handle_key(struct wl_listener *listener, void *data) {
    struct gemstone_keyboard *keyboard = wl_container_of(listener, keyboard, key);
    struct gemstone_server *server = keyboard->server;
    struct wlr_keyboard_key_event *event = data;
    struct wlr_seat *seat = server->seat;

    // physical press to logical keycode
    uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state, keycode, &syms);

    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);

    // quit bind (Alt + Esc)
    if ((modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++) {
            if (syms[i] == XKB_KEY_Escape) {
                wl_display_terminate(server->wl_display);
                handled = true;
            }
        }
    }

    // if not registered as shortcut just type as usual etc
    if (!handled) {
        wlr_seat_set_keyboard(seat, keyboard->wlr_keyboard);
        wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
    }
}
void server_cursor_motion(struct wl_listener *listener, void *data) {
    struct gemstone_server *server = wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;

    wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);

    if (server->cursor_mode == GEMSTONE_CURSOR_MOVE && server->grabbed_window != NULL) {
        // new coords calculations
        int new_x = server->cursor->x - server->grab_x;
        int new_y = server->cursor->y - server->grab_y;
        
        // update coords
        server->grabbed_window->position.x = new_x;
        server->grabbed_window->position.y = new_y;
        
        // move window visually
        wlr_scene_node_set_position(&server->grabbed_window->xdg_toplevel->base->data->node, new_x, new_y);
        return; // dont click while dragging
    }

    double sx, sy;
    struct wlr_surface *surface = NULL;
    struct gemstone_windows *window = desktop_window_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);

    if (surface) {
        wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(server->seat, event->time_msec, sx, sy);
    } else {
        wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
        wlr_seat_pointer_clear_focus(server->seat);
    }
}

void server_cursor_button(struct wl_listener *listener, void *data) {
    struct gemstone_server *server = wl_container_of(listener, server, cursor_button);
    struct wlr_pointer_button_event *event = data;

    wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);

    if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
        // stop dragging
        server->cursor_mode = GEMSTONE_CURSOR_PASSTHROUGH;
        server->grabbed_window = NULL;
    } else {
        double sx, sy;
        struct wlr_surface *surface = NULL;
        struct gemstone_windows *window = desktop_window_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);
        
        if (window != NULL) {
            focus_window(window, surface);
            
            // start to move
            server->cursor_mode = GEMSTONE_CURSOR_MOVE;
            server->grabbed_window = window;
            
            // offset calculations
            // window pos = cursor pos - offset
            server->grab_x = server->cursor->x - window->position.x;
            server->grab_y = server->cursor->y - window->position.y;
        }
    }
}

void server_new_input(struct wl_listener *listener, void *data) {
    struct gemstone_server *server = wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;

    if (device->type == WLR_INPUT_DEVICE_KEYBOARD) {
        struct gemstone_keyboard *keyboard = calloc(1, sizeof(struct gemstone_keyboard));
        keyboard->server = server;
        keyboard->wlr_keyboard = wlr_keyboard_from_input_device(device);

        // linux auto keyboard layout stuff
        struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
        wlr_keyboard_set_keymap(keyboard->wlr_keyboard, keymap);
        xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        wlr_keyboard_set_repeat_info(keyboard->wlr_keyboard, 25, 600);

        // waiting key presses
        keyboard->modifiers.notify = keyboard_handle_modifiers;
        wl_signal_add(&keyboard->wlr_keyboard->events.modifiers, &keyboard->modifiers);
        keyboard->key.notify = keyboard_handle_key;
        wl_signal_add(&keyboard->wlr_keyboard->events.key, &keyboard->key);

        wlr_seat_set_keyboard(server->seat, keyboard->wlr_keyboard);
        wl_list_insert(&server->keyboards, &keyboard->link);

        // actually telling wayland that yep i have a fucking keyboard
        wlr_seat_set_capabilities(server->seat, WL_SEAT_CAPABILITY_KEYBOARD);
    }

    if (device->type == WLR_INPUT_DEVICE_POINTER) {
        wlr_cursor_attach_input_device(server->cursor, device);
        
        // hey wayland, i have a fucking mouse now
        uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
        if (!wl_list_empty(&server->keyboards)) {
            caps |= WL_SEAT_CAPABILITY_KEYBOARD;
        }
        wlr_seat_set_capabilities(server->seat, caps);
    }
}

void server_new_output(struct wl_listener *listener, void *data) {
    struct gemstone_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);
    // mouse thing
    wlr_output_layout_add_auto(server->output_layout, wlr_output);

    // creating output struct
    struct gemstone_output *output = calloc(1, sizeof(struct gemstone_output));
    output->wlr_output = wlr_output;
    output->server = server;
    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    // creating scene output and adding it to the list
    wlr_scene_output_create(server->scene, wlr_output);
    wl_list_insert(&server->outputs, &output->link);

    // enabling the output
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode != NULL) {
        wlr_output_state_set_mode(&state, mode);
    }
    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);
}
void xdg_toplevel_commit(struct wl_listener *listener, void *data) {
    // get window
    struct gemstone_windows *window = wl_container_of(listener, window, commit);

    if (window->xdg_toplevel->base->initial_commit) {
        wlr_xdg_toplevel_set_size(window->xdg_toplevel, 0, 0); // 0,0 is just telling app to choose size
    }
}
void xdg_toplevel_map(struct wl_listener *listener, void *data) {
    struct gemstone_windows *window = wl_container_of(listener, window, map);
    struct gemstone_server *server = window->server;

    // auto focusing since window is ready
    wlr_xdg_toplevel_set_activated(window->xdg_toplevel, true);
    
    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
    if (keyboard != NULL) {
        wlr_seat_keyboard_notify_enter(
            server->seat, window->xdg_toplevel->base->surface, 
            keyboard->keycodes, keyboard->num_keycodes, 
            &keyboard->modifiers
        );
    }
}
void server_new_xdg_toplevel(struct wl_listener *listener, void *data) {
    struct gemstone_server *server = wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *toplevel = data;

    struct gemstone_windows *window = calloc(1, sizeof(struct gemstone_windows));
    window->xdg_toplevel = toplevel; 
    window->server = server;
    
    window->position.x = 0;
    window->position.y = 0;

    window->commit.notify = xdg_toplevel_commit;
    wl_signal_add(&toplevel->base->surface->events.commit, &window->commit);

    // window is ready to be displayed
    window->map.notify = xdg_toplevel_map;
    wl_signal_add(&toplevel->base->surface->events.map, &window->map);

    wl_list_insert(&server->windows, &window->link);
    wlr_scene_xdg_surface_create(&server->scene->tree, toplevel->base);

    // attach window to tree
    struct wlr_scene_tree *scene_tree = wlr_scene_xdg_surface_create(&server->scene->tree, toplevel->base);
    scene_tree->node.data = window;

    wlr_log(WLR_INFO, "Gemstone : New toplevel window created and added to list!");
    
}