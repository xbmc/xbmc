#include "mplayer.h"
#include "mplayer/mplayer.h"

extern "C" void dllReleaseAll( );

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
	m_iPTS=0;
	m_bPaused=false;
	closefile();
	if (!m_pDLL)
	{
		m_pDLL = new DllLoader("Q:\\mplayer\\mplayer.dll");
		if( !m_pDLL->Parse() )
		{
			delete m_pDLL;
			m_pDLL=false;
			return false;
		}
		if( !m_pDLL->ResolveImports()  )
		{
#ifndef _DEBUG
			delete m_pDLL;
			m_pDLL=false;
			return false;
#endif
		}

		mplayer_load_dll(*m_pDLL);



		int argc=2;
		char *argv[] = {"xbmc.xbe", "1.avi",NULL};
		mplayer_init(argc,argv);
		mplayer_put_key('o');
		Create();
	}

	int iRet=mplayer_open_file(strFile.c_str());
	if (iRet < 0)
	{
		closefile();
		return false;
	}
	
	m_bStopPlaying=false;
	m_bIsPlaying=true;
	m_startEvent.Set();
	return true;
}

bool CMPlayer::closefile()
{
	if (m_bIsPlaying && m_pDLL)
	{
		m_bStopPlaying=true;
		while (m_bIsPlaying)
		{
			Sleep(10);
		}
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
	DWORD dwTimer;
	while (!m_bStop)
	{
		dwTimer=timeGetTime();
		bool bGotStartEvent=m_startEvent.WaitMSec(6000);
		if (!bGotStartEvent && timeGetTime() - dwTimer > 5000)
		{
			// unload the dll if we didnt play anything for > 5 sec
			if (m_pDLL)
			{
				mplayer_close_file();		
				delete m_pDLL;
				dllReleaseAll( );
				m_pDLL=NULL;
			}
			return;
		}

		if (m_pDLL && bGotStartEvent )
		{
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
	StopThread();
	if (m_pDLL)
	{
		mplayer_close_file();		
		delete m_pDLL;
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