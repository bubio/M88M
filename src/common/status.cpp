// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Portable StatusDisplay implementation.
// ----------------------------------------------------------------------------
#include "status.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <chrono>

StatusDisplay statusdisplay;

namespace {
long long NowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
}

StatusDisplay::StatusDisplay()
    : priority_(0), duration_ms_(0), expire_at_ms_(0)
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
    const long long now = NowMs();

    // 表示中で、かつ優先度がより高いメッセージは上書きしない
    if (msg_[0] && priority_ > priority && now < expire_at_ms_)
        return false;

    va_list ap;
    va_start(ap, msg);
    vsnprintf(msg_, sizeof(msg_), msg, ap);
    va_end(ap);

    priority_    = priority;
    duration_ms_ = duration;
    // duration <= 0 は「期限なし」(毎フレーム更新されるデバッグ表示などが使う)。
    // ここで now を入れてしまうと初回のポーリングで即座に消えてしまう
    expire_at_ms_ = (duration > 0) ? now + duration : 0;
    return true;
}

bool StatusDisplay::GetCurrentMessage(char* dst, size_t cap, int* duration_ms_out)
{
    CriticalSection::Lock lock(cs_);
    if (!msg_[0] || !dst || cap == 0) return false;

    long long remaining = -1;   // 期限なし
    if (expire_at_ms_ != 0)
    {
        remaining = expire_at_ms_ - NowMs();
        if (remaining <= 0)
        {
            msg_[0]      = 0;
            priority_    = 0;
            duration_ms_ = 0;
            return false;
        }
    }

    size_t n = strlen(msg_);
    if (n + 1 > cap) n = cap - 1;
    memcpy(dst, msg_, n);
    dst[n] = 0;
    if (duration_ms_out) *duration_ms_out = (int)remaining;
    return true;
}

int StatusDisplay::GetFDState(uint dr) const
{
    if (dr >= 3) return 0;
    CriticalSection::Lock lock(cs_);
    return litstat_[dr];
}
