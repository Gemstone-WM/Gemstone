#include "output.h"
#include <stdlib.h>
#include <time.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>

static void on_output_frame(struct wl_listener *listener, void *data) {
    struct Output *output = wl_container_of(listener, output, frame);
    
    // 1. Render the scene (No longer crashing because scene_output is guaranteed to exist!)
    wlr_scene_output_commit(output->scene_output, NULL);

    // 2. THE MISSING LINK: Tell apps we finished drawing so they can unfreeze!
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(output->scene_output, &now);
}

static void on_output_destroy(struct wl_listener *listener, void *data) {
    struct Output *output = wl_container_of(listener, output, destroy);
    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->destroy.link);
    free(output);
}

void on_new_output(struct wl_listener *listener, void *data) {
    struct Server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode) wlr_output_state_set_mode(&state, mode);
    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);

    wlr_output_create_global(wlr_output, server->display);

    struct Output *output = calloc(1, sizeof(struct Output));
    output->wlr_output = wlr_output;
    
    // Explicitly create the scene output so we don't hit a NULL pointer later
    output->scene_output = wlr_scene_output_create(server->scene, wlr_output);

    output->frame.notify = on_output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    output->destroy.notify = on_output_destroy;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    // Arrange the monitor in the global coordinate space
    wlr_output_layout_add_auto(server->output_layout, wlr_output);

    // Create a background specifically for this monitor
    struct wlr_scene_rect *bg = wlr_scene_rect_create(
        &server->scene->tree,
        wlr_output->width, wlr_output->height,
        (float[4]){0.2f, 0.1f, 0.2f, 1.0f}
    );
    
    // Pass the box by reference to be filled with the correct monitor coordinates
    struct wlr_box layout_box;
    wlr_output_layout_get_box(server->output_layout, wlr_output, &layout_box);
    wlr_scene_node_set_position(&bg->node, layout_box.x, layout_box.y);
    
    // Push background to the very bottom so it doesn't block window clicks!
    wlr_scene_node_lower_to_bottom(&bg->node);
}