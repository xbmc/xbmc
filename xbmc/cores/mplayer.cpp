#include "mplayer.h"

extern "C" void dllReleaseAll( );

CMPlayer::CMPlayer(IPlayerCallback& callback)
:IPlayer(callback)
{
	m_pDLL=NULL;
	m_bIsPlaying=NULL;
}

CMPlayer::~CMPlayer()
{
	Unload();
}

bool CMPlayer::openfile(const CStdString& strFile)
{
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
			delete m_pDLL;
			m_pDLL=false;
			return false;
		}

		mplayer_load_dll(*m_pDLL);


		int argc=2;
		char *argv[] = {"xbmc.xbe","1.avi",NULL};
		mplayer_init(argc,argv);
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
					int iRet=mplayer_process();
					if (iRet < 0)
					{
						m_bIsPlaying=false;
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