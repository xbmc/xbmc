#ifndef _V3D_H
#define _V3D_H

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "mathtools.h"

typedef struct {
    float x,y,z;
} v3d;

typedef struct {
    int x,y;
} v2d;

typedef struct {
    double x,y;
} v2g;

/* 
 * projete le vertex 3D sur le plan d'affichage
 * retourne (0,0) si le point ne doit pas etre affiche.
 *
 * bonne valeur pour distance : 256
 */
#define V3D_TO_V2D(v3,v2,width,height,distance) \
{ \
  int Xp, Yp; \
  if (v3.z > 2) { \
	 F2I((distance * v3.x / v3.z),Xp) ; \
 	 F2I((distance * v3.y / v3.z),Yp) ; \
 	 v2.x = Xp + (width>>1); \
	 v2.y = -Yp + (height>>1); \
  } \
  else v2.x=v2.y=-666; \
}

void v3d_to_v2d(v3d *src, int nbvertex, int width, int height, float distance, v2d *v2_array);

/*
 * rotation selon Y du v3d vi d'angle a (cosa=cos(a), sina=sin(a))
 * centerz = centre de rotation en z
 */
#define Y_ROTATE_V3D(vi,vf,sina,cosa)\
{\
 vf.x = vi.x * cosa - vi.z * sina;\
 vf.z = vi.x * sina + vi.z * cosa;\
 vf.y = vi.y;\
}

/*
 * translation
 */
#define TRANSLATE_V3D(vsrc,vdest)\
{\
 vdest.x += vsrc.x;\
 vdest.y += vsrc.y;\
 vdest.z += vsrc.z;\
}

#define MUL_V3D(lf,v) {v.x*=lf;v.y*=lf;v.z*=lf;}

#endif
