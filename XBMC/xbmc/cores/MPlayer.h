#pragma once
#include "iplayer.h"
#include "dllLoader/dll.h"
#include "mplayer/mplayer.h"
#include "stdstring.h"
#include "../utils/thread.h"
#include "../utils/event.h"

class CMPlayer : public IPlayer, public CThread
{
public:
	CMPlayer(IPlayerCallback& callback);
	virtual ~CMPlayer();
	virtual bool		openfile(const CStdString& strFile);
	virtual bool		closefile();
	virtual bool		IsPlaying() const;
	virtual void		Pause();
	virtual bool		IsPaused() const;
	virtual void		Unload();
	virtual __int64	GetPTS();
	virtual bool		HasVideo();
	virtual bool		HasAudio();
	virtual void    VideoWindowChanged();

protected:
	virtual void		OnStartup();
	virtual void		OnExit();
	virtual void		Process();
	bool						m_bPaused;
	bool						m_bIsPlaying;
	bool						m_bStopPlaying;
	DllLoader*			m_pDLL;
	CEvent					m_startEvent;
	__int64					m_iPTS;
	DWORD						m_dwTime;
};