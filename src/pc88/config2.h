


//			type	 name				default value	����	���
DECLARE_CONFIG_INT	(BASICMode,			0x31,			0,		0xff)
DECLARE_CONFIG_INT	(ERAMBanks,			0,				0,		256)
DECLARE_CONFIG_INT	(DipSwitch,			1829,			0,		INT_MAX)


// CPU
DECLARE_CONFIG_INT	(CPUClock,			40,				1,		1000)
DECLARE_CONFIG_SYM	(CPUMSSpeed,		CPUMSSpeedType,	msauto)


DECLARE_CONFIG_BOOL	(SubCPUControl,		true)								// Sub CPU �̋쓮�𐧌䂷��
DECLARE_CONFIG_BOOL	(FullSpeed,			false)								// �S�͓���
DECLARE_CONFIG_INT	(Speed,				1000,			500,	2000)
DECLARE_CONFIG_BOOL	(NoWaitMode,		false)								// �m�[�E�F�C�g
DECLARE_CONFIG_BOOL	(FDDWait,			true)								// FDD �E�F�C�g
DECLARE_CONFIG_BOOL	(CPUWait,			true)								// �E�F�C�g�̃G�~�����[�V����


DECLARE_CONFIG_BOOL	(EnablePCG,			false)								// PCG �n�̃t�H���g����������L��
DECLARE_CONFIG_SYM	(DisplayMode,		DispModeType,	24k)				// 15KHz ���j�^�[���[�h
DECLARE_CONFIG_SYM	(PaletteMode,		PalModeType,	Analogue)			// �p���b�g���[�h
DECLARE_CONFIG_BOOL	(Force480,			false)								// �S��ʂ���� 640x480 ��

DECLARE_CONFIG_BOOL	(FullLine,			true)								// �������C���\��
DECLARE_CONFIG_BOOL	(DrawPriorityLow,	false)								// �`��̗D��x�𗎂Ƃ�
DECLARE_CONFIG_INT	(DrawInterval,		3,				1,		4)



DECLARE_CONFIG_SYM	(SoundDriver,		SoundDriverType,DirectSound)		// PCM �̍Đ��Ɏg�p����h���C�o
DECLARE_CONFIG_INT	(SoundBuffer,		200,			50,		1000)
DECLARE_CONFIG_BOOL	(SoundPriorityHigh,	false)								// �������d���������̍����𑱂���

DECLARE_CONFIG_BOOL	(EnableCMDSING,		true)								// CMD SING �L��
DECLARE_CONFIG_SYM	(OpnType44h,		OpnType,		OPN)				// OPN (44h)
DECLARE_CONFIG_SYM	(OpnTypeA8h,		OpnType,		None)				// OPN (a8h)

DECLARE_CONFIG_INT	(PCMRate,			22100,			8000,	96000)
DECLARE_CONFIG_BOOL	(FMMix55k,			true)								// FM �����̍����ɖ{���̃N���b�N���g�p
DECLARE_CONFIG_BOOL	(PreciseMixing,		true)								// �����x�ȍ������s��
DECLARE_CONFIG_BOOL	(EnableLPF,			false)								// LPF ���g���Ă݂�
DECLARE_CONFIG_INT	(LPFCutoff,			9000,			3000,	48000)
DECLARE_CONFIG_INT	(LPFOrder,			4,				2,		16)

DECLARE_CONFIG_INT	(VolumeFM,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumePSG,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeADPCM,		0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeRhythm,		0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeBD,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeSD,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeTOP,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeHH,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeTOM,			0,				-100,	40)
DECLARE_CONFIG_INT	(VolumeRIM,			0,				-100,	40)



DECLARE_CONFIG_BOOL	(ShowStatusBar,		false)								// �X�e�[�^�X�o�[�\��
DECLARE_CONFIG_BOOL	(ShowFDCStatus,		false)								// FDC �̃X�e�[�^�X��\��

DECLARE_CONFIG_BOOL	(UseF12AsReset,		true)								// F12 �� COPY �̑���� RESET �Ƃ��Ďg�p
DECLARE_CONFIG_BOOL	(ShowPlacesBar,		false)								// �t�@�C���_�C�A���O�� PLACESBAR ��\������
DECLARE_CONFIG_SYM	(JoyPortMode,		JoyPortModeType,None)				//
DECLARE_CONFIG_BOOL	(UseALTasGRPH,		false)								// ALT �� GRPH ��
DECLARE_CONFIG_BOOL	(UseArrowAs10key,	false)
DECLARE_CONFIG_BOOL	(UseNumRowAs10key,	false)								// �����L�[���e���L�[��
DECLARE_CONFIG_BOOL	(SwapPadButtons,	false)								// �p�b�h�̃{�^�������ւ�
DECLARE_CONFIG_SYM	(ScreenShotName,	ScreenShotNameType, Ask)			// �X�N���[���V���b�g�t�@�C�����̎w��@
DECLARE_CONFIG_BOOL	(CompressSnapshot,	true)								// �X�i�b�v�V���b�g�����k����
DECLARE_CONFIG_SYM	(KeyboardType,		KeyType,		AT106)
DECLARE_CONFIG_BOOL	(SaveDirectory,		true)								// �N�����ɑO��I�����̃f�B���N�g���Ɉړ�
DECLARE_CONFIG_BOOL	(AskBeforeReset,	false)								// �I���E���Z�b�g���Ɋm�F



