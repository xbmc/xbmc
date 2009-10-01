/*-----------------------------------------------------------------------------

	ST-Sound ( YM files player library )

	Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )

	Main header to use the StSound "C" like API in your production.

-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------

	This file is part of ST-Sound

	ST-Sound is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	ST-Sound is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ST-Sound; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

-----------------------------------------------------------------------------*/


#ifndef __STSOUNDLIBRARY__
#define __STSOUNDLIBRARY__

#include "YmTypes.h"

typedef	void			YMMUSIC;

typedef struct
{
	ymchar	*	pSongName;
	ymchar	*	pSongAuthor;
	ymchar	*	pSongComment;
	ymchar	*	pSongType;
	ymchar	*	pSongPlayer;
	yms32		musicTimeInSec;
} ymMusicInfo_t;

// Create object
extern	YMMUSIC *	ymMusicCreate();

// Release object
extern	void		ymMusicDestroy(YMMUSIC *pMusic);

// Functions
extern	ymbool			ymMusicLoad(YMMUSIC *pMusic,const char *fName);
extern	ymbool			ymMusicLoadMemory(YMMUSIC *pMusic,void *pBlock,ymu32 size);
extern	ymbool			ymMusicCompute(YMMUSIC *pMusic,ymsample *pBuffer,ymint nbSample);
extern	void			ymMusicSetLoopMode(YMMUSIC *pMusic,ymbool bLoop);
extern	const char	*	ymMusicGetLastError(YMMUSIC *pMusic);
extern	int				ymMusicGetRegister(YMMUSIC *pMusic,ymint reg);
extern	void			ymMusicGetInfo(YMMUSIC *pMusic,ymMusicInfo_t *pInfo);
extern	void			ymMusicPlay(YMMUSIC *pMusic);
extern	void			ymMusicPause(YMMUSIC *pMusic);
extern	void			ymMusicStop(YMMUSIC *pMusic);

extern	ymbool			ymMusicIsSeekable(YMMUSIC *pMusic);
extern	ymu32			ymMusicGetPos(YMMUSIC *pMusic);
extern	void			ymMusicSeek(YMMUSIC *pMusic,ymu32 timeInMs);

#endif
