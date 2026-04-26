// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Portable LOADBEGIN/LOADEND no-op shim (replaces win32/loadmon.h on
//	cross-platform builds).
//
//	The real LoadMonitor is a Win32 GDI debug window guarded by
//	ENABLE_LOADMONITOR (off in release). Core code only ever calls the
//	LOADBEGIN(name)/LOADEND(name) macros, which collapse to nothing here.
// ----------------------------------------------------------------------------
#pragma once

#define LOADBEGIN(a) ((void)0)
#define LOADEND(a)   ((void)0)
