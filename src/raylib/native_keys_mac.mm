#include "native_keys.h"

#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h> // kVK_JIS_Yen, kVK_JIS_Underscore

static bool yenDown = false;
static bool roDown = false;

// アプリ内に配送されるキーイベントを横取りせずに覗き見る。
// ローカルモニタなので入力監視の権限は不要。
static void InstallMonitor() {
    static bool installed = false;
    if (installed) return;
    installed = true;

    [NSEvent addLocalMonitorForEventsMatchingMask:(NSEventMaskKeyDown | NSEventMaskKeyUp)
                                          handler:^NSEvent*(NSEvent* event) {
        bool down = [event type] == NSEventTypeKeyDown;
        switch ([event keyCode]) {
        case kVK_JIS_Yen:        yenDown = down; break;
        case kVK_JIS_Underscore: roDown = down;  break;
        }
        return event;
    }];

    // フォーカスを失っている間の keyUp は届かないので押しっぱなしを防ぐ
    [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidResignActiveNotification
                                                      object:nil
                                                       queue:nil
                                                  usingBlock:^(NSNotification*) {
        yenDown = false;
        roDown = false;
    }];
}

bool NativeKeys_IsDown(NativeKey key) {
    InstallMonitor();
    switch (key) {
    case NativeKey::JisYen: return yenDown;
    case NativeKey::JisRo:  return roDown;
    }
    return false;
}
