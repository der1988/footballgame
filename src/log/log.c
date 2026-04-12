#include "types.h"
#include "log.h"
#include <stdarg.h>
#include <stdio.h>

#define LOG_LEVEL LT_DEBUG

const char *utils_log_systems_desc[LS_COUNT] = {
	"GAME",
	"INPUT",
	"SOUND",
	"ASSETS",
	"ENTITIES",
	"MEMORY",
	"UTILS",
	"TESTS",
	"LAYERS",
	"OTHER",
	"SCENES",
	"EVENTS",
	"TIME_MACHINE",
	"DEBUGUI",
	"VIDEO_EXPORT",
};

void log_line(char *file,
			  u16 line,
			  enum log_type type,
			  enum log_system system,
			  const char *ctx,
			  const char *format,
			  ...)
{
	static const char *log_type_prefixes[LT_COUNT] = {
		"ERROR",
		"WARN",
		"INFO",
		"DEBUG",
		"TRACE",
	};

	if (type <= LOG_LEVEL) {
		double run_time = GetTime();
		if (ctx != NULL) {
			printf("%f %s: %s: [%s] %s:%d -> ",
				   run_time,
				   log_type_prefixes[(usize)type],
				   utils_log_systems_desc[(usize)system],
				   ctx,
				   file,
				   line);
		} else {
			printf("%f %s: %s: %s:%d -> ",
				   run_time,
				   log_type_prefixes[(usize)type],
				   utils_log_systems_desc[(usize)system],
				   file,
				   line);
		}

		va_list arg_list;
		va_start(arg_list, format);
		vprintf(format, arg_list);
		va_end(arg_list);

		printf("\n");
	}
}
