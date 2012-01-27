/*
    SDL_anigif:  An example animated GIF image loading library for use with SDL
    SDL_image Copyright (C) 1997-2006 Sam Lantinga
    Animated GIF "derived work" Copyright (C) 2006 Doug McFadyen

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <string.h>
#include "SDL_anigif.h"



/* Code from here to end of file has been adapted from XPaint:           */
/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993 David Koblas.                          | */
/* | Copyright 1996 Torsten Martinsen.                                 | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.	       | */
/* +-------------------------------------------------------------------+ */
/* Adapted for use in SDL by Sam Lantinga -- 7/20/98 */
/* Animated GIF support by Doug McFadyen -- 10/19/06 */

#define	MAXCOLORMAPSIZE			256

#define	TRUE					1
#define	FALSE					0

#define CM_RED					0
#define CM_GREEN				1
#define CM_BLUE					2

#define	MAX_LWZ_BITS			12

#define INTERLACE				0x40
#define LOCALCOLORMAP			0x80
#define BitSet(byte,bit)		(((byte) & (bit)) == (bit))
#define LM_to_uint(a,b)			(((b)<<8)|(a))

#define SDL_SetError(t)			((void)0)		/* We're not SDL so ignore error reporting */


typedef struct
{
	unsigned int	Width;
	unsigned int	Height;
	unsigned char	ColorMap[3][MAXCOLORMAPSIZE];
	unsigned int	BitPixel;
	unsigned int	ColorResolution;
	unsigned int	Background;
	unsigned int	AspectRatio;
} gifscreen;

typedef struct
{
	int				transparent;
	int				delayTime;
	int				inputFlag;
	int				disposal;
} gif89;

typedef struct
{
	/* global data */
	SDL_RWops*		src;
	gifscreen		gs;
	gif89			g89;
	int				zerodatablock;
	/* AG_LoadGIF_RW data */
	unsigned char	localColorMap[3][MAXCOLORMAPSIZE];
	/* GetCode data */
	unsigned char	buf[280];
	int				curbit, lastbit, done, lastbyte;
	/* LWZReadByte data */
	int				fresh, code, incode;
	int				codesize, setcodesize;
	int				maxcode, maxcodesize;
	int				firstcode, oldcode;
	int				clearcode, endcode;
	int				table[2][(1 << MAX_LWZ_BITS)];
	int				stack[(1 << (MAX_LWZ_BITS))*2], *sp;
} gifdata;



static int			ReadColorMap( gifdata* gd, int number, unsigned char buffer[3][MAXCOLORMAPSIZE] );
static int			DoExtension( gifdata* gd, int label );
static int			GetDataBlock( gifdata* gd, unsigned char* buf );
static int			GetCode( gifdata* gd, int code_size, int flag );
static int			LWZReadByte( gifdata* gd, int flag, int input_code_size );
static SDL_Surface*	ReadImage( gifdata* gd, int len, int height, int, unsigned char cmap[3][MAXCOLORMAPSIZE], int interlace, int ignore );



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
int AG_isGIF( SDL_RWops* src )
{
	int isGIF = FALSE;

	if ( src )
	{
		int start = SDL_RWtell( src );
		char magic[6];

		if ( SDL_RWread(src,magic,sizeof(magic),1) )
		{
			if ( (strncmp(magic,"GIF",3) == 0) && ((memcmp(magic+3,"87a",3) == 0) || (memcmp(magic+3,"89a",3) == 0)) )
			{
				isGIF = TRUE;
			}
		}

		SDL_RWseek( src, start, SEEK_SET );
	}

	return isGIF;
}



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
int AG_LoadGIF( const char* file, AG_Frame* frames, int size )
{
	int n = 0;

	SDL_RWops* src = SDL_RWFromFile( file, "rb" );

	if ( src )
	{
		n = AG_LoadGIF_RW( src, frames, size );
		SDL_RWclose( src );
	}

	return n;
}



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
void AG_FreeSurfaces( AG_Frame* frames, int nFrames )
{
	int i;

	if ( frames )
	{
		for ( i = 0; i < nFrames; i++ )
		{
			if ( frames[i].surface )
			{
				SDL_FreeSurface( frames[i].surface );
				frames[i].surface = NULL;
			}
		}
	}
}



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
int AG_ConvertSurfacesToDisplayFormat( AG_Frame* frames, int nFrames )
{
	int i;
	int n = 0;

	if ( frames )
	{
		for ( i = 0; i < nFrames; i++ )
		{
			if ( frames[i].surface )
			{
				SDL_Surface* surface = (frames[i].surface->flags & SDL_SRCCOLORKEY) ? SDL_DisplayFormatAlpha(frames[i].surface) : SDL_DisplayFormat(frames[i].surface);

				if ( surface )
				{
					SDL_FreeSurface( frames[i].surface );
					frames[i].surface = surface;
					n++;
				}
			}
		}
	}

	return n;
}



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
int AG_NormalizeSurfacesToDisplayFormat( AG_Frame* frames, int nFrames )
{
	int n = 0;

	if ( nFrames > 0 && frames && frames[0].surface )
	{
		SDL_Surface* mainSurface = (frames[0].surface->flags & SDL_SRCCOLORKEY) ? SDL_DisplayFormatAlpha(frames[0].surface) : SDL_DisplayFormat(frames[0].surface);
		const int newDispose = (frames[0].surface->flags & SDL_SRCCOLORKEY) ? AG_DISPOSE_RESTORE_BACKGROUND : AG_DISPOSE_NONE;

		if ( mainSurface )
		{
			int i;
			int lastDispose = AG_DISPOSE_NA;
			int iRestore = 0;
			const Uint8 alpha = (frames[0].disposal == AG_DISPOSE_NONE) ? SDL_ALPHA_OPAQUE : SDL_ALPHA_TRANSPARENT;

			SDL_FillRect( mainSurface, NULL, SDL_MapRGBA(mainSurface->format,0,0,0,alpha) );

			for ( i = 0; i < nFrames; i++ )
			{
				if ( frames[i].surface )
				{
					SDL_Surface* surface = SDL_ConvertSurface( mainSurface, mainSurface->format, mainSurface->flags );

					if ( surface )
					{
						SDL_Rect r;

						if ( lastDispose == AG_DISPOSE_NONE )
							SDL_BlitSurface( frames[i-1].surface, NULL, surface, NULL );

						if ( lastDispose == AG_DISPOSE_RESTORE_PREVIOUS )
							SDL_BlitSurface( frames[iRestore].surface, NULL, surface, NULL );
						if ( frames[i].disposal != AG_DISPOSE_RESTORE_PREVIOUS )
							iRestore = i;

						r.x = (Sint16)frames[i].x;
						r.y = (Sint16)frames[i].y;
						SDL_BlitSurface( frames[i].surface, NULL, surface, &r );

						SDL_FreeSurface( frames[i].surface );
						frames[i].surface = surface;
						frames[i].x = frames[i].y = 0;
						lastDispose = frames[i].disposal;
						frames[i].disposal = newDispose;
						n++;
					}
				}
			}

			SDL_FreeSurface( mainSurface );
		}
	}

	return n;
}



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
int AG_LoadGIF_RW( SDL_RWops* src, AG_Frame* frames, int maxFrames )
{
	int start;
	unsigned char buf[16];
	unsigned char c;
	int useGlobalColormap;
	int bitPixel;
	int iFrame = 0;
	char version[4];
	SDL_Surface* image = NULL;
	gifdata* gd;

	if ( src == NULL )
		return 0;

	gd = (gifdata*)malloc( sizeof(*gd) );
	memset( gd, 0, sizeof(*gd) );
	gd->src = src;

	start = SDL_RWtell( src );

	if ( !SDL_RWread(src,buf,6,1) )
	{
		SDL_SetError( "error reading magic number" );
		goto done;
	}

	if ( strncmp((char*)buf,"GIF",3) != 0 )
	{
		SDL_SetError( "not a GIF file" );
		goto done;
	}

	strncpy( version, (char*)buf+3, 3 );
	version[3] = '\0';

	if ( (strcmp(version,"87a") != 0) && (strcmp(version,"89a") != 0) )
	{
		SDL_SetError( "bad version number, not '87a' or '89a'" );
		goto done;
	}

	gd->g89.transparent	= -1;
	gd->g89.delayTime	= -1;
	gd->g89.inputFlag	= -1;
	gd->g89.disposal	= AG_DISPOSE_NA;

	if ( !SDL_RWread(src,buf,7,1) )
	{
		SDL_SetError( "failed to read screen descriptor" );
		goto done;
	}

	gd->gs.Width			= LM_to_uint(buf[0],buf[1]);
	gd->gs.Height			= LM_to_uint(buf[2],buf[3]);
	gd->gs.BitPixel			= 2 << (buf[4] & 0x07);
	gd->gs.ColorResolution	= (((buf[4] & 0x70) >> 3) + 1);
	gd->gs.Background		= buf[5];
	gd->gs.AspectRatio		= buf[6];

	if ( BitSet(buf[4],LOCALCOLORMAP) )		/* Global Colormap */
	{
		if ( ReadColorMap(gd,gd->gs.BitPixel,gd->gs.ColorMap) )
		{
			SDL_SetError( "error reading global colormap" );
			goto done;
		}
	}

	do
	{
		if ( !SDL_RWread(src,&c,1,1) )
		{
			SDL_SetError( "EOF / read error on image data" );
			goto done;
		}

		if ( c == ';' )		/* GIF terminator */
			goto done;

		if ( c == '!' )		/* Extension */
		{
			if ( !SDL_RWread(src,&c,1,1) )
			{
				SDL_SetError( "EOF / read error on extention function code" );
				goto done;
			}
			DoExtension( gd, c );
			continue;
		}

		if ( c != ',' )		/* Not a valid start character */
			continue;

		if ( !SDL_RWread(src,buf,9,1) )
		{
			SDL_SetError( "couldn't read left/top/width/height" );
			goto done;
		}

		useGlobalColormap = !BitSet(buf[8],LOCALCOLORMAP);
		bitPixel = 1 << ((buf[8] & 0x07) + 1);

		if ( !useGlobalColormap )
		{
			if ( ReadColorMap(gd,bitPixel,gd->localColorMap) )
			{
				SDL_SetError( "error reading local colormap" );
				goto done;
			}
			image = ReadImage( gd, LM_to_uint(buf[4],buf[5]), LM_to_uint(buf[6],buf[7]), bitPixel, gd->localColorMap, BitSet(buf[8],INTERLACE), (frames==NULL) );
		}
		else
		{
			image = ReadImage( gd, LM_to_uint(buf[4],buf[5]), LM_to_uint(buf[6],buf[7]), gd->gs.BitPixel, gd->gs.ColorMap, BitSet(buf[8],INTERLACE), (frames==NULL) );
		}

		if ( frames )
		{
			if ( image == NULL )
				goto done;

			if ( gd->g89.transparent >= 0 )
				SDL_SetColorKey( image, SDL_SRCCOLORKEY, gd->g89.transparent );

			frames[iFrame].surface	= image;
			frames[iFrame].x		= LM_to_uint(buf[0], buf[1]);
			frames[iFrame].y		= LM_to_uint(buf[2], buf[3]);
			frames[iFrame].disposal	= gd->g89.disposal;
			frames[iFrame].delay	= gd->g89.delayTime*10;
/*			gd->g89.transparent		= -1;			** Hmmm, not sure if this should be reset for each frame? */
		}

		iFrame++;
	} while ( iFrame < maxFrames || frames == NULL );

done:
	if ( image == NULL )
		SDL_RWseek( src, start, SEEK_SET );

	free( gd );

	return iFrame;
}



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
static int ReadColorMap( gifdata* gd, int number, unsigned char buffer[3][MAXCOLORMAPSIZE] )
{
	int i;
	unsigned char rgb[3];
	int flag;

	flag = TRUE;

	for ( i = 0; i < number; ++i )
	{
		if ( !SDL_RWread(gd->src,rgb,sizeof(rgb),1) )
		{
			SDL_SetError( "bad colormap" );
			return 1;
		}

		buffer[CM_RED][i]	= rgb[0];
		buffer[CM_GREEN][i]	= rgb[1];
		buffer[CM_BLUE][i]	= rgb[2];
		flag &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
	}

	return FALSE;
}



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
static int DoExtension( gifdata* gd, int label )
{
	unsigned char buf[256];

	switch ( label )
	{
	  case 0x01:		/* Plain Text Extension */
		break;

	  case 0xff:		/* Application Extension */
		break;

	  case 0xfe:		/* Comment Extension */
		while ( GetDataBlock(gd,buf) != 0 )
			;
		return FALSE;

	  case 0xf9:		/* Graphic Control Extension */
		(void)GetDataBlock( gd, buf );
		gd->g89.disposal	= (buf[0] >> 2) & 0x7;
		gd->g89.inputFlag	= (buf[0] >> 1) & 0x1;
		gd->g89.delayTime	= LM_to_uint(buf[1],buf[2]);
		if ( (buf[0] & 0x1) != 0 )
			gd->g89.transparent = buf[3];

		while ( GetDataBlock(gd,buf) != 0 )
			;
		return FALSE;
	}

	while ( GetDataBlock(gd,buf) != 0 )
		;

	return FALSE;
}



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
static int GetDataBlock( gifdata* gd, unsigned char* buf )
{
	unsigned char count;

	if ( !SDL_RWread(gd->src,&count,1,1) )
	{
		/* pm_message("error in getting DataBlock size" ); */
		return -1;
	}

	gd->zerodatablock = count == 0;

	if ( (count != 0) && !SDL_RWread(gd->src,buf,count,1) )
	{
		/* pm_message("error in reading DataBlock" ); */
		return -1;
	}

	return count;
}



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
static int GetCode( gifdata* gd, int code_size, int flag )
{
	int i, j, ret;
	int count;

	if ( flag )
	{
		gd->curbit = 0;
		gd->lastbit = 0;
		gd->done = FALSE;
		return 0;
	}

	if ( (gd->curbit + code_size) >= gd->lastbit )
	{
		if ( gd->done )
		{
			if ( gd->curbit >= gd->lastbit )
			SDL_SetError( "ran off the end of my bits" );
			return -1;
		}

		gd->buf[0] = gd->buf[gd->lastbyte - 2];
		gd->buf[1] = gd->buf[gd->lastbyte - 1];

		if ( (count = GetDataBlock(gd, &gd->buf[2])) == 0 )
			gd->done = TRUE;

		gd->lastbyte = 2 + count;
		gd->curbit = (gd->curbit - gd->lastbit) + 16;
		gd->lastbit = (2 + count)*8;
	}

	ret = 0;
	for ( i = gd->curbit, j = 0; j < code_size; ++i, ++j )
		ret |= ((gd->buf[i / 8] & (1 << (i % 8))) != 0) << j;

	gd->curbit += code_size;

	return ret;
}



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
static int LWZReadByte( gifdata* gd, int flag, int input_code_size )
{
	int i, code, incode;

	if ( flag )
	{
		gd->setcodesize	= input_code_size;
		gd->codesize	= gd->setcodesize + 1;
		gd->clearcode	= 1 << gd->setcodesize;
		gd->endcode		= gd->clearcode + 1;
		gd->maxcodesize	= gd->clearcode*2;
		gd->maxcode		= gd->clearcode + 2;

		GetCode( gd, 0, TRUE );

		gd->fresh = TRUE;

		for ( i = 0; i < gd->clearcode; ++i )
		{
			gd->table[0][i] = 0;
			gd->table[1][i] = i;
		}

		for ( ; i < (1 << MAX_LWZ_BITS); ++i )
			gd->table[0][i] = gd->table[1][0] = 0;

		gd->sp = gd->stack;
		return 0;
	}
	else if ( gd->fresh )
	{
		gd->fresh = FALSE;
		do
		{
			gd->firstcode = gd->oldcode = GetCode( gd, gd->codesize, FALSE );
		} while ( gd->firstcode == gd->clearcode );
		return gd->firstcode;
	}

	if ( gd->sp > gd->stack )
		return *--gd->sp;

	while ( (code = GetCode(gd,gd->codesize,FALSE)) >= 0 )
	{
		if ( code == gd->clearcode )
		{
			for ( i = 0; i < gd->clearcode; ++i )
			{
				gd->table[0][i] = 0;
				gd->table[1][i] = i;
			}

			for ( ; i < (1 << MAX_LWZ_BITS); ++i )
				gd->table[0][i] = gd->table[1][i] = 0;

			gd->codesize	= gd->setcodesize + 1;
			gd->maxcodesize	= gd->clearcode*2;
			gd->maxcode		= gd->clearcode + 2;
			gd->sp			= gd->stack;
			gd->firstcode	= gd->oldcode = GetCode( gd, gd->codesize, FALSE );
			return gd->firstcode;
		}
		else if ( code == gd->endcode )
		{
			int count;
			unsigned char buf[260];

			if ( gd->zerodatablock )
				return -2;

			while ( (count = GetDataBlock(gd,buf)) > 0 )
				;

			if ( count != 0 )
			{
				/* pm_message("missing EOD in data stream (common occurence)"); */
			}
			return -2;
		}

		incode = code;

		if ( code >= gd->maxcode )
		{
			*gd->sp++ = gd->firstcode;
			code = gd->oldcode;
		}

		while ( code >= gd->clearcode )
		{
			*gd->sp++ = gd->table[1][code];
			if ( code == gd->table[0][code] )
				SDL_SetError( "circular table entry BIG ERROR" );
			code = gd->table[0][code];
		}

		*gd->sp++ = gd->firstcode = gd->table[1][code];

		if ( (code = gd->maxcode) < (1 << MAX_LWZ_BITS) )
		{
			gd->table[0][code] = gd->oldcode;
			gd->table[1][code] = gd->firstcode;
			++gd->maxcode;
			if ( (gd->maxcode >= gd->maxcodesize) && (gd->maxcodesize < (1 << MAX_LWZ_BITS)) )
			{
				gd->maxcodesize *= 2;
				++gd->codesize;
			}
		}

		gd->oldcode = incode;

		if ( gd->sp > gd->stack )
			return *--gd->sp;
	}

	return code;
}



/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/
static SDL_Surface* ReadImage( gifdata* gd, int len, int height, int cmapSize, unsigned char cmap[3][MAXCOLORMAPSIZE], int interlace, int ignore )
{
	SDL_Surface* image;
	unsigned char c;
	int i, v;
	int xpos = 0, ypos = 0, pass = 0;

	/* Initialize the compression routines */
	if ( !SDL_RWread(gd->src,&c,1,1) )
	{
		SDL_SetError( "EOF / read error on image data" );
		return NULL;
	}

	if ( LWZReadByte(gd,TRUE,c) < 0 )
	{
		SDL_SetError( "error reading image" );
		return NULL;
	}

	/* If this is an "uninteresting picture" ignore it. */
	if ( ignore )
	{
		while ( LWZReadByte(gd,FALSE,c) >= 0 )
			;
		return NULL;
	}

	image = SDL_AllocSurface( SDL_SWSURFACE, len, height, 8, 0, 0, 0, 0 );

	for ( i = 0; i < cmapSize; i++ )
	{
		image->format->palette->colors[i].r = cmap[CM_RED][i];
		image->format->palette->colors[i].g = cmap[CM_GREEN][i];
		image->format->palette->colors[i].b = cmap[CM_BLUE][i];
	}

	while ( (v = LWZReadByte(gd,FALSE,c)) >= 0 )
	{
		((Uint8*)image->pixels)[xpos + ypos*image->pitch] = (Uint8)v;
		++xpos;

		if ( xpos == len )
		{
			xpos = 0;
			if ( interlace )
			{
				switch ( pass )
				{
				  case 0:
				  case 1:	ypos += 8;	break;
				  case 2:	ypos += 4;	break;
				  case 3:	ypos += 2;	break;
				}

				if ( ypos >= height )
				{
					++pass;
					switch ( pass )
					{
					  case 1:	ypos = 4;	break;
					  case 2:	ypos = 2;	break;
					  case 3:	ypos = 1;	break;
					  default:	goto fini;
					}
				}
			}
			else
			{
				++ypos;
			}
		}

		if ( ypos >= height )
			break;
	}

fini:
	return image;
}

