// ----------------------------------------------------------------------------
//	M88 - PC-8801 emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	�g�����W���[���p�C���^�[�t�F�[�X��`
// ----------------------------------------------------------------------------
//	$Id: ifcommon.h,v 1.8 2002/04/07 05:40:09 cisc Exp $

#pragma once

#include "types.h"

// IFCALL / IOCALL are now defined in compat.h (pulled in via types.h).
// They expand to __stdcall on 32-bit MSVC + the legacy x86 Z80 engine,
// and to nothing everywhere else.

#define RELEASE(r)	if (r) r->Release(), r=0

// ----------------------------------------------------------------------------
//
//
interface IUnk
{
	virtual long IFCALL QueryInterface(REFIID, void**) = 0;
	virtual ulong IFCALL AddRef() = 0;
	virtual ulong IFCALL Release() = 0; 
};

// ----------------------------------------------------------------------------
//	�����̃C���^�[�t�F�[�X
//
struct ISoundControl;
struct ISoundSource
{
	virtual bool IFCALL Connect(ISoundControl* sc) = 0;
	virtual bool IFCALL SetRate(uint rate) = 0;
	virtual void IFCALL Mix(int32* s, int length) = 0;
};

// ----------------------------------------------------------------------------
//	��������̃C���^�[�t�F�[�X
//
struct ISoundControl
{
	virtual bool IFCALL Connect(ISoundSource* src) = 0;
	virtual bool IFCALL Disconnect(ISoundSource* src) = 0;
	
	virtual bool IFCALL Update(ISoundSource* src) = 0;
	virtual int  IFCALL GetSubsampleTime(ISoundSource* src) = 0;
};

// ----------------------------------------------------------------------------
//	�������Ǘ��̃C���^�[�t�F�[�X
//
struct IMemoryManager
{
	virtual int  IFCALL Connect(void* inst, bool highpriority = false) = 0;
	virtual bool IFCALL	Disconnect(uint pid) = 0;

	virtual bool IFCALL AllocR(uint pid, uint addr, uint length, uint8* ptr) = 0;
	virtual bool IFCALL AllocR(uint pid, uint addr, uint length, uint(MEMCALL*)(void*,uint)) = 0;
	virtual bool IFCALL ReleaseR(uint pid, uint addr, uint length) = 0;
	virtual uint IFCALL Read8P(uint pid, uint addr) = 0;
	
	virtual bool IFCALL AllocW(uint pid, uint addr, uint length, uint8* ptr) = 0;
	virtual bool IFCALL AllocW(uint pid, uint addr, uint length, void(MEMCALL*)(void*,uint,uint)) = 0;
	virtual bool IFCALL ReleaseW(uint pid, uint addr, uint length) = 0;
	virtual	void IFCALL Write8P(uint pid, uint addr, uint data) = 0;
};

// ----------------------------------------------------------------------------
//	��������ԂɃA�N�Z�X���邽�߂̃C���^�[�t�F�[�X
//
struct IMemoryAccess
{
	virtual uint IFCALL	Read8(uint addr) = 0;
	virtual void IFCALL Write8(uint addr, uint data) = 0;
};

// ----------------------------------------------------------------------------
//	IO ��ԂɃA�N�Z�X���邽�߂̃C���^�[�t�F�[�X
//
struct IIOAccess
{
	virtual uint IFCALL In(uint port) = 0;
	virtual void IFCALL Out(uint port, uint data) = 0;
};

// ----------------------------------------------------------------------------
//	�f�o�C�X�̃C���^�[�t�F�[�X
//	
struct IDevice
{
	typedef uint32 ID;
	typedef uint (IOCALL IDevice::*InFuncPtr)(uint port);
	typedef void (IOCALL IDevice::*OutFuncPtr)(uint port, uint data);
	typedef void (IOCALL IDevice::*TimeFunc)(uint arg);
	struct Descriptor
	{
		const InFuncPtr* indef;
		const OutFuncPtr* outdef;
	};
	
	virtual const ID& IFCALL GetID() const = 0;
	virtual const Descriptor* IFCALL GetDesc() const = 0;
	virtual uint IFCALL GetStatusSize() = 0;
	virtual bool IFCALL LoadStatus(const uint8* status) = 0;
	virtual bool IFCALL SaveStatus(uint8* status) = 0;
};

// ----------------------------------------------------------------------------
//	IO ��ԂɃf�o�C�X��ڑ����邽�߂̃C���^�[�t�F�[�X
//
struct IIOBus
{
	enum ConnectRule
	{
		end = 0, portin = 1, portout = 2, sync = 4,
	};
	struct Connector
	{
		ushort bank;
		uint8 rule;
		uint8 id;
	};
	
