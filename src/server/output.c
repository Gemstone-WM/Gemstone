#include "output.h"
#include <stdlib.h>

void on_output_frame(struct wl_listener *listener, void *data) {
    struct Output *output = wl_container_of(listener, output, frame);
    wlr_scene_output_commit(output->scene_output, NULL);
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

    wlr_scene_rect_create(
        &server->scene->tree,
        wlr_output->width, wlr_output->height,
        (float[4]){0.2f, 0.1f, 0.2f, 1.0f}
    );

    struct Output *output = calloc(1, sizeof(struct Output));
    output->wlr_output   = wlr_output;
    output->scene_output = wlr_scene_output_create(server->scene, wlr_output);
    output->frame.notify = on_output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);
}