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
	virtual bool	openfile(const CStdString& strFile);
	virtual bool  closefile();
	virtual bool  IsPlaying() const;
	void					Unload();
protected:
	virtual void		OnStartup();
	virtual void		OnExit();
	virtual void		Process();

	bool						m_bIsPlaying;
	bool						m_bStopPlaying;
	DllLoader*			m_pDLL;
	CEvent					m_startEvent;
};