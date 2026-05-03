// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: diskmgr.cpp,v 1.13 1999/11/26 10:13:46 cisc Exp $

#include "headers.h"
#include "diskmgr.h"
#include "status.h"
#include "misc.h"

using namespace D88;

// ---------------------------------------------------------------------------
//	�\�z�E�j��
//
DiskImageHolder::DiskImageHolder()
{
	ref = 0;
}

DiskImageHolder::~DiskImageHolder()
{
	Close();
}

// ---------------------------------------------------------------------------
//	�t�@�C�����J��
//
bool DiskImageHolder::Open(const char* filename, bool ro, bool create)
{
	// ���Ɏ����Ă���t�@�C�����ǂ������m�F
	if (Connect(filename))
		return true;
	
	if (ref > 0)
		return false;
	
	// �t�@�C�����J��
	readonly = ro;
	
	if (readonly || !fio.Open(filename, 0))
	{
		if (fio.Open(filename, FileIO::readonly))
		{
			if (!readonly)
				statusdisplay.Show(100, 3000, "�ǎ��p�t�@�C���ł�");
			readonly = true;
		}
		else
		{
			// �V�����f�B�X�N�C���[�W�H
			if (!create || !fio.Open(filename, FileIO::create))
			{
				statusdisplay.Show(80, 3000, "�f�B�X�N�C���[�W���J���܂���");
				return false;
			}
		}
	}
	
	// �t�@�C������o�^
	strncpy(diskname, filename, MAX_PATH-1);
	diskname[MAX_PATH-1] = 0;

	if (!ReadHeaders())
		return false;
	
	ref = 1;
	return true;
}

// ---------------------------------------------------------------------------
//	�V�����f�B�X�N�C���[�W��������
//	type:	2D 0 / 2DD 1 / 2HD 2
//
bool DiskImageHolder::AddDisk(const char* title, uint type)
{
	if (ndisks >= max_disks)
		return false;

	int32 diskpos = 0;
	if (ndisks > 0)
	{
		diskpos = disks[ndisks-1].pos + disks[ndisks-1].size;
	}
	DiskInfo& disk = disks[ndisks++];
	disk.pos = diskpos;
	disk.size = sizeof(ImageHeader);

	ImageHeader ih;
	memset(&ih, 0, sizeof(ImageHeader));
	strncpy(ih.title, title, 16);
	ih.disktype = type * 0x10;
	ih.disksize = sizeof(ImageHeader);
	fio.SetLogicalOrigin(0);
	fio.Seek(diskpos, FileIO::begin);
	fio.Write(&ih, sizeof(ImageHeader));
	return true;
}

// ---------------------------------------------------------------------------
//	�f�B�X�N�C���[�W�̏��𓾂�
//
bool DiskImageHolder::ReadHeaders()
{
	fio.SetLogicalOrigin(0);
	fio.Seek(0, FileIO::end);
	if (fio.Tellp() == 0)
	{
		// new file
		ndisks = 0;
		return true;
	}
	
	fio.Seek(0, FileIO::begin);
	
	ImageHeader ih;
	for (ndisks = 0; ndisks < max_disks; ndisks++)
	{
		// �w�b�_�[�ǂݍ���
		DiskInfo& disk = disks[ndisks];
		disk.pos = fio.Tellp();
		
		// 256+16 �� Raw �C���[�W�̍ŏ��T�C�Y
		if (fio.Read(&ih, sizeof(ImageHeader)) < 256+16)
			break;
		
		if (memcmp(ih.title, "M88 RawDiskImage", 16))
		{
			if (!IsValidHeader(ih))
			{
				statusdisplay.Show(90, 3000, "�C���[�W�ɖ����ȃf�[�^���܂܂�Ă��܂�");
				break;
			}
			
			strncpy(disk.title, ih.title, 16);
			disk.title[16] = 0;
			disk.size = ih.disksize;
			fio.Seek(disk.pos + disk.size, FileIO::begin);
		}
		else
		{
			if (ndisks != 0)
			{
				statusdisplay.Show(80, 3000, "READER �n�f�B�X�N�C���[�W�͘A���ł��܂���");
				return false;
			}

			strncpy(disk.title, "(no name)", 16);
			fio.Seek(0, FileIO::end);
			disk.size = fio.Tellp() - disk.pos;
		}
	}
	if (!ndisks)
		return false;

	return true;
}

