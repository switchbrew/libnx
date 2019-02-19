/**
 * @file switch/types.h
 * @brief Various system types.
 * @copyright libnx Authors
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

/// The maximum value of a u64.
#define U64_MAX	UINT64_MAX

#ifndef SSIZE_MAX
#ifdef SIZE_MAX
#define SSIZE_MAX ((SIZE_MAX) >> 1)
#endif
#endif

typedef uint8_t u8;       ///<   8-bit unsigned integer.
typedef uint16_t u16;     ///<  16-bit unsigned integer.
typedef uint32_t u32;     ///<  32-bit unsigned integer.
typedef uint64_t u64;     ///<  64-bit unsigned integer.
typedef __uint128_t u128; ///< 128-bit unsigned integer.

typedef int8_t s8;       ///<   8-bit signed integer.
typedef int16_t s16;     ///<  16-bit signed integer.
typedef int32_t s32;     ///<  32-bit signed integer.
typedef int64_t s64;     ///<  64-bit signed integer.
typedef __int128_t s128; ///< 128-bit unsigned integer.

typedef volatile u8 vu8;     ///<   8-bit volatile unsigned integer.
typedef volatile u16 vu16;   ///<  16-bit volatile unsigned integer.
typedef volatile u32 vu32;   ///<  32-bit volatile unsigned integer.
typedef volatile u64 vu64;   ///<  64-bit volatile unsigned integer.
typedef volatile u128 vu128; ///< 128-bit volatile unsigned integer.

typedef volatile s8 vs8;     ///<   8-bit volatile signed integer.
typedef volatile s16 vs16;   ///<  16-bit volatile signed integer.
typedef volatile s32 vs32;   ///<  32-bit volatile signed integer.
typedef volatile s64 vs64;   ///<  64-bit volatile signed integer.
typedef volatile s128 vs128; ///< 128-bit volatile signed integer.

typedef u32 Handle;                 ///< Kernel object handle.
typedef u32 Result;                 ///< Function error code result type.
typedef void (*ThreadFunc)(void *); ///< Thread entrypoint function.
typedef void (*VoidFn)(void);       ///< Function without arguments nor return value.

/// Creates a bitmask from a bit number.
#ifndef BIT
#define BIT(n) (1U<<(n))
#endif

/// Packs a struct so that it won't include padding bytes.
#ifndef PACKED
#define PACKED     __attribute__((packed))
#endif

/// Marks a function as not returning, for the purposes of compiler optimization.
#ifndef NORETURN
#define NORETURN   __attribute__((noreturn))
#endif

/// Performs a dummy operation on the specified argument in order to silence compiler warnings about unused arguments.
#ifndef IGNORE_ARG
#define IGNORE_ARG(x) (void)(x)
#endif

/// Flags a function as deprecated.
#ifndef DEPRECATED
#ifndef LIBNX_NO_DEPRECATION
#define DEPRECATED __attribute__ ((deprecated))
#else
#define DEPRECATED
#endif
#endif

/// Invalid handle.
#define INVALID_HANDLE ((Handle) 0)
