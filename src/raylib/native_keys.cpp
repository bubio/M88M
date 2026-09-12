#include "native_keys.h"

// macOS 版は native_keys_mac.mm に実装がある。
#if !defined(__APPLE__)

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// スキャンコードから現在のキーボードレイアウトでの VK を引く。
// JIS レイアウトでは 0x7D → VK_OEM_5, 0x73 → VK_OEM_102 になる。
static bool IsScanCodeDown(UINT scancode) {
    UINT vk = MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK);
    if (vk == 0) return false;
    return (GetAsyncKeyState((int)vk) & 0x8000) != 0;
}

bool NativeKeys_IsDown(NativeKey key) {
    switch (key) {
    case NativeKey::JisYen: return IsScanCodeDown(0x7D);
    case NativeKey::JisRo:  return IsScanCodeDown(0x73);
    }
    return false;
}

#else

// X11 では ¥ / ろ は GLFW のキーシンボル変換で KEY_BACKSLASH になり、
// ] キーと区別できない。Wayland / SDL でも取得手段がないため未対応。
bool NativeKeys_IsDown(NativeKey) {
    return false;
}

#endif // _WIN32
#endif // !__APPLE__
