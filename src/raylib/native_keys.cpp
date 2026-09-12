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

#elif defined(__HAIKU__)
#include <InterfaceDefs.h>

// Haiku のキーコード (JIS キーボード)。0x6a = ¥, 0x6b = ろ
static bool IsHaikuKeyDown(uint32 code) {
    key_info info;
    if (get_key_info(&info) != B_OK) return false;
    return (info.key_states[code >> 3] & (1 << (7 - (code & 7)))) != 0;
}

bool NativeKeys_IsDown(NativeKey key) {
    switch (key) {
    case NativeKey::JisYen: return IsHaikuKeyDown(0x6a);
    case NativeKey::JisRo:  return IsHaikuKeyDown(0x6b);
    }
    return false;
}

#elif defined(M88M_NATIVE_KEYS_GLFW)
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <cstring>

// GLFW はキーコードを割り当てられないキーでも、スキャンコード付きで
// キーコールバックを呼ぶ。raylib のコールバックの前に割り込んで
// ¥ / ろ の押下状態を記録する。
// raylib のプリビルドには glfw3.h が同梱されていないので必要な分だけ宣言する。
struct GLFWwindow;
typedef void (*GLFWkeyfun)(GLFWwindow*, int, int, int, int);
typedef void (*GLFWwindowfocusfun)(GLFWwindow*, int);
extern "C" {
int glfwGetPlatform(void);
GLFWwindow* glfwGetCurrentContext(void);
Display* glfwGetX11Display(void);
GLFWkeyfun glfwSetKeyCallback(GLFWwindow* window, GLFWkeyfun callback);
GLFWwindowfocusfun glfwSetWindowFocusCallback(GLFWwindow* window, GLFWwindowfocusfun callback);
}
static const int GLFW_RELEASE          = 0;
static const int GLFW_PLATFORM_WAYLAND = 0x00060003;
static const int GLFW_PLATFORM_X11     = 0x00060004;

static bool installed = false;
static int yenScancode = -1;
static int roScancode  = -1;
static bool yenDown = false;
static bool roDown  = false;
static GLFWkeyfun prevKeyCallback = nullptr;
static GLFWwindowfocusfun prevFocusCallback = nullptr;

// X11 のスキャンコードは X のキーコード。XKB のキー名から物理キーを引く。
// evdev ルールでは AE13 (¥) = 132, AB11 (ろ) = 97。
static void LookupX11Keycodes(Display* dpy) {
    yenScancode = 132;
    roScancode  = 97;
    if (!dpy) return;
    XkbDescPtr desc = XkbGetMap(dpy, 0, XkbUseCoreKbd);
    if (!desc) return;
    if (XkbGetNames(dpy, XkbKeyNamesMask, desc) == Success && desc->names) {
        for (int kc = desc->min_key_code; kc <= desc->max_key_code; kc++) {
            const char* name = desc->names->keys[kc].name;
            if (memcmp(name, "AE13", 4) == 0) yenScancode = kc;
            else if (memcmp(name, "AB11", 4) == 0) roScancode = kc;
        }
    }
    XkbFreeKeyboard(desc, 0, True);
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (scancode == yenScancode) yenDown = (action != GLFW_RELEASE);
    else if (scancode == roScancode) roDown = (action != GLFW_RELEASE);
    if (prevKeyCallback) prevKeyCallback(window, key, scancode, action, mods);
}

// GLFW はフォーカスを失ったとき既知のキーしか離さないので、ここで離す
static void FocusCallback(GLFWwindow* window, int focused) {
    if (!focused) yenDown = roDown = false;
    if (prevFocusCallback) prevFocusCallback(window, focused);
}

static void Install() {
    // raylib はメインスレッドでウィンドウのコンテキストを current にしている
    GLFWwindow* window = glfwGetCurrentContext();
    if (!window) return;
    installed = true;

    int platform = glfwGetPlatform();
    if (platform == GLFW_PLATFORM_X11) {
        LookupX11Keycodes(glfwGetX11Display());
    } else if (platform == GLFW_PLATFORM_WAYLAND) {
        // Wayland のスキャンコードは evdev のキーコード (KEY_YEN, KEY_RO)
        yenScancode = 124;
        roScancode  = 89;
    } else {
        return;
    }
    prevKeyCallback = glfwSetKeyCallback(window, KeyCallback);
    prevFocusCallback = glfwSetWindowFocusCallback(window, FocusCallback);
}

bool NativeKeys_IsDown(NativeKey key) {
    if (!installed) Install();
    switch (key) {
    case NativeKey::JisYen: return yenDown;
    case NativeKey::JisRo:  return roDown;
    }
    return false;
}

#else

// その他のプラットフォームでは取得手段がないため未対応。
bool NativeKeys_IsDown(NativeKey) {
    return false;
}

#endif // _WIN32
#endif // !__APPLE__
