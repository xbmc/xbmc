#include "modplayer.h"
#include "../util.h"
#include "../application.h"
#include "../xbox/iosupport.h"
#include "../utils/log.h"

static IAudioCallback* m_pCallback=NULL;

bool ModPlayer::IsSupportedFormat(const CStdString& strFmt)
{
	CStdString fmt(strFmt);
	fmt.Normalize();
	if (fmt == "mod" ||
			fmt == "amf" ||
			fmt == "669" ||
			fmt == "dmf" ||
			fmt == "dsm" ||
			fmt == "far" ||
			fmt == "gdm" ||
			fmt == "imf" ||
			fmt == "it"  ||
			fmt == "m15" ||
			fmt == "med" ||
			fmt == "okt" ||
			fmt == "s3m" ||
			fmt == "stm" ||
			fmt == "sfx" ||
			fmt == "ult" ||
			fmt == "uni" ||
			fmt == "xm")
	{
		return true;
	}
	return false;
}


ModPlayer::ModPlayer(IPlayerCallback& callback) :IPlayer(callback)
{
	m_bIsPlaying=false;
	m_bPaused=false;

	if (!mikwinInit(44100, true, true, true, 32, 32, 16))
	{
		CLog::Log("ModPlayer: Could not initialize sound, reason: %s", MikMod_strerror(mikwinGetErrno()));
	}

	mikwinSetMusicVolume(120);
	mikwinSetSfxVolume(127);
}

ModPlayer::~ModPlayer()
{
	closefile();
	mikwinExit();
}

bool ModPlayer::openfile(const CStdString& strFile)
{
	closefile();

	char* str = strdup(strFile.c_str());
	m_pModule = Mod_Player_Load(str, 32, 0);
	free(str);
	if (m_pModule)
	{
		Mod_Player_Start(m_pModule);
	}
	else
	{
		CLog::Log("ModPlayer: Could not load module, reason: %s\n", MikMod_strerror(mikwinGetErrno()));
		return false;
	}

	m_bIsPlaying=true;
	m_bPaused=false;
	m_bStopPlaying=false;
	m_iPTS=0;

	if ( ThreadHandle() == NULL)
	{
		Create();
	}

	return true;
}

bool ModPlayer::closefile()
{
	if (IsPaused())
		Pause();

	m_bStopPlaying=true;
	StopThread();
	return true;
}

bool ModPlayer::IsPlaying() const
{
	return m_bIsPlaying;
}


void  ModPlayer::Pause()
{
	if (!m_bIsPlaying) return;
	m_bPaused=!m_bPaused;
	Mod_Player_TogglePause();
}

bool ModPlayer::IsPaused() const
{
	return m_bPaused;
}


__int64	ModPlayer::GetPTS()
{
	return m_iPTS;
}

bool ModPlayer::HasVideo()
{
	return false;
}

bool ModPlayer::HasAudio()
{
	return true;
}

void ModPlayer::ToggleOSD()
{
}

void ModPlayer::SwitchToNextLanguage()
{
}

void ModPlayer::ToggleSubtitles()
{
}


void ModPlayer::SubtitleOffset(bool bPlus)
{
}

void ModPlayer::Seek(bool bPlus, bool bLargeStep)
{
}

void ModPlayer::ToggleFrameDrop()
{
}


void ModPlayer::SetVolume(bool bPlus)
{
}

void ModPlayer::SetContrast(bool bPlus)
{
}

void ModPlayer::SetBrightness(bool bPlus)
{
}
void ModPlayer::SetHue(bool bPlus)
{
}
void ModPlayer::SetSaturation(bool bPlus)
{
}



void ModPlayer::GetAudioInfo( CStdString& strAudioInfo)
{
}

void ModPlayer::GetVideoInfo( CStdString& strVideoInfo)
{
}


void ModPlayer::GetGeneralInfo( CStdString& strVideoInfo)
{
}

void ModPlayer::Update(bool bPauseDrawing)
{
}

void ModPlayer::OnStartup()
{
}

void ModPlayer::OnExit()
{
	Mod_Player_Free(m_pModule);
}

void ModPlayer::Process()
{
	//this thing should get pumped at least once a frame at 60 fps
	DWORD dwQuantum = 1000 / 60;
	while (!m_bStopPlaying)
	{
		MikMod_Update();
		Sleep(dwQuantum);
	}
}	
void ModPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
	m_pCallback=pCallback;
	if (m_bIsPlaying)
		m_pCallback->OnInitialize(2, 44100, 16);
}

void ModPlayer::UnRegisterAudioCallback()
{
	m_pCallback=NULL;
}
