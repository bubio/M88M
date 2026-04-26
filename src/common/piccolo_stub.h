// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Stub for the c86ctl/Romeo passthrough interface used by OPNIF.
//
//	The real Piccolo lives under src/win32/romeo/ and bridges to the
//	c86ctl.dll real-OPNA hardware driver, which only exists on Windows.
//	On macOS/Linux we fall back to fmgen's emulated OPNA — opnif.cpp checks
//	`piccolo` against null before using it, so a stub that fails to acquire
//	an instance suffices.
// ----------------------------------------------------------------------------
#pragma once

#include "types.h"

enum PICCOLO_CHIPTYPE
{
    PICCOLO_INVALID = 0,
    PICCOLO_YMF288,
    PICCOLO_YM2608,
};

class PiccoloChip
{
public:
    virtual ~PiccoloChip() = default;
    virtual int  Init(uint)                          { return -1; }
    virtual void Reset(bool)                         {}
    virtual bool SetReg(uint32, uint, uint)          { return false; }
    virtual void SetChannelMask(uint)                {}
    virtual void SetVolume(int, int)                 {}
};

class Piccolo
{
public:
    enum PICCOLO_ERROR
    {
        PICCOLO_SUCCESS = 0,
        PICCOLOE_UNKNOWN = -32768,
        PICCOLOE_DLL_NOT_FOUND,
        PICCOLOE_ROMEO_NOT_FOUND,
        PICCOLOE_HARDWARE_NOT_AVAILABLE,
        PICCOLOE_HARDWARE_IN_USE,
        PICCOLOE_TIME_OUT_OF_RANGE,
        PICCOLOE_THREAD_ERROR,
    };

    virtual ~Piccolo() = default;

    // The portable build never exposes real hardware: GetInstance() returns
    // null so opnif.cpp's `if (piccolo)` guard skips the entire path.
    static Piccolo* GetInstance() { return nullptr; }
    static void     DeleteInstance() {}

    bool   SetLatencyBufferSize(uint)        { return false; }
    bool   SetMaximumLatency(uint)           { return false; }
    uint32 GetCurrentTime()                  { return 0; }
    int    IsDriverBased()                   { return 0; }

    virtual int  GetChip(PICCOLO_CHIPTYPE, PiccoloChip**) { return -1; }
    virtual void SetReg(uint, uint) {}
};
