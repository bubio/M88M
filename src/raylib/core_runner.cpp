#include "core_runner.h"
#include "paths.h"
#include "config.h"
#include "opnif.h"
#include "beep.h"
#include <chrono>
#include <unistd.h>
#include <vector>

CoreRunner::CoreRunner() : running(false), paused(false), showSettings(false) {}
CoreRunner::~CoreRunner() { Stop(); }

std::string CoreRunner::CheckMandatoryRoms(const std::string& romDir) {
    std::vector<std::string> mandatory = { "N88.ROM", "DISK.ROM", "FONT.ROM" };
    std::vector<std::string> missing;
    for (const auto& file : mandatory) {
        std::string path = romDir + "/" + file;
        if (access(path.c_str(), F_OK) != 0) missing.push_back(file);
    }
    if (missing.empty()) return "";
    std::string msg = "Mandatory ROM files are missing:\n";
    for (const auto& m : missing) msg += " - " + m + "\n";
    msg += "\nPlease place them in this folder:\n\n" + romDir;
    return msg;
}

bool CoreRunner::Init(Draw* draw) {
    std::string romDir = Paths::GetRomDir();
    romError = CheckMandatoryRoms(romDir);
    if (!romError.empty()) return false;
    chdir(romDir.c_str());
    if (!diskmgr.Init()) return false;
    if (!pc88.Init(draw, &diskmgr, &tapemgr)) return false;
    
    pc88.ApplyConfig(&Config::Get());
    // Win32オリジナルと同様、ApplyConfig後にResetを呼んで設定を反映する
    pc88.Reset();
    sound.Init();
    sound.Connect(pc88.GetOPN1());
    sound.Connect(pc88.GetOPN2());
    sound.Connect(pc88.GetBEEP());
    keyInput.Init(&pc88);
    return true;
}

void CoreRunner::UpdateInput() { if (running) keyInput.Update(); }
void CoreRunner::DrawUI() { diskDialog.Draw(&diskmgr); diskDialog.DrawSettings(Config::Get(), &showSettings); }
void CoreRunner::OpenDiskDialog(int drive) { diskDialog.OpenNativeDialog(&diskmgr, drive); }

void CoreRunner::Start() {
    if (running || HasRomError()) return;
    running = true;
    sound.Start();
    thread = std::thread(&CoreRunner::Run, this);
}

void CoreRunner::Stop() {
    if (running) {
        running = false;
        if (thread.joinable()) thread.join();
    }
}

void CoreRunner::Pause(bool p) { paused = p; }

void CoreRunner::Run() {
    while (running) {
        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        pc88.Proceed(100, 40, 100);
        pc88.UpdateScreen(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
