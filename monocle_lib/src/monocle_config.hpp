#pragma once

#define MON_F_FMT "{:.9g}"
#define MON_D_FMT "{:.17g}"

#include <assert.h>
#define MON_ASSERT(x) assert(x)
#define MON_ASSERT_MSG(x, msg) assert((msg, (x)))
