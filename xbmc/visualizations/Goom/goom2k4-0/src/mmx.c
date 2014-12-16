#ifdef HAVE_MMX

#define BUFFPOINTNB 16
#define BUFFPOINTMASK 0xffff
#define BUFFINCR 0xff

#include "mmx.h"
#include "goom_graphic.h"

#define sqrtperte 16
// faire : a % sqrtperte <=> a & pertemask
#define PERTEMASK 0xf
// faire : a / sqrtperte <=> a >> PERTEDEC
#define PERTEDEC 4

int mmx_supported (void) {
	return (mm_support()&0x1);
}

void zoom_filter_mmx (int prevX, int prevY,
		      Pixel *expix1, Pixel *expix2,
		      int *brutS, int *brutD, int buffratio,
		      int precalCoef[16][16])
{
	unsigned int ax = (prevX-1)<<PERTEDEC, ay = (prevY-1)<<PERTEDEC;

	int bufsize = prevX * prevY;
	int loop;

	__asm__ __volatile__ ("pxor %mm7,%mm7");

	for (loop=0; loop<bufsize; loop++)
	{
		/*      int couleur; */
		int px,py;
		int pos;
		int coeffs;

		int myPos = loop << 1,
		myPos2 = myPos + 1;
		int brutSmypos = brutS[myPos];

		px = brutSmypos + (((brutD[myPos] - brutSmypos)*buffratio) >> BUFFPOINTNB);
		brutSmypos = brutS[myPos2];
		py = brutSmypos + (((brutD[myPos2] - brutSmypos)*buffratio) >> BUFFPOINTNB);

		if ((py>=ay) || (px>=ax)) {
			pos=coeffs=0;
		}
		else {
			pos = ((px >> PERTEDEC) + prevX * (py >> PERTEDEC));
			// coef en modulo 15
			coeffs = precalCoef [px & PERTEMASK][py & PERTEMASK];
		}

		__asm__ __volatile__ (
		"movd %2, %%mm6 \n\t"

		/* recuperation des deux premiers pixels dans mm0 et mm1 */
		"movq (%3,%1,4), %%mm0 \n\t"	/* b1-v1-r1-a1-b2-v2-r2-a2 */
		"movq %%mm0, %%mm1 \n\t"		/* b1-v1-r1-a1-b2-v2-r2-a2 */

		/* depackage du premier pixel */
		"punpcklbw %%mm7, %%mm0 \n\t"	/* 00-b2-00-v2-00-r2-00-a2 */

		"movq %%mm6, %%mm5 \n\t"		/* ??-??-??-??-c4-c3-c2-c1 */
		/* depackage du 2ieme pixel */
		"punpckhbw %%mm7, %%mm1 \n\t"	/* 00-b1-00-v1-00-r1-00-a1 */

		/* extraction des coefficients... */
		"punpcklbw %%mm5, %%mm6 \n\t"	/* c4-c4-c3-c3-c2-c2-c1-c1 */
		"movq %%mm6, %%mm4 \n\t"		/* c4-c4-c3-c3-c2-c2-c1-c1 */
		"movq %%mm6, %%mm5 \n\t"		/* c4-c4-c3-c3-c2-c2-c1-c1 */

		"punpcklbw %%mm5, %%mm6 \n\t"	/* c2-c2-c2-c2-c1-c1-c1-c1 */
		"punpckhbw %%mm5, %%mm4 \n\t"	/* c4-c4-c4-c4-c3-c3-c3-c3 */

		"movq %%mm6, %%mm3 \n\t"		/* c2-c2-c2-c2-c1-c1-c1-c1 */

		"punpcklbw %%mm7, %%mm6 \n\t"	/* 00-c1-00-c1-00-c1-00-c1 */
		"punpckhbw %%mm7, %%mm3 \n\t"	/* 00-c2-00-c2-00-c2-00-c2 */

		/* multiplication des pixels par les coefficients */
		"pmullw %%mm6, %%mm0 \n\t"		/* c1*b2-c1*v2-c1*r2-c1*a2 */
		"pmullw %%mm3, %%mm1 \n\t"		/* c2*b1-c2*v1-c2*r1-c2*a1 */
		"paddw %%mm1, %%mm0 \n\t"

		/* ...extraction des 2 derniers coefficients */
		"movq %%mm4, %%mm5 \n\t"		/* c4-c4-c4-c4-c3-c3-c3-c3 */
		"punpcklbw %%mm7, %%mm4 \n\t"	/* 00-c3-00-c3-00-c3-00-c3 */
		"punpckhbw %%mm7, %%mm5 \n\t"	/* 00-c4-00-c4-00-c4-00-c4 */

		/* ajouter la longueur de ligne a esi */
		"addl %4,%1 \n\t"

		/* recuperation des 2 derniers pixels */
		"movq (%3,%1,4), %%mm1 \n\t"
		"movq %%mm1, %%mm2 \n\t"

		/* depackage des pixels */
		"punpcklbw %%mm7, %%mm1 \n\t"
		"punpckhbw %%mm7, %%mm2 \n\t"

		/* multiplication pas les coeffs */
		"pmullw %%mm4, %%mm1 \n\t"
		"pmullw %%mm5, %%mm2 \n\t"

		/* ajout des valeurs obtenues à la valeur finale */
		"paddw %%mm1, %%mm0 \n\t"
		"paddw %%mm2, %%mm0 \n\t"

		/* division par 256 = 16+16+16+16, puis repackage du pixel final */
		"psrlw $8, %%mm0 \n\t"
		"packuswb %%mm7, %%mm0 \n\t"

		"movd %%mm0,%0 \n\t"
		  :"=g"(expix2[loop])
		  :"r"(pos),"r"(coeffs),"r"(expix1),"r"(prevX)

		);

		emms();
	}
}

