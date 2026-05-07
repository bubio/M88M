#include "raylib.h"
#include "headers.h"
#include "if/ifui.h"
#include "if/ifguid.h"

class RaylibMouseUI : public IMouseUI
{
public:
    RaylibMouseUI() {}
    virtual ~RaylibMouseUI() {}

    // IUnk implementation
    virtual ulong IFCALL AddRef() { return 1; }
    virtual ulong IFCALL Release() { return 1; }
    virtual long IFCALL QueryInterface(const GUID& iid, void** ppv) {
        if (iid == ChIID_MouseUI) { *ppv = (void*)this; return 0; } // S_OK
        return -1; // E_NOINTERFACE
    }

    // IMouseUI implementation
    virtual bool IFCALL Enable(bool en) { return true; }
    virtual bool IFCALL GetMovement(POINT* pt) {
        Vector2 delta = GetMouseDelta();
        pt->x = (long)delta.x;
        pt->y = (long)delta.y;
        return true;
    }
    virtual uint IFCALL GetButton() {
        uint buttons = 0;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) buttons |= 1;
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) buttons |= 2;
        return buttons;
    }
};
