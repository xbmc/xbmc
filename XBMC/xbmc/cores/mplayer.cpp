#include "mplayer.h"
#include "mplayer/mplayer.h"
#include "../util.h"
extern "C" void dllReleaseAll( );

#define KEY_ENTER 13
#define KEY_TAB 9

#define KEY_BASE 0x100

/*  Function keys  */
#define KEY_F (KEY_BASE+64)

/* Control keys */
#define KEY_CTRL (KEY_BASE)
#define KEY_BACKSPACE (KEY_CTRL+0)
#define KEY_DELETE (KEY_CTRL+1)
#define KEY_INSERT (KEY_CTRL+2)
#define KEY_HOME (KEY_CTRL+3)
#define KEY_END (KEY_CTRL+4)
#define KEY_PAGE_UP (KEY_CTRL+5)
#define KEY_PAGE_DOWN (KEY_CTRL+6)
#define KEY_ESC (KEY_CTRL+7)

/* Control keys short name */
#define KEY_BS KEY_BACKSPACE
#define KEY_DEL KEY_DELETE
#define KEY_INS KEY_INSERT
#define KEY_PGUP KEY_PAGE_UP
#define KEY_PGDOWN KEY_PAGE_DOWN
#define KEY_PGDWN KEY_PAGE_DOWN

/* Cursor movement */
#define KEY_CRSR (KEY_BASE+16)
#define KEY_RIGHT (KEY_CRSR+0)
#define KEY_LEFT (KEY_CRSR+1)
#define KEY_DOWN (KEY_CRSR+2)
#define KEY_UP (KEY_CRSR+3)

/* XF86 Multimedia keyboard keys */
#define KEY_XF86_BASE (0x100+384)
#define KEY_XF86_PAUSE (KEY_XF86_BASE+1)
#define KEY_XF86_STOP (KEY_XF86_BASE+2)
#define KEY_XF86_PREV (KEY_XF86_BASE+3)
#define KEY_XF86_NEXT (KEY_XF86_BASE+4)
  
/* Keypad keys */
#define KEY_KEYPAD (KEY_BASE+32)
#define KEY_KP0 (KEY_KEYPAD+0)
#define KEY_KP1 (KEY_KEYPAD+1)
#define KEY_KP2 (KEY_KEYPAD+2)
#define KEY_KP3 (KEY_KEYPAD+3)
#define KEY_KP4 (KEY_KEYPAD+4)
#define KEY_KP5 (KEY_KEYPAD+5)
#define KEY_KP6 (KEY_KEYPAD+6)
#define KEY_KP7 (KEY_KEYPAD+7)
#define KEY_KP8 (KEY_KEYPAD+8)
#define KEY_KP9 (KEY_KEYPAD+9)
#define KEY_KPDEC (KEY_KEYPAD+10)
#define KEY_KPINS (KEY_KEYPAD+11)
#define KEY_KPDEL (KEY_KEYPAD+12)
#define KEY_KPENTER (KEY_KEYPAD+13)


CMPlayer::CMPlayer(IPlayerCallback& callback)
:IPlayer(callback)
{
	m_pDLL=NULL;
	m_bIsPlaying=NULL;
	m_bPaused=false;
}

CMPlayer::~CMPlayer()
{
	Unload();
}

bool CMPlayer::openfile(const CStdString& strFile)
{
	m_dwTime=timeGetTime();

	m_iPTS=0;
	m_bPaused=false;
	closefile();
	m_bStopPlaying=false;
	m_bIsPlaying=true;

	if (!m_pDLL)
	{
		OutputDebugString("cmplayer::openfile() load dll\n");
		m_pDLL = new DllLoader("Q:\\mplayer\\mplayer.dll");
		if( !m_pDLL->Parse() )
		{
			OutputDebugString("cmplayer::openfile() parse failed\n");
			delete m_pDLL;
			m_pDLL=NULL;
			return false;
		}
		OutputDebugString("cmplayer::openfile() resolve imports\n");
		if( !m_pDLL->ResolveImports()  )
		{
			OutputDebugString("cmplayer::openfile() resolve imports failed\n");
		}

		OutputDebugString("cmplayer::openfile() load functions\n");
		mplayer_load_dll(*m_pDLL);



		int argc=6;
		char *argv[] = {"xbmc.xbe", "-autoq", "6", "-vf", "pp", "1.avi",NULL};
		OutputDebugString("cmplayer::openfile() call init\n");
		mplayer_init(argc,argv);
		
		Create();
	}

	OutputDebugString("cmplayer::openfile() setcachesize\n");
	mplayer_setcache_size(1024);
	if (CUtil::IsAudio(strFile) )
	{
		mplayer_setcache_size(0);
	}

	OutputDebugString("cmplayer::openfile() openfile\n");
	m_dwTime=timeGetTime();
	int iRet=mplayer_open_file(strFile.c_str());
	if (iRet < 0)
	{
		OutputDebugString("cmplayer::openfile() openfile failed\n");
		closefile();
		return false;
	}
	
	m_dwTime=timeGetTime();
	m_startEvent.Set();
	return true;
}

