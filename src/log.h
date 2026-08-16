#ifndef LOG_H
#define LOG_H

#include "types.h"
#include <stdio.h>
#include <stdlib.h>

enum log_type {
	LT_FATAL,
	LT_ERROR,
	LT_WARN,
	LT_INFO,
	LT_DEBUG,
	LT_TRACE,
	LT_COUNT,
};

enum log_system {
	LS_GAME,
	LS_INPUT,
	LS_SOUND,
	LS_ASSETS,
	LS_ENTITIES,
	LS_MEMORY,
	LS_UTILS,
	LS_TESTS,
	LS_LAYERS,
	LS_OTHER,
	LS_SCENES,
	LS_EVENTS,
	LS_TIME_MACHINE,
	LS_DEBUGUI,
	LS_VIDEO_EXPORT,
	LS_STATE_MACHINE,
	LS_ASSERT,
	LS_COUNT,
};

#ifdef NDEBUG
#define LOG_LEVEL LT_WARN
#else
#define LOG_LEVEL LT_DEBUG
#endif

extern const char *utils_log_systems_desc[LS_COUNT];

void log_line(char *file,
			  u16 line,
			  enum log_type type,
			  enum log_system system,
			  const char *ctx,
			  const char *format,
			  ...);

// The level test lives here rather than inside log_line so that a suppressed
// call compiles away entirely -- argument expressions included. Every macro
// below routes through it; log_line itself does no filtering.
#define GLOG_IF(TYPE, SYSTEM, CTX, ...)                                                            \
	do {                                                                                           \
		if ((TYPE) <= LOG_LEVEL) {                                                                 \
			log_line(__FILE__, __LINE__, TYPE, SYSTEM, CTX, __VA_ARGS__);                          \
		}                                                                                          \
	} while (0)

// Prints and aborts in every build, release included: unlike assert it is never
// compiled away, and unlike the levels below it is never filtered.
#define GLOG_PANIC_AT(SYSTEM, CTX, ...)                                                            \
	do {                                                                                           \
		log_line(__FILE__, __LINE__, LT_FATAL, SYSTEM, CTX, __VA_ARGS__);                          \
		/* abort() does not flush stdio, and a lost panic message is no panic */                   \
		fflush(NULL);                                                                              \
		abort();                                                                                   \
	} while (0)

#define GLOG_PANIC(SYSTEM, TEXT)		  GLOG_PANIC_AT(SYSTEM, NULL, TEXT)
#define GLOG_PANIC_CTX(SYSTEM, CTX, TEXT) GLOG_PANIC_AT(SYSTEM, CTX, TEXT)
#define GLOGF_PANIC(SYSTEM, FORMAT, ...)  GLOG_PANIC_AT(SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_PANIC_CTX(SYSTEM, CTX, FORMAT, ...)                                                  \
	GLOG_PANIC_AT(SYSTEM, CTX, FORMAT, __VA_ARGS__)

#define GLOG(TYPE, SYSTEM, TEXT) GLOG_IF(TYPE, SYSTEM, NULL, TEXT)
#define GLOG_INFO(SYSTEM, TEXT)	 GLOG_IF(LT_INFO, SYSTEM, NULL, TEXT)
#define GLOG_WARN(SYSTEM, TEXT)	 GLOG_IF(LT_WARN, SYSTEM, NULL, TEXT)
#define GLOG_ERROR(SYSTEM, TEXT) GLOG_IF(LT_ERROR, SYSTEM, NULL, TEXT)
#define GLOG_DEBUG(SYSTEM, TEXT) GLOG_IF(LT_DEBUG, SYSTEM, NULL, TEXT)
#define GLOG_TRACE(SYSTEM, TEXT) GLOG_IF(LT_TRACE, SYSTEM, NULL, TEXT)

#define GLOG_INFO_CTX(SYSTEM, CTX, TEXT)  GLOG_IF(LT_INFO, SYSTEM, CTX, TEXT)
#define GLOG_WARN_CTX(SYSTEM, CTX, TEXT)  GLOG_IF(LT_WARN, SYSTEM, CTX, TEXT)
#define GLOG_ERROR_CTX(SYSTEM, CTX, TEXT) GLOG_IF(LT_ERROR, SYSTEM, CTX, TEXT)
#define GLOG_DEBUG_CTX(SYSTEM, CTX, TEXT) GLOG_IF(LT_DEBUG, SYSTEM, CTX, TEXT)
#define GLOG_TRACE_CTX(SYSTEM, CTX, TEXT) GLOG_IF(LT_TRACE, SYSTEM, CTX, TEXT)

#define GLOGF(TYPE, SYSTEM, FORMAT, ...) GLOG_IF(TYPE, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_INFO(SYSTEM, FORMAT, ...)	 GLOG_IF(LT_INFO, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_WARN(SYSTEM, FORMAT, ...)	 GLOG_IF(LT_WARN, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_ERROR(SYSTEM, FORMAT, ...) GLOG_IF(LT_ERROR, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_DEBUG(SYSTEM, FORMAT, ...) GLOG_IF(LT_DEBUG, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_TRACE(SYSTEM, FORMAT, ...) GLOG_IF(LT_TRACE, SYSTEM, NULL, FORMAT, __VA_ARGS__)

#define GLOGF_CTX(TYPE, SYSTEM, CTX, FORMAT, ...) GLOG_IF(TYPE, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_INFO_CTX(SYSTEM, CTX, FORMAT, ...)  GLOG_IF(LT_INFO, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_WARN_CTX(SYSTEM, CTX, FORMAT, ...)  GLOG_IF(LT_WARN, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_ERROR_CTX(SYSTEM, CTX, FORMAT, ...)                                                  \
	GLOG_IF(LT_ERROR, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_DEBUG_CTX(SYSTEM, CTX, FORMAT, ...)                                                  \
	GLOG_IF(LT_DEBUG, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_TRACE_CTX(SYSTEM, CTX, FORMAT, ...)                                                  \
	GLOG_IF(LT_TRACE, SYSTEM, CTX, FORMAT, __VA_ARGS__)

#endif // !LOG_H
