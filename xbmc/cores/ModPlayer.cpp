#include "modplayer.h"
#include "../util.h"
#include "../application.h"
#include "../xbox/iosupport.h"
#include "../utils/log.h"
#include "../SectionLoader.h"
#include "../URL.h"
#include "../FileSystem/FileSmb.h"

#pragma comment(linker,"/merge:MOD_RD=MOD_RX")
#pragma comment(linker,"/section:MOD_RX,REN")
#pragma comment(linker,"/section:MOD_RW,RWN")

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

void ModCallback(unsigned char* p, int s)
{
	static bool call = true;
	if (m_pCallback)
	{
		if (call)
			m_pCallback->OnAudioData(p, s);
		call = !call;
	}
}

ModPlayer::ModPlayer(IPlayerCallback& callback) :IPlayer(callback)
{
	m_bIsPlaying=false;
	m_bPaused=false;

	CSectionLoader::Load("MOD_RX");
	CSectionLoader::Load("MOD_RW");

	if (!mikwinInit(44100, true, true, true, 32, 32, 20))
	{
		CLog::Log("ModPlayer: Could not initialize sound, reason: %s", MikMod_strerror(mikwinGetErrno()));
	}

	mikwinSetMusicVolume(120);
	mikwinSetSfxVolume(127);
	mikxboxSetCallback(ModCallback);
}

ModPlayer::~ModPlayer()
{
	closefile();
	mikwinExit();

	CSectionLoader::Unload("MOD_RX");
	CSectionLoader::Unload("MOD_RW");
}

bool ModPlayer::openfile(const CStdString& strFile)
{
	closefile();

	char* str = NULL;
	if (!CUtil::IsHD(strFile))
	{
		CFile file;
		if (!file.Cache(strFile.c_str(),"Z:\\cachedmod",NULL,NULL))
		{
			::DeleteFile("Z:\\cachedmod");
			CLog::Log("ModPlayer: Unable to cache file %s\n", strFile.c_str());
			return false;
		}
		str = strdup("Z:\\cachedmod");
	}
	else
		str = strdup(strFile.c_str());

	m_pModule = Mod_Player_Load(str, 32, 0);
	free(str);

	if (m_pModule)
	{
		Mod_Player_Start(m_pModule);
	}
	else
	{
		CLog::Log("ModPlayer: Could not load module %s: %s\n", strFile.c_str(), MikMod_strerror(mikwinGetErrno()));
		return false;
	}

	m_bIsPlaying=true;
	m_bPaused=false;
	m_bStopPlaying=false;

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

#ifdef _DEBUG
static DWORD AveUpdate = 0;
static DWORD NumUpdates = 0;
static DWORD LastUpdate = 0;
#endif // _DEBUG

__int64	ModPlayer::GetPTS()
{
	if (!m_bIsPlaying) return 0;
#ifdef _DEBUG		
	// in debug mode, show mod update time
	if (NumUpdates)
	{
		CStdStringW wszText;
		wszText.Format(L"ModUpdate: %2dms/%2dms", LastUpdate, AveUpdate / NumUpdates);

		CGUIFont* pFont=g_fontManager.GetFont("font13");
		if (pFont)
		{
			pFont->DrawText( 480, 40, LastUpdate > 30 ? 0xffff0f0f : 0xffffffff, wszText);
		}
	}
#endif
	return mikxboxGetPTS();
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
	//this thing should get pumped at least once a frame at 30 fps
	DWORD dwQuantum = 1000 / 35;
#ifdef _DEBUG
	AveUpdate = 0;
	NumUpdates = 0;
#endif // _DEBUG
	while (!m_bStopPlaying)
	{
		DWORD t = timeGetTime();
		MikMod_Update();
		t = timeGetTime() - t;
#ifdef _DEBUG
		LastUpdate = t;
		AveUpdate += t;
		++NumUpdates;
#endif // _DEBUG

		if (t < dwQuantum)
			Sleep(dwQuantum - t);
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