bool CMPlayer::closefile()
{
	if (m_bIsPlaying && m_pDLL)
	{
		OutputDebugString("cmplayer::openfile() closefile\n");
		m_bStopPlaying=true;
		while (m_bIsPlaying)
		{
			Sleep(10);
		}
		OutputDebugString("cmplayer::openfile() closefile done\n");
	}
	return true;
}

bool CMPlayer::IsPlaying() const
{
	return m_bIsPlaying;
}


void CMPlayer::OnStartup()
{
}

void CMPlayer::OnExit()
{
}

void CMPlayer::Process()
{
	while (!m_bStop)
	{
		m_dwTime=timeGetTime();
		bool bGotStartEvent=m_startEvent.WaitMSec(6000);
		if (!m_bIsPlaying && !bGotStartEvent && timeGetTime() - m_dwTime > 5000)
		{
			// unload the dll if we didnt play anything for > 5 sec
			if (m_pDLL)
			{
				OutputDebugString("cmplayer::process() timeout closefile\n");
				mplayer_close_file();		
				OutputDebugString("cmplayer::process() timeout delete dll\n");
				delete m_pDLL;
				OutputDebugString("cmplayer::process() timeout release all\n");
				dllReleaseAll( );
				m_pDLL=NULL;
			}
//			SetThreadPriority( GetCurrentThread(),THREAD_PRIORITY_NORMAL);
			return;
		}

		if (m_pDLL && bGotStartEvent )
		{
			//SetThreadPriority( GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
			m_startEvent.Reset();
			if (m_bIsPlaying) 
			{
				m_callback.OnPlayBackStarted();
				do 
				{
					if (!m_bPaused)
					{
						int iRet=mplayer_process();
						if (iRet < 0)
						{
							m_bIsPlaying=false;
						}
						
						__int64 iPTS=mplayer_get_pts();
						if (iPTS)
						{
							m_iPTS=iPTS;
						}
					}
					else 
					{
						Sleep(100);
					}
				} while (!m_bStopPlaying && m_bIsPlaying && !m_bStop);
				OutputDebugString("cmplayer::process() end playing\n");
				mplayer_close_file();
				m_bIsPlaying=false;
				if (!m_bStopPlaying && !m_bStop)
				{
					m_callback.OnPlayBackEnded();
				}
			}
		}
	}
}

void CMPlayer::Unload()
{
	OutputDebugString("cmplayer::process() unload() stopthread\n");
	StopThread();
	if (m_pDLL)
	{
		OutputDebugString("cmplayer::process() unload() closefile\n");
		mplayer_close_file();		
		OutputDebugString("cmplayer::process() unload() delete dll\n");
		delete m_pDLL;
		OutputDebugString("cmplayer::process() unload() release all\n");
		dllReleaseAll( );
		m_pDLL=NULL;
	}
}

void  CMPlayer::Pause()
{
	m_bPaused=!m_bPaused;
}

bool CMPlayer::IsPaused() const
{
	return m_bPaused;
}


__int64	CMPlayer::GetPTS()
{
	if (!m_pDLL) return 0;
	if (!m_bIsPlaying) return 0;
	return m_iPTS;
}

bool CMPlayer::HasVideo()
{
	return (mplayer_HasVideo()==TRUE);
}

bool CMPlayer::HasAudio()
{
	return (mplayer_HasAudio()==TRUE);
}

