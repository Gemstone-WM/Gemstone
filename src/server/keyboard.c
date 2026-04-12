#include "keyboard.h"
#include "xdg_shell.h" // <-- RESTORED: Needed for unminimize_all_views
#include <wlr/types/wlr_xdg_shell.h> // <-- NEW: Needed for wlroots window closing
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_seat.h>
#include <xkbcommon/xkbcommon.h>
#include <stdlib.h>
#include <stdio.h>     
#include <string.h>    
#include <wlr/util/log.h>

struct Keyboard {
    struct Server *server;
    struct wlr_keyboard *wlr_keyboard;
    struct wl_listener modifiers;
    struct wl_listener key;
};

static void on_modifiers(struct wl_listener *listener, void *data) {
    struct Keyboard *kb = wl_container_of(listener, kb, modifiers);
    wlr_seat_set_keyboard(kb->server->seat, kb->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(kb->server->seat,
        &kb->wlr_keyboard->modifiers);
}

static void on_key(struct wl_listener *listener, void *data) {
    struct Keyboard *kb = wl_container_of(listener, kb, key);
    struct wlr_keyboard_key_event *event = data;
    struct wlr_seat *seat = kb->server->seat;

    uint32_t modifiers = wlr_keyboard_get_modifiers(kb->wlr_keyboard);
    bool super_pressed = modifiers & WLR_MODIFIER_LOGO;

    xkb_keysym_t sym = xkb_state_key_get_one_sym(kb->wlr_keyboard->xkb_state, event->keycode + 8);

    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED && super_pressed) {
        if (sym == XKB_KEY_u) {
            //unminimize_all_views(kb->server);
            return; 
        }
        else if (sym == XKB_KEY_q) { 
            // Close the currently focused window
            struct wlr_surface *focused = seat->keyboard_state.focused_surface;
            if (focused != NULL) {
                struct wlr_xdg_surface *xdg_surface = wlr_xdg_surface_try_from_wlr_surface(focused);
                if (xdg_surface != NULL && xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
                    wlr_xdg_toplevel_send_close(xdg_surface->toplevel);
                }
            }
            return;
        }
    }

    wlr_seat_set_keyboard(seat, kb->wlr_keyboard);
    wlr_seat_keyboard_notify_key(seat, event->time_msec,
        event->keycode, event->state);
}

// --- SYSTEM LAYOUT DETECTION ---
static void get_system_layout(char *layout_buffer, size_t size) {
    const char *env_layout = getenv("XKB_DEFAULT_LAYOUT");
    if (env_layout != NULL && strlen(env_layout) > 0) {
        strncpy(layout_buffer, env_layout, size);
        return;
    }

    FILE *fp = popen("localectl status 2>/dev/null | grep 'X11 Layout' | awk '{print $3}'", "r");
    if (fp != NULL) {
        if (fgets(layout_buffer, size, fp) != NULL) {
            layout_buffer[strcspn(layout_buffer, "\n")] = 0; 
            pclose(fp);
            if (strlen(layout_buffer) > 0) return;
        } else {
            pclose(fp);
        }
    }

    strncpy(layout_buffer, "us", size);
}

void on_new_keyboard(struct wl_listener *listener, void *data) {
    struct Server *server = wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;
    if (device->type != WLR_INPUT_DEVICE_KEYBOARD) return;

    struct Keyboard *kb = calloc(1, sizeof(struct Keyboard));
    kb->server = server;
    kb->wlr_keyboard = wlr_keyboard_from_input_device(device);

    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_rule_names rules = {0};
    
    char layout[64] = {0};
    get_system_layout(layout, sizeof(layout));
    rules.layout = layout;
    
    wlr_log(WLR_INFO, "🎹 Loaded system keyboard layout: %s", layout);

    struct xkb_keymap *keymap = xkb_keymap_new_from_names(ctx, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);
    
    wlr_keyboard_set_keymap(kb->wlr_keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    wlr_seat_set_keyboard(server->seat, kb->wlr_keyboard);

    kb->modifiers.notify = on_modifiers;
    wl_signal_add(&kb->wlr_keyboard->events.modifiers, &kb->modifiers);

    kb->key.notify = on_key;
    wl_signal_add(&kb->wlr_keyboard->events.key, &kb->key);
}