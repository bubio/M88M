#pragma once

#include "pc88.h"
#include "diskmgr.h"
#include "tapemgr.h"
#include "audio_out.h"
#include <thread>
#include <atomic>

class CoreRunner {
public:
    CoreRunner();
    ~CoreRunner();

    bool Init(Draw* draw);
    void Start();
    void Stop();
    void Pause(bool pause);

    PC88* GetPC88() { return &pc88; }
    DiskManager* GetDiskManager() { return &diskmgr; }

private:
    void Run();

    PC88 pc88;
    DiskManager diskmgr;
    TapeManager tapemgr;
    RaylibSound sound;
    
    std::thread thread;
    std::atomic<bool> running;
    std::atomic<bool> paused;
};