void CMPlayer::ToggleOSD()
{
	mplayer_put_key('o');
}

void CMPlayer::SwitchToNextLanguage()
{
	mplayer_put_key('l');
}

void CMPlayer::ToggleSubtitles()
{
	mplayer_put_key('s');
}


void CMPlayer::SubtitleOffset(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('x');
	else
		mplayer_put_key('z');
}

void CMPlayer::Seek(bool bPlus, bool bLargeStep)
{
	if (bLargeStep)
	{
		if (bPlus)
			mplayer_put_key(KEY_PAGE_UP);
		else
			mplayer_put_key(KEY_PAGE_DOWN);
	}
	else
	{
		if (bPlus)
			mplayer_put_key(KEY_UP);
		else
			mplayer_put_key(KEY_DOWN);
	}
}

void CMPlayer::ToggleFrameDrop()
{
	mplayer_put_key('d');
}


void CMPlayer::SetVolume(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('0');
	else
		mplayer_put_key('9');
}

void CMPlayer::SetContrast(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('2');
	else
		mplayer_put_key('1');
}

void CMPlayer::SetBrightness(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('4');
	else
		mplayer_put_key('3');
}
void CMPlayer::SetHue(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('6');
	else
		mplayer_put_key('5');
}
void CMPlayer::SetSaturation(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('8');
	else
		mplayer_put_key('7');
}



void CMPlayer::GetAudioInfo( CStdString& strAudioInfo)
{
	char strFourCC[10];
	char strAudioCodec[128];
	long lBitRate;
	long lSampleRate;
	int	 iChannels;
	BOOL bVBR;
	mplayer_GetAudioInfo(strFourCC,strAudioCodec, &lBitRate, &lSampleRate, &iChannels, &bVBR);
	float fSampleRate=((float)lSampleRate) / 1000.0f;
	if (bVBR)
	{
		strAudioInfo.Format("audio:%s (%s) VBR br:%i sr:%02.2f khz chns:%i",
									strFourCC,strAudioCodec,lBitRate,fSampleRate,iChannels);
	}
	else
	{
		strAudioInfo.Format("audio:%s (%s) CBR br:%i sr:%02.2f khz chns:%i",
									strFourCC,strAudioCodec,lBitRate,fSampleRate,iChannels);
	}
}

void CMPlayer::GetVideoInfo( CStdString& strVideoInfo)
{

	char  strFourCC[10];
	char  strVideoCodec[128];
	float fFPS;
	unsigned int   iWidth;
	unsigned int   iHeight;
	long  lFrames2Early;
	long  lFrames2Late;
	mplayer_GetVideoInfo(strFourCC,strVideoCodec, &fFPS, &iWidth,&iHeight, &lFrames2Early, &lFrames2Late);
	strVideoInfo.Format("video:%s fps:%02.2f %ix%i early/late:%i/%i", 
											strFourCC,fFPS,iWidth,iHeight,lFrames2Early,lFrames2Late);
}


void CMPlayer::GetGeneralInfo( CStdString& strVideoInfo)
{
	long lFramesDropped;
	int iQuality;
	int iCacheFilled;
	float fTotalCorrection;
	float fAVDelay;
	mplayer_GetGeneralInfo(&lFramesDropped, &iQuality, &iCacheFilled, &fTotalCorrection, &fAVDelay);
	strVideoInfo.Format("dropped:%i Q:%i cache:%i ct:%2.2f av:%2.2f", 
												lFramesDropped, iQuality, iCacheFilled, fTotalCorrection, fAVDelay);
}

extern void xbox_audio_registercallback(IAudioCallback* pCallback);
extern void xbox_audio_unregistercallback();
extern void xbox_video_getRect(RECT& SrcRect, RECT& DestRect);
extern void xbox_video_update();

void CMPlayer::Update()
{
	xbox_video_update();
}

void CMPlayer::GetVideoRect(RECT& SrcRect, RECT& DestRect)
{
	xbox_video_getRect(SrcRect, DestRect);
}

void CMPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
	xbox_audio_registercallback(pCallback);
}

void CMPlayer::UnRegisterAudioCallback()
{
	xbox_audio_unregistercallback();
}