#include "mplayer.h"
#include "mplayer/mplayer.h"
#include "../util.h"

extern "C" void dllReleaseAll( );
extern	void mplayer_reset_video_window();

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
	if (!m_pDLL)
	{
		OutputDebugString("cmplayer::openfile() load dll\n");
		m_pDLL = new DllLoader("Q:\\mplayer\\mplayer.dll");
		if( !m_pDLL->Parse() )
		{
			OutputDebugString("cmplayer::openfile() parse failed\n");
			delete m_pDLL;
			m_pDLL=false;
			return false;
		}
		OutputDebugString("cmplayer::openfile() resolve imports\n");
		if( !m_pDLL->ResolveImports()  )
		{
			OutputDebugString("cmplayer::openfile() resolve imports failed\n");
#ifndef _DEBUG
			delete m_pDLL;
			m_pDLL=false;
			return false;
#endif
		}

		OutputDebugString("cmplayer::openfile() load functions\n");
		mplayer_load_dll(*m_pDLL);



		int argc=2;
		char *argv[] = {"xbmc.xbe", "1.avi",NULL};
		OutputDebugString("cmplayer::openfile() call init\n");
		mplayer_init(argc,argv);
		mplayer_put_key('o');
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
	m_bStopPlaying=false;
	m_bIsPlaying=true;
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
		if (!bGotStartEvent && timeGetTime() - m_dwTime > 5000)
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
				m_bIsPlaying=false;
				mplayer_close_file();
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


void CMPlayer::VideoWindowChanged()
{
	mplayer_reset_video_window();
}