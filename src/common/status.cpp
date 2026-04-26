// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Portable StatusDisplay implementation.
// ----------------------------------------------------------------------------
#include "status.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

StatusDisplay statusdisplay;

StatusDisplay::StatusDisplay()
    : priority_(0), duration_ms_(0)
{
    msg_[0] = 0;
    litstat_[0] = litstat_[1] = litstat_[2] = 0;
    litcurrent_[0] = litcurrent_[1] = litcurrent_[2] = 0;
}

StatusDisplay::~StatusDisplay() = default;

void StatusDisplay::FDAccess(uint dr, bool hd, bool active)
{
    if (dr >= 2) return;
    CriticalSection::Lock lock(cs_);
    litstat_[dr] = active ? (hd ? 2 : 1) : 0;
}

bool StatusDisplay::Show(int priority, int duration, const char* msg, ...)
{
    CriticalSection::Lock lock(cs_);
    if (msg_[0] && priority_ > priority && duration_ms_ > 0)
        return false;

    va_list ap;
    va_start(ap, msg);
    vsnprintf(msg_, sizeof(msg_), msg, ap);
    va_end(ap);

    priority_    = priority;
    duration_ms_ = duration;
    return true;
}

bool StatusDisplay::GetCurrentMessage(char* dst, size_t cap, int* duration_ms_out)
{
    CriticalSection::Lock lock(cs_);
    if (!msg_[0] || !dst || cap == 0) return false;
    size_t n = strlen(msg_);
    if (n + 1 > cap) n = cap - 1;
    memcpy(dst, msg_, n);
    dst[n] = 0;
    if (duration_ms_out) *duration_ms_out = duration_ms_;
    return true;
}

int StatusDisplay::GetFDState(uint dr) const
{
    if (dr >= 3) return 0;
    CriticalSection::Lock lock(cs_);
    return litstat_[dr];
}
