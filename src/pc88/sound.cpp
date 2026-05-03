// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//	$Id: sound.cpp,v 1.32 2003/05/19 01:10:32 cisc Exp $

#include "headers.h"
#include "types.h"
#include "misc.h"
#include "pc88/sound.h"
#include "pc88/pc88.h"
#include "pc88/config.h"

//#define LOGNAME "sound"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	�����E�j��
//
Sound::Sound()
: Device(0), sslist(0), mixingbuf(0), enabled(false), cfgflg(0)
{
}

Sound::~Sound()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	�������Ƃ�
//
bool Sound::Init(PC88* pc88, uint rate, int bufsize)
{
	pc = pc88;
	prevtime = pc->GetCPUTick();
	enabled = false;
	mixthreshold = 16;
	
	if (!SetRate(rate, bufsize))
		return false;
	
	// ���ԃJ�E���^��������Ȃ��悤�ɒ���I�ɍX�V����
	pc88->AddEvent(5000, this, STATIC_CAST(TimeFunc, &Sound::UpdateCounter), 0, true);
	return true;
}

// ---------------------------------------------------------------------------
//	���[�g�ݒ�
//	clock:		OPN �ɗ^����N���b�N
//	bufsize:	�o�b�t�@�� (�T���v���P��?)
//
bool Sound::SetRate(uint rate, int bufsize)
{
	mixrate = 55467;

	// �e�����̃��[�g�ݒ��ύX
	for (SSNode* n = sslist; n; n = n->next)
		n->ss->SetRate(mixrate);
	
	enabled = false;
	
	// �Â��o�b�t�@���폜
	soundbuf.Cleanup();
	delete[] mixingbuf;	mixingbuf = 0;

	// �V�����o�b�t�@��p��
	samplingrate = rate;
	buffersize = bufsize;
	if (bufsize > 0)
	{
//		if (!soundbuf.Init(this, bufsize))
//			return false;
		if (!soundbuf.Init(this, bufsize, rate))
			return false;

		mixingbuf = new int32[2 * bufsize];
		if (!mixingbuf)
			return false;

		rate50 = mixrate / 50;
		tdiff = 0;
		enabled = true;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	��Еt��
//
void Sound::Cleanup()
{
	// �e������؂藣���B(�������̂̍폜�͍s��Ȃ�)
	for (SSNode* n = sslist; n; )
	{
		SSNode* next = n->next;
		delete[] n;
		n = next;
	}
	sslist = 0;

	// �o�b�t�@���J��
	soundbuf.Cleanup();
	delete[] mixingbuf; mixingbuf = 0;
}

void IOCALL Sound::Reset(uint, uint)
{
	soundbuf.Clear();
	if (pc) prevtime = pc->GetCPUTick();
	tdiff = 0;
}

// ---------------------------------------------------------------------------
//	������
//
int Sound::Get(Sample* dest, int nsamples)
{
	int mixsamples = Min(nsamples, buffersize);
	if (mixsamples > 0)
	{
		// ����
		{
			memset(mixingbuf, 0, mixsamples * 2 * sizeof(int32));
			CriticalSection::Lock lock(cs_ss);
			for (SSNode* s = sslist; s; s = s->next)
				s->ss->Mix(mixingbuf, mixsamples);
		}

		int32* src = mixingbuf;
		for (int n = mixsamples; n>0; n--)
		{
			*dest++ = Limit(*src++, 32767, -32768);
			*dest++ = Limit(*src++, 32767, -32768);
		}
	}
	return mixsamples;
}

// ---------------------------------------------------------------------------
//	������
//
int Sound::Get(SampleL* dest, int nsamples)
{
	// ����
	memset(dest, 0, nsamples * 2 * sizeof(int32));
	CriticalSection::Lock lock(cs_ss);
	for (SSNode* s = sslist; s; s = s->next)
		s->ss->Mix(dest, nsamples);
	return nsamples;
}


// ---------------------------------------------------------------------------
//	�ݒ�X�V
//
void Sound::ApplyConfig(const Config* config)
{
	mixthreshold = (config->flags & Config::precisemixing) ? 100 : 2000;
}

// ---------------------------------------------------------------------------
//	������ǉ�����
//	Sound �����������X�g�ɁCss �Ŏw�肳�ꂽ������ǉ��C
//	ss �� SetRate ���Ăяo���D
//
//	arg:	ss		�ǉ����鉹�� (ISoundSource)
//	ret:	S_OK, E_FAIL, E_OUTOFMEMORY
//
bool Sound::Connect(ISoundSource* ss)
{
	CriticalSection::Lock lock(cs_ss);

	// �����͊��ɓo�^�ς݂��H;
	SSNode** n;
	for (n = &sslist; *n; n=&((*n)->next))
	{
		if ((*n)->ss == ss)
			return false;
	}
	
	SSNode* nn = new SSNode;
	if (nn)
	{
		*n = nn;
		nn->next = 0;
		nn->ss = ss;
		ss->SetRate(mixrate);
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	�������X�g����w�肳�ꂽ�������폜����
//
//	arg:	ss		�폜���鉹��
//	ret:	S_OK, E_HANDLE
//
bool Sound::Disconnect(ISoundSource* ss)
{
	CriticalSection::Lock lock(cs_ss);
	
	for (SSNode** r = &sslist; *r; r=&((*r)->next))
	{
		if ((*r)->ss == ss)
		{
			SSNode* d = *r;
			*r = d->next;
			delete d;
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
//	�X�V����
//	(�w�肳�ꂽ)������ Mix ���Ăяo���C���݂̎��Ԃ܂ōX�V����	
//	�����̓�����Ԃ��ς��C�����ω����钼�O�̒i�K�ŌĂяo����
//	���x�̍������Č����\�ɂȂ�(����)�D
//
//	arg:	src		�X�V���鉹�����w��(���̎����ł͖�������܂�)
//
bool Sound::Update(ISoundSource* /*src*/)
{
	uint32 currenttime = pc->GetCPUTick();
	
	uint32 time = currenttime - prevtime;
	if (enabled && time > mixthreshold)
	{
		prevtime = currenttime;
		// nsamples = �o�ߎ���(s) * �T���v�����O���[�g
		// sample = ticks * rate / clock / 100000
		// sample = ticks * (rate/50) / clock / 2000

		// MulDiv(a, b, c) = (int64) a * b / c 
		int a = MulDiv(time, rate50, pc->GetEffectiveSpeed()) + tdiff;
//		a = MulDiv(a, mixrate, samplingrate);
		int samples = a / 2000;
		tdiff = a % 2000;
		
		Log("Store = %5d samples\n", samples);
		soundbuf.Fill(samples);
	}
	return true;
}

// ---------------------------------------------------------------------------
//	���܂ō������ꂽ���Ԃ́C1�T���v�������̒[��(0-1999)�����߂�
//
int IFCALL Sound::GetSubsampleTime(ISoundSource* /*src*/)
{
	return tdiff;
}

// ---------------------------------------------------------------------------
//	IɓJE^XV
//
void IOCALL Sound::UpdateCounter(uint)
{
	if ((pc->GetCPUTick() - prevtime) > 40000)
	{
		Log("Update Counter\n");
		Update(0);
	}
}

// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor Sound::descriptor =
{
	0, outdef
};

const Device::OutFuncPtr Sound::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, &Sound::Reset),
};