#define DRAWMETHOD_PLUS_MMX(_out,_backbuf,_col) \
{ \
	movd_m2r(_backbuf, mm0); \
	paddusb_m2r(_col, mm0); \
	movd_r2m(mm0, _out); \
}

#define DRAWMETHOD DRAWMETHOD_PLUS_MMX(*p,*p,col)

void draw_line_mmx (Pixel *data, int x1, int y1, int x2, int y2, int col, int screenx, int screeny)
{
	int x, y, dx, dy, yy, xx;
	Pixel *p;

	if ((y1 < 0) || (y2 < 0) || (x1 < 0) || (x2 < 0) || (y1 >= screeny) || (y2 >= screeny) || (x1 >= screenx) || (x2 >= screenx))
		goto end_of_line;

	dx = x2 - x1;
	dy = y2 - y1;
	if (x1 >= x2) {
		int tmp;

		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
		dx = x2 - x1;
		dy = y2 - y1;
	}

	/* vertical line */
	if (dx == 0) {
		if (y1 < y2) {
			p = &(data[(screenx * y1) + x1]);
			for (y = y1; y <= y2; y++) {
				DRAWMETHOD;
				p += screenx;
			}
		}
		else {
			p = &(data[(screenx * y2) + x1]);
			for (y = y2; y <= y1; y++) {
				DRAWMETHOD;
				p += screenx;
			}
		}
		goto end_of_line;
	}
	/* horizontal line */
	if (dy == 0) {
		if (x1 < x2) {
			p = &(data[(screenx * y1) + x1]);
			for (x = x1; x <= x2; x++) {
				DRAWMETHOD;
				p++;
			}
			goto end_of_line;
		}
		else {
			p = &(data[(screenx * y1) + x2]);
			for (x = x2; x <= x1; x++) {
				DRAWMETHOD;
				p++;
			}
			goto end_of_line;
		}
	}
	/* 1    */
	/*  \   */
	/*   \  */
	/*    2 */
	if (y2 > y1) {
		/* steep */
		if (dy > dx) {
			dx = ((dx << 16) / dy);
			x = x1 << 16;
			for (y = y1; y <= y2; y++) {
				xx = x >> 16;
				p = &(data[(screenx * y) + xx]);
				DRAWMETHOD;
				if (xx < (screenx - 1)) {
					p++;
					/* DRAWMETHOD; */
				}
				x += dx;
			}
			goto end_of_line;
		}
		/* shallow */
		else {
			dy = ((dy << 16) / dx);
			y = y1 << 16;
			for (x = x1; x <= x2; x++) {
				yy = y >> 16;
				p = &(data[(screenx * yy) + x]);
				DRAWMETHOD;
				if (yy < (screeny - 1)) {
					p += screeny;
					/* DRAWMETHOD; */
				}
				y += dy;
			}
		}
	}
	/*    2 */
	/*   /  */
	/*  /   */
	/* 1    */
	else {
		/* steep */
		if (-dy > dx) {
			dx = ((dx << 16) / -dy);
			x = (x1 + 1) << 16;
			for (y = y1; y >= y2; y--) {
				xx = x >> 16;
				p = &(data[(screenx * y) + xx]);
				DRAWMETHOD;
				if (xx < (screenx - 1)) {
					p--;
					/* DRAWMETHOD; */
				}
				x += dx;
			}
			goto end_of_line;
		}
		/* shallow */
		else {
			dy = ((dy << 16) / dx);
			y = y1 << 16;
			for (x = x1; x <= x2; x++) {
				yy = y >> 16;
				p = &(data[(screenx * yy) + x]);
				DRAWMETHOD;
				if (yy < (screeny - 1)) {
					p += screeny;
					/* DRAWMETHOD; */
				}
				y += dy;
			}
			goto end_of_line;
		}
	}
end_of_line:
	emms();
	/* __asm__ __volatile__ ("emms"); */
}

#endif
