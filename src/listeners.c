#include "compositor.h"
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include <wlr/types/wlr_xdg_shell.h>


// monitor connected event
void server_new_output(struct wl_listener *listener, void *data) {
    // get the server
    struct gemstone_server *server = wl_container_of(listener, server, new_output);
    
    // monitor data
    struct wlr_output *wlr_output = data;

    wlr_log(WLR_INFO, "Gemstone : New monitor connected: %s", wlr_output->name);

    // sending monitor to wlroots
    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    // activating monitor
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    
    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode != NULL) {
        wlr_output_state_set_mode(&state, mode);
    }

    // applying the stuff
    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);
}

void server_new_xdg_surface(struct wl_listener *listener, void *data) {
    struct gemstone_server *server = wl_container_of(listener, server, new_xdg_surface);
    struct wlr_xdg_surface *xdg_surface = data;

    wlr_log(WLR_INFO, "Gemstone : New xdg surface created!");

    // sending response back
    wlr_xdg_surface_schedule_configure(xdg_surface);
}