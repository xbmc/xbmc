/*
 * ifs.c --- modified iterated functions system for goom.
 */

/*-
 * Copyright (c) 1997 by Massimino Pascal <Pascal.Massimon@ens.fr>
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * If this mode is weird and you have an old MetroX server, it is buggy.
 * There is a free SuSE-enhanced MetroX X server that is fine.
 *
 * When shown ifs, Diana Rose (4 years old) said, "It looks like dancing."
 *
 * Revision History:
 * 13-Dec-2003: Added some goom specific stuffs (to make ifs a VisualFX).
 * 11-Apr-2002: jeko@ios-software.com: Make ifs.c system-indendant. (ifs.h added)
 * 01-Nov-2000: Allocation checks
 * 10-May-1997: jwz@jwz.org: turned into a standalone program.
 *              Made it render into an offscreen bitmap and then copy
 *              that onto the screen, to reduce flicker.
 */

/* #ifdef STANDALONE */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "goom_config.h"

#ifdef HAVE_MMX
#include "mmx.h"
#endif

#include "goom_graphic.h"
#include "ifs.h"
#include "goom_tools.h"

typedef struct _ifsPoint
{
	gint32  x, y;
}
IFSPoint;


#define MODE_ifs

#define PROGCLASS "IFS"

#define HACK_INIT init_ifs
#define HACK_DRAW draw_ifs

#define ifs_opts xlockmore_opts

#define DEFAULTS "*delay: 20000 \n" \
"*ncolors: 100 \n"

#define SMOOTH_COLORS

#define LRAND()            ((long) (goom_random(goomInfo->gRandom) & 0x7fffffff))
#define NRAND(n)           ((int) (LRAND() % (n)))

#if RAND_MAX < 0x10000
#define MAXRAND (((float)(RAND_MAX<16)+((float)RAND_MAX)+1.0f)/127.0f)
#else
#define MAXRAND            (2147483648.0/127.0)           /* unsigned 1<<31 / 127.0 (cf goom_tools) as a float */
#endif

/*****************************************************/

typedef float DBL;
typedef int F_PT;

/* typedef float               F_PT; */

/*****************************************************/

#define FIX 12
#define UNIT   ( 1<<FIX )
#define MAX_SIMI  6

#define MAX_DEPTH_2  10
#define MAX_DEPTH_3  6
#define MAX_DEPTH_4  4
#define MAX_DEPTH_5  2

/* PREVIOUS VALUE 
#define MAX_SIMI  6

 * settings for a PC 120Mhz... *
#define MAX_DEPTH_2  10
#define MAX_DEPTH_3  6
#define MAX_DEPTH_4  4
#define MAX_DEPTH_5  3
*/

#define DBL_To_F_PT(x)  (F_PT)( (DBL)(UNIT)*(x) )

typedef struct Similitude_Struct SIMI;
typedef struct Fractal_Struct FRACTAL;

struct Similitude_Struct
{

	DBL     c_x, c_y;
	DBL     r, r2, A, A2;
	F_PT    Ct, St, Ct2, St2;
	F_PT    Cx, Cy;
	F_PT    R, R2;
};


struct Fractal_Struct
{

	int     Nb_Simi;
	SIMI    Components[5 * MAX_SIMI];
	int     Depth, Col;
	int     Count, Speed;
	int     Width, Height, Lx, Ly;
	DBL     r_mean, dr_mean, dr2_mean;
	int     Cur_Pt, Max_Pt;

	IFSPoint *Buffer1, *Buffer2;
};

typedef struct _IFS_DATA {
	FRACTAL *Root;
	FRACTAL *Cur_F;

	/* Used by the Trace recursive method */
	IFSPoint *Buf;
	int Cur_Pt;
	int initalized;
} IfsData;


/*****************************************************/

static  DBL
Gauss_Rand (PluginInfo *goomInfo, DBL c, DBL A, DBL S)
{
	DBL     y;

	y = (DBL) LRAND () / MAXRAND;
	y = A * (1.0 - exp (-y * y * S)) / (1.0 - exp (-S));
	if (NRAND (2))
		return (c + y);
	return (c - y);
}

static  DBL
Half_Gauss_Rand (PluginInfo *goomInfo, DBL c, DBL A, DBL S)
{
	DBL     y;

	y = (DBL) LRAND () / MAXRAND;
	y = A * (1.0 - exp (-y * y * S)) / (1.0 - exp (-S));
	return (c + y);
}

