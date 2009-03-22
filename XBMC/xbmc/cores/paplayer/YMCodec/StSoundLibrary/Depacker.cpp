/*-----------------------------------------------------------------------------

	ST-Sound ( YM files player library )

	Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )

	LzhXLib wrapper. 

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

#include "LZH/lzh.h"
#include "YmMusic.h"
#include "Depacker.h"
#include <string.h>


static	const	ymu8	*	s_pSrc = 0;
static			ymu8	*	s_pDst = 0;
static			ymint		s_depackedSize = 0;
static			ymint		s_depackedPos = 0;


static	ymint		ReadCallback(void *pBuffer,ymint size)
{
	memcpy(pBuffer,s_pSrc,size);
	s_pSrc += size;
	return size;
}

static	ymint		WriteCallback(void *pBuffer,ymint size)
{
	ymint safeSize = (s_depackedPos + size <= s_depackedSize) ? size : s_depackedSize - s_depackedPos;

	if (safeSize > 0)
	{
		memcpy(s_pDst,pBuffer,safeSize);
		s_depackedPos += safeSize;
		s_pDst += safeSize;
		return safeSize;
	}
	else
		return -1;
}

static	void	*	MallocCallback(unsigned int size)
{
	ymu8 *p = new ymu8[size];
	return (void*)p;
}

static	void		FreeCallback(void *p)
{
	if (p)
	{
		ymu8 *cp = (ymu8 *)p;
		delete [] cp;
	}
}

ymbool		LzhDepackBlock(const ymu8 *pSrc,ymu8 *pDst,ymint depackedSize)
{

	s_pSrc = pSrc;
	s_pDst = pDst;
	s_depackedPos = 0;
	s_depackedSize = depackedSize;

	ymint error = lzh_melt(ReadCallback,WriteCallback,MallocCallback,FreeCallback,s_depackedSize);

	return ((0 == error) ? YMTRUE : YMFALSE);
}

