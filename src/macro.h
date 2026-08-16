#ifndef MACRO_H
#define MACRO_H

#include "constants.h"
#include "log.h"

// Replaces <assert.h>: the condition is evaluated in every build and a failure
// aborts rather than being compiled away by NDEBUG.
#define ASSERT(COND)                                                                               \
	do {                                                                                           \
		if (!(COND)) {                                                                             \
			GLOGF_PANIC(LS_ASSERT, "%s", #COND);                                                   \
		}                                                                                          \
	} while (0)

#define UNUSED(x)	 (void)(x)
#define UNUSED_PARAM __attribute__((unused))

// `static` matters: these are defined in headers and included by many
// translation units. Without it they would have external linkage, which makes
// referencing a file-static from one of them a compiler extension, and taking
// its address a link error.
#define INLINE static inline __attribute__((always_inline))

#define ARRAY_LEN(A) sizeof(A) / sizeof(A[0])

#define STRINGIFY(v) #v

#define HAS_TIME_PASSED(current_time, saved_time, threshold)                                       \
	((current_time - saved_time) > threshold)
#define IS_TIME_PASSED_IN_RANGE(current_time, saved_time, min, max)                                \
	((current_time - saved_time) > min && (current_time - saved_time) < max)

#define JSON_GET_INT(J, K)	  cJSON_GetObjectItemCaseSensitive(J, K)->valueint;
#define JSON_GET_DOUBLE(J, K) cJSON_GetObjectItemCaseSensitive(J, K)->valuedouble;
#define JSON_GET_STRING(J, K) cJSON_GetObjectItemCaseSensitive(J, K)->valuestring;

#define VELOCITY_TO_KMH(V) (V / PIXELS_PER_METER * 3.6f)

#define RESET_COMPONENT(c)                                                                         \
	memset(((void *)c) + sizeof(struct component_header),                                          \
		   0,                                                                                      \
		   sizeof(*c) - sizeof(struct component_header));

#define OPPONENT_TEAM_SIDE(t) 1 - t

// List
#define LIST_CREATE(list_type, list_name, list_size, arena)                                        \
	list_type list_name = {.array = NULL, .count = 0, .size = list_size};                          \
	list_name.array = memory_arena_push(arena, sizeof(*list_name.array) * list_size);              \
	ASSERT(list_name.array);

#define LIST_ADD(list, item)                                                                       \
	ASSERT(list.count < list.size);                                                                \
	list.array[list.count++] = item;

#define LIST_RESET(list) list.count = 0;

#define LIST_ITERATE(item, list)                                                                   \
	usize item##_i;                                                                                \
	__typeof__(*list.array) item;                                                                  \
	if (list.count > 0)                                                                            \
		item = list.array[0];                                                                      \
	for (item##_i = 0; item##_i < list.count; ++item##_i, item = list.array[item##_i])

// Game
#define GOALIE_HOLD_OR_BOUNCE(V)                                                                   \
	(V < (20 * PIXELS_PER_METER) || (V < (30 * PIXELS_PER_METER) && GetRandomValue(0, 99) < 33))

#define SHOOT_POWER(holding_time)	  Clamp01(holding_time *SHOOT_CHARGE_MULTIPLIER)
#define PASS_POWER(holding_time)	  Clamp01(holding_time *PASS_CHARGE_MULTIPLIER)
#define LONG_PASS_POWER(holding_time) Clamp01(holding_time *LONG_PASS_CHARGE_MULTIPLIER)

#define RANDOM_01(granularity) (GetRandomValue(0, granularity) / granularity##.0f)

#endif // !MACRO_H
