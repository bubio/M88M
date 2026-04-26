// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Portable critical-section primitive (replaces win32/CritSect.h).
//
//	The original Win32 wrapper is included by core sound/buffer code as
//	"critsect.h" (lowercase). On macOS/Linux/MinGW the include path picks up
//	this std::recursive_mutex-backed equivalent, preserving the
//	CriticalSection / CriticalSection::Lock RAII contract.
// ----------------------------------------------------------------------------
#ifndef M88_Common_CriticalSection_h
#define M88_Common_CriticalSection_h

#include <mutex>

class CriticalSection
{
public:
    class Lock
    {
        CriticalSection& cs;
    public:
        Lock(CriticalSection& c) : cs(c) { cs.lock(); }
        ~Lock() { cs.unlock(); }
        Lock(const Lock&) = delete;
        Lock& operator=(const Lock&) = delete;
    };

    CriticalSection() = default;
    ~CriticalSection() = default;

    void lock()   { mutex_.lock(); }
    void unlock() { mutex_.unlock(); }

private:
    std::recursive_mutex mutex_;
};

#endif
