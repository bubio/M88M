// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: config.h,v 1.23 2003/09/28 14:35:35 cisc Exp $

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
		// bit6 CDROM �L��
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
		subcpucontrol	= 1 <<  0,	// Sub CPU �̋쓮�𐧌䂷��
		savedirectory	= 1 <<  1,	// �N�����ɑO��I�����̃f�B���N�g���Ɉړ�
		fullspeed		= 1 <<  2,	// �S�͓���
		enablepad		= 1 <<  3,	// �p�b�h�L��
		enableopna		= 1 <<  4,	// OPNA ���[�h (44h)
		watchregister	= 1 <<  5,	// ���W�X�^�\��
		askbeforereset	= 1 <<  6,	// �I���E���Z�b�g���Ɋm�F
		enablepcg		= 1 <<  7,	// PCG �n�̃t�H���g����������L��
		fv15k			= 1 <<  8,	// 15KHz ���j�^�[���[�h
		cpuburst        = 1 <<  9,	// �m�[�E�F�C�g
		suppressmenu	= 1 << 10,	// ALT �� GRPH ��
		cpuclockmode	= 1 << 11,	// �N���b�N�P�ʂŐ؂�ւ�
		usearrowfor10	= 1 << 12,	// �����L�[���e���L�[��
		swappadbuttons	= 1 << 13,	// �p�b�h�̃{�^�������ւ�
		disablesing		= 1 << 14,	// CMD SING ����
		digitalpalette	= 1 << 15,	// �f�B�W�^���p���b�g���[�h
		useqpc			= 1 << 16,	// QueryPerformanceCounter ����
		force480		= 1 << 17,	// �S��ʂ���� 640x480 ��
		opnona8			= 1 << 18,	// OPN (a8h)
		opnaona8		= 1 << 19,	// OPNA (a8h)
		drawprioritylow	= 1 << 20,	// �`��̗D��x�𗎂Ƃ�
		disablef12reset = 1 << 21,  // F12 �� RESET �Ƃ��Ďg�p���Ȃ�(COPY �L�[�ɂȂ�)
		fullline		= 1 << 22,  // �������C���\��
		showstatusbar	= 1 << 23,	// �X�e�[�^�X�o�[�\��
		showfdcstatus	= 1 << 24,	// FDC �̃X�e�[�^�X��\��
		enablewait		= 1 << 25,	// Wait ������
		enablemouse		= 1 << 26,	// Mouse ���g�p
		mousejoymode	= 1 << 27,	// Mouse ���W���C�X�e�B�b�N���[�h�Ŏg�p
		specialpalette	= 1 << 28,	// �f�o�b�N�p���b�g���[�h
		mixsoundalways	= 1 << 29,	// �������d���������̍����𑱂���
		precisemixing	= 1 << 30,	// �����x�ȍ������s��
	};
	enum Flag2
	{
		disableopn44	= 1 <<  0,	// OPN(44h) �𖳌��� (V2 ���[�h���� OPN)
		usewaveoutdrv	= 1 <<	1,	// PCM �̍Đ��� waveOut ���g�p����
		mask0			= 1 <<  2,	// �I��\�����[�h
		mask1			= 1 <<  3,
		mask2			= 1 <<  4,
		genscrnshotname = 1 <<  5,	// �X�N���[���V���b�g�t�@�C��������������
		usefmclock		= 1 <<  6,	// FM �����̍����ɖ{���̃N���b�N���g�p
		compresssnapshot= 1 <<  7,  // �X�i�b�v�V���b�g�����k����
		synctovsync		= 1 <<  8,	// �S��ʃ��[�h�� vsync �Ɠ�������
		showplacesbar	= 1 <<  9,	// �t�@�C���_�C�A���O�� PLACESBAR ��\������
		lpfenable		= 1 << 10,	// LPF ���g���Ă݂�
		fddnowait		= 1 << 11,	// FDD �m�[�E�F�C�g
		usedsnotify		= 1 << 12,
		saveposition	= 1 << 13,	// �N�����ɑO��I�����̃E�C���h�E�ʒu�𕜌�
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
	uint lpffc;				// LPF �̃J�b�g�I�t���g�� (Hz)
	uint lpforder;
	int romeolatency;
	int winposx;
	int winposy;

	BASICMode basicmode;

	// 15kHz ���[�h�̔�����s���D
	// (����: option ���� N80/SR ���[�h��)
	bool IsFV15k() const { return (basicmode & 2) || (flags & fv15k); }
};

}

