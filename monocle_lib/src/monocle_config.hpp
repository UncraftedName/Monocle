#pragma once

#ifdef MON_CFG_INCLUDE
#include MON_CFG_INCLUDE
#endif

#define MON_LIB_VERSION_MAJOR 1
#define MON_LIB_VERSION_MINOR 0

#ifndef MON_F_FMT
#define MON_F_FMT "{:.9g}"
#endif
#ifndef MON_D_FMT
#define MON_D_FMT "{:.17g}"
#endif

#if !defined(MON_ASSERT) || !defined(MON_ASSERT_MSG)
#include <assert.h>
#ifndef MON_ASSERT
#define MON_ASSERT(x) assert(x)
#endif
#ifndef MON_ASSERT_MSG
#define MON_ASSERT_MSG(x, msg) assert((msg, (x)))
#endif
#endif

#ifndef MON_UNREACHABLE
#include <utility>
#ifdef __cpp_lib_unreachable
#define MON_UNREACHABLE() std::unreachable()
#else
#define MON_UNREACHABLE() \
    do {                  \
        MON_ASSERT(0);    \
        __assume(0);      \
    } while (0)
#endif
#endif
