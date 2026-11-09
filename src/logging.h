#ifndef LOGGING_H
#define LOGGING_H

#include <libretro.h>

void js2000_log_cb(enum retro_log_level level, const char *fmt, ...);

#define js2000_info(fmt, ...) js2000_log_cb(RETRO_LOG_INFO,  fmt, ##__VA_ARGS__)
#define js2000_warn(fmt, ...) js2000_log_cb(RETRO_LOG_WARN,  fmt, ##__VA_ARGS__)
#define js2000_err(fmt, ...)  js2000_log_cb(RETRO_LOG_ERROR, fmt, ##__VA_ARGS__)
#define js2000_dbg(fmt, ...)  js2000_log_cb(RETRO_LOG_DEBUG, fmt, ##__VA_ARGS__)

#endif // LOGGING_H