static void
Random_Simis (PluginInfo *goomInfo, FRACTAL * F, SIMI * Cur, int i)
{
	while (i--) {
		Cur->c_x = Gauss_Rand (goomInfo, 0.0, .8, 4.0);
		Cur->c_y = Gauss_Rand (goomInfo, 0.0, .8, 4.0);
		Cur->r = Gauss_Rand (goomInfo, F->r_mean, F->dr_mean, 3.0);
		Cur->r2 = Half_Gauss_Rand (goomInfo, 0.0, F->dr2_mean, 2.0);
		Cur->A = Gauss_Rand (goomInfo, 0.0, 360.0, 4.0) * (M_PI / 180.0);
		Cur->A2 = Gauss_Rand (goomInfo, 0.0, 360.0, 4.0) * (M_PI / 180.0);
		Cur++;
	}
}

static void
free_ifs_buffers (FRACTAL * Fractal)
{
	if (Fractal->Buffer1 != NULL) {
		(void) free ((void *) Fractal->Buffer1);
		Fractal->Buffer1 = (IFSPoint *) NULL;
	}
	if (Fractal->Buffer2 != NULL) {
		(void) free ((void *) Fractal->Buffer2);
		Fractal->Buffer2 = (IFSPoint *) NULL;
	}
}


static void
free_ifs (FRACTAL * Fractal)
{
	free_ifs_buffers (Fractal);
}

/***************************************************************/

static void
init_ifs (PluginInfo *goomInfo, IfsData *data)
{
	int     i;
	FRACTAL *Fractal;
	int width = goomInfo->screen.width;
	int height = goomInfo->screen.height;

	if (data->Root == NULL) {
		data->Root = (FRACTAL *) malloc (sizeof (FRACTAL));
		if (data->Root == NULL)
			return;
		data->Root->Buffer1 = (IFSPoint *) NULL;
		data->Root->Buffer2 = (IFSPoint *) NULL;
	}
	Fractal = data->Root;

	free_ifs_buffers (Fractal);

	i = (NRAND (4)) + 2;					/* Number of centers */
	switch (i) {
		case 3:
			Fractal->Depth = MAX_DEPTH_3;
			Fractal->r_mean = .6;
			Fractal->dr_mean = .4;
			Fractal->dr2_mean = .3;
			break;

		case 4:
			Fractal->Depth = MAX_DEPTH_4;
			Fractal->r_mean = .5;
			Fractal->dr_mean = .4;
			Fractal->dr2_mean = .3;
			break;

		case 5:
			Fractal->Depth = MAX_DEPTH_5;
			Fractal->r_mean = .5;
			Fractal->dr_mean = .4;
			Fractal->dr2_mean = .3;
			break;

		default:
		case 2:
			Fractal->Depth = MAX_DEPTH_2;
			Fractal->r_mean = .7;
			Fractal->dr_mean = .3;
			Fractal->dr2_mean = .4;
			break;
	}
	Fractal->Nb_Simi = i;
	Fractal->Max_Pt = Fractal->Nb_Simi - 1;
	for (i = 0; i <= Fractal->Depth + 2; ++i)
		Fractal->Max_Pt *= Fractal->Nb_Simi;

	if ((Fractal->Buffer1 = (IFSPoint *) calloc (Fractal->Max_Pt,
						     sizeof (IFSPoint))) == NULL) {
		free_ifs (Fractal);
		return;
	}
	if ((Fractal->Buffer2 = (IFSPoint *) calloc (Fractal->Max_Pt,
						     sizeof (IFSPoint))) == NULL) {
		free_ifs (Fractal);
		return;
	}

	Fractal->Speed = 6;
	Fractal->Width = width;				/* modif by JeKo */
	Fractal->Height = height;			/* modif by JeKo */
	Fractal->Cur_Pt = 0;
	Fractal->Count = 0;
	Fractal->Lx = (Fractal->Width - 1) / 2;
	Fractal->Ly = (Fractal->Height - 1) / 2;
	Fractal->Col = rand () % (width * height);	/* modif by JeKo */

	Random_Simis (goomInfo, Fractal, Fractal->Components, 5 * MAX_SIMI);
}


