#ifndef _SURF3D_H
#define _SURF3D_H

#include "v3d.h"
#include "goom_graphic.h"
#include "goom_typedefs.h"

typedef struct {
	v3d *vertex;
	v3d *svertex;
	int nbvertex;

	v3d center;
} surf3d;

typedef struct {
	surf3d surf;
	
	int defx;
	int sizex;
	int defz;
	int sizez;
	int mode;
} grid3d;

/* hi-level */

/* works on grid3d */
grid3d *grid3d_new (int sizex, int defx, int sizez, int defz, v3d center);
void grid3d_update (grid3d *s, float angle, float *vals, float dist);

/* low level */
void surf3d_draw (surf3d *s, int color, int dist, int *buf, int *back, int W,int H);
void grid3d_draw (PluginInfo *plug, grid3d *g, int color, int colorlow, int dist, Pixel *buf, Pixel *back, int W,int H);
void surf3d_rotate (surf3d *s, float angle);
void surf3d_translate (surf3d *s);

#endif
