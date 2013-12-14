
#ifndef _PLEXGLOBALCACHER_H_
#define _PLEXGLOBALCACHER_H_

#include "VideoThumbLoader.h"
#include "threads/Thread.h"
#include "threads/Event.h"

class CPlexGlobalCacher : public CPlexThumbCacher, CThread
{
public :
	CPlexGlobalCacher();
	~CPlexGlobalCacher();
	void Start();
	void Process();
	void OnJobComplete(unsigned int jobID, bool success, CJob *job);

protected :
	int m_MediaTotal;
	int m_MediaCount;

	CEvent m_CompletedEvent;
};

#endif /* _PLEXGLOBALCACHER_H_*/ 