	virtual bool IFCALL Connect(IDevice* device, const Connector* connector) = 0;
	virtual bool IFCALL Disconnect(IDevice* device) = 0;
};

// ----------------------------------------------------------------------------
//	�^�C�}�[�Ǘ��̂��߂̃C���^�[�t�F�[�X
//
struct SchedulerEvent;

struct IScheduler
{
	typedef SchedulerEvent* Handle;

	virtual Handle IFCALL AddEvent(int count, IDevice* dev, IDevice::TimeFunc func, int arg=0, bool repeat=false) = 0;
	virtual void IFCALL SetEvent(Handle ev, int count, IDevice* dev, IDevice::TimeFunc func, int arg=0, bool repeat=false) = 0;
	virtual bool IFCALL DelEvent(IDevice* dev) = 0;
	virtual bool IFCALL DelEvent(Handle ev) = 0;
};

// ----------------------------------------------------------------------------
//	�V�X�e�������Ԏ擾�̂��߂̃C���^�[�t�F�[�X
//
struct ITime
{
	virtual int IFCALL GetTime() = 0;
};

// ----------------------------------------------------------------------------
//	��萸�x�̍������Ԃ��擾���邽�߂̃C���^�[�t�F�[�X
//
struct ICPUTime
{
	virtual uint IFCALL GetCPUTick() = 0;
	virtual uint IFCALL GetCPUSpeed() = 0;
};

// ----------------------------------------------------------------------------
//	�V�X�e�����̃C���^�[�t�F�[�X�ɐڑ����邽�߂̃C���^�[�t�F�[�X
//
struct ISystem
{
	virtual void* IFCALL QueryIF(REFIID iid) = 0;
};

// ----------------------------------------------------------------------------
//	���W���[���̊�{�C���^�[�t�F�[�X
//
struct IModule
{
	virtual void IFCALL Release() = 0;
	virtual void* IFCALL QueryIF(REFIID iid) = 0;
};

// ----------------------------------------------------------------------------
//	�u�ݒ�v�_�C�A���O�𑀍삷�邽�߂̃C���^�[�t�F�[�X
//
// ----------------------------------------------------------------------------
//	The remainder of this file are Win32-only UI hooks (HWND / PROPSHEETPAGE
//	/ WPARAM / LPARAM). They are consumed exclusively by src/win32/ and the
//	.m88 extension modules, so the portable build elides them.
//
#if defined(_WIN32) && !defined(M88_PORTABLE)

struct IConfigPropSheet;

struct IConfigPropBase
{
	virtual bool IFCALL Add(IConfigPropSheet*) = 0;
	virtual bool IFCALL Remove(IConfigPropSheet*) = 0;

	virtual bool IFCALL Apply() = 0;
	virtual bool IFCALL PageSelected(IConfigPropSheet*) = 0;
	virtual bool IFCALL PageChanged(HWND) = 0;
	virtual void IFCALL _ChangeVolume(bool) = 0;
};

// ----------------------------------------------------------------------------
//	uݒṽvpeBV[g̊{C^[tF[X
//
struct IConfigPropSheet
{
	virtual bool IFCALL Setup(IConfigPropBase*, PROPSHEETPAGE* psp) = 0;
};

// ----------------------------------------------------------------------------
//	UI gpC^[tF[X
//
struct IWinUIExtention
{
	virtual bool IFCALL WinProc(HWND, UINT, WPARAM, LPARAM) = 0;
	virtual bool IFCALL Connect(HWND hwnd, uint index) = 0;
	virtual bool IFCALL Disconnect() = 0;
};

// ----------------------------------------------------------------------------
//	UI ɑ΂C^[tF[X
//
struct IWinUI2
{
	virtual HWND IFCALL GetHWnd() = 0;
};

#endif // _WIN32 && !M88_PORTABLE

// ----------------------------------------------------------------------------
//	�G�~�����[�^��̃V�X�e���������鎞�Ɏg�����b�N
//	IMemoryManager / IIOBus / IScheduler ���ɑ΂��鑀���
//	�������C�I�����C�܂��̓�����, IO, �^�C�}�[�R�[���o�b�N�ȊO����
//	�s���ꍇ�C���b�N��������K�v������
//
struct ILockCore
{
	virtual void IFCALL Lock() = 0;
	virtual void IFCALL Unlock() = 0;
};

// ----------------------------------------------------------------------------
//	���݃A�N�e�B�u�ɂȂ��Ă��郁�����̎�ނ��擾
//
struct IGetMemoryBank
{
	virtual uint IFCALL GetRdBank(uint) = 0;
	virtual uint IFCALL GetWrBank(uint) = 0;
};

