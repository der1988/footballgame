#ifndef LOG_H
#define LOG_H

#include "../types.h"

#define GLOG(TYPE, SYSTEM, TEXT) log_line(__FILE__, __LINE__, TYPE, SYSTEM, NULL, TEXT)
#define GLOG_INFO(SYSTEM, TEXT)	 log_line(__FILE__, __LINE__, LT_INFO, SYSTEM, NULL, TEXT)
#define GLOG_WARN(SYSTEM, TEXT)	 log_line(__FILE__, __LINE__, LT_WARN, SYSTEM, NULL, TEXT)
#define GLOG_ERROR(SYSTEM, TEXT) log_line(__FILE__, __LINE__, LT_ERROR, SYSTEM, NULL, TEXT)
#define GLOG_DEBUG(SYSTEM, TEXT) log_line(__FILE__, __LINE__, LT_DEBUG, SYSTEM, NULL, TEXT)
#define GLOG_TRACE(SYSTEM, TEXT) log_line(__FILE__, __LINE__, LT_TRACE, SYSTEM, NULL, TEXT)

#define GLOG_INFO_CTX(SYSTEM, CTX, TEXT)  log_line(__FILE__, __LINE__, LT_INFO, SYSTEM, CTX, TEXT)
#define GLOG_WARN_CTX(SYSTEM, CTX, TEXT)  log_line(__FILE__, __LINE__, LT_WARN, SYSTEM, CTX, TEXT)
#define GLOG_ERROR_CTX(SYSTEM, CTX, TEXT) log_line(__FILE__, __LINE__, LT_ERROR, SYSTEM, CTX, TEXT)
#define GLOG_DEBUG_CTX(SYSTEM, CTX, TEXT) log_line(__FILE__, __LINE__, LT_DEBUG, SYSTEM, CTX, TEXT)
#define GLOG_TRACE_CTX(SYSTEM, CTX, TEXT) log_line(__FILE__, __LINE__, LT_TRACE, SYSTEM, CTX, TEXT)

#define GLOGF(TYPE, SYSTEM, FORMAT, ...)                                                           \
	log_line(__FILE__, __LINE__, TYPE, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_INFO(SYSTEM, FORMAT, ...)                                                            \
	log_line(__FILE__, __LINE__, LT_INFO, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_WARN(SYSTEM, FORMAT, ...)                                                            \
	log_line(__FILE__, __LINE__, LT_WARN, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_ERROR(SYSTEM, FORMAT, ...)                                                           \
	log_line(__FILE__, __LINE__, LT_ERROR, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_DEBUG(SYSTEM, FORMAT, ...)                                                           \
	log_line(__FILE__, __LINE__, LT_DEBUG, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_TRACE(SYSTEM, FORMAT, ...)                                                           \
	log_line(__FILE__, __LINE__, LT_TRACE, SYSTEM, NULL, FORMAT, __VA_ARGS__)

#define GLOGF_CTX(TYPE, SYSTEM, CTX, FORMAT, ...)                                                  \
	log_line(__FILE__, __LINE__, TYPE, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_INFO_CTX(SYSTEM, CTX, FORMAT, ...)                                                   \
	log_line(__FILE__, __LINE__, LT_INFO, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_WARN_CTX(SYSTEM, CTX, FORMAT, ...)                                                   \
	log_line(__FILE__, __LINE__, LT_WARN, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_ERROR_CTX(SYSTEM, CTX, FORMAT, ...)                                                  \
	log_line(__FILE__, __LINE__, LT_ERROR, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_DEBUG_CTX(SYSTEM, CTX, FORMAT, ...)                                                  \
	log_line(__FILE__, __LINE__, LT_DEBUG, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_TRACE_CTX(SYSTEM, CTX, FORMAT, ...)                                                  \
	log_line(__FILE__, __LINE__, LT_TRACE, SYSTEM, CTX, FORMAT, __VA_ARGS__)

enum log_type {
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
	LS_TWEAKUI,
	LS_LAYERS,
	LS_OTHER,
	LS_SCENES,
	LS_EVENTS,
	LS_TIME_MACHINE,
	LS_VCRUI,
	LS_COUNT,
};

extern const char *utils_log_systems_desc[LS_COUNT];

void log_line(char *file,
			  u16 line,
			  enum log_type type,
			  enum log_system system,
			  const char *ctx,
			  const char *format,
			  ...);

#endif // !LOG_H
