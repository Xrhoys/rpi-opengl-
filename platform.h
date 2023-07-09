/* date = July 9th 2023 11:53 pm */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdlib.h>

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    r32;
typedef double   r64;

typedef int32_t  b32;

#define Assert(expression) if(!(expression)) { *(int *)0 = 0; }

#endif //PLATFORM_H
