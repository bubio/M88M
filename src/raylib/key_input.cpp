#include "key_input.h"
#include "raylib.h"
#include "pc88.h"
#include "ifui.h"
#include "device_i.h"
#include <cstring>

static const IDevice::ID KEY_ID = DEV_ID('K', 'E', 'Y', 'B');

KeyInput::KeyInput() : Device(KEY_ID) {
    memset(matrix, 0xff, sizeof(matrix));
}

KeyInput::~KeyInput() {}

const Device::Descriptor* IFCALL KeyInput::GetDesc() const {
    return &descriptor;
}

uint IOCALL KeyInput::In(uint port) {
    return matrix[port & 0x0f];
}

bool KeyInput::Init(PC88* pc88) {
    static const IIOBus::Connector connectors[] = {
        { 0x00, IIOBus::portin, 0 },
        { 0x01, IIOBus::portin, 0 },
        { 0x02, IIOBus::portin, 0 },
        { 0x03, IIOBus::portin, 0 },
        { 0x04, IIOBus::portin, 0 },
        { 0x05, IIOBus::portin, 0 },
        { 0x06, IIOBus::portin, 0 },
        { 0x07, IIOBus::portin, 0 },
        { 0x08, IIOBus::portin, 0 },
        { 0x09, IIOBus::portin, 0 },
        { 0x0a, IIOBus::portin, 0 },
        { 0x0b, IIOBus::portin, 0 },
        { 0x0c, IIOBus::portin, 0 },
        { 0x0d, IIOBus::portin, 0 },
        { 0x0e, IIOBus::portin, 0 },
        { 0x0f, IIOBus::portin, 0 },
        { 0, 0, 0 }
    };
    return pc88->GetBus1()->Connect(this, connectors);
}

void KeyInput::Update(PC88* pc88) {
    memset(matrix, 0xff, sizeof(matrix));

    auto set_key = [&](int row, int bit, bool down) {
        if (down) matrix[row] &= ~(1 << bit);
    };

    // --- Row 2: @, A, B, C, D, E, F, G ---
    set_key(2, 1, IsKeyDown(KEY_A));
    set_key(2, 2, IsKeyDown(KEY_B));
    set_key(2, 3, IsKeyDown(KEY_C));
    set_key(2, 4, IsKeyDown(KEY_D));
    set_key(2, 5, IsKeyDown(KEY_E));
    set_key(2, 6, IsKeyDown(KEY_F));
    set_key(2, 7, IsKeyDown(KEY_G));

    // --- Row 3: H, I, J, K, L, M, N, O ---
    set_key(3, 0, IsKeyDown(KEY_H));
    set_key(3, 1, IsKeyDown(KEY_I));
    set_key(3, 2, IsKeyDown(KEY_J));
    set_key(3, 3, IsKeyDown(KEY_K));
    set_key(3, 4, IsKeyDown(KEY_L));
    set_key(3, 5, IsKeyDown(KEY_M));
    set_key(3, 6, IsKeyDown(KEY_N));
    set_key(3, 7, IsKeyDown(KEY_O));

    // --- Row 4: P, Q, R, S, T, U, V, W ---
    set_key(4, 0, IsKeyDown(KEY_P));
    set_key(4, 1, IsKeyDown(KEY_Q));
    set_key(4, 2, IsKeyDown(KEY_R));
    set_key(4, 3, IsKeyDown(KEY_S));
    set_key(4, 4, IsKeyDown(KEY_T));
    set_key(4, 5, IsKeyDown(KEY_U));
    set_key(4, 6, IsKeyDown(KEY_V));
    set_key(4, 7, IsKeyDown(KEY_W));

    // --- Row 5: X, Y, Z, [, \, ], ^, - ---
    set_key(5, 0, IsKeyDown(KEY_X));
    set_key(5, 1, IsKeyDown(KEY_Y));
    set_key(5, 2, IsKeyDown(KEY_Z));
    set_key(5, 3, IsKeyDown(KEY_LEFT_BRACKET));
    set_key(5, 4, IsKeyDown(KEY_BACKSLASH));
    set_key(5, 5, IsKeyDown(KEY_RIGHT_BRACKET));
    set_key(5, 7, IsKeyDown(KEY_MINUS));

    // --- Row 6: 0-7 ---
    for (int i=0; i<=7; i++) set_key(6, i, IsKeyDown((KeyboardKey)(KEY_ZERO + i)));

    // --- Row 7: 8, 9, :, ;, ,, ., /, _ ---
    set_key(7, 0, IsKeyDown(KEY_EIGHT));
    set_key(7, 1, IsKeyDown(KEY_NINE));
    set_key(7, 4, IsKeyDown(KEY_COMMA));
    set_key(7, 5, IsKeyDown(KEY_PERIOD));
    set_key(7, 6, IsKeyDown(KEY_SLASH));

    // --- Row 8: CLR, UP, RIGHT, BS, GRPH, KANA, SHIFT, CTRL ---
    set_key(8, 0, IsKeyDown(KEY_HOME));
    set_key(8, 1, IsKeyDown(KEY_UP));
    set_key(8, 2, IsKeyDown(KEY_RIGHT));
    set_key(8, 3, IsKeyDown(KEY_BACKSPACE));
    set_key(8, 4, IsKeyDown(KEY_LEFT_ALT)); // GRPH
    set_key(8, 6, IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
    set_key(8, 7, IsKeyDown(KEY_LEFT_CONTROL));

    // --- Row 9: STOP, F1, F2, F3, F4, F5, SPACE, ESC ---
    set_key(9, 0, IsKeyDown(KEY_END)); // STOP (End as alternative)
    set_key(9, 1, IsKeyDown(KEY_F1));
    set_key(9, 2, IsKeyDown(KEY_F2));
    set_key(9, 3, IsKeyDown(KEY_F3));
    set_key(9, 4, IsKeyDown(KEY_F4));
    set_key(9, 5, IsKeyDown(KEY_F5));
    set_key(9, 6, IsKeyDown(KEY_SPACE));
    set_key(9, 7, IsKeyDown(KEY_ESCAPE));

    // --- Row 10: TAB, DOWN, LEFT, HELP, COPY... ---
    set_key(0xa, 0, IsKeyDown(KEY_TAB));
    set_key(0xa, 1, IsKeyDown(KEY_DOWN));
    set_key(0xa, 2, IsKeyDown(KEY_LEFT));
    set_key(0xa, 3, IsKeyDown(KEY_INSERT)); // HELP
    set_key(0xa, 4, IsKeyDown(KEY_F12));    // COPY
    
    // --- Row 1: Return (Enter) ---
    set_key(1, 2, IsKeyDown(KEY_ENTER) || IsKeyDown(KEY_KP_ENTER));
}

const Device::Descriptor KeyInput::descriptor = { indef, nullptr };

const Device::InFuncPtr KeyInput::indef[] = {
    STATIC_CAST(Device::InFuncPtr, &KeyInput::In),
};
