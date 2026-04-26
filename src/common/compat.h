// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Cross-platform compatibility shims for the emulation core.
//
//	Lets src/common, src/devices, src/pc88, src/if compile cleanly on
//	macOS / Linux / MinGW / MSVC without dragging in <windows.h>.
// ----------------------------------------------------------------------------
#pragma once

// --- Calling-convention macros -------------------------------------------
//
//	Original M88 used __stdcall for the I/F vtables (interop with the
//	x86 inline-assembler Z80 core). On non-MSVC / non-x86-32 we drop them.
//
#if defined(_WIN32) && defined(_MSC_VER) && !defined(_WIN64) && defined(USE_Z80_X86)
  #ifndef IFCALL
    #define IFCALL  __stdcall
  #endif
  #ifndef IOCALL
    #define IOCALL  __stdcall
  #endif
#else
  #ifndef IFCALL
    #define IFCALL
  #endif
  #ifndef IOCALL
    #define IOCALL
  #endif
#endif

// --- Endian --------------------------------------------------------------
#if !defined(ENDIAN_IS_SMALL) && !defined(ENDIAN_IS_BIG)
  #if defined(__BYTE_ORDER__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      #define ENDIAN_IS_SMALL
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      #define ENDIAN_IS_BIG
    #endif
  #else
    // x86, x64, arm64 default to little-endian on supported targets.
    #define ENDIAN_IS_SMALL
  #endif
#endif

// --- POSIX/CRT compatibility ---------------------------------------------
#if !defined(_WIN32)

  #include <limits.h>     // PATH_MAX
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <strings.h>    // strcasecmp
  #include <stdarg.h>
  #include <ctype.h>

  #ifndef MAX_PATH
    #define MAX_PATH PATH_MAX
  #endif

  #ifndef _MAX_PATH
    #define _MAX_PATH PATH_MAX
  #endif
  #ifndef _MAX_DRIVE
    #define _MAX_DRIVE 8
  #endif
  #ifndef _MAX_DIR
    #define _MAX_DIR  PATH_MAX
  #endif
  #ifndef _MAX_FNAME
    #define _MAX_FNAME 256
  #endif
  #ifndef _MAX_EXT
    #define _MAX_EXT  64
  #endif
  #ifndef _TRUNCATE
    #define _TRUNCATE ((size_t)-1)
  #endif

  #ifndef _T
    #define _T(s) s
  #endif
  #ifndef TEXT
    #define TEXT(s) s
  #endif

  // Microsoft "_s" safe-string family → POSIX equivalents.
  #define _stricmp        strcasecmp
  #define stricmp         strcasecmp
  #define _strnicmp       strncasecmp
  #define strnicmp        strncasecmp

  static inline int m88_strcpy_s(char* dst, size_t cap, const char* src) {
      if (!dst || cap == 0) return 22;
      size_t n = strlen(src);
      if (n + 1 > cap) { dst[0] = 0; return 34; }
      memcpy(dst, src, n + 1);
      return 0;
  }
  static inline int m88_strncpy_s(char* dst, size_t cap, const char* src, size_t cnt) {
      if (!dst || cap == 0) return 22;
      size_t n = strlen(src);
      if (cnt != _TRUNCATE && cnt < n) n = cnt;
      if (n + 1 > cap) { n = cap - 1; }
      memcpy(dst, src, n);
      dst[n] = 0;
      return 0;
  }
  static inline int m88_strcat_s(char* dst, size_t cap, const char* src) {
      if (!dst || cap == 0) return 22;
      size_t l = strlen(dst);
      if (l >= cap) return 22;
      return m88_strcpy_s(dst + l, cap - l, src);
  }
  static inline int m88_strncat_s(char* dst, size_t cap, const char* src, size_t cnt) {
      if (!dst || cap == 0) return 22;
      size_t l = strlen(dst);
      if (l >= cap) return 22;
      return m88_strncpy_s(dst + l, cap - l, src, cnt);
  }
  #define strcpy_s        m88_strcpy_s
  #define strncpy_s       m88_strncpy_s
  #define strcat_s        m88_strcat_s
  #define strncat_s       m88_strncat_s

  // sprintf_s / vsprintf_s collapse to the bounded snprintf family.
  #define sprintf_s       snprintf
  #define vsprintf_s      vsnprintf
  #define _snprintf       snprintf
  #define _vsnprintf      vsnprintf

  // Microsoft path-splitter is rare in core; provide a minimal shim.
  static inline void _splitpath(const char* path, char* drv, char* dir,
                                char* fn, char* ext) {
      if (drv) drv[0] = 0;
      const char* slash = strrchr(path, '/');
      const char* base  = slash ? slash + 1 : path;
      if (dir) {
          size_t n = (size_t)(base - path);
          if (n >= _MAX_DIR) n = _MAX_DIR - 1;
          memcpy(dir, path, n); dir[n] = 0;
      }
      const char* dot = strrchr(base, '.');
      if (!dot) {
          if (fn)  m88_strcpy_s(fn,  _MAX_FNAME, base);
          if (ext) ext[0] = 0;
      } else {
          size_t n = (size_t)(dot - base);
          if (fn) {
              if (n >= _MAX_FNAME) n = _MAX_FNAME - 1;
              memcpy(fn, base, n); fn[n] = 0;
          }
          if (ext) m88_strcpy_s(ext, _MAX_EXT, dot);
      }
  }

  // Microsoft localtime_s(struct tm*, const time_t*) → POSIX localtime_r,
  // which has the argument order reversed. Wrap so callers compile unchanged.
  #include <time.h>
  static inline int m88_localtime_s(struct tm* out, const time_t* t) {
      return localtime_r(t, out) ? 0 : 1;
  }
  #define localtime_s m88_localtime_s

  // Win32 kernel32!MulDiv computes (a * b) / c with 64-bit intermediate.
  static inline int MulDiv(int a, int b, int c) {
      if (c == 0) return -1;
      long long n = (long long)a * (long long)b;
      // Round to nearest, away from zero — matches Win32 semantics.
      long long q = n / c;
      long long r = n % c;
      if (r * 2 >= c) ++q;
      else if (r * 2 <= -c) --q;
      return (int)q;
  }

  // OutputDebugString is occasionally used in debug logging; no-op it.
  #define OutputDebugString(s) ((void)0)
  #define OutputDebugStringA(s) ((void)0)

