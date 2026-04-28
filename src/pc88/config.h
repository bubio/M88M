#pragma once

#include "types.h"

// ---------------------------------------------------------------------------

namespace PC8801
{

class Config
{
public:
	enum BASICMode
	{
		// bit0 H/L
		// bit1 N/N80 (bit5=0)
		// bit4 V1/V2
		// bit5 N/N88
		// bit6 CDROM
		N80 = 0x00, N802 = 0x02, N80V2 = 0x12,
		N88V1 = 0x20, N88V1H = 0x21, N88V2 = 0x31, 
		N88V2CD = 0x71,
	};
	enum KeyType
	{
		AT106=0, PC98=1, AT101=2
	};
	enum CPUType
	{
		ms11 = 0, ms21, msauto,
	};

	enum Flags
	{
		subcpucontrol	= 1 <<  0,
		savedirectory	= 1 <<  1,
		fullspeed		= 1 <<  2,
		enablepad		= 1 <<  3,
		enableopna		= 1 <<  4,
		watchregister	= 1 <<  5,
		askbeforereset	= 1 <<  6,
		enablepcg		= 1 <<  7,
		fv15k			= 1 <<  8,
		cpuburst        = 1 <<  9,
		suppressmenu	= 1 << 10,
		cpuclockmode    = 1 << 11,
		usearrowfor10   = 1 << 12,
		swappadbuttons  = 1 << 13,
		disablesing		= 1 << 14,
		digitalpalette  = 1 << 15,
		useqpc			= 1 << 16,
		force480		= 1 << 17,
		opnona8			= 1 << 18,
		opnaona8		= 1 << 19,
		drawprioritylow	= 1 << 20,
		disablef12reset = 1 << 21,
		fullline		= 1 << 22,
		showstatusbar	= 1 << 23,
		showfdcstatus	= 1 << 24,
		enablewait		= 1 << 25,
		enablemouse		= 1 << 26,
		mousejoymode	= 1 << 27,
		specialpalette	= 1 << 28,
		mixsoundalways	= 1 << 29,
		precisemixing	= 1 << 30,
	};
	enum Flag2
	{
		disableopn44	= 1 <<  0,
		usewaveoutdrv	= 1 <<	1,
		mask0			= 1 <<  2,
		mask1			= 1 <<  3,
		mask2			= 1 <<  4,
		genscrnshotname = 1 <<  5,
		usefmclock		= 1 <<  6,
		compresssnapshot= 1 <<  7,
		synctovsync		= 1 <<  8,
		showplacesbar	= 1 <<  9,
		lpfenable		= 1 << 10,
		fddnowait		= 1 << 11,
		usedsnotify		= 1 << 12,
		saveposition	= 1 << 13,
		scanline        = 1 << 14,
	};

	int flags;
	int flag2;
	int clock;
	int speed;
	int refreshtiming;
	int mainsubratio;
	int opnclock;
	int sound;
	int erambanks;
	int keytype;
	int volfm, volssg, voladpcm, volrhythm;
	int volbd, volsd, voltop, volhh, voltom, volrim;
	int mastervol;
	int dipsw;
	uint soundbuffer;
	uint mousesensibility;
	int cpumode;
	uint lpffc;
	uint lpforder;
	int romeolatency;
	int winposx;
	int winposy;

	BASICMode basicmode;

	bool IsFV15k() const { return (basicmode & 2) || (flags & fv15k); }
};

}
