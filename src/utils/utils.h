#ifndef UTILS_H
#define UTILS_H

#include "../types.h"
#include "components/components.h"
#include <raylib.h>
#include <stdarg.h>

#define ARRAY_LEN(A) sizeof(A) / sizeof(A[0])
#define V2_TO_V3(vector2, height)                                                                  \
    (v3) { .x = vector2.x, .y = height, .z = vector2.y }
#define V3_TO_V2(vector3)                                                                          \
    (v2) { .x = vector3.x, .y = vector3.z }
#define STRINGIFY(v) #v
#define JSON_GET_INT(J, K) cJSON_GetObjectItemCaseSensitive(J, K)->valueint;
#define JSON_GET_DOUBLE(J, K) cJSON_GetObjectItemCaseSensitive(J, K)->valuedouble;
#define JSON_GET_STRING(J, K) cJSON_GetObjectItemCaseSensitive(J, K)->valuestring;

inline __attribute__((always_inline)) Vector2 Vector2Left() {
    Vector2 v = {.x = -1, .y = 0};
    return v;
}

inline __attribute__((always_inline)) Vector2 Vector2Right() {
    Vector2 v = {.x = 1, .y = 0};
    return v;
}

inline __attribute__((always_inline)) Vector2 Vector2Up() {
    Vector2 v = {.x = 0, .y = -1};
    return v;
}

inline __attribute__((always_inline)) Vector2 Vector2Down() {
    Vector2 v = {.x = 0, .y = 1};
    return v;
}

#define GLOG(TYPE, SYSTEM, TEXT) utils_log(__FILE__, __LINE__, TYPE, SYSTEM, NULL, TEXT)
#define GLOG_INFO(SYSTEM, TEXT) utils_log(__FILE__, __LINE__, LT_INFO, SYSTEM, NULL, TEXT)
#define GLOG_WARN(SYSTEM, TEXT) utils_log(__FILE__, __LINE__, LT_WARN, SYSTEM, NULL, TEXT)
#define GLOG_ERROR(SYSTEM, TEXT) utils_log(__FILE__, __LINE__, LT_ERROR, SYSTEM, NULL, TEXT)
#define GLOG_DEBUG(SYSTEM, TEXT) utils_log(__FILE__, __LINE__, LT_DEBUG, SYSTEM, NULL, TEXT)
#define GLOG_TRACE(SYSTEM, TEXT) utils_log(__FILE__, __LINE__, LT_TRACE, SYSTEM, NULL, TEXT)

#define GLOG_INFO_CTX(SYSTEM, CTX, TEXT) utils_log(__FILE__, __LINE__, LT_INFO, SYSTEM, CTX, TEXT)
#define GLOG_WARN_CTX(SYSTEM, CTX, TEXT) utils_log(__FILE__, __LINE__, LT_WARN, SYSTEM, CTX, TEXT)
#define GLOG_ERROR_CTX(SYSTEM, CTX, TEXT) utils_log(__FILE__, __LINE__, LT_ERROR, SYSTEM, CTX, TEXT)
#define GLOG_DEBUG_CTX(SYSTEM, CTX, TEXT) utils_log(__FILE__, __LINE__, LT_DEBUG, SYSTEM, CTX, TEXT)
#define GLOG_TRACE_CTX(SYSTEM, CTX, TEXT) utils_log(__FILE__, __LINE__, LT_TRACE, SYSTEM, CTX, TEXT)

#define GLOGF(TYPE, SYSTEM, FORMAT, ...)                                                           \
    utils_log(__FILE__, __LINE__, TYPE, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_INFO(SYSTEM, FORMAT, ...)                                                            \
    utils_log(__FILE__, __LINE__, LT_INFO, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_WARN(SYSTEM, FORMAT, ...)                                                            \
    utils_log(__FILE__, __LINE__, LT_WARN, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_ERROR(SYSTEM, FORMAT, ...)                                                           \
    utils_log(__FILE__, __LINE__, LT_ERROR, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_DEBUG(SYSTEM, FORMAT, ...)                                                           \
    utils_log(__FILE__, __LINE__, LT_DEBUG, SYSTEM, NULL, FORMAT, __VA_ARGS__)
#define GLOGF_TRACE(SYSTEM, FORMAT, ...)                                                           \
    utils_log(__FILE__, __LINE__, LT_TRACE, SYSTEM, NULL, FORMAT, __VA_ARGS__)

#define GLOGF_CTX(TYPE, SYSTEM, CTX, FORMAT, ...)                                                  \
    utils_log(__FILE__, __LINE__, TYPE, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_INFO_CTX(SYSTEM, CTX, FORMAT, ...)                                                   \
    utils_log(__FILE__, __LINE__, LT_INFO, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_WARN_CTX(SYSTEM, CTX, FORMAT, ...)                                                   \
    utils_log(__FILE__, __LINE__, LT_WARN, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_ERROR_CTX(SYSTEM, CTX, FORMAT, ...)                                                  \
    utils_log(__FILE__, __LINE__, LT_ERROR, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_DEBUG_CTX(SYSTEM, CTX, FORMAT, ...)                                                  \
    utils_log(__FILE__, __LINE__, LT_DEBUG, SYSTEM, CTX, FORMAT, __VA_ARGS__)
#define GLOGF_TRACE_CTX(SYSTEM, CTX, FORMAT, ...)                                                  \
    utils_log(__FILE__, __LINE__, LT_TRACE, SYSTEM, CTX, FORMAT, __VA_ARGS__)

typedef enum {
    LT_ERROR,
    LT_WARN,
    LT_INFO,
    LT_DEBUG,
    LT_TRACE,
    LT_COUNT,
} ULogType;

typedef enum {
    LS_GAME,
    LS_INPUT,
    LS_SOUND,
    LS_ASSETS,
    LS_ENTITIES,
    LS_MEMORY,
    LS_UTILS,
    LS_TESTS,
    LS_TWEAKUI,
    LS_OTHER,
    LS_COUNT,
} ULogSystem;

void utils_log(char *file, u16 line, ULogType type, ULogSystem system, const char *ctx,
               const char *format, ...);

usize utils_align(usize to_align, u16 alignment);

Color utils_color_lerp(Color from, Color to, f32 t);

f32 utils_math_constrain_angle_360(f32 angle);
f32 utils_math_constrain_angle_180(f32 angle);
v2 Vector2RotateAroundPivot(v2 vector, v2 pivot, f32 angle);
float Vector2ProjectScalar(Vector2 project_to, Vector2 project_from);
Vector2 Vector2Project(Vector2 project_to, Vector2 project_from);

#endif // !UTILS_H
