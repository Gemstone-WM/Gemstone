#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static FILE *log_file = NULL;

static void log_callback(enum wlr_log_importance importance, const char *fmt, va_list args) {
    if (!log_file) return;

    // 1. Get a nice human-readable timestamp
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_buf[26];
    strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    // 2. Map the wlroots verbosity to a string label
    const char *level = "";
    switch (importance) {
        case WLR_SILENT: level = "SILENT"; break;
        case WLR_ERROR:  level = "ERROR "; break;
        case WLR_INFO:   level = "INFO  "; break;
        case WLR_DEBUG:  level = "DEBUG "; break;
    }

    // 3. Write to our dedicated gemstone.log file
    fprintf(log_file, "[%s] [%s] ", time_buf, level);
    
    va_list file_args;
    va_copy(file_args, args);
    vfprintf(log_file, fmt, file_args);
    fprintf(log_file, "\n");
    va_end(file_args);
    
    fflush(log_file); // Force it to save to disk immediately in case of a crash!

    // 4. (Optional) Also print to the terminal so you can still watch it live
    fprintf(stderr, "[%s] [%s] ", time_buf, level);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void gemstone_log_init(enum wlr_log_importance verbosity, const char *filepath) {
    log_file = fopen(filepath, "w");
    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", filepath);
        // Fall back to default terminal logging if file fails
        wlr_log_init(verbosity, NULL); 
        return;
    }
    
    // Override the default wlroots logger with our custom file-writer!
    wlr_log_init(verbosity, log_callback);
}

void gemstone_log_terminate(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}