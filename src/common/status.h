// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Portable StatusDisplay (replaces win32/status.h on cross-platform builds).
//
//	Core code uses only Show() / FDAccess() / WaitSubSys() / UpdateDisplay()
//	and the global `statusdisplay`. The Win32 build still resolves "status.h"
//	to src/win32/status.h via the PCH search path; on macOS/Linux/MinGW this
//	header replaces it with a thin in-memory store the raylib frontend can
//	poll for the OSD ticker.
// ----------------------------------------------------------------------------
#pragma once

#include "types.h"
#include "critsect.h"

class StatusDisplay
{
public:
    StatusDisplay();
    ~StatusDisplay();

    // Lifecycle (no-op in portable build).
    bool Init()    { return true; }
    void Cleanup() {}

    bool Enable(bool /*sfs*/ = false) { return true; }
    bool Disable()                    { return true; }

    int  GetHeight() const            { return 0; }

    void FDAccess(uint dr, bool hd, bool active);
    void UpdateDisplay() {}
    void WaitSubSys()    { litstat_[2] = 9; }

    // Posts a transient message to the OSD ticker. Returns true if accepted.
    bool Show(int priority, int duration, const char* msg, ...);
    void Update() {}

    // --- Frontend-facing accessors (portable extension) ---
    // Pull the current message + remaining duration in milliseconds.
    // Thread-safe: takes the internal critical section.
    bool GetCurrentMessage(char* dst, size_t cap, int* duration_ms_out);

    // FD access lamp state for drive `dr` (0..3). bit0=on, bit1=hd-mode hint.
    int  GetFDState(uint dr) const;

private:
    mutable CriticalSection cs_;

    char    msg_[128];
    int     priority_;
    int     duration_ms_;

    int     litstat_[3];   // [0]=FD0, [1]=FD1, [2]=Subsys wait
    int     litcurrent_[3];
};

extern StatusDisplay statusdisplay;
