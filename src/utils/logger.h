#pragma once
#include <wlr/util/log.h>

// Initializes the custom logger and opens the target file
void gemstone_log_init(enum wlr_log_importance verbosity, const char *filepath);

// Closes the file safely when the compositor shuts down
void gemstone_log_terminate(void);