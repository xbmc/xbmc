#ifndef FILTERS_H
#define FILTERS_H

#include "goom_config.h"
#include "goom_typedefs.h"
#include "goom_visual_fx.h"
#include "goom_graphic.h"

VisualFX zoomFilterVisualFXWrapper_create(void);

struct _ZOOM_FILTER_DATA
{
	int     vitesse;           /* 128 = vitesse nule... * * 256 = en arriere 
	                            * hyper vite.. * * 0 = en avant hype vite. */
	unsigned char pertedec;
	unsigned char sqrtperte;
	int     middleX, middleY;  /* milieu de l'effet */
	char    reverse;           /* inverse la vitesse */
	char    mode;              /* type d'effet à appliquer (cf les #define) */
	/** @since June 2001 */
	int     hPlaneEffect;      /* deviation horitontale */
	int     vPlaneEffect;      /* deviation verticale */
	/** @since April 2002 */
	int     waveEffect;        /* applique une "surcouche" de wave effect */
	int     hypercosEffect;    /* applique une "surcouche de hypercos effect */

	char    noisify;           /* ajoute un bruit a la transformation */
};

#define NORMAL_MODE 0
#define WAVE_MODE 1
#define CRYSTAL_BALL_MODE 2
#define SCRUNCH_MODE 3
#define AMULETTE_MODE 4
#define WATER_MODE 5
#define HYPERCOS1_MODE 6
#define HYPERCOS2_MODE 7
#define YONLY_MODE 8
#define SPEEDWAY_MODE 9

void pointFilter (PluginInfo *goomInfo, Pixel * pix1, Color c,
                  float t1, float t2, float t3, float t4, guint32 cycle);

/* filtre de zoom :
 * le contenu de pix1 est copie dans pix2.
 * zf : si non NULL, configure l'effet.
 * resx,resy : taille des buffers.
 */
void zoomFilterFastRGB (PluginInfo *goomInfo, Pixel * pix1, Pixel * pix2, ZoomFilterData * zf, guint32 resx,
                        guint32 resy, int switchIncr, float switchMult);

#endif
