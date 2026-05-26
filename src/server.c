#include "compositor.h"
#include "server.h"
#include <wlr/util/log.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_data_device.h>

#include <wlr/types/wlr_xdg_shell.h>

#include <stdlib.h>
#include <stdio.h>

struct gemstone_server server;

int init_server()
{
    wlr_log_init(WLR_DEBUG, NULL);


    // starting
    server.wl_display = wl_display_create();
    if (!server.wl_display)
    {
        fprintf(stderr, "Failed to create Wayland display\n");
        return EXIT_FAILURE; // failed too early
    }

    // ok first step done now keep going
    server.backend = wlr_backend_autocreate(wl_display_get_event_loop(server.wl_display), NULL);
    if (!server.backend)
    {
        fprintf(stderr, "Failed to create wlr_backend\n");
        goto fail_display;
    }

    // yes it works fine
    server.renderer = wlr_renderer_autocreate(server.backend);
    if (!server.renderer)
    {
        fprintf(stderr, "Failed to create wlr_renderer\n");
        goto fail_backend;
    }

    if (!wlr_renderer_init_wl_display(server.renderer, server.wl_display))
    {
        fprintf(stderr, "Failed to initialize renderer with display\n");
        goto fail_renderer;
    }

    // okay please work
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
    if (!server.allocator)
    {
        fprintf(stderr, "Failed to create wlr_allocator\n");
        goto fail_renderer;
    }

    // xdg stuff
    wlr_compositor_create(server.wl_display,5,server.renderer);
    wlr_subcompositor_create(server.wl_display);
    wlr_data_device_manager_create(server.wl_display);

    // xdg shell
    server.xdg_shell = wlr_xdg_shell_create(server.wl_display, 3);
    server.new_xdg_toplevel.notify = server_new_xdg_toplevel;

    // listening for new windows
    wl_signal_add(&server.xdg_shell->events.new_toplevel, &server.new_xdg_toplevel);

    // window list
    wl_list_init(&server.windows);

    wl_list_init(&server.outputs);
    server.scene = wlr_scene_create();

    // cursor stuff
    server.output_layout = wlr_output_layout_create(server.wl_display);
    
    server.cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server.cursor, server.output_layout);
    server.cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

    server.cursor_motion.notify = server_cursor_motion;
    wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);

    server.cursor_motion.notify = server_cursor_motion;
    wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);
    
    // listen to click
    server.cursor_button.notify = server_cursor_button;
    wl_signal_add(&server.cursor->events.button, &server.cursor_button);

    // keyboard stuff
    wl_list_init(&server.keyboards);
    server.seat = wlr_seat_create(server.wl_display, "seat0");
    
    server.new_input.notify = server_new_input;
    wl_signal_add(&server.backend->events.new_input, &server.new_input);

    server.new_output.notify = server_new_output;
    // listening for new monitors
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

    // please please please please
    if (!wlr_backend_start(server.backend))
    {
        fprintf(stderr, "Failed to start backend\n");
        goto fail_allocator;
    }

    // yipee
    wlr_log(WLR_INFO, "Starting Gemstone Wayland Server...");

    const char *socket = wl_display_add_socket_auto(server.wl_display);
    if (!socket)
    {
        fprintf(stderr, "Failed to add socket\n");
        goto fail_allocator;
    }
    setenv("WAYLAND_DISPLAY", socket, true);
    wlr_log(WLR_INFO, "Gemstone : WAYLAND_DISPLAY=%s", socket);

    wl_display_run(server.wl_display);

    cleanup_server(&server);

    return EXIT_SUCCESS;

    // erm if this gets called, something fucked up entirely
    fail_allocator:
        // self dies
    fail_renderer:
        wlr_renderer_destroy(server.renderer);
    fail_backend:
        wlr_backend_destroy(server.backend);
    fail_display:
        wl_display_destroy(server.wl_display);
        return EXIT_FAILURE;
}

int cleanup_server(struct gemstone_server *server)
{
    // kill apps
    wl_display_destroy_clients(server->wl_display);

    // removing listeners
    wl_list_remove(&server->new_xdg_toplevel.link);
    wl_list_remove(&server->new_output.link);
    wl_list_remove(&server->new_input.link);

    wlr_xcursor_manager_destroy(server->cursor_mgr);
    wlr_cursor_destroy(server->cursor);
    wlr_output_layout_destroy(server->output_layout);

    // finishing the work
    wl_display_destroy(server->wl_display);
    return EXIT_SUCCESS;
}
