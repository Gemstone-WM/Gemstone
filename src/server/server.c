#include "server.h"
#include "output.h"
#include "input.h"
#include <wlr/util/log.h>
#include <stdlib.h>

#include <wlr/types/wlr_output_layout.h>

void server_init(struct Server *server) {
    server->display   = wl_display_create();
    server->backend   = wlr_backend_autocreate(
                            wl_display_get_event_loop(server->display), NULL);
    server->renderer  = wlr_renderer_autocreate(server->backend);
    wlr_renderer_init_wl_display(server->renderer, server->display);
    server->allocator = wlr_allocator_autocreate(server->backend, server->renderer);
    server->scene     = wlr_scene_create();

    server->output_layout = wlr_output_layout_create(server->display);
    wlr_scene_attach_output_layout(server->scene, server->output_layout);

    wlr_compositor_create(server->display, 5, server->renderer);

    server->cursor     = wlr_cursor_create();
    server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    wlr_cursor_attach_output_layout(server->cursor, server->output_layout);
    wlr_xcursor_manager_load(server->cursor_mgr, 1);
    wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "left_ptr");
    server->cursor_scene = wlr_scene_tree_create(&server->scene->tree);

    server->cursor_motion.notify = on_cursor_motion;
    wl_signal_add(&server->cursor->events.motion, &server->cursor_motion);

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