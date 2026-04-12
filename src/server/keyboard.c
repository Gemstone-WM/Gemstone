#include "keyboard.h"
#include <wlr/types/wlr_input_device.h>
#include <xkbcommon/xkbcommon.h>
#include <stdlib.h>
#include <wlr/util/log.h>

struct Keyboard {
    struct wlr_keyboard *wlr_keyboard;
    struct wl_listener   key;
};

void on_key(struct wl_listener *listener, void *data) {
    struct Keyboard *kb = wl_container_of(listener, kb, key);
    struct wlr_keyboard_key_event *event = data;
    wlr_log(WLR_DEBUG, "keycode: %d", event->keycode);
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        if (event->keycode == 1) {
            exit(0);
        }
    }
}

void on_new_keyboard(struct wl_listener *listener, void *data) {
    struct Server *server = wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;
    if (device->type != WLR_INPUT_DEVICE_KEYBOARD) return;

    struct Keyboard *kb = calloc(1, sizeof(struct Keyboard));
    kb->wlr_keyboard = wlr_keyboard_from_input_device(device);

    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap *keymap = xkb_keymap_new_from_names(ctx, NULL, 0);
    wlr_keyboard_set_keymap(kb->wlr_keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    kb->key.notify = on_key;
    wl_signal_add(&kb->wlr_keyboard->events.key, &kb->key);
}