/***************************************************************/

static inline void
Transform (SIMI * Simi, F_PT xo, F_PT yo, F_PT * x, F_PT * y)
{
	F_PT    xx, yy;

	xo = xo - Simi->Cx;
	xo = (xo * Simi->R) >> FIX; /* / UNIT; */
	yo = yo - Simi->Cy;
	yo = (yo * Simi->R) >> FIX; /* / UNIT; */

	xx = xo - Simi->Cx;
	xx = (xx * Simi->R2) >> FIX; /* / UNIT; */
	yy = -yo - Simi->Cy;
	yy = (yy * Simi->R2) >> FIX; /* / UNIT; */

	*x =
	  ((xo * Simi->Ct - yo * Simi->St + xx * Simi->Ct2 - yy * Simi->St2)
	   >> FIX /* / UNIT */ ) + Simi->Cx;
	*y =
	  ((xo * Simi->St + yo * Simi->Ct + xx * Simi->St2 + yy * Simi->Ct2)
	   >> FIX /* / UNIT */ ) + Simi->Cy;
}

/***************************************************************/

static void
Trace (FRACTAL * F, F_PT xo, F_PT yo, IfsData *data)
{
	F_PT    x, y, i;
	SIMI   *Cur;

	Cur = data->Cur_F->Components;
	for (i = data->Cur_F->Nb_Simi; i; --i, Cur++) {
		Transform (Cur, xo, yo, &x, &y);

		data->Buf->x = F->Lx + ((x * F->Lx) >> (FIX+1) /* /(UNIT*2) */ );
		data->Buf->y = F->Ly - ((y * F->Ly) >> (FIX+1) /* /(UNIT*2) */ );
		data->Buf++;

		data->Cur_Pt++;

		if (F->Depth && ((x - xo) >> 4) && ((y - yo) >> 4)) {
			F->Depth--;
			Trace (F, x, y, data);
			F->Depth++;
		}
	}
}

static void
Draw_Fractal (IfsData *data)
{
	FRACTAL *F = data->Root;
	int     i, j;
	F_PT    x, y, xo, yo;
	SIMI   *Cur, *Simi;

	for (Cur = F->Components, i = F->Nb_Simi; i; --i, Cur++) {
		Cur->Cx = DBL_To_F_PT (Cur->c_x);
		Cur->Cy = DBL_To_F_PT (Cur->c_y);

		Cur->Ct = DBL_To_F_PT (cos (Cur->A));
		Cur->St = DBL_To_F_PT (sin (Cur->A));
		Cur->Ct2 = DBL_To_F_PT (cos (Cur->A2));
		Cur->St2 = DBL_To_F_PT (sin (Cur->A2));

		Cur->R = DBL_To_F_PT (Cur->r);
		Cur->R2 = DBL_To_F_PT (Cur->r2);
	}


	data->Cur_Pt = 0;
	data->Cur_F = F;
	data->Buf = F->Buffer2;
	for (Cur = F->Components, i = F->Nb_Simi; i; --i, Cur++) {
		xo = Cur->Cx;
		yo = Cur->Cy;
		for (Simi = F->Components, j = F->Nb_Simi; j; --j, Simi++) {
			if (Simi == Cur)
				continue;
			Transform (Simi, xo, yo, &x, &y);
			Trace (F, x, y, data);
		}
	}

	/* Erase previous */

	F->Cur_Pt = data->Cur_Pt;
	data->Buf = F->Buffer1;
	F->Buffer1 = F->Buffer2;
	F->Buffer2 = data->Buf;
}


