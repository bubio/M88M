#pragma once

#include "device.h"
#include <cstdint>

class IOBus;

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

    void Update();
    bool Init(IOBus* bus);

private:
    uint8_t matrix[16];
    static const Descriptor descriptor;
    static const InFuncPtr indef[];
};
