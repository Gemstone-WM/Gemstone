#include "server/server.h"
#include "utils/logger.h" // <-- Include our new utility

int main(void) {
    // Tell Gemstone to create a log file in the directory you run it from
    gemstone_log_init(WLR_DEBUG, "gemstone.log");

    struct Server server = {0};
    server_init(&server);
    server_run(&server);
    server_destroy(&server);

    // Safely close the file when we exit
    gemstone_log_terminate();

    return 0;
}