#ifndef TYPES_H
#define TYPES_H

#include <raylib.h>
#include <stddef.h>
#include <stdint.h>

// General
#define Kilobytes(S) (S * 1024)
#define Megabytes(S) (S * 1024 * 1024)
#define Gigabytes(S) ((u64)S * 1024 * 1024 * 1024)

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t i16;
typedef int32_t b32;
typedef int32_t i32;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef uintptr_t uptr;
typedef char byte;
typedef ptrdiff_t size;
typedef size_t usize;

// Raylib
typedef Vector2 v2;
typedef Vector3 v3;

// Game
typedef u32 EntityId;
typedef f64 Time;
typedef f32 DeltaTime;
typedef i16 RunningActionIdx;

#endif // !TYPES_H