static IFSPoint *
draw_ifs (PluginInfo *goomInfo, int *nbpt, IfsData *data)
{
	int     i;
	DBL     u, uu, v, vv, u0, u1, u2, u3;
	SIMI   *S, *S1, *S2, *S3, *S4;
	FRACTAL *F;

	if (data->Root == NULL)
		return NULL;
	F = data->Root;
	if (F->Buffer1 == NULL)
		return NULL;

	u = (DBL) (F->Count) * (DBL) (F->Speed) / 1000.0;
	uu = u * u;
	v = 1.0 - u;
	vv = v * v;
	u0 = vv * v;
	u1 = 3.0 * vv * u;
	u2 = 3.0 * v * uu;
	u3 = u * uu;

	S = F->Components;
	S1 = S + F->Nb_Simi;
	S2 = S1 + F->Nb_Simi;
	S3 = S2 + F->Nb_Simi;
	S4 = S3 + F->Nb_Simi;

	for (i = F->Nb_Simi; i; --i, S++, S1++, S2++, S3++, S4++) {
		S->c_x = u0 * S1->c_x + u1 * S2->c_x + u2 * S3->c_x + u3 * S4->c_x;
		S->c_y = u0 * S1->c_y + u1 * S2->c_y + u2 * S3->c_y + u3 * S4->c_y;
		S->r = u0 * S1->r + u1 * S2->r + u2 * S3->r + u3 * S4->r;
		S->r2 = u0 * S1->r2 + u1 * S2->r2 + u2 * S3->r2 + u3 * S4->r2;
		S->A = u0 * S1->A + u1 * S2->A + u2 * S3->A + u3 * S4->A;
		S->A2 = u0 * S1->A2 + u1 * S2->A2 + u2 * S3->A2 + u3 * S4->A2;
	}

	Draw_Fractal (data);

	if (F->Count >= 1000 / F->Speed) {
		S = F->Components;
		S1 = S + F->Nb_Simi;
		S2 = S1 + F->Nb_Simi;
		S3 = S2 + F->Nb_Simi;
		S4 = S3 + F->Nb_Simi;

		for (i = F->Nb_Simi; i; --i, S++, S1++, S2++, S3++, S4++) {
			S2->c_x = 2.0 * S4->c_x - S3->c_x;
			S2->c_y = 2.0 * S4->c_y - S3->c_y;
			S2->r = 2.0 * S4->r - S3->r;
			S2->r2 = 2.0 * S4->r2 - S3->r2;
			S2->A = 2.0 * S4->A - S3->A;
			S2->A2 = 2.0 * S4->A2 - S3->A2;

			*S1 = *S4;
		}
		Random_Simis (goomInfo, F, F->Components + 3 * F->Nb_Simi, F->Nb_Simi);

		Random_Simis (goomInfo, F, F->Components + 4 * F->Nb_Simi, F->Nb_Simi);

		F->Count = 0;
	}
	else
		F->Count++;

	F->Col++;

	(*nbpt) = data->Cur_Pt;
	return F->Buffer2;
}


/***************************************************************/

static void release_ifs (IfsData *data)
{
	if (data->Root != NULL) {
		free_ifs (data->Root);
		(void) free ((void *) data->Root);
		data->Root = (FRACTAL *) NULL;
	}
}

#define RAND() goom_random(goomInfo->gRandom)