#endif  // !_WIN32

// --- Minimal POINT / GUID / REFIID / DEFINE_GUID for non-Windows ---------
//	The interfaces that survive on non-Windows still mention these as
//	parameter types (e.g. ISystem::QueryIF takes REFIID). Provide ABI-light
//	stand-ins so headers parse without pulling in <windows.h>.
//
#if !defined(_WIN32)

  struct M88_GUID {
      unsigned int  d1;
      unsigned short d2;
      unsigned short d3;
      unsigned char  d4[8];
  };
  inline bool operator==(const M88_GUID& a, const M88_GUID& b) {
      return a.d1 == b.d1 && a.d2 == b.d2 && a.d3 == b.d3 &&
             __builtin_memcmp(a.d4, b.d4, 8) == 0;
  }
  inline bool operator!=(const M88_GUID& a, const M88_GUID& b) { return !(a == b); }

  typedef M88_GUID GUID;
  typedef const GUID& REFIID;
  #ifndef IID
    #define IID GUID
  #endif

  #define DEFINE_GUID(name, l, w1, w2, b1,b2,b3,b4,b5,b6,b7,b8) \
      static const GUID name = { (unsigned int)(l), (unsigned short)(w1), (unsigned short)(w2), \
                                 { (unsigned char)(b1),(unsigned char)(b2),(unsigned char)(b3),(unsigned char)(b4), \
                                   (unsigned char)(b5),(unsigned char)(b6),(unsigned char)(b7),(unsigned char)(b8) } }

  struct POINT { long x, y; };
  struct RECT  { long left, top, right, bottom; };

  typedef long          LONG;
  typedef unsigned long DWORD;
  typedef unsigned int  UINT;
  typedef long          HRESULT;
  typedef int           BOOL;
  typedef long          LONG_PTR;

  #ifndef FAILED
    #define FAILED(hr) ((HRESULT)(hr) < 0)
  #endif
  #ifndef SUCCEEDED
    #define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
  #endif
  #ifndef S_OK
    #define S_OK ((HRESULT)0)
  #endif
  #ifndef E_FAIL
    #define E_FAIL ((HRESULT)0x80004005L)
  #endif

#endif  // !_WIN32

// --- COM-style 'interface' keyword ---------------------------------------
//	On Windows <objbase.h> defines `interface` as `struct`. The portable
//	build doesn't pull that in, so emulate it.
//
#if !defined(_WIN32)
  #ifndef interface
    #define interface struct
  #endif
#endif

// --- Cross-platform Point used by ifui.h ---------------------------------
//	Use struct Point (lowercase) for new code; legacy core code uses POINT.
//
struct Point { int x, y; };
