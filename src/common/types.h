// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Cross-platform fixed-width types and feature switches.
// ----------------------------------------------------------------------------
#pragma once

#include "compat.h"

// --- Fixed-width aliases used throughout the codebase -------------------
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;

typedef signed char    sint8;
typedef signed short   sint16;
typedef signed int     sint32;

typedef signed char    int8;
typedef signed short   int16;
typedef signed int     int32;

// --- 8-bit values packed for batch writes -------------------------------
typedef uint32 packed;
#define PACK(p) ((p) | ((p) << 8) | ((p) << 16) | ((p) << 24))

// --- Pointer-sized integer ----------------------------------------------
#if defined(_WIN32)
  typedef LONG_PTR intpointer;
#else
  #include <stdint.h>
  typedef intptr_t  intpointer;
#endif

// --- Z80 engine selection -----------------------------------------------
//	The x86 inline-assembler Z80 engine is only viable on 32-bit MSVC and
//	requires the function-pointer low-bit trick (PTR_IDBIT). Everywhere
//	else we use the portable C++ engine (Z80c.cpp).
//
#if defined(_MSC_VER) && !defined(_WIN64) && !defined(M88_NO_Z80_X86)
  #define USE_Z80_X86
  #if defined(_DEBUG)
    #define PTR_IDBIT  0x80000000
  #else
    #define PTR_IDBIT  0x1
  #endif
#endif

// --- Misc feature flags --------------------------------------------------
#define ALLOWBOUNDARYACCESS

// MSVC accepted `static_cast<Device::OutFuncPtr>(&MemberFn)` without an
// explicit class qualification; Clang/GCC require `&Class::MemberFn`.
// Rather than touch hundreds of call sites, fall back to a C-style cast on
// non-MSVC compilers — STATIC_CAST is only used to project member-function
// pointers into the type-erased descriptor tables, so the safety loss is
// contained.
#if defined(_MSC_VER)
  #define USE_NEW_CAST
#endif

#ifdef USE_Z80_X86
  #define MEMCALL __stdcall
#else
  #define MEMCALL
#endif

#if defined(USE_NEW_CAST) && defined(__cplusplus)
  #define STATIC_CAST(t, o)       static_cast<t>(o)
  #define REINTERPRET_CAST(t, o)  reinterpret_cast<t>(o)
#else
  #define STATIC_CAST(t, o)       ((t)(o))
  #define REINTERPRET_CAST(t, o)  (*(t*)(void*)&(o))
#endif
