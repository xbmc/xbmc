#ifndef _SDL_rotozoom_h
#define _SDL_rotozoom_h

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C"
{
#endif

#include <math.h>
#ifndef M_PI
#define M_PI	3.141592654
#endif
#include "SDL.h"


/* ---- Defines */

#define SMOOTHING_OFF		0
#define SMOOTHING_ON		1

/* ---- Structures */

  typedef struct tColorRGBA
  {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
  }
  tColorRGBA;

  typedef struct tColorY
  {
    Uint8 y;
  }
  tColorY;


/* ---- Prototypes */

/* 
 
 rotozoomSurface()

 Rotates and zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
 'angle' is the rotation in degrees. 'zoom' a scaling factor. If 'smooth' is 1
 then the destination 32bit surface is anti-aliased. If the surface is not 8bit
 or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

*/

  SDL_Surface *rotozoomSurface (SDL_Surface * src, double angle, double zoom,
				int smooth);


/* 
 
 zoomSurface()

 Zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
 'zoomx' and 'zoomy' are scaling factors for width and height. If 'smooth' is 1
 then the destination 32bit surface is anti-aliased. If the surface is not 8bit
 or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

*/

  SDL_Surface *zoomSurface (SDL_Surface * src, double zoomx, double zoomy,
			    int smooth);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
};
#endif

#endif /* _SDL_rotozoom_h */
