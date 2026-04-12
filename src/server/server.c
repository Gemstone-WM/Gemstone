#include "server.h"
#include "output.h"
#include "input.h"
#include "xdg_shell.h"

#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/util/log.h>
#include <stdlib.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_xdg_output_v1.h>

void server_init(struct Server *server) {
    server->display   = wl_display_create();
    server->backend   = wlr_backend_autocreate(
                            wl_display_get_event_loop(server->display), NULL);
    server->renderer  = wlr_renderer_autocreate(server->backend);
    wlr_renderer_init_wl_display(server->renderer, server->display);
    server->allocator = wlr_allocator_autocreate(server->backend, server->renderer);

    server->scene = wlr_scene_create();
    server->output_layout = wlr_output_layout_create(server->display);

    wlr_scene_attach_output_layout(server->scene, server->output_layout);

    init_xdg_shell(server);

    server->seat = wlr_seat_create(server->display, "seat0");
    wlr_subcompositor_create(server->display);
    wlr_seat_set_capabilities(server->seat,
        WL_SEAT_CAPABILITY_KEYBOARD | WL_SEAT_CAPABILITY_POINTER);
    wlr_data_device_manager_create(server->display);

    wlr_xdg_output_manager_v1_create(server->display, server->output_layout);
    wlr_compositor_create(server->display, 5, server->renderer);

    server->cursor     = wlr_cursor_create();
    server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    wlr_cursor_attach_output_layout(server->cursor, server->output_layout);
    wlr_xcursor_manager_load(server->cursor_mgr, 1);
    wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "left_ptr");

    server->cursor_scene = wlr_scene_tree_create(&server->scene->tree);
    server->cursor_mode  = CURSOR_PASSTHROUGH;

    server->cursor_motion.notify = on_cursor_motion;
    wl_signal_add(&server->cursor->events.motion, &server->cursor_motion);

    server->cursor_motion_absolute.notify = on_cursor_motion_absolute;
    wl_signal_add(&server->cursor->events.motion_absolute,
        &server->cursor_motion_absolute);

    server->cursor_button.notify = on_cursor_button;
    wl_signal_add(&server->cursor->events.button, &server->cursor_button);

    // Wire up client cursor-shape requests (fixes cursor not changing)
    on_request_set_cursor_init(server);

    server->new_output.notify = on_new_output;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);

    server->new_input.notify = on_new_input;
    wl_signal_add(&server->backend->events.new_input, &server->new_input);
}

void server_run(struct Server *server) {
    const char *socket = wl_display_add_socket_auto(server->display);
    setenv("WAYLAND_DISPLAY", socket, true);
    wlr_backend_start(server->backend);
    wl_display_run(server->display);
}

void server_destroy(struct Server *server) {
    wl_display_destroy(server->display);
}