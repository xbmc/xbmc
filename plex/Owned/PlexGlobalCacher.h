
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

protected :

};

#endif /* _PLEXGLOBALCACHER_H_*/ 
