#ifndef TYPES_H
#define TYPES_H

#include <raylib.h>
#include <stddef.h>
#include <stdint.h>

// General
#define Kilobytes(S)  (S * 1024)
#define Megabytes(S)  (S * 1024 * 1024)
#define Gigabytes(S)  ((u64)S * 1024 * 1024 * 1024)
#define EID_NOT_VALID 0xffffffff

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef int32_t b32;
typedef int32_t i32;
typedef int64_t i64;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef uintptr_t uptr;
typedef char byte;
typedef ptrdiff_t size;
typedef size_t usize;

// Raylib
typedef struct Vector2 v2;
typedef struct Vector3 v3;
typedef struct Color color_t;
typedef struct Rectangle rect_t;
typedef struct Font font_t;
typedef struct Camera2D camera2d_t;
typedef struct RenderTexture render_texture_t;
typedef struct Texture texture_t;
typedef struct Shader shader_t;
typedef struct Ray ray_t;
typedef struct RayCollision ray_collision_t;

// Game
typedef u32 entity_id_t;
typedef f64 time_t;
typedef f32 time_low_prec_t;
typedef time_t delta_time_t;
typedef i16 running_action_idx_t;
typedef u32 human_id_t;
typedef u32 game_controller_id_t;

#endif // !TYPES_H
