#include "server/server.h"
#include <wlr/util/log.h>

int main(void) {
    wlr_log_init(WLR_DEBUG, NULL);

    struct Server server = {0};
    server_init(&server);
    server_run(&server);
    server_destroy(&server);

    return 0;
}