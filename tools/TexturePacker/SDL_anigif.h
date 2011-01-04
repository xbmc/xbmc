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

#ifndef _SDL_ANIGIF_H
#define _SDL_ANIGIF_H

#include <SDL/SDL.h>
#include <SDL/begin_code.h>
#ifdef __cplusplus
	extern "C" {
#endif



typedef struct
{
	SDL_Surface*	surface;				/* SDL surface for this frame */
	int				x, y;					/* Frame offset position */
	int				disposal;				/* Disposal code */
	int				delay;					/* Frame delay in ms */
	int				user;					/* User data (not used by aniGIF) */
} AG_Frame;

#define AG_DISPOSE_NA					0	/* No disposal specified */
#define AG_DISPOSE_NONE					1	/* Do not dispose */
#define AG_DISPOSE_RESTORE_BACKGROUND	2	/* Restore to background */
#define AG_DISPOSE_RESTORE_PREVIOUS		3	/* Restore to previous */



extern DECLSPEC int		AG_isGIF( SDL_RWops* src );
extern DECLSPEC int		AG_LoadGIF( const char* file, AG_Frame* frames, int maxFrames );
extern DECLSPEC void	AG_FreeSurfaces( AG_Frame* frames, int nFrames );
extern DECLSPEC int		AG_ConvertSurfacesToDisplayFormat( AG_Frame* frames, int nFrames );
extern DECLSPEC int		AG_NormalizeSurfacesToDisplayFormat( AG_Frame* frames, int nFrames );
extern DECLSPEC int		AG_LoadGIF_RW( SDL_RWops* src, AG_Frame* frames, int size );



#ifdef __cplusplus
	}
#endif
#include <SDL/close_code.h>

#endif /* _SDL_ANIGIF_H */
