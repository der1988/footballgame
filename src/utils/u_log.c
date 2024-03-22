#include "../types.h"
#include "../utils/utils.h"
#include <raylib.h>
#include <stdarg.h>
#include <stdio.h>

#define LOG_LEVEL LT_INFO

void utils_log(char *file, u16 line, ULogType type, ULogSystem system, const char *ctx,
               const char *format, ...) {
    static char *log_type_prefixes[LT_COUNT] = {
        "ERROR", "WARN", "INFO", "DEBUG", "TRACE",
    };

    if (type <= LOG_LEVEL) {
        static char *log_systems[9] = {
            "GAME", "INPUT", "SOUND", "ASSETS", "ENTITIES", "MEMORY", "UTILS", "TESTS", "OTHER",
        };

        double run_time = GetTime();
        if (ctx != NULL) {
            printf("%f %s: %s: [%s] %s:%d -> ", run_time, log_type_prefixes[(usize)type],
                   log_systems[(usize)system], ctx, file, line);
        } else {
            printf("%f %s: %s: %s:%d -> ", run_time, log_type_prefixes[(usize)type],
                   log_systems[(usize)system], file, line);
        }

        va_list arg_list;
        va_start(arg_list, format);
        vprintf(format, arg_list);
        va_end(arg_list);

        printf("\n");
    }
}
