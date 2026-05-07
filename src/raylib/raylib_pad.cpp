#include "raylib.h"
#include "headers.h"
#include "if/ifui.h"
#include "if/ifguid.h"

class RaylibPadBridge : public IPadInput
{
public:
    RaylibPadBridge() : refcount(0) {}
    virtual ~RaylibPadBridge() {}

    // IUnk implementation
    virtual ulong IFCALL AddRef() { return ++refcount; }
    virtual ulong IFCALL Release() { if (--refcount == 0) { /* delete this; */ } return refcount; }
    virtual long IFCALL QueryInterface(const GUID& iid, void** ppv) {
        if (iid == ChIID_PadInput) { *ppv = (void*)this; return 0; }
        return -1;
    }

    // IPadInput implementation
    virtual void IFCALL GetState(PadState* ps) {
        ps->direction = 0;
        ps->button = 0;
        
        if (IsGamepadAvailable(0)) {
            // Direction (D-Pad or Left Stick)
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) ps->direction |= 1;
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) ps->direction |= 2;
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) ps->direction |= 4;
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) ps->direction |= 8;
            
            // Sticks (approx)
            if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) < -0.5f) ps->direction |= 1;
            if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) >  0.5f) ps->direction |= 2;
            if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) < -0.5f) ps->direction |= 4;
            if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) >  0.5f) ps->direction |= 8;

            // Buttons
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) ps->button |= 1; // A
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) ps->button |= 2; // B
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) ps->button |= 4; // X
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) ps->button |= 8; // Y
        }
    }

private:
    ulong refcount;
};
