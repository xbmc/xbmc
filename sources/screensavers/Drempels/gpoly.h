/* 
 *  This source file is part of Drempels, a program that falls
 *  under the Gnu Public License (GPL) open-source license.  
 *  Use and distribution of this code is regulated by law under 
 *  this license.  For license details, visit:
 *    http://www.gnu.org/copyleft/gpl.html
 * 
 *  The Drempels open-source project is accessible at 
 *  sourceforge.net; the direct URL is:
 *    http://sourceforge.net/projects/drempels/
 *  
 *  Drempels was originally created by Ryan M. Geiss in 2001
 *  and was open-sourced in February 2005.  The original
 *  Drempels homepage is available here:
 *    http://www.geisswerks.com/drempels/
 *
 */

#ifndef GEISS_POLYCORE_H
#define GEISS_POLYCORE_H 1

class td_cellcornerinfo
{
public:
	float u, dudy;
	float r, drdy;	// r = dudx
	float v, dvdy;
	float s, dsdy;	// s = dvdx
	// 1. get u's
	// 2. get dudy's
	// 3. get dudx's
	// 4. get dudxdy's from (3)
};

#define INTFACTOR (65536*256) / 8     // -> for max of 2048x2048 (vs. 256x256)
#define SHIFTFACTOR (16+8      -3 )

void BlitWarp(float u0, float v0, 
			  float u1, float v1, 
			  float u2, float v2, 
			  float u3, float v3, 
			  int x0, int y0, 
			  int x1, int y1, 
			  unsigned char *dest, int W, int H, 
			  unsigned char *tex, int texW, int texH);

void BlitWarp256(td_cellcornerinfo cell0, 
				 td_cellcornerinfo cell1, 
				 td_cellcornerinfo cell2, 
				 td_cellcornerinfo cell3, 
				 int x0, int y0, 
				 int x1, int y1, 
				 unsigned char *dest, int W, int H, 
				 unsigned char *tex);

void BlitWarp256AndMix(td_cellcornerinfo cell0, 
					td_cellcornerinfo cell1, 
					td_cellcornerinfo cell2, 
					td_cellcornerinfo cell3, 
					int x0, int y0, 
					int x1, int y1, 
					unsigned char *old_dest,
					int old_dest_mult,
					int new_mult,
					unsigned char *dest, 
					int W, 
					int H, 
					unsigned char *tex,
					bool bHighQuality);

void BlitWarpNon256AndMix(td_cellcornerinfo cell0, 
					td_cellcornerinfo cell1, 
					td_cellcornerinfo cell2, 
					td_cellcornerinfo cell3, 
					int x0, int y0, 
					int x1, int y1, 
					unsigned char *old_dest,
					int old_dest_mult,
					int new_mult,
					unsigned char *dest, 
					int W, 
					int H, 
					unsigned char *tex,
                     int texW,
                     int texH,
					bool bHighQuality);

void BlitWarp256(float u0, float v0, 
			  float u1, float v1, 
			  float u2, float v2, 
			  float u3, float v3, 
			  int x0, int y0, 
			  int x1, int y1, 
			  unsigned char *dest, int W, int H, 
			  unsigned char *tex);


#endif