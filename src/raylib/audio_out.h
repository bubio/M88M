#pragma once

#include "ifcommon.h"

class RaylibSound : public ISoundControl {
public:
    RaylibSound();
    virtual ~RaylibSound();

    bool Connect(ISoundSource* src) override;
    bool Disconnect(ISoundSource* src) override;
    
    bool Update(ISoundSource* src) override;
    int  GetSubsampleTime(ISoundSource* src) override;

    void Init();
    void Cleanup();
};
