#pragma once

#pragma warning(push, 1)
#define WINVER         0x0400
#define _WIN32_WINNT   0x0400
#define _WIN32_WINDOWS 0x0400
#define _WIN32_IE      0x0400

#include <windows.h>
#include <gl/gl.h>
#pragma warning(pop)

typedef unsigned __int8   u8;
typedef unsigned __int16  u16;
typedef unsigned __int32  u32;
typedef unsigned __int64  u64;

typedef __int8    s8;
typedef __int16  s16;
typedef __int32  s32;
typedef __int64  s64;

typedef float    f32;
typedef double   f64;