// ---------------------------------------------------------------------------
//	�Ƃ���
//
void DiskImageHolder::Close()
{
	fio.Close();
	ndisks = 0;
	diskname[0] = 0;
	ref = 0;
}

// ---------------------------------------------------------------------------
//	Connect
//
bool DiskImageHolder::Connect(const char* filename)
{
	// ���Ɏ����Ă���t�@�C�����ǂ������m�F
	if (!strnicmp(diskname, filename, MAX_PATH))
	{
		ref++;
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	Disconnect
//
bool DiskImageHolder::Disconnect()
{
	if (--ref <= 0)
		Close();
	return true;
}

// ---------------------------------------------------------------------------
//	�w�b�_�[���L�����ǂ������m�F
//	
bool DiskImageHolder::IsValidHeader(ImageHeader& ih)
{
	int i;
	// 2D �C���[�W�̏ꍇ�]�v�ȗ̈�͌��Ȃ��������Ƃɂ���
	if (ih.disktype == 0)
		memset(&ih.trackptr[84], 0, 4*80);

	// ����: title �� 25 �����ȉ��ł��邱��
	for (i=0; i<25 && ih.title[i]; i++)
		;
	if (i==25)
		return false;
	
	// ����: disksize <= 4M
	if (ih.disksize > 4 * 1024 * 1024)
		return false;

	// ����: trackptr[0-159] < disksize
	uint trackstart = sizeof(ImageHeader);
	for (int t=0; t<160; t++)
	{
		if (ih.trackptr[t] >= ih.disksize)
			break;
		if (ih.trackptr[t] && ih.trackptr[t] < trackstart)
			trackstart = uint(ih.trackptr[t]);
	}
	
	// ����: 32+4*84 <= trackstart
	if (trackstart < 32 + 4 * 84)
		return false;
	
	return true;
}

// ---------------------------------------------------------------------------
//	GetTitle
//
const char* DiskImageHolder::GetTitle(int index)
{
	if (index < ndisks)
		return disks[index].title;
	return 0;
}

// ---------------------------------------------------------------------------
//	GetDisk
//
FileIO* DiskImageHolder::GetDisk(int index)
{
	if (index < ndisks)
	{
		fio.SetLogicalOrigin(disks[index].pos);
		return &fio;
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	SetDiskSize
//
bool DiskImageHolder::SetDiskSize(int index, int newsize)
{
	int i;
	if (index >= ndisks)
		return false;

	int32 sizediff = newsize - disks[index].size;
	if (!sizediff)
		return true;

	// �ړ�������K�v�̂���f�[�^�̃T�C�Y���v�Z����
	int32 sizemove=0;
	for (i=index+1; i<ndisks; i++)
	{
		sizemove += disks[i].size;
	}

	fio.SetLogicalOrigin(0);
	if (sizemove)
	{
		int32 moveorg = disks[index+1].pos;
		uint8* data = new uint8[sizemove];
		if (!data)
			return false;

		fio.Seek(moveorg, FileIO::begin);
		fio.Read(data, sizemove);
		fio.Seek(moveorg + sizediff, FileIO::begin);
		fio.Write(data, sizemove);

		delete[] data;

		for (i=index+1; i<ndisks; i++)
			disks[i].pos += sizemove;
	}
	else
	{
		fio.Seek(disks[index].pos + newsize, FileIO::begin);
	}
	fio.SetEndOfFile();
	disks[index].size = newsize;
	return true;
}

// ---------------------------------------------------------------------------
//	�\�z�E�j��
//
DiskManager::DiskManager()
{
	for (int i=0; i<max_drives; i++)
	{
		drive[i].holder = 0;
		drive[i].index = -1;
		drive[i].fdu.Init(this, i);
	}
}

DiskManager::~DiskManager()
{
	for (int i=0; i<max_drives; i++)
		Unmount(i);
}

// ---------------------------------------------------------------------------
//	������
//
bool DiskManager::Init()
{
	for (int i=0; i<max_drives; i++)
	{
		drive[i].holder = 0;
		if (!drive[i].fdu.Init(this, i))
			return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	�f�B�X�N�C���[�W�����ɊJ����Ă��邩�ǂ����m�F
//	arg:diskname	�f�B�X�N�C���[�W�̃t�@�C���l�[��
//	
bool DiskManager::IsImageOpen(const char* diskname)
{
	CriticalSection::Lock lock(cs);
	
	for (int i=0; i<max_drives; i++)
	{
		if (holder[i].Connect(diskname))
		{
			holder[i].Disconnect();
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
//	Mount
//	arg:dr			Mount ����h���C�u
//		diskname	�f�B�X�N�C���[�W�̃t�@�C���l�[��
//		readonly	�ǂݍ��݂̂�
//		index		mount ����f�B�X�N�C���[�W�̔ԍ� (-1 == no disk)
//
bool DiskManager::Mount
(uint dr, const char* diskname, bool readonly, int index, bool create)
{
	int i;

	Unmount(dr);
	
	CriticalSection::Lock lock(cs);
	// �f�B�X�N�C���[�W�����ł� hold ����Ă��邩�ǂ������m�F
	DiskImageHolder* h = 0;
	for (i=0; i<max_drives; i++)
	{
		if (holder[i].Connect(diskname))
		{
			h = &holder[i];
			// ���ꂩ��J���f�B�X�N�����ɊJ����Ă��Ȃ����Ƃ��m�F����
			if (index >= 0)
			{
				for (uint d=0; d<max_drives; d++)
				{
					if ((d != dr) && (drive[d].holder == h) && (drive[d].index == index))
					{
						index = -1;		// no disk
						statusdisplay.Show(90, 3000, "���̃f�B�X�N�͎g�p���ł�");
						break;
					}
				}
			}
			break;
		}
	}
	if (!h)			// �󂢂Ă��� holder �� hold ������
	{
		for (i=0; i<max_drives; i++)
		{
			if (!holder[i].IsOpen())
			{
				h = &holder[i];
				break;
			}
		}
		if (!h || !h->Open(diskname, readonly, create))
		{
			if (h)
				h->Disconnect();
			return 0;
		}
	}

	if (!h->GetNumDisks())
		index = -1;

	FileIO* fio = 0;
	if (index >= 0)
	{
		fio = h->GetDisk(index);
		if (!fio)
		{
			h->Disconnect();
			return false;
		}
	}
	drive[dr].holder = h;
	drive[dr].index = index;
	drive[dr].sizechanged = false;
	
	if (fio)
	{
		fio->Seek(0, FileIO::begin);
		if (!ReadDiskImage(fio, &drive[dr]))
		{
			h->Disconnect();
			drive[dr].holder = 0;
			return false;
		}
		memset(drive[dr].modified, 0, 164);
		
		drive[dr].fdu.Mount(&drive[dr].disk);
	}
	return true;
}	

// ---------------------------------------------------------------------------
//	�f�B�X�N�����O��
//
bool DiskManager::Unmount(uint dr)
{
	CriticalSection::Lock lock(cs);
	
	bool ret = true;
	Drive& drv = drive[dr];
	drive[dr].fdu.Unmount();
	if (drv.holder)
	{
		if (drv.index >= 0)
		{
			for (int t=0; t<164; t++)
			{
				if (drv.modified[t])
				{
					uint32 disksize = GetDiskImageSize(&drv);
					if (!drv.holder->SetDiskSize(drv.index, disksize))
					{
						ret = false;
						break;
					}

					FileIO* fio = drv.holder->GetDisk(drv.index);
					ret = fio ? WriteDiskImage(fio, &drv) : false;
					break;
				}
			}
		}
		drv.holder->Disconnect();
		drv.holder = 0;
	}
	if (!ret)
		statusdisplay.Show(50, 3000, "�f�B�X�N�̍X�V�Ɏ��s���܂���");
	return ret;
}

// ---------------------------------------------------------------------------
//	�f�B�X�N�C���[�W��ǂݍ���
//
bool DiskManager::ReadDiskImage(FileIO* fio, Drive* drive)
{
	uint t;
	ImageHeader ih;
	fio->Read(&ih, sizeof(ImageHeader));
	if (!memcmp(ih.title, "M88 RawDiskImage", 16))
		return ReadDiskImageRaw(fio, drive);
	
	// �f�B�X�N�̃^�C�v�`�F�b�N
	FloppyDisk::DiskType type;
	uint hd = 0;
	switch (ih.disktype)
	{
	case 0x00:
		type = FloppyDisk::MD2D; 
		memset(&ih.trackptr[84], 0, 4*80);
		break;

	case 0x10: 
		type = FloppyDisk::MD2DD; 
		break;

	case 0x20: 
		type = FloppyDisk::MD2HD; 
		hd = FloppyDisk::highdensity; 
		break;

	default:
		statusdisplay.Show(90, 3000, "�T�|�[�g���Ă��Ȃ����f�B�A�ł�");
		return false;
	}
	bool readonly = drive->holder->IsReadOnly() || ih.readonly;
	
	FloppyDisk& disk = drive->disk;
	if (!disk.Init(type, readonly))
	{
		statusdisplay.Show(70, 3000, "��Ɨp�̈�����蓖�Ă邱�Ƃ��ł��܂���ł���");
		return false;
	}

	// ���݂��������̂P
	for (t=0; t<disk.GetNumTracks(); t++)
	{
		if (ih.trackptr[t] >= ih.disksize)
			break;
	}
	if (t<164)
		memset(&ih.trackptr[t], 0, (164-t) * 4);
	if (t<(uint) Min(160, disk.GetNumTracks()))
		statusdisplay.Show(80, 3000, "�w�b�_�[�ɖ����ȃf�[�^���܂܂�Ă��܂�");

	// trackptr �̂��݂�����
	uint trackstart = sizeof(ImageHeader);
	for (t=0; t<84; t++)
	{
		if (ih.trackptr[t] && ih.trackptr[t] < trackstart)
			trackstart = (uint) ih.trackptr[t];
	}
	if (trackstart < sizeof(ImageHeader))
		memset(((char*) &ih) + trackstart, 0, sizeof(ImageHeader)-trackstart);

	// trackptr �f�[�^�̕ۑ�
	for (t=0; t<164; t++)
	{
		drive->trackpos[t] = ih.trackptr[t];
		drive->tracksize[t] = 0;
	}
	for (t=0; t<168; t++)
	{
		disk.Seek(t);
		disk.FormatTrack(0, 0);
	}

	// �e�g���b�N�̓ǂݍ���
	for (t=0; t<disk.GetNumTracks(); t++)
	{
		int cy = t >> 1;
		if (type == FloppyDisk::MD2D)
			cy *= 2;
		disk.Seek((cy * 2) + (t & 1));
		if (ih.trackptr[t])
		{
			fio->Seek(ih.trackptr[t], FileIO::begin);
			int sot = 0;
			int i = 0;
			SectorHeader sh;
			do
			{
				if (fio->Read(&sh, sizeof(sh)) != sizeof(sh))
					break;
				
				FloppyDisk::Sector* sec = disk.AddSector(sh.length);
				if (!sec)
					break;
				sec->id = sh.id;
				sec->size = sh.length;
				sec->flags = (sh.density ^ 0x40) | hd;
				if (sh.deleted == 0x10)
					sec->flags |= FloppyDisk::deleted;

				switch (sh.status)
				{
				case 0xa0: sec->flags |= FloppyDisk::idcrc;   break;
				case 0xb0: sec->flags |= FloppyDisk::datacrc; break;
				case 0xf0: sec->flags |= FloppyDisk::mam;     break;
				}
				if (fio->Read(sec->image, sh.length) != sh.length)
					break;
				sot += 0x10 + sh.length;
			} while (++i < sh.sectors);

			drive->tracksize[t] = sot;
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
//	�f�B�X�N�C���[�W (READER �`��) ��ǂݍ���
//
bool DiskManager::ReadDiskImageRaw(FileIO* fio, Drive* drive)
{
	fio->Seek(16, FileIO::begin);

	bool readonly = drive->holder->IsReadOnly();
	
	FloppyDisk& disk = drive->disk;
	if (!disk.Init(FloppyDisk::MD2D, readonly))
	{
		statusdisplay.Show(70, 3000, "��Ɨp�̈�����蓖�Ă邱�Ƃ��ł��܂���ł���");
		return false;
	}

	int t;
	for (t=0; t<164; t++)
	{
		drive->trackpos[t] = 0;
		drive->tracksize[t] = 0;
	}
	for (t=0; t<168; t++)
	{
		disk.Seek(t);
		disk.FormatTrack(0, 0);
	}

	// �e�g���b�N�̓ǂݍ���
	uint8 buf[256];
	FloppyDisk::IDR id;
	id.n = 1;
	for (t=0; t<80; t++)
	{
		id.c = t / 2;
		id.h = t & 1;

		disk.Seek(id.c * 4 + id.h);
		disk.FormatTrack(0, 0);

		for (int r=1; r<=16; r++)
		{
			id.r = r;
			
			if (fio->Read(buf, 256) != 256)
				break;

			FloppyDisk::Sector* sec = disk.AddSector(256);
			if (!sec)
				break;
			sec->id = id;
			sec->size = 256;
			sec->flags = 0x40;
			memcpy(sec->image, buf,256);
		}
	}
	drive->sizechanged = false;

	return true;
}


// ---------------------------------------------------------------------------
//	�f�B�X�N�C���[�W�̃T�C�Y���v�Z����
//
uint DiskManager::GetDiskImageSize(Drive* drv)
{
	uint disksize = sizeof(ImageHeader);

	for (int t=drv->disk.GetNumTracks()-1; t>=0; t--)
	{
		int tr = (drv->disk.GetType() == FloppyDisk::MD2D) ? t & ~1 : t >> 1;
		tr = (tr << 1) | (t & 1);

		FloppyDisk::Sector* sec;
		for (sec = drv->disk.GetFirstSector(tr); sec; sec=sec->next)
		{
			disksize += sec->size + sizeof(SectorHeader);
		}
	}
	return disksize;
}
	
// ---------------------------------------------------------------------------
//	�f�B�X�N�C���[�W�̏����o��
//	�K�v�ƂȂ�̈�͂��炩���ߊm�ۂ���Ă��邱�ƂƂ���
//
bool DiskManager::WriteDiskImage(FileIO* fio, Drive* drv)
{
	static const uint8 typetbl[3] = { 0x00, 0x10, 0x20 };
	int t;

	// Header �̍쐬
	ImageHeader ih;
	memset(&ih, 0, sizeof(ImageHeader));
	strcpy(ih.title, drv->holder->GetTitle(drv->index));
	
	ih.disktype = typetbl[drv->disk.GetType()];
	ih.readonly = drv->disk.IsReadOnly() ? 0x10 : 0;
	
	uint32 disksize = sizeof(ImageHeader);
	int ntracks = drv->disk.GetNumTracks();

	for (t=0; t<ntracks; t++)
	{
		int tracksize = 0;
		int tr = (drv->disk.GetType() == FloppyDisk::MD2D) ? t & ~1 : t >> 1;
		tr = (tr << 1) | (t & 1);

		FloppyDisk::Sector* sec;
		for (sec = drv->disk.GetFirstSector(tr); sec; sec=sec->next)
			tracksize += sec->size + sizeof(SectorHeader);

		ih.trackptr[t] = tracksize ? disksize : 0;
		disksize += tracksize;
	}
	for (; t<164; t++)
		ih.trackptr[t] = 0;
	
	ih.disksize = disksize;

	if (!fio->Seek(0, FileIO::begin))
		return false;
	if (fio->Write(&ih, sizeof(ImageHeader)) != sizeof(ImageHeader))
		return false;

	for (t=0; t<ntracks; t++)
	{
		int tr = (drv->disk.GetType() == FloppyDisk::MD2D) ? t & ~1 : t >> 1;
		tr = (tr << 1) | (t & 1);
		if (!WriteTrackImage(fio, drv, tr))
			return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	�g���b�N����̃C���[�W������
//
bool DiskManager::WriteTrackImage(FileIO* fio, Drive* drv, int t)
{
	SectorHeader sh;
	memset(&sh, 0, sizeof(SectorHeader));
	
	FloppyDisk::Sector* sec;
	int nsect = 0;
	for (sec = drv->disk.GetFirstSector(t); sec; sec=sec->next)
		nsect++;
	sh.sectors = nsect;
	
	for (sec = drv->disk.GetFirstSector(t); sec; sec=sec->next)
	{
		sh.id = sec->id;
		sh.density = (~sec->flags) & 0x40;
		sh.deleted = sec->flags & 1 ? 0x10 : 0;
		sh.length = sec->size;
		sh.status = 0;
		switch (sec->flags & 14)
		{
		case FloppyDisk::idcrc:		sh.status = 0xa0; break;
		case FloppyDisk::datacrc:	sh.status = 0xb0; break;
		case FloppyDisk::mam:		sh.status = 0xf0; break;
		}
		if (fio->Write(&sh, sizeof(SectorHeader)) != sizeof(SectorHeader))
			return false;
		if (uint(fio->Write(sec->image, sec->size)) != sec->size)
			return false;;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	Unlock
//	Disk �ύX�錾
//
void DiskManager::Modified(int dr, int tr)
{
	if (0 <= tr && tr < 164 && !drive[dr].disk.IsReadOnly())
	{
		drive[dr].modified[tr] = true;
	}
}

// ---------------------------------------------------------------------------
//	Update
//	�g���b�N�̈ʒu��ς����ɍX�V�ł���ύX����������
//
void DiskManager::Update()
{
	for (int d=0; d<max_drives; d++)
		UpdateDrive(&drive[d]);
}

// ---------------------------------------------------------------------------
//	UpdateDrive
//
void DiskManager::UpdateDrive(Drive* drv)
{
	if (!drv->holder || drv->sizechanged)
		return;

	CriticalSection::Lock lock(cs);
	int t;
	for (t=0; t<164 && !drv->modified[t]; t++)
		;
	if (t < 164)
	{
		FileIO* fio = drv->holder->GetDisk(drv->index);
		if (fio)
		{
			for (; t<164; t++)
			{
				if (drv->modified[t])
				{
					FloppyDisk::Sector* sec;
					int tracksize = 0;
					
					for (sec = drv->disk.GetFirstSector(t); sec; sec=sec->next)
						tracksize += sec->size + sizeof(SectorHeader);
					
					if (tracksize <= drv->tracksize[t])
					{
						drv->modified[t] = false;
						fio->Seek(drv->trackpos[t], FileIO::begin);
						WriteTrackImage(fio, drv, t);
					}
					else
					{
						drv->sizechanged = true;
						break;
					}
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
//	�C���[�W�^�C�g���擾
//
const char* DiskManager::GetImageTitle(uint dr, uint index)
{
	CriticalSection::Lock lock(cs);
	if (dr < max_drives && drive[dr].holder)
	{
		return drive[dr].holder->GetTitle(index);
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	C[W̐擾
//
uint DiskManager::GetNumDisks(uint dr)
{
	CriticalSection::Lock lock(cs);
	if (dr < max_drives)
	{
		if (drive[dr].holder)
			return drive[dr].holder->GetNumDisks();
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	ݑIĂfBXN̔ԍ擾
//
int DiskManager::GetCurrentDisk(uint dr)
{
	CriticalSection::Lock lock(cs);
	if (dr < max_drives)
	{
		if (drive[dr].holder)
			return drive[dr].index;
	}
	return -1;
}


// ---------------------------------------------------------------------------
//	�f�B�X�N�ǉ�
//	dr		�Ώۃh���C�u
//	title	�f�B�X�N�^�C�g��
//	type	b1-0	�f�B�X�N�̃��f�B�A�^�C�v
//					00 = 2D, 01 = 2DD, 10 = 2HD
//
bool DiskManager::AddDisk(uint dr, const char* title, uint type)
{
	if (dr < max_drives)
	{
		if (drive[dr].holder && drive[dr].holder->AddDisk(title, type))
			return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	N88-BASIC �W���t�H�[�}�b�g���|����
//	�����ȕ��@��(^^;
//
bool DiskManager::FormatDisk(uint dr)
{
	if (!drive[dr].holder || drive[dr].disk.GetType() != FloppyDisk::MD2D)
		return false;
//	statusdisplay.Show(10, 5000, "Format drive : %d", dr);
	
	uint8* buf = new uint8[80*16*256];
	if (!buf)
		return false;

	// �t�H�[�}�b�g
	memset(buf, 0xff, 80*16*256);
	// IPL
	buf[0] = 0xc9;
	// ID
	memset(&buf[0x25c00], 0, 256);
	buf[0x25c01] = 0xff;
	// FAT
	buf[0x25d4a] = 0xfe; buf[0x25d4b] = 0xfe;
	buf[0x25e4a] = 0xfe; buf[0x25e4b] = 0xfe;
	buf[0x25f4a] = 0xfe; buf[0x25f4b] = 0xfe;
	
	// ��������
	FloppyDisk& disk = drive[dr].disk;
	FloppyDisk::IDR id;
	id.n = 1;
	uint8* dest = buf;

	for (int t=0; t<80; t++)
	{
		id.c = t / 2, id.h = t & 1;

		disk.Seek(id.c * 4 + id.h);
		disk.FormatTrack(0, 0);

		for (int r=1; r<=16; r++)
		{
			id.r = r;

			FloppyDisk::Sector* sec = disk.AddSector(256);
			if (!sec)
				break;
			sec->id = id, sec->size = 256;
			sec->flags = 0x40;
			memcpy(sec->image, dest, 256);
			dest += 256;
		}
	}
	drive->sizechanged = true;
	drive->modified[0] = true;
	delete[] buf;
	return true;
}
