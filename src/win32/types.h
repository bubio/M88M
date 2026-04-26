// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Win32 build forwarder.
//
//	The canonical type/feature definitions now live in src/common/types.h
//	so non-Windows builds (CMake/raylib frontend) can share them. The
//	original Visual Studio project still includes "types.h" via the win32
//	precompiled header search path; this stub keeps that path working.
// ----------------------------------------------------------------------------
#pragma once

#include "../common/types.h"
