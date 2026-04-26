// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Portable copy of the legacy diag.h shim.
//
//	The Win32 build resolves "diag.h" to src/win32/diag.h. On non-Windows,
//	the include path order picks up this header instead. Behaviour matches
//	the original release-mode contract: every LOG*/Log call collapses to a
//	no-op, so emulation cost is unchanged.
// ----------------------------------------------------------------------------
#pragma once

#include <stdio.h>

class Diag
{
public:
    Diag(const char*) {}
    ~Diag() {}
    void Put(const char*, ...) {}
    static int GetCPUTick() { return 0; }
};

// Mirrors the LOG* / Log API exposed by src/win32/diag.h in release builds.
#if defined(_DEBUG) && defined(LOGNAME)
    static Diag diag__(LOGNAME ".dmp");
    #define LOG0 diag__.Put
    #define LOG1 diag__.Put
    #define LOG2 diag__.Put
    #define LOG3 diag__.Put
    #define LOG4 diag__.Put
    #define LOG5 diag__.Put
    #define LOG6 diag__.Put
    #define LOG7 diag__.Put
    #define LOG8 diag__.Put
    #define LOG9 diag__.Put
    #define DIAGINIT(z)
    #define LOGGING
    #define Log diag__.Put
#else
    // The original Win32 diag.h numbered each LOG variant by the count of
    // substitution arguments it expected, but a few call sites in the
    // codebase pass the wrong count. Make them variadic so any pattern
    // collapses cleanly to a no-op.
    #define LOG0(...) ((void)0)
    #define LOG1(...) ((void)0)
    #define LOG2(...) ((void)0)
    #define LOG3(...) ((void)0)
    #define LOG4(...) ((void)0)
    #define LOG5(...) ((void)0)
    #define LOG6(...) ((void)0)
    #define LOG7(...) ((void)0)
    #define LOG8(...) ((void)0)
    #define LOG9(...) ((void)0)
    #define DIAGINIT(z)
    #define Log 0 ? 0 :
#endif
