#pragma once

#include "device.h"
#include <cstdint>

class PC88;

class KeyInput : public Device {
public:
    KeyInput();
    virtual ~KeyInput();

    // IDevice implementation
    const Descriptor* IFCALL GetDesc() const override;
    uint IFCALL GetStatusSize() override { return 0; }
    bool IFCALL LoadStatus(const uint8* status) override { return true; }
    bool IFCALL SaveStatus(uint8* status) override { return true; }

    // IO Handlers
    uint IOCALL In(uint port);

    void Update(PC88* pc88);
    bool Init(PC88* pc88);

private:
    uint8_t matrix[16];
    static const Descriptor descriptor;
    static const InFuncPtr indef[];
};
