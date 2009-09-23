/*-----------------------------------------------------------------------------

	ST-Sound ( YM files player library )

	Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )

	Manage YM file depacking and parsing

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

#ifndef __YMLOAD__
#define	__YMLOAD__

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	ymu8	size;
	ymu8	sum;
	char	id[5];
	ymu32	packed;
	ymu32	original;
	ymu8	reserved[5];
	ymu8	level;
	ymu8	name_lenght;
} lzhHeader_t;


#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif


