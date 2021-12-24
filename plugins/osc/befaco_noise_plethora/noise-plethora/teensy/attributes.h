#pragma once

#ifdef _MSC_VER
#define ATTRIBUTE_ALWAYS_INLINE
#define ATTRIBUTE_UNUSED
#else
#define ATTRIBUTE_ALWAYS_INLINE  __attribute__((always_inline, unused))
#define ATTRIBUTE_UNUSED         __attribute__((unused))
#endif // _MSC_VER
