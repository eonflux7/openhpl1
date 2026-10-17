// OpenHPL1: stand-ins for the ScummVM headers this Newton 2.36 copy
// was adapted to (common/scummsys.h, common/util.h, common/system.h, hpl1/debug.h).
#ifndef NEWTON_COMPAT_H
#define NEWTON_COMPAT_H

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cfloat>
#include <cassert>
#include <new>

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef unsigned int uint;
typedef uint8_t byte;

#ifndef FORCEINLINE
#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline __attribute__((always_inline))
#endif
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) ((int)(sizeof(x) / sizeof(x[0])))
#endif

[[noreturn]] inline void newton_fatal(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	std::vfprintf(stderr, fmt, va);
	va_end(va);
	std::fputc('\n', stderr);
	std::abort();
}

inline void newton_debug(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	std::vfprintf(stderr, fmt, va);
	va_end(va);
}

#define HPL1_UNIMPLEMENTED(fnName) newton_fatal("Newton: call to unimplemented function " #fnName)

#endif
