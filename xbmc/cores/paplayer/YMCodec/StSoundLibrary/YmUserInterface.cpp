/*-----------------------------------------------------------------------------

	ST-Sound ( YM files player library )

	Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )

	ST-Sound library "C-like" interface wrapper

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


#include "YmMusic.h"
#include "StSoundLibrary.h"


// Static assert to check various type len

YMMUSIC	*	ymMusicCreate()
{
	return (YMMUSIC*)(new CYmMusic);
}


ymbool	ymMusicLoad(YMMUSIC *pMus,const char *fName)
{
	CYmMusic *pMusic = (CYmMusic*)pMus;
	return pMusic->load(fName);
}

ymbool	ymMusicLoadMemory(YMMUSIC *pMus,void *pBlock,ymu32 size)
{
	CYmMusic *pMusic = (CYmMusic*)pMus;
	return pMusic->loadMemory(pBlock,size);
}

void	ymMusicDestroy(YMMUSIC *pMus)
{
	CYmMusic *pMusic = (CYmMusic*)pMus;
	delete pMusic;
}

ymbool	ymMusicCompute(YMMUSIC *_pMus,ymsample *pBuffer,int nbSample)
{
	CYmMusic *pMusic = (CYmMusic*)_pMus;
	return pMusic->update(pBuffer,nbSample);
}

void	ymMusicSetLoopMode(YMMUSIC *_pMus,ymbool bLoop)
{
	CYmMusic *pMusic = (CYmMusic*)_pMus;
	pMusic->setLoopMode(bLoop);
}

const char	*ymMusicGetLastError(YMMUSIC *_pMus)
{
	CYmMusic *pMusic = (CYmMusic*)_pMus;
	return pMusic->getLastError();
}

int		ymMusicGetRegister(YMMUSIC *_pMus,ymint reg)
{
	CYmMusic *pMusic = (CYmMusic*)_pMus;
	return pMusic->readYmRegister(reg);
}

void	ymMusicGetInfo(YMMUSIC *_pMus,ymMusicInfo_t *pInfo)
{
	CYmMusic *pMusic = (CYmMusic*)_pMus;
	pMusic->getMusicInfo(pInfo);
}

void	ymMusicPlay(YMMUSIC *_pMus)
{
	CYmMusic *pMusic = (CYmMusic*)_pMus;
	pMusic->play();
}

void	ymMusicPause(YMMUSIC *_pMus)
{
	CYmMusic *pMusic = (CYmMusic*)_pMus;
	pMusic->pause();
}

void	ymMusicStop(YMMUSIC *_pMus)
{
	CYmMusic *pMusic = (CYmMusic*)_pMus;
	pMusic->stop();
}

ymbool		ymMusicIsSeekable(YMMUSIC *_pMus)
{
	CYmMusic *pMusic = (CYmMusic*)_pMus;
	return pMusic->isSeekable() ? YMTRUE : YMFALSE;
}

unsigned long	ymMusicGetPos(YMMUSIC *_pMus)
{
	CYmMusic *pMusic = (CYmMusic*)_pMus;
	if (!pMusic->isSeekable())
		return 0;

	return pMusic->getPos();
}

void		ymMusicSeek(YMMUSIC *_pMus,ymu32 timeInMs)
{
	CYmMusic *pMusic = (CYmMusic*)_pMus;
	if (pMusic->isSeekable())
	{
		pMusic->setMusicTime(timeInMs);
	}
}
