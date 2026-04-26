// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Portable stand-in for the Win32 PCH (src/win32/headers.h).
//
//	A handful of legacy core files (lz77d.cpp, memview.cpp, mouse.cpp...)
//	historically include "headers.h" assuming the Visual Studio precompiled
//	header. On non-Windows builds the include path resolves to this file
//	instead, which only pulls the standard headers the core actually needs.
// ----------------------------------------------------------------------------
#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include "types.h"

using std::min;
using std::max;
using std::swap;
