#include "input.h"
#include "keyboard.h"
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_pointer.h>

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
}
