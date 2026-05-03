//	$Id: srcbuf.cpp,v 1.2 2003/05/12 22:26:34 cisc Exp $

#include "headers.h"
#include "srcbuf.h"
#include "misc.h"


#ifndef PI
#define PI			3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
//	Sound Buffer
//
SamplingRateConverter::SamplingRateConverter()
: source(0), buffer(0), buffersize(0), h2(0), outputrate(0)
{
	fillwhenempty = true;
}

SamplingRateConverter::~SamplingRateConverter()
{
	Cleanup();
}

bool SamplingRateConverter::Init(SoundSourceL* _source, int _buffersize, ulong outrate)
{
	CriticalSection::Lock lock(cs);
	
	delete[] buffer; buffer = 0;
	
	source = 0;
	if (!_source)
		return true;

	buffersize = _buffersize;
	assert(buffersize > (2 * M + 1));

	ch = _source->GetChannels();
	read = 0; write = 0;

	if (!ch || buffersize <= 0)
		return false;
	
	buffer = new SampleL[ch * buffersize];
	if (!buffer)
		return false;

	memset(buffer, 0, ch * buffersize * sizeof(SampleL));
	source = _source;

	outputrate = outrate;

	MakeFilter(outrate);
	read = 2 * M + 1;		// zero fill
	return true;
}

void SamplingRateConverter::Cleanup()
{
	CriticalSection::Lock lock(cs);
	
	delete[] buffer; buffer = 0;
	delete[] h2; h2 = 0;
}

void SamplingRateConverter::Clear()
{
	CriticalSection::Lock lock(cs);
	if (buffer)
	{
		memset(buffer, 0, ch * buffersize * sizeof(SampleL));
		read = 2 * M + 1;
		write = 0;
		oo = 0;
	}
}

// ---------------------------------------------------------------------------
//	�o�b�t�@�ɉ���ǉ�
//
int SamplingRateConverter::Fill(int samples)
{
	CriticalSection::Lock lock(cs);
	if (source)
		return FillMain(samples);
	return 0;
}

int SamplingRateConverter::FillMain(int samples)
{
	// �����O�o�b�t�@�̋󂫂��v�Z
	int free = buffersize - Avail();
	
	if (!fillwhenempty && (samples > free-1))
	{
		int skip = Min(samples-free+1, buffersize-free);
		free += skip;
		read += skip;
		if (read > buffersize)
			read -= buffersize;
	}
	
	// �������ނׂ��f�[�^�ʂ��v�Z
	samples = Min(samples, free-1);
	if (samples > 0)
	{
		// ��������
		if (buffersize - write >= samples)
		{
			// ��x�ŏ�����ꍇ
			source->Get(buffer + write * ch, samples);
		}
		else
		{
			// �Q�x�ɕ����ď����ꍇ
			source->Get(buffer + write * ch, buffersize - write);
			source->Get(buffer, samples - (buffersize - write));
		}
		write += samples;
		if (write >= buffersize)
			write -= buffersize;
	}
	return samples;
}



// ---------------------------------------------------------------------------
//	�t�B���^���\�z
//
void SamplingRateConverter::MakeFilter(ulong out)
{
	ulong in = source->GetRate();

	// �ϊ��O�A�ϊ��ヌ�[�g�̔�����߂�
	// �\�[�X�� ic �{�A�b�v�T���v�����O���� LPF ���|������
	// oc ���� 1 �Ƀ_�E���T���v�����O����

	if (in == 55467)		// FM �����΍�(w
	{
		in = 166400;
		out *= 3;
	}
	int32 g = gcd(in, out);
	ic = out / g;
	oc = in / g;

	// ���܂莟����������������ƁA�W���e�[�u��������ɂȂ��Ă��܂��̂łĂ��Ƃ��ɐ��x�𗎂Ƃ�
	while (ic > osmax && oc >= osmin)
	{
		ic = (ic + 1) / 2;
		oc = (oc + 1) / 2;
	}

	double r = ic * in;			// r = lpf �����鎞�̃��[�g

	// �J�b�g�I�t ���g��
	double c = .95 * PI / Max(ic, oc);	// c = �J�b�g�I�t
	double fc = c * r / (2 * PI);

	// �t�B���^������Ă݂�
	// FIR LPF (���֐��̓J�C�U�[��)
	n = (M+1) * ic;						// n = �t�B���^�̎���
	
	delete[] h2;
	h2 = new float[(ic+1)*(M+1)];
	
	double gain = 2 * ic * fc / r;
	double a = 10.;					// a = �j�~��ł̌����ʂ����߂�
	double d = bessel0(a);

	int j=0;
	for (int i=0; i<=ic; i++)
	{
		int ii=i;
		for (int o=0; o<=M; o++)
		{
			if (ii > 0)
			{
				double x = (double)ii/(double)(n);
				double x2 = x * x;
				double w = bessel0(sqrt(1.0-x2) * a) / d;
				double g = c * (double)ii;
				double z = sin(g) / g * w;
				h2[j] = gain * z;
			}
			else
			{
				h2[j] = gain;
			}
			j++;
			ii += ic;
		}
	}
	oo=0;
}

// ---------------------------------------------------------------------------
//	�o�b�t�@���特��Ⴄ
//
int SamplingRateConverter::Get(Sample* dest, int samples)
{
	CriticalSection::Lock lock(cs);
	if (!buffer)
		return 0;

	int count;
	int ss = samples;
	for (count=samples; count>0; count--)
	{
		int p = read;

		int i;
		float* h;

		float z0=0.f, z1=0.f;

		h = &h2[(ic-oo) * (M+1) + (M)];
		for (i=-M; i<=0; i++)
		{
			z0 += *h * buffer[p*2];
			z1 += *h * buffer[p*2+1];
			h--;
			p++;
			if (p == buffersize)
				p = 0;
		}

		h = &h2[oo * (M+1)];
		for (; i<=M; i++)
		{
			z0 += *h * buffer[p*2];
			z1 += *h * buffer[p*2+1];
			h++;
			p++;
			if (p == buffersize)
				p = 0;
		}
		*dest++ = Limit(z0, 32767, -32768);
		*dest++ = Limit(z1, 32767, -32768);

		oo -= oc;
		while (oo < 0)
		{
			read++;
			if (read == buffersize)
				read = 0;
			if (Avail() < 2*M+1)
				FillMain(Max(ss, count));
			ss = 0;
			oo += ic;
		}
	}
	return samples;
}