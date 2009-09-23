#ifndef _LINES_H
#define _LINES_H

/*
 *  lines.h
 *  Goom
 *  Copyright (c) 2000-2003 iOS-software. All rights reserved.
 */

#include "goom_typedefs.h"
#include "goom_graphic.h"
#include "goom_config.h"

struct _GMUNITPOINTER
{
	float   x;
	float   y;
	float   angle;
};

/* tableau de points */
struct _GMLINE
{

	GMUnitPointer *points;
	GMUnitPointer *points2;
	int     IDdest;
	float   param;
	float   amplitudeF;
	float   amplitude;

	int     nbPoints;
	guint32 color;     /* pour l'instant je stocke la couleur a terme, on stockera le mode couleur et l'on animera */
	guint32 color2;

	int     screenX;
	int     screenY;

	float   power;
	float   powinc;

	PluginInfo *goomInfo;
};

/* les ID possibles */

#define GML_CIRCLE 0
/* (param = radius) */

#define GML_HLINE 1
/* (param = y) */

#define GML_VLINE 2
/* (param = x) */

/* les modes couleur possible (si tu mets un autre c'est noir) */

#define GML_BLEUBLANC 0
#define GML_RED 1
#define GML_ORANGE_V 2
#define GML_ORANGE_J 3
#define GML_VERT 4
#define GML_BLEU 5
#define GML_BLACK 6

/* construit un effet de line (une ligne horitontale pour commencer) */
GMLine *goom_lines_init (PluginInfo *goomInfo, int rx, int ry,
			 int IDsrc, float paramS, int modeCoulSrc,
			 int IDdest, float paramD, int modeCoulDest);

void    goom_lines_switch_to (GMLine * gml, int IDdest, float param,
			float amplitude,
			int modeCoul);

void    goom_lines_set_res (GMLine * gml, int rx, int ry);

void    goom_lines_free (GMLine ** gml);

void    goom_lines_draw (PluginInfo *plugInfo, GMLine * gml, gint16 data[512], Pixel *p);

#endif /* _LINES_H */
