#include "v3d.h"

void v3d_to_v2d(v3d *v3, int nbvertex, int width, int height, float distance, v2d *v2) {
	int i;
	for (i=0;i<nbvertex;++i) {
		if (v3[i].z > 2) {
			int Xp, Yp;
			F2I((distance * v3[i].x / v3[i].z),Xp);
			F2I((distance * v3[i].y / v3[i].z),Yp);
			v2[i].x = Xp + (width>>1);
			v2[i].y = -Yp + (height>>1);
		}
		else v2[i].x=v2[i].y=-666;
	}
}