static void ifs_update (PluginInfo *goomInfo, Pixel * data, Pixel * back, int increment, IfsData *fx_data)
{
	static int couleur = 0xc0c0c0c0;
	static int v[4] = { 2, 4, 3, 2 };
	static int col[4] = { 2, 4, 3, 2 };

#define MOD_MER 0
#define MOD_FEU 1
#define MOD_MERVER 2
	static int mode = MOD_MERVER;
	static int justChanged = 0;
	static int cycle = 0;
	int     cycle10;

	int     nbpt;
	IFSPoint *points;
	int     i;

	int     couleursl = couleur;
	int width = goomInfo->screen.width;
	int height = goomInfo->screen.height;

	cycle++;
	if (cycle >= 80)
		cycle = 0;

	if (cycle < 40)
		cycle10 = cycle / 10;
	else
		cycle10 = 7 - cycle / 10;

	{
		unsigned char *tmp = (unsigned char *) &couleursl;

		for (i = 0; i < 4; i++) {
			*tmp = (*tmp) >> cycle10;
			tmp++;
		}
	}

	points = draw_ifs (goomInfo, &nbpt, fx_data);
	nbpt--;

#ifdef HAVE_MMX
	movd_m2r (couleursl, mm1);
	punpckldq_r2r (mm1, mm1);
	for (i = 0; i < nbpt; i += increment) {
		int     x = points[i].x;
		int     y = points[i].y;

		if ((x < width) && (y < height) && (x > 0) && (y > 0)) {
			int     pos = x + (y * width);
			movd_m2r (back[pos], mm0);
			paddusb_r2r (mm1, mm0);
			movd_r2m (mm0, data[pos]);
		}
	}
	emms();/*__asm__ __volatile__ ("emms");*/
#else
	for (i = 0; i < nbpt; i += increment) {
		int     x = (int) points[i].x & 0x7fffffff;
		int     y = (int) points[i].y & 0x7fffffff;

		if ((x < width) && (y < height)) {
			int     pos = x + (int) (y * width);
			int     tra = 0, i = 0;
			unsigned char *bra = (unsigned char *) &back[pos];
			unsigned char *dra = (unsigned char *) &data[pos];
			unsigned char *cra = (unsigned char *) &couleursl;

			for (; i < 4; i++) {
				tra = *cra;
				tra += *bra;
				if (tra > 255)
					tra = 255;
				*dra = tra;
				++dra;
				++cra;
				++bra;
			}
		}
	}
#endif /*MMX*/
		justChanged--;

	col[ALPHA] = couleur >> (ALPHA * 8) & 0xff;
	col[BLEU] = couleur >> (BLEU * 8) & 0xff;
	col[VERT] = couleur >> (VERT * 8) & 0xff;
	col[ROUGE] = couleur >> (ROUGE * 8) & 0xff;

	if (mode == MOD_MER) {
		col[BLEU] += v[BLEU];
		if (col[BLEU] > 255) {
			col[BLEU] = 255;
			v[BLEU] = -(RAND() % 4) - 1;
		}
		if (col[BLEU] < 32) {
			col[BLEU] = 32;
			v[BLEU] = (RAND() % 4) + 1;
		}

		col[VERT] += v[VERT];
		if (col[VERT] > 200) {
			col[VERT] = 200;
			v[VERT] = -(RAND() % 3) - 2;
		}
		if (col[VERT] > col[BLEU]) {
			col[VERT] = col[BLEU];
			v[VERT] = v[BLEU];
		}
		if (col[VERT] < 32) {
			col[VERT] = 32;
			v[VERT] = (RAND() % 3) + 2;
		}

		col[ROUGE] += v[ROUGE];
		if (col[ROUGE] > 64) {
			col[ROUGE] = 64;
			v[ROUGE] = -(RAND () % 4) - 1;
		}
		if (col[ROUGE] < 0) {
			col[ROUGE] = 0;
			v[ROUGE] = (RAND () % 4) + 1;
		}

		col[ALPHA] += v[ALPHA];
		if (col[ALPHA] > 0) {
			col[ALPHA] = 0;
			v[ALPHA] = -(RAND () % 4) - 1;
		}
		if (col[ALPHA] < 0) {
			col[ALPHA] = 0;
			v[ALPHA] = (RAND () % 4) + 1;
		}

		if (((col[VERT] > 32) && (col[ROUGE] < col[VERT] + 40)
				 && (col[VERT] < col[ROUGE] + 20) && (col[BLEU] < 64)
				 && (RAND () % 20 == 0)) && (justChanged < 0)) {
			mode = RAND () % 3 ? MOD_FEU : MOD_MERVER;
			justChanged = 250;
		}
	}
	else if (mode == MOD_MERVER) {
		col[BLEU] += v[BLEU];
		if (col[BLEU] > 128) {
			col[BLEU] = 128;
			v[BLEU] = -(RAND () % 4) - 1;
		}
		if (col[BLEU] < 16) {
			col[BLEU] = 16;
			v[BLEU] = (RAND () % 4) + 1;
		}

		col[VERT] += v[VERT];
		if (col[VERT] > 200) {
			col[VERT] = 200;
			v[VERT] = -(RAND () % 3) - 2;
		}
		if (col[VERT] > col[ALPHA]) {
			col[VERT] = col[ALPHA];
			v[VERT] = v[ALPHA];
		}
		if (col[VERT] < 32) {
			col[VERT] = 32;
			v[VERT] = (RAND () % 3) + 2;
		}

		col[ROUGE] += v[ROUGE];
		if (col[ROUGE] > 128) {
			col[ROUGE] = 128;
			v[ROUGE] = -(RAND () % 4) - 1;
		}
		if (col[ROUGE] < 0) {
			col[ROUGE] = 0;
			v[ROUGE] = (RAND () % 4) + 1;
		}

		col[ALPHA] += v[ALPHA];
		if (col[ALPHA] > 255) {
			col[ALPHA] = 255;
			v[ALPHA] = -(RAND () % 4) - 1;
		}
		if (col[ALPHA] < 0) {
			col[ALPHA] = 0;
			v[ALPHA] = (RAND () % 4) + 1;
		}

		if (((col[VERT] > 32) && (col[ROUGE] < col[VERT] + 40)
				 && (col[VERT] < col[ROUGE] + 20) && (col[BLEU] < 64)
				 && (RAND () % 20 == 0)) && (justChanged < 0)) {
			mode = RAND () % 3 ? MOD_FEU : MOD_MER;
			justChanged = 250;
		}
	}
	else if (mode == MOD_FEU) {

		col[BLEU] += v[BLEU];
		if (col[BLEU] > 64) {
			col[BLEU] = 64;
			v[BLEU] = -(RAND () % 4) - 1;
		}
		if (col[BLEU] < 0) {
			col[BLEU] = 0;
			v[BLEU] = (RAND () % 4) + 1;
		}

		col[VERT] += v[VERT];
		if (col[VERT] > 200) {
			col[VERT] = 200;
			v[VERT] = -(RAND () % 3) - 2;
		}
		if (col[VERT] > col[ROUGE] + 20) {
			col[VERT] = col[ROUGE] + 20;
			v[VERT] = -(RAND () % 3) - 2;
			v[ROUGE] = (RAND () % 4) + 1;
			v[BLEU] = (RAND () % 4) + 1;
		}
		if (col[VERT] < 0) {
			col[VERT] = 0;
			v[VERT] = (RAND () % 3) + 2;
		}

		col[ROUGE] += v[ROUGE];
		if (col[ROUGE] > 255) {
			col[ROUGE] = 255;
			v[ROUGE] = -(RAND () % 4) - 1;
		}
		if (col[ROUGE] > col[VERT] + 40) {
			col[ROUGE] = col[VERT] + 40;
			v[ROUGE] = -(RAND () % 4) - 1;
		}
		if (col[ROUGE] < 0) {
			col[ROUGE] = 0;
			v[ROUGE] = (RAND () % 4) + 1;
		}

		col[ALPHA] += v[ALPHA];
		if (col[ALPHA] > 0) {
			col[ALPHA] = 0;
			v[ALPHA] = -(RAND () % 4) - 1;
		}
		if (col[ALPHA] < 0) {
			col[ALPHA] = 0;
			v[ALPHA] = (RAND () % 4) + 1;
		}

		if (((col[ROUGE] < 64) && (col[VERT] > 32) && (col[VERT] < col[BLEU])
				 && (col[BLEU] > 32)
				 && (RAND () % 20 == 0)) && (justChanged < 0)) {
			mode = RAND () % 2 ? MOD_MER : MOD_MERVER;
			justChanged = 250;
		}
	}

	couleur = (col[ALPHA] << (ALPHA * 8))
		| (col[BLEU] << (BLEU * 8))
		| (col[VERT] << (VERT * 8))
		| (col[ROUGE] << (ROUGE * 8));
}

/** VISUAL_FX WRAPPER FOR IFS */

static void ifs_vfx_apply(VisualFX *_this, Pixel *src, Pixel *dest, PluginInfo *goomInfo) {

	IfsData *data = (IfsData*)_this->fx_data;
	if (!data->initalized) {
		data->initalized = 1;
		init_ifs(goomInfo, data);
	}
	ifs_update (goomInfo, dest, src, goomInfo->update.ifs_incr, data);
	/*TODO: trouver meilleur soluce pour increment (mettre le code de gestion de l'ifs dans ce fichier: ifs_vfx_apply) */
}

static void ifs_vfx_init(VisualFX *_this, PluginInfo *info) {

	IfsData *data = (IfsData*)malloc(sizeof(IfsData));
	data->Root = (FRACTAL*)NULL;
	data->initalized = 0;
	_this->fx_data = data;
}

static void ifs_vfx_free(VisualFX *_this) {
	IfsData *data = (IfsData*)_this->fx_data;
	release_ifs(data);
	free(data);
}

VisualFX ifs_visualfx_create(void) {
	VisualFX vfx;
	vfx.init = ifs_vfx_init;
	vfx.free = ifs_vfx_free;
	vfx.apply = ifs_vfx_apply;
	return vfx;
}
