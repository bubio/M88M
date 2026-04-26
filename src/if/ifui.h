//
//	chime core ui interface
//	$Id: ifui.h,v 1.2 2003/04/22 13:16:34 cisc Exp $
//

#pragma once

#include "types.h"
#include "ifcommon.h"

// IFCALL/IOCALL come from compat.h via types.h; no local override needed.

interface IMouseUI : public IUnk
{
	virtual bool IFCALL Enable(bool en) = 0;
	virtual bool IFCALL GetMovement(POINT*) = 0;
	virtual uint IFCALL GetButton() = 0;
};



struct PadState
{
	uint8 direction;		// b0:�� b1:�� b2:�� b3:��  active high
	uint8 button;			// b0-3, active high
};

interface IPadInput 
{
	virtual void IFCALL GetState(PadState*) = 0;
};

