#include "haiku_drop.h"

#ifdef __HAIKU__

#include <Application.h>
#include <Entry.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Path.h>
#include <Window.h>
#include <cstdio>
#include <mutex>
#include <vector>

namespace {
std::mutex gDroppedFilesMutex;
std::vector<std::string> gDroppedFiles;
bool gDropHandlerInstalled = false;

class M88HaikuDropFilter : public BMessageFilter {
public:
    M88HaikuDropFilter()
        : BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_SIMPLE_DATA) {
    }

    filter_result Filter(BMessage* message, BHandler**) override {
        if (!message || message->what != B_SIMPLE_DATA) return B_DISPATCH_MESSAGE;

        bool handled = false;
        for (int32 i = 0; ; i++) {
            entry_ref ref;
            if (message->FindRef("refs", i, &ref) != B_OK) break;

            BEntry entry(&ref, true);
            BPath path;
            if (entry.InitCheck() == B_OK && entry.GetPath(&path) == B_OK && path.Path()) {
                {
                    std::lock_guard<std::mutex> lock(gDroppedFilesMutex);
                    gDroppedFiles.emplace_back(path.Path());
                }
                std::fprintf(stderr, "M88M: Haiku dropped file: %s\n", path.Path());
                handled = true;
            }
        }

        return handled ? B_SKIP_MESSAGE : B_DISPATCH_MESSAGE;
    }
};
}

void HaikuInstallDropHandler() {
    if (gDropHandlerInstalled || !be_app) return;

    int32 installedCount = 0;
    for (int32 i = 0; ; i++) {
        BWindow* window = be_app->WindowAt(i);
        if (!window) break;

        if (window->Lock()) {
            window->AddCommonFilter(new M88HaikuDropFilter());
            window->Unlock();
            installedCount++;
        }
    }

    gDropHandlerInstalled = (installedCount > 0);
    std::fprintf(stderr, "M88M: Haiku drop handler installed on %ld window(s)\n", installedCount);
}

bool HaikuPollDroppedFile(std::string& path) {
    std::lock_guard<std::mutex> lock(gDroppedFilesMutex);
    if (gDroppedFiles.empty()) return false;

    path = gDroppedFiles.front();
    gDroppedFiles.erase(gDroppedFiles.begin());
    return true;
}

#else

void HaikuInstallDropHandler() {
}

bool HaikuPollDroppedFile(std::string&) {
    return false;
}

#endif
