#ifndef MACRO_H
#define MACRO_H

#include <raylib.h>

#define UNUSED(x) (void)(x)

#define ARRAY_LEN(A) sizeof(A) / sizeof(A[0])

#define STRINGIFY(v) #v

#define HAS_TIME_PASSED(current_time, saved_time, threshold)                                       \
    ((current_time - saved_time) > threshold)
#define IS_TIME_PASSED_IN_RANGE(current_time, saved_time, min, max)                                \
    ((current_time - saved_time) > min && (current_time - saved_time) < max)

#define JSON_GET_INT(J, K)    cJSON_GetObjectItemCaseSensitive(J, K)->valueint;
#define JSON_GET_DOUBLE(J, K) cJSON_GetObjectItemCaseSensitive(J, K)->valuedouble;
#define JSON_GET_STRING(J, K) cJSON_GetObjectItemCaseSensitive(J, K)->valuestring;

#endif // !MACRO_H
