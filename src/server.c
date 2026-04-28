#include "compositor.h"
#include "server.h"
#include <wlr/util/log.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_data_device.h>
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
    wl_display_destroy(server->wl_display);
    return EXIT_SUCCESS;
}
