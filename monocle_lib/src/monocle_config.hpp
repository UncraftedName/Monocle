#pragma once

#ifdef MON_CFG_INCLUDE_EXTRA
#include MON_CFG_INCLUDE_EXTRA
#endif

#define MON_LIB_VERSION_MAJOR 1
#define MON_LIB_VERSION_MINOR 1

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

/*
* These will be inlined in the respective classes - can be used for implicit conversions to/from
* your types. The easiest way to handle these is to create a custom header different from
* MON_CFG_INCLUDE_EXTRA and include it before any monocle headers.
*/

#ifndef MON_VECTOR_CLASS_EXTRA
#define MON_VECTOR_CLASS_EXTRA
#endif

#ifndef MON_QANGLE_CLASS_EXTRA
#define MON_QANGLE_CLASS_EXTRA
#endif

#ifndef MON_MATRIX3X4_CLASS_EXTRA
#define MON_MATRIX3X4_CLASS_EXTRA
#endif

#ifndef MON_VMATRIX_CLASS_EXTRA
#define MON_VMATRIX_CLASS_EXTRA
#endif

#ifndef MON_VPLANE_CLASS_EXTRA
#define MON_VPLANE_CLASS_EXTRA
#endif

#ifndef MON_ENTITY_CLASS_EXTRA
#define MON_ENTITY_CLASS_EXTRA
#endif

#ifndef MON_PORTAL_CLASS_EXTRA
#define MON_PORTAL_CLASS_EXTRA
#endif

#ifndef MON_PORTAL_PAIR_CLASS_EXTRA
#define MON_PORTAL_PAIR_CLASS_EXTRA
#endif
