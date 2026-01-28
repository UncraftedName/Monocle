#pragma once

#include <utility>
#include <assert.h>

#define MON_F_FMT "{:.9g}"
#define MON_D_FMT "{:.17g}"

#define MON_ASSERT(x) assert(x)
#define MON_ASSERT_MSG(x, msg) assert((msg, (x)))

#ifdef __cpp_lib_unreachable
#define MON_UNREACHABLE() std::unreachable()
#else
#define MON_UNREACHABLE() \
    do {                  \
        MON_ASSERT(0);    \
        __assume(0);      \
    } while (0)

#endif
