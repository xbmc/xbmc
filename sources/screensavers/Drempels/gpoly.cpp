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

#include "math.h"
#include "time.h"
#include "gpoly.h"
#include <assert.h>
#include <stdio.h>

inline void iCubicInterp(float uL, float uR, float rL, float rR, 
				  signed __int32 *u, signed __int32 *du, signed __int32 *ddu, signed __int32 *dddu,
				  float dx)
{
	{
		float f[6];
		float fddu;
		f[0] = dx*dx*dx;
		f[1] = dx*dx;
		f[2] = uR - uL - rL*dx;
		f[3] = 3*dx*dx;
		f[4] = 2*dx;
		f[5] = rR - rL;

		*u = (int)(uL);
		*du = (int)(rL);
		float denom = (f[4]*f[0] - f[3]*f[1]);
		if (denom != 0)
		{
			fddu = (f[5]*f[0] - f[2]*f[3]) / denom;
			*ddu = (int)(fddu);
			*dddu = (int)(((f[2] - f[1]*fddu) / f[0]));
		}
	}

	//FILE *fout = fopen("c:\\dr.txt", "a");
	//fprintf(fout, "%d %d %d %d\n", *u, *du, *ddu, *dddu);
	//fclose(fout);
}

inline void fCubicInterp(float uL, float uR, float rL, float rR, 
				  float *u, float *du, float *ddu, float *dddu,
				  float dx)
{
	float f[6];
	float fddu;
	f[0] = dx*dx*dx;
	f[1] = dx*dx;
	f[2] = uR - uL - rL*dx;
	f[3] = 3*dx*dx;
	f[4] = 2*dx;
	f[5] = rR - rL;

	*u = uL;
	*du = rL;
	float denom = (f[4]*f[0] - f[3]*f[1]);
	if (denom != 0)
	{
		fddu = (f[5]*f[0] - f[2]*f[3]) / denom;
		*ddu = fddu;
		*dddu = (f[2] - f[1]*fddu) / f[0];
	}

	//FILE *fout = fopen("c:\\dr.txt", "a");
	//fprintf(fout, "%f %f %f %f\n", *u, *du, *ddu, *dddu);
	//fclose(fout);
}





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
					bool bHighQuality)
{
	if (!tex) return;
	if (!dest) return;

	//  (x0y0)
	//   u0v0 ------ u1v1
	//    |            |
	//    |            |
	//   u2v2 ------->uv3 
	//				(x1y1)

	//unsigned __int16 texW16 = texW;
	//unsigned __int16 texH16 = texH;
	//int temp;

	unsigned int *dest32 = (unsigned int *)dest;
	unsigned int *tex32 = (unsigned int *)tex;

	int dx = x1 - x0;
	int _dx = x1 - x0;
	int _dy = y1 - y0;
	float fdy = (float)(y1 - y0);
	float fdx = (float)(x1 - x0);
	int dest_pos;
	float min;

	min = (cell0.u < cell1.u) ? cell0.u : cell1.u;
	min = (min < cell2.u) ? min : cell2.u;
	min = (min < cell3.u) ? min : cell3.u;
	if (min < 0) 
	{
		float ufix = (2.0f - (int)(min/(INTFACTOR*256))) * INTFACTOR*256.0f;
		cell0.u += ufix;
		cell1.u += ufix;
		cell2.u += ufix;
		cell3.u += ufix;
	}

	min = (cell0.v < cell1.v) ? cell0.v : cell1.v;
	min = (min < cell2.v) ? min : cell2.v;
	min = (min < cell3.v) ? min : cell3.v;
	if (min < 0) 
	{
		float vfix = (2.0f - (int)(min/(INTFACTOR*256))) * INTFACTOR*256.0f;
		cell0.v += vfix;
		cell1.v += vfix;
		cell2.v += vfix;
		cell3.v += vfix;
	}

	dest_pos = (y0*W + x0)*4;
	old_dest += dest_pos;

	float uL   ;
	float duL  ;
	float dduL ;
	float ddduL;
	float rL   ;
	float drL  ;
	float ddrL ;
	float dddrL;
	float vL   ;
	float dvL  ;
	float ddvL ;
	float dddvL;
	float sL   ;
	float dsL  ;
	float ddsL ;
	float dddsL;
	float uR   ;
	float duR  ;
	float dduR ;
	float ddduR;
	float rR   ;
	float drR  ;
	float ddrR ;
	float dddrR;
	float vR   ;
	float dvR  ;
	float ddvR ;
	float dddvR;
	float sR   ;
	float dsR  ;
	float ddsR ;
	float dddsR;
	float uL_now;
	float rL_now;
	float vL_now;
	float sL_now;
	float uR_now;
	float rR_now;
	float vR_now;
	float sR_now;

	// next 24 vars are with respect to y:
	fCubicInterp(cell0.u, cell2.u, cell0.dudy, cell2.dudy, &uL, &duL, &dduL, &ddduL, fdy);
	fCubicInterp(cell0.r, cell2.r, cell0.drdy, cell2.drdy, &rL, &drL, &ddrL, &dddrL, fdy);
	fCubicInterp(cell0.v, cell2.v, cell0.dvdy, cell2.dvdy, &vL, &dvL, &ddvL, &dddvL, fdy);
	fCubicInterp(cell0.s, cell2.s, cell0.dsdy, cell2.dsdy, &sL, &dsL, &ddsL, &dddsL, fdy);
	fCubicInterp(cell1.u, cell3.u, cell1.dudy, cell3.dudy, &uR, &duR, &dduR, &ddduR, fdy);
	fCubicInterp(cell1.r, cell3.r, cell1.drdy, cell3.drdy, &rR, &drR, &ddrR, &dddrR, fdy);
	fCubicInterp(cell1.v, cell3.v, cell1.dvdy, cell3.dvdy, &vR, &dvR, &ddvR, &dddvR, fdy);
	fCubicInterp(cell1.s, cell3.s, cell1.dsdy, cell3.dsdy, &sR, &dsR, &ddsR, &dddsR, fdy);

	unsigned __int16 m_old[4] = { old_dest_mult, old_dest_mult, old_dest_mult, old_dest_mult };
	unsigned __int16 m_new[4] = { new_mult, new_mult, new_mult, new_mult };

	//int u, du, ddu, dddu;
	//int v, dv, ddv, dddv;
	signed __int32 u0[1024];
	signed __int32 v0[1024];

	//void *u0;
	//void *v0;

	for (int i=0; i<_dy; i++)
	{
		uL_now = uL + i*(duL + i*(dduL + i*(ddduL)));
		rL_now = rL + i*(drL + i*(ddrL + i*(dddrL)));
		vL_now = vL + i*(dvL + i*(ddvL + i*(dddvL)));
		sL_now = sL + i*(dsL + i*(ddsL + i*(dddsL)));
		uR_now = uR + i*(duR + i*(dduR + i*(ddduR)));
		rR_now = rR + i*(drR + i*(ddrR + i*(dddrR)));
		vR_now = vR + i*(dvR + i*(ddvR + i*(dddvR)));
		sR_now = sR + i*(dsR + i*(ddsR + i*(dddsR)));

		iCubicInterp(uL_now, uR_now, rL_now, rR_now, &u0[0], &u0[1], &u0[2], &u0[3], fdx);
		iCubicInterp(vL_now, vR_now, sL_now, sR_now, &v0[0], &v0[1], &v0[2], &v0[3], fdx);
	
		if (bHighQuality)
		{
			if (old_dest_mult != 0)		// blend with old
			__asm
			{
				xor ecx, ecx

				mov esi, tex
				mov edi, dest
				add edi, dest_pos

				movq mm2, m_new
				movq mm3, m_old
				pxor mm6, mm6		// sub-u coord
				pxor mm7, mm7		// sub-v coord

				// pre-cache the old image, to consolidate stalls
				/*
				mov edx, old_dest
				mov ecx, _dx
				shr ecx, 3
				LOOP0:
					mov   eax, dword ptr [edx + ecx*8]
					dec   ecx
					jnz   LOOP0
				*/
							/*
							mov eax, dddu
							imul ecx
							add eax, ddu
							imul ecx
							add eax, du
							imul ecx
							add eax, u
							mov ebx, eax
							*/
							mov ebx, dword ptr [u0]

				ALIGN 8
				LOOP1:

					mov eax, dword ptr [v0+12]
					imul ecx
	// high quality mode: lowest SHIFTFACTOR bits of this register will be the sub-u coordinate
	movd mm6, ebx

						shr ebx, SHIFTFACTOR
					add eax, dword ptr [v0+8]
					imul ecx
						and ebx, 0x00FF
					add eax, dword ptr [v0+4]
					imul ecx
						inc ecx
					add eax, dword ptr [v0]

	// high quality mode: lowest SHIFTFACTOR bits of this register will be the sub-v coordinate
	movd mm7, eax
					shr eax, SHIFTFACTOR - 8
					and eax, 0xFF00
					or  ebx, eax

				 
	
	//bilinear texel interp:
	// mm4: 1-u
	// mm5: 1-v
	// mm6: u
	// mm7: v

	// get u- and v-blend multipliers

	psllq mm7, 32                   // ?v??|0000
	por   mm7, mm6					// ?v??|?u??
	pslld mm7, 32-SHIFTFACTOR		// v??0|u??0
	psrld mm7, 24           		// 000v|000u
	movq  mm6, mm7			
	psllw mm6, 8
	por   mm6, mm7                  // 00vv|00uu
	movq  mm7, mm6
	pslld mm7, 16
	 pxor  mm0, mm0
	por   mm6, mm7                  // vvvv|uuuu
	movq  mm7, mm6
	punpcklbw  mm6, mm0				// 0u0u|0u0u
	punpckhbw  mm7, mm0				// 0v0v|0v0v

	// get their inverses
	pcmpeqb    mm4, mm4
	psrlw      mm4, 8
	psubb      mm4, mm6
	pcmpeqb    mm5, mm5
	psrlw      mm5, 8
	psubb      mm5, mm7

    // current state:
	// mm6 ~ 0u0u|0u0u      mm4 ~ inverse
	// mm7 ~ 0v0v|0v0v      mm5 ~ inverse

		// edx <- offset of pixel to the right
		mov edx, ebx
		mov eax, ebx
		inc edx
		and edx, 0x00ff
		and eax, 0xff00
		or  edx, eax

	// blend on u (top)
	punpcklbw  mm0, dword ptr [esi+ebx*4]
	punpcklbw  mm3, dword ptr [esi+edx*4]
	psrlw      mm0, 8
	psrlw      mm3, 8
	pmullw     mm0, mm4
		// ebx <- offset of pixel below
		add ebx, 256
		and ebx, 0xffff
	pmullw     mm3, mm6
		// edx <- offset of pixel below and to the right
		add edx, 256
		and edx, 0xffff
	paddusb    mm0, mm3
	psrlw      mm0, 8

		// blend on u (bottom)
		punpcklbw  mm2, dword ptr [esi+ebx*4]
		punpcklbw  mm1, dword ptr [esi+edx*4]
		psrlw      mm2, 8
		psrlw      mm1, 8
		pmullw     mm2, mm4
		pmullw     mm1, mm6
		paddusb    mm2, mm1
		psrlw      mm2, 8

	// blend on v
	pmullw     mm2, mm7
	pmullw     mm0, mm5
	paddusb    mm0, mm2
	psrlw      mm0, 8




						// put new pixel into mm0
	//					punpcklbw  mm0, dword ptr [esi+ebx*4]  // each 'unpack' only accesses 32 bits of memory and they go into the high bytes of the 4 words in the mmx register.
							mov eax, dword ptr [u0+12]
							imul ecx
	//					psrlw      mm0, 8
							add eax, dword ptr [u0+8]

									/*
									//for alpha_src_color blending:
									movq    mm2, mm0
									psrlw   mm2, 2		// to intensify
									pcmpeqb mm3, mm3
									psrlw   mm3, 8
									psubb   mm3, mm2
									*/

						// put old pixel into mm1
						pmullw      mm0, m_new
						mov        edx, old_dest
						punpcklbw  mm1, dword ptr [edx+ecx*4-4]  // each 'unpack' only accesses 32 bits of memory.
							imul ecx

						psrlw      mm1, 8
						// multiply & add
						pmullw      mm1, m_old
							add eax, dword ptr [u0+4]
							// ---stall: 2 cycles---
						paddusw     mm0, mm1
							imul ecx
						
						// pack & store
						psrlw       mm0, 8		 // bytes: 0a0b0g0r
							add eax, dword ptr [u0]
						packuswb    mm0, mm0    // bytes: abgrabgr
							mov ebx, eax
						movd        dword ptr [edi+ecx*4-4], mm0  // store 

					mov   eax, ecx
					sub   eax, _dx
					jl    LOOP1

				EMMS
			}
			else	// paste (no blend), high-quality
			__asm
			{
				xor ecx, ecx

				mov esi, tex
				mov edi, dest
				add edi, dest_pos

				movq mm2, m_new
				movq mm3, m_old
				pxor mm6, mm6		// sub-u coord
				pxor mm7, mm7		// sub-v coord

				// pre-cache the old image, to consolidate stalls
				/*
				mov edx, old_dest
				mov ecx, _dx
				shr ecx, 3
				LOOP0:
					mov   eax, dword ptr [edx + ecx*8]
					dec   ecx
					jnz   LOOP0
				*/
							/*
							mov eax, dddu
							imul ecx
							add eax, ddu
							imul ecx
							add eax, du
							imul ecx
							add eax, u
							mov ebx, eax
							*/
							mov ebx, dword ptr [u0]

				ALIGN 8
				LOOP1c:

					mov eax, dword ptr [v0+12]
					imul ecx
	// high quality mode: lowest SHIFTFACTOR bits of this register will be the sub-u coordinate
	movd mm6, ebx

						shr ebx, SHIFTFACTOR
					add eax, dword ptr [v0+8]
					imul ecx
						and ebx, 0x00FF
					add eax, dword ptr [v0+4]
					imul ecx
						inc ecx
					add eax, dword ptr [v0]

	// high quality mode: lowest SHIFTFACTOR bits of this register will be the sub-v coordinate
	movd mm7, eax
					shr eax, SHIFTFACTOR - 8
					and eax, 0xFF00
					or  ebx, eax

				 
	
	//bilinear texel interp:
	// mm4: 1-u
	// mm5: 1-v
	// mm6: u
	// mm7: v

	// get u- and v-blend multipliers

	psllq mm7, 32                   // ?v??|0000
	por   mm7, mm6					// ?v??|?u??
	pslld mm7, 32-SHIFTFACTOR		// v??0|u??0
	psrld mm7, 24           		// 000v|000u
	movq  mm6, mm7			
	psllw mm6, 8
	por   mm6, mm7                  // 00vv|00uu
	movq  mm7, mm6
	pslld mm7, 16
	 pxor  mm0, mm0
	por   mm6, mm7                  // vvvv|uuuu
	movq  mm7, mm6
	punpcklbw  mm6, mm0				// 0u0u|0u0u
	punpckhbw  mm7, mm0				// 0v0v|0v0v

	// get their inverses
	pcmpeqb    mm4, mm4
	psrlw      mm4, 8
	psubb      mm4, mm6
	pcmpeqb    mm5, mm5
	psrlw      mm5, 8
	psubb      mm5, mm7

		// edx <- offset of pixel to the right
		mov edx, ebx
		mov eax, ebx
		inc edx
		and edx, 0x00ff
		and eax, 0xff00
		or  edx, eax

	// blend on u (top)
	punpcklbw  mm0, dword ptr [esi+ebx*4]
	punpcklbw  mm3, dword ptr [esi+edx*4]
	psrlw      mm0, 8
	psrlw      mm3, 8
	pmullw     mm0, mm4
		// ebx <- offset of pixel below
		add ebx, 256
		and ebx, 0xffff
	pmullw     mm3, mm6
		// edx <- offset of pixel below and to the right
		add edx, 256
		and edx, 0xffff
	paddusb    mm0, mm3
	psrlw      mm0, 8

		// blend on u (bottom)
		punpcklbw  mm2, dword ptr [esi+ebx*4]
		punpcklbw  mm1, dword ptr [esi+edx*4]
		psrlw      mm2, 8
		psrlw      mm1, 8
		pmullw     mm2, mm4
		pmullw     mm1, mm6
		paddusb    mm2, mm1
		psrlw      mm2, 8

	// blend on v
	pmullw     mm2, mm7
	pmullw     mm0, mm5
	paddusb    mm0, mm2
	psrlw      mm0, 8




						// put new pixel into mm0
	//					punpcklbw  mm0, dword ptr [esi+ebx*4]  // each 'unpack' only accesses 32 bits of memory and they go into the high bytes of the 4 words in the mmx register.
							mov eax, dword ptr [u0+12]
							imul ecx
	//					psrlw      mm0, 8
							add eax, dword ptr [u0+8]

									/*
									//for alpha_src_color blending:
									movq    mm2, mm0
									psrlw   mm2, 2		// to intensify
									pcmpeqb mm3, mm3
									psrlw   mm3, 8
									psubb   mm3, mm2
									*/

						// put old pixel into mm1
	//////				pmullw      mm0, m_new
						mov        edx, old_dest
	//////				punpcklbw  mm1, dword ptr [edx+ecx*4-4]  // each 'unpack' only accesses 32 bits of memory.
							imul ecx

	//////				psrlw      mm1, 8
						// multiply & add
	//////				pmullw      mm1, m_old
							add eax, dword ptr [u0+4]
							// ---stall: 2 cycles---
	//////				paddusw     mm0, mm1
							imul ecx
						
						// pack & store
	//////				psrlw       mm0, 8		 // bytes: 0a0b0g0r
							add eax, dword ptr [u0]
						packuswb    mm0, mm0    // bytes: abgrabgr
							mov ebx, eax
						movd        dword ptr [edi+ecx*4-4], mm0  // store 

					mov   eax, ecx
					sub   eax, _dx
					jl    LOOP1c

				EMMS
			}
		}
		else        // LOW QUALITY
		{
			if (old_dest_mult != 0)		// blend with old
			__asm
			{
				xor ecx, ecx

				mov esi, tex
				mov edi, dest
				add edi, dest_pos

				movq mm2, m_new
				movq mm3, m_old
				pxor mm6, mm6		// sub-u coord
				pxor mm7, mm7		// sub-v coord

				mov ebx, dword ptr [u0]

				ALIGN 8
				LOOP1b:

					mov eax, dword ptr [v0+12]
					imul ecx
						shr ebx, SHIFTFACTOR
					add eax, dword ptr [v0+8]
					imul ecx
						and ebx, 0x00FF
					add eax, dword ptr [v0+4]
					imul ecx
						inc ecx
					add eax, dword ptr [v0]
					
					shr eax, SHIFTFACTOR - 8
					and eax, 0xFF00
					or  ebx, eax

					// put new pixel into mm0
					punpcklbw  mm0, dword ptr [esi+ebx*4]  // each 'unpack' only accesses 32 bits of memory and they go into the high bytes of the 4 words in the mmx register.
						mov eax, dword ptr [u0+12]
						imul ecx
					psrlw      mm0, 8
						add eax, dword ptr [u0+8]

								/*
								//for alpha_src_color blending:
								movq    mm2, mm0
								psrlw   mm2, 2		// to intensify
								pcmpeqb mm3, mm3
								psrlw   mm3, 8
								psubb   mm3, mm2
								*/

					// put old pixel into mm1
					pmullw      mm0, m_new
					mov        edx, old_dest
					punpcklbw  mm1, dword ptr [edx+ecx*4-4]  // each 'unpack' only accesses 32 bits of memory.
						imul ecx

					psrlw      mm1, 8
					// multiply & add
					pmullw      mm1, m_old
						add eax, dword ptr [u0+4]
						// ---stall: 2 cycles---
					paddusw     mm0, mm1
						imul ecx
					
					// pack & store
					psrlw       mm0, 8		 // bytes: 0a0b0g0r
						add eax, dword ptr [u0]
					packuswb    mm0, mm0    // bytes: abgrabgr
						mov ebx, eax
					movd        dword ptr [edi+ecx*4-4], mm0  // store 

					mov   eax, ecx
					sub   eax, _dx
					jl    LOOP1b

				EMMS
			}
			else    // paste (no blend)
			__asm
			{
				xor ecx, ecx

				mov esi, tex
				mov edi, dest
				add edi, dest_pos

				movq mm2, m_new
				movq mm3, m_old
				pxor mm6, mm6		// sub-u coord
				pxor mm7, mm7		// sub-v coord

				mov ebx, dword ptr [u0]

				ALIGN 8
				LOOP1d:

					mov eax, dword ptr [v0+12]
					imul ecx
						shr ebx, SHIFTFACTOR
					add eax, dword ptr [v0+8]
					imul ecx
						and ebx, 0x00FF
					add eax, dword ptr [v0+4]
					imul ecx
						inc ecx
					add eax, dword ptr [v0]
					
					shr eax, SHIFTFACTOR - 8
					and eax, 0xFF00
					or  ebx, eax

					// put new pixel into mm0
					punpcklbw  mm0, dword ptr [esi+ebx*4]  // each 'unpack' only accesses 32 bits of memory and they go into the high bytes of the 4 words in the mmx register.
						mov eax, dword ptr [u0+12]
						imul ecx
					psrlw      mm0, 8
						add eax, dword ptr [u0+8]

								/*
								//for alpha_src_color blending:
								movq    mm2, mm0
								psrlw   mm2, 2		// to intensify
								pcmpeqb mm3, mm3
								psrlw   mm3, 8
								psubb   mm3, mm2
								*/

					// put old pixel into mm1
					//////pmullw      mm0, m_new
					mov        edx, old_dest
					//////punpcklbw  mm1, dword ptr [edx+ecx*4-4]  // each 'unpack' only accesses 32 bits of memory.
						imul ecx

					//////psrlw      mm1, 8
					// multiply & add
					//////pmullw      mm1, m_old
						add eax, dword ptr [u0+4]
						// ---stall: 2 cycles---
					//////paddusw     mm0, mm1
						imul ecx
					
					// pack & store
					//////psrlw       mm0, 8		 // bytes: 0a0b0g0r
						add eax, dword ptr [u0]
					packuswb    mm0, mm0    // bytes: abgrabgr
						mov ebx, eax
					movd        dword ptr [edi+ecx*4-4], mm0  // store 

					mov   eax, ecx
					sub   eax, _dx
					jl    LOOP1d

				EMMS
			}
		}

		dest_pos += W*4;
		old_dest += W*4;

	}

}








///////////////////////////////////////////////
//
//          NON-256-BY-256 VERSION
//  
// -slow thanks to awful modding of u, v (6 divs!)
// 
///////////////////////////////////////////////




/*

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
					int W,  // of dest. pixelbuffer
					int H,  // of dest. pixelbuffer
					unsigned char *tex,
                     int texW,
                     int texH,
					bool bHighQuality)
{
	if (!tex) return;
	if (!dest) return;

    int DIVFACTOR_U = (INTFACTOR / (float)texW * 256.0f);// / 256.0f;
    int DIVFACTOR_V = (INTFACTOR / (float)texH * 256.0f);// / 256.0f;
    int DIVFACTOR_U_OVER_256 = DIVFACTOR_U/256;
    int DIVFACTOR_V_OVER_256 = DIVFACTOR_V/256;
    int temp, temp2;

	//  (x0y0)
	//   u0v0 ------ u1v1
	//    |            |
	//    |            |
	//   u2v2 ------->uv3 
	//				(x1y1)

	//unsigned __int16 texW16 = texW;
	//unsigned __int16 texH16 = texH;
	//int temp;

	unsigned int *dest32 = (unsigned int *)dest;
	unsigned int *tex32 = (unsigned int *)tex;

	int dx = x1 - x0;
	int _dx = x1 - x0;
	int _dy = y1 - y0;
	float fdy = (float)(y1 - y0);
	float fdx = (float)(x1 - x0);
	int dest_pos;
	float min;

	min = (cell0.u < cell1.u) ? cell0.u : cell1.u;
	min = (min < cell2.u) ? min : cell2.u;
	min = (min < cell3.u) ? min : cell3.u;
	if (min < 0) 
	{
		float ufix = (2.0f - (int)(min/(INTFACTOR*256))) * INTFACTOR*256.0f;
		cell0.u += ufix;
		cell1.u += ufix;
		cell2.u += ufix;
		cell3.u += ufix;
	}

	min = (cell0.v < cell1.v) ? cell0.v : cell1.v;
	min = (min < cell2.v) ? min : cell2.v;
	min = (min < cell3.v) ? min : cell3.v;
	if (min < 0) 
	{
		float vfix = (2.0f - (int)(min/(INTFACTOR*256))) * INTFACTOR*256.0f;
		cell0.v += vfix;
		cell1.v += vfix;
		cell2.v += vfix;
		cell3.v += vfix;
	}

	dest_pos = (y0*W + x0)*4;
	old_dest += dest_pos;

	float uL   ;
	float duL  ;
	float dduL ;
	float ddduL;
	float rL   ;
	float drL  ;
	float ddrL ;
	float dddrL;
	float vL   ;
	float dvL  ;
	float ddvL ;
	float dddvL;
	float sL   ;
	float dsL  ;
	float ddsL ;
	float dddsL;
	float uR   ;
	float duR  ;
	float dduR ;
	float ddduR;
	float rR   ;
	float drR  ;
	float ddrR ;
	float dddrR;
	float vR   ;
	float dvR  ;
	float ddvR ;
	float dddvR;
	float sR   ;
	float dsR  ;
	float ddsR ;
	float dddsR;
	float uL_now;
	float rL_now;
	float vL_now;
	float sL_now;
	float uR_now;
	float rR_now;
	float vR_now;
	float sR_now;

	// next 24 vars are with respect to y:
	fCubicInterp(cell0.u, cell2.u, cell0.dudy, cell2.dudy, &uL, &duL, &dduL, &ddduL, fdy);
	fCubicInterp(cell0.r, cell2.r, cell0.drdy, cell2.drdy, &rL, &drL, &ddrL, &dddrL, fdy);
	fCubicInterp(cell0.v, cell2.v, cell0.dvdy, cell2.dvdy, &vL, &dvL, &ddvL, &dddvL, fdy);
	fCubicInterp(cell0.s, cell2.s, cell0.dsdy, cell2.dsdy, &sL, &dsL, &ddsL, &dddsL, fdy);
	fCubicInterp(cell1.u, cell3.u, cell1.dudy, cell3.dudy, &uR, &duR, &dduR, &ddduR, fdy);
	fCubicInterp(cell1.r, cell3.r, cell1.drdy, cell3.drdy, &rR, &drR, &ddrR, &dddrR, fdy);
	fCubicInterp(cell1.v, cell3.v, cell1.dvdy, cell3.dvdy, &vR, &dvR, &ddvR, &dddvR, fdy);
	fCubicInterp(cell1.s, cell3.s, cell1.dsdy, cell3.dsdy, &sR, &dsR, &ddsR, &dddsR, fdy);

	unsigned __int16 m_old[4] = { old_dest_mult, old_dest_mult, old_dest_mult, old_dest_mult };
	unsigned __int16 m_new[4] = { new_mult, new_mult, new_mult, new_mult };

	//int u, du, ddu, dddu;
	//int v, dv, ddv, dddv;
	signed __int32 u0[1024];
	signed __int32 v0[1024];

	//void *u0;
	//void *v0;

    int temp_u;
    int temp_v;
    int temp_v_times_texW;
    int temp_u_plus_1_mod_texW;

	for (int i=0; i<_dy; i++)
	{
		uL_now = uL + i*(duL + i*(dduL + i*(ddduL)));
		rL_now = rL + i*(drL + i*(ddrL + i*(dddrL)));
		vL_now = vL + i*(dvL + i*(ddvL + i*(dddvL)));
		sL_now = sL + i*(dsL + i*(ddsL + i*(dddsL)));
		uR_now = uR + i*(duR + i*(dduR + i*(ddduR)));
		rR_now = rR + i*(drR + i*(ddrR + i*(dddrR)));
		vR_now = vR + i*(dvR + i*(ddvR + i*(dddvR)));
		sR_now = sR + i*(dsR + i*(ddsR + i*(dddsR)));

		iCubicInterp(uL_now, uR_now, rL_now, rR_now, &u0[0], &u0[1], &u0[2], &u0[3], fdx);
		iCubicInterp(vL_now, vR_now, sL_now, sR_now, &v0[0], &v0[1], &v0[2], &v0[3], fdx);
	
		if (bHighQuality)
		{
			if (old_dest_mult != 0)		// blend with old
            / *********************************** /
            / *          HIGH QUALITY           * /
            / *           BLEND-PASTE           * /
            / *          (NON-256x256)          * /
            / *********************************** /
			__asm
			{
				xor ecx, ecx

				mov esi, tex
				mov edi, dest
				add edi, dest_pos

				movq mm2, m_new
				movq mm3, m_old
				pxor mm6, mm6		// sub-u coord
				pxor mm7, mm7		// sub-v coord

				mov eax, dword ptr [u0]

				ALIGN 8
				LOOP1:

                    // [at this point, eax holds u0*DIVFACTOR]

                    xor  edx, edx
                    div  DIVFACTOR_U        
                    mov  temp, edx    // preserve remainder
                    // modulate integer u-coord by texW
                    xor  edx, edx
                    div  texW
                    mov  ebx, edx
                     mov  temp_u, ebx        // u, in range [0..texW)
                        // bilinear interp: save sub-u coordinate (0..255)
                        mov  eax, temp
                        xor  edx, edx
                        div  DIVFACTOR_U_OVER_256 
                        movd mm6, eax 

					mov eax, dword ptr [v0+12]
					imul ecx
					add eax, dword ptr [v0+8]
					imul ecx
					add eax, dword ptr [v0+4]
					imul ecx
					add eax, dword ptr [v0]

                    inc ecx

                    
                    
                    // [at this point, eax holds v0*DIVFACTOR]

                    xor  edx, edx
                    div  DIVFACTOR_V
                    mov  temp, edx        //  push remainder
                    // modulate integer v-coord by texH
                    xor  edx, edx
                    div  texH
                    mov  eax, edx
                    // multiply by texW for v-coord
                    xor  edx, edx
                     mov  temp_v, eax        // v, in range texW*[0..texH)
                    imul texW
                     mov  temp_v_times_texW, eax        // v, in range texW*[0..texH)
                    add  ebx, eax
                        // bilinear interp: save sub-u coordinate (0..255)
                        mov  eax, temp    // pop remainder
                        xor  edx, edx
                        div  DIVFACTOR_V_OVER_256 
                        movd mm7, eax 

					//mov ebx, [esi+eax*4]	// RGB
					//mov [edi], ebx
					 
	/ *
	// dither
	pslld      mm6, 28
	pslld      mm7, 28
	psrld      mm6, 28
	psrld      mm7, 28
	pslld      mm7, 8
	por        mm6, mm7

	movq       mm7, mm6
	pslld      mm7, 16
	por        mm6, mm7

	punpcklbw  mm6, mm6		// abcdefgh -> eeffgghh
	psrlw      mm6, 8
	paddusb    mm0, mm6
	* /

	// ***bilinear texel interp***
	// mm4: 1-u
	// mm5: 1-v
	// mm6: u
	// mm7: v

	// get u- and v-blend multipliers
	psllq mm7, 32                   // ?v??|0000
	por   mm7, mm6				    // ?v??|?u??
	movq  mm6, mm7			
	psllw mm6, 8
	por   mm6, mm7                  // 00vv|00uu
	movq  mm7, mm6
	pslld mm7, 16
	 pxor  mm0, mm0
	por   mm6, mm7                  // vvvv|uuuu
	movq  mm7, mm6
	punpcklbw  mm6, mm0				// 0u0u|0u0u
	punpckhbw  mm7, mm0				// 0v0v|0v0v

	// get their inverses
		    		            // ebx <- offset of upper-left pixel (already there)
		    		            // edx <- offset of pixel to the right
            		            mov edx, temp_u
	pcmpeqb mm4, mm4
            		            inc edx
	psrlw   mm4, 8
            		            mov eax, edx
	psubb   mm4, mm6
                		        xor eax, texW
                		        jnz ok1
            		            xor edx, edx                
           		               ok1:
	pcmpeqb mm5, mm5
            		            mov temp_u_plus_1_mod_texW, edx
	psrlw   mm5, 8
            		            add edx, temp_v_times_texW
	psubb   mm5, mm7

    // current state:
	// mm6 ~ 0u0u|0u0u      mm4 ~ inverse
	// mm7 ~ 0v0v|0v0v      mm5 ~ inverse

	// blend on u (top)
	punpcklbw  mm0, dword ptr [esi+ebx*4]
	punpcklbw  mm3, dword ptr [esi+edx*4]
		                            // ebx <- offset of pixel below
		                            // edx <- offset of pixel below and to the right
                                    mov eax, temp_v
	psrlw      mm0, 8
                                    inc eax
	psrlw      mm3, 8
                                    mov ebx, eax
	pmullw     mm0, mm4
                                    xor ebx, texH
                                    jnz ok2
                                    xor eax, eax                
                                   ok2:
    pmullw     mm3, mm6
                                    xor edx, edx
                                    imul texW
                                    mov ebx, eax
                                    mov edx, eax
	paddusb    mm0, mm3
                                    add ebx, temp_u
                                    add edx, temp_u_plus_1_mod_texW
	psrlw      mm0, 8

		// blend on u (bottom)
		punpcklbw  mm2, dword ptr [esi+ebx*4]
		punpcklbw  mm1, dword ptr [esi+edx*4]
		psrlw      mm2, 8
		psrlw      mm1, 8
		pmullw     mm2, mm4
		pmullw     mm1, mm6
		paddusb    mm2, mm1
		psrlw      mm2, 8

	// blend on v
	pmullw     mm2, mm7
	pmullw     mm0, mm5
	paddusb    mm0, mm2
	psrlw      mm0, 8




						// put new pixel into mm0
	//					punpcklbw  mm0, dword ptr [esi+ebx*4]  // each 'unpack' only accesses 32 bits of memory and they go into the high bytes of the 4 words in the mmx register.
							mov eax, dword ptr [u0+12]
							imul ecx
	//					psrlw      mm0, 8
							add eax, dword ptr [u0+8]

									/ *
									//for alpha_src_color blending:
									movq    mm2, mm0
									psrlw   mm2, 2		// to intensify
									pcmpeqb mm3, mm3
									psrlw   mm3, 8
									psubb   mm3, mm2
									* /

						// put old pixel into mm1
						pmullw      mm0, m_new
						mov        edx, old_dest
						punpcklbw  mm1, dword ptr [edx+ecx*4-4]  // each 'unpack' only accesses 32 bits of memory.
							imul ecx

						psrlw      mm1, 8
						// multiply & add
						pmullw      mm1, m_old
							add eax, dword ptr [u0+4]
							// ---stall: 2 cycles---
						paddusw     mm0, mm1
							imul ecx
						
						// pack & store
						psrlw       mm0, 8		 // bytes: 0a0b0g0r
							add eax, dword ptr [u0]
						packuswb    mm0, mm0    // bytes: abgrabgr
							//mov ebx, eax
						movd        dword ptr [edi+ecx*4-4], mm0  // store 

					mov   ebx, ecx
					sub   ebx, _dx
					jl    LOOP1

				EMMS
			}
			else
            / *********************************** /
            / *          HIGH QUALITY           * /
            / *           SOLID-PASTE           * /
            / *          (NON-256x256)          * /
            / *********************************** /
			__asm
			{
				xor ecx, ecx

				mov esi, tex
				mov edi, dest
				add edi, dest_pos

				movq mm2, m_new
				movq mm3, m_old
				pxor mm6, mm6		// sub-u coord
				pxor mm7, mm7		// sub-v coord

				mov eax, dword ptr [u0]

				ALIGN 8
				LOOP1c:

                    // [at this point, eax holds u0*DIVFACTOR]

                    xor  edx, edx
                    div  DIVFACTOR_U        
                    mov  temp, edx    // preserve remainder
                    // modulate integer u-coord by texW
                    xor  edx, edx
                    div  texW
                    mov  ebx, edx
                     mov  temp_u, ebx        // u, in range [0..texW)
                        // bilinear interp: save sub-u coordinate (0..255)
                        mov  eax, temp
                        xor  edx, edx
                        div  DIVFACTOR_U_OVER_256 
                        movd mm6, eax 

					mov eax, dword ptr [v0+12]
					imul ecx
					add eax, dword ptr [v0+8]
					imul ecx
					add eax, dword ptr [v0+4]
					imul ecx
					add eax, dword ptr [v0]

                    inc ecx

                    
                    
                    // [at this point, eax holds v0*DIVFACTOR]

                    xor  edx, edx
                    div  DIVFACTOR_V
                    mov  temp, edx        //  push remainder
                    // modulate integer v-coord by texH
                    xor  edx, edx
                    div  texH
                    mov  eax, edx
                    // multiply by texW for v-coord
                    xor  edx, edx
                     mov  temp_v, eax        // v, in range texW*[0..texH)
                    imul texW
                     mov  temp_v_times_texW, eax        // v, in range texW*[0..texH)
                    add  ebx, eax
                        // bilinear interp: save sub-u coordinate (0..255)
                        mov  eax, temp    // pop remainder
                        xor  edx, edx
                        div  DIVFACTOR_V_OVER_256 
                        movd mm7, eax 

					//mov ebx, [esi+eax*4]	// RGB
					//mov [edi], ebx
					 
	/ *
	// dither
	pslld      mm6, 28
	pslld      mm7, 28
	psrld      mm6, 28
	psrld      mm7, 28
	pslld      mm7, 8
	por        mm6, mm7

	movq       mm7, mm6
	pslld      mm7, 16
	por        mm6, mm7

	punpcklbw  mm6, mm6		// abcdefgh -> eeffgghh
	psrlw      mm6, 8
	paddusb    mm0, mm6
	* /

	// ***bilinear texel interp***
	// mm4: 1-u
	// mm5: 1-v
	// mm6: u
	// mm7: v

	// get u- and v-blend multipliers
	psllq mm7, 32                   // ?v??|0000
	por   mm7, mm6				    // ?v??|?u??
	movq  mm6, mm7			
	psllw mm6, 8
	por   mm6, mm7                  // 00vv|00uu
	movq  mm7, mm6
	pslld mm7, 16
	 pxor  mm0, mm0
	por   mm6, mm7                  // vvvv|uuuu
	movq  mm7, mm6
	punpcklbw  mm6, mm0				// 0u0u|0u0u
	punpckhbw  mm7, mm0				// 0v0v|0v0v

	// get their inverses
		    		            // ebx <- offset of upper-left pixel (already there)
		    		            // edx <- offset of pixel to the right
            		            mov edx, temp_u
	pcmpeqb mm4, mm4
            		            inc edx
	psrlw   mm4, 8
            		            mov eax, edx
	psubb   mm4, mm6
                		        xor eax, texW
                		        jnz ok1c
            		            xor edx, edx                
           		               ok1c:
	pcmpeqb mm5, mm5
            		            mov temp_u_plus_1_mod_texW, edx
	psrlw   mm5, 8
            		            add edx, temp_v_times_texW
	psubb   mm5, mm7

    // current state:
	// mm6 ~ 0u0u|0u0u      mm4 ~ inverse
	// mm7 ~ 0v0v|0v0v      mm5 ~ inverse

	// blend on u (top)
	punpcklbw  mm0, dword ptr [esi+ebx*4]
	punpcklbw  mm3, dword ptr [esi+edx*4]
		                            // ebx <- offset of pixel below
		                            // edx <- offset of pixel below and to the right
                                    mov eax, temp_v
	psrlw      mm0, 8
                                    inc eax
	psrlw      mm3, 8
                                    mov ebx, eax
	pmullw     mm0, mm4
                                    xor ebx, texH
                                    jnz ok2c
                                    xor eax, eax                
                                   ok2c:
    pmullw     mm3, mm6
                                    xor edx, edx
                                    imul texW
                                    mov ebx, eax
                                    mov edx, eax
	paddusb    mm0, mm3
                                    add ebx, temp_u
                                    add edx, temp_u_plus_1_mod_texW
	psrlw      mm0, 8

		// blend on u (bottom)
		punpcklbw  mm2, dword ptr [esi+ebx*4]
		punpcklbw  mm1, dword ptr [esi+edx*4]
		psrlw      mm2, 8
		psrlw      mm1, 8
		pmullw     mm2, mm4
		pmullw     mm1, mm6
		paddusb    mm2, mm1
		psrlw      mm2, 8

	// blend on v
	pmullw     mm2, mm7
	pmullw     mm0, mm5
	paddusb    mm0, mm2
	psrlw      mm0, 8




						// put new pixel into mm0
	//					punpcklbw  mm0, dword ptr [esi+ebx*4]  // each 'unpack' only accesses 32 bits of memory and they go into the high bytes of the 4 words in the mmx register.
							mov eax, dword ptr [u0+12]
							imul ecx
	//					psrlw      mm0, 8
							add eax, dword ptr [u0+8]

									/ *
									//for alpha_src_color blending:
									movq    mm2, mm0
									psrlw   mm2, 2		// to intensify
									pcmpeqb mm3, mm3
									psrlw   mm3, 8
									psubb   mm3, mm2
									* /

						// put old pixel into mm1
//////					pmullw      mm0, m_new
						mov        edx, old_dest
//////					punpcklbw  mm1, dword ptr [edx+ecx*4-4]  // each 'unpack' only accesses 32 bits of memory.
							imul ecx

//////					psrlw      mm1, 8
						// multiply & add
//////					pmullw      mm1, m_old
							add eax, dword ptr [u0+4]
							// ---stall: 2 cycles---
//////					paddusw     mm0, mm1
							imul ecx
						
						// pack & store
//////					psrlw       mm0, 8		 // bytes: 0a0b0g0r
							add eax, dword ptr [u0]
						packuswb    mm0, mm0    // bytes: abgrabgr
							//mov ebx, eax
						movd        dword ptr [edi+ecx*4-4], mm0  // store 

					mov   ebx, ecx
					sub   ebx, _dx
					jl    LOOP1c

				EMMS
			}
        }
		else        
		{
            / *********************************** /
            / *           LOW QUALITY           * /
            / *           BLEND-PASTE           * /
            / *          (NON-256x256)          * /
            / *********************************** /
			if (old_dest_mult != 0)		// blend with old
			__asm
			{
				xor ecx, ecx

				mov esi, tex
				mov edi, dest
				add edi, dest_pos

				movq mm2, m_new
				movq mm3, m_old
				pxor mm6, mm6		// sub-u coord
				pxor mm7, mm7		// sub-v coord

				mov eax, dword ptr [u0]

				ALIGN 8
				LOOP1b:

                    // [at this point, eax holds u0*DIVFACTOR]

                    xor  edx, edx
                    div  DIVFACTOR_U        
//////              mov  temp, edx    // preserve remainder
                    // modulate integer u-coord by texW
                    xor  edx, edx
                    div  texW
                    mov  ebx, edx
                     mov  temp_u, ebx        // u, in range [0..texW)
                        // bilinear interp: save sub-u coordinate (0..255)
//////                  mov  eax, temp
//////                  xor  edx, edx
//////                  div  DIVFACTOR_U_OVER_256 
//////                  movd mm6, eax 

					mov eax, dword ptr [v0+12]
					imul ecx
					add eax, dword ptr [v0+8]
					imul ecx
					add eax, dword ptr [v0+4]
					imul ecx
					add eax, dword ptr [v0]

                    inc ecx

                    
                    
                    // [at this point, eax holds v0*DIVFACTOR]

                    xor  edx, edx
                    div  DIVFACTOR_V
//////              mov  temp, edx        //  push remainder
                    // modulate integer v-coord by texH
                    xor  edx, edx
                    div  texH
                    mov  eax, edx
                    // multiply by texW for v-coord
                    xor  edx, edx
                     mov  temp_v, eax        // v, in range texW*[0..texH)
                    imul texW
                     mov  temp_v_times_texW, eax        // v, in range texW*[0..texH)
                    add  ebx, eax
                        // bilinear interp: save sub-u coordinate (0..255)
//////                  mov  eax, temp    // pop remainder
//////                  xor  edx, edx
//////                  div  DIVFACTOR_V_OVER_256 
//////                  movd mm7, eax 

					//mov ebx, [esi+eax*4]	// RGB
					//mov [edi], ebx
					 


						// put new pixel into mm0
						punpcklbw  mm0, dword ptr [esi+ebx*4]  // each 'unpack' only accesses 32 bits of memory and they go into the high bytes of the 4 words in the mmx register.
							mov eax, dword ptr [u0+12]
							imul ecx
						psrlw      mm0, 8
							add eax, dword ptr [u0+8]

									/ *
									//for alpha_src_color blending:
									movq    mm2, mm0
									psrlw   mm2, 2		// to intensify
									pcmpeqb mm3, mm3
									psrlw   mm3, 8
									psubb   mm3, mm2
									* /

						// put old pixel into mm1
						pmullw      mm0, m_new
						mov        edx, old_dest
						punpcklbw  mm1, dword ptr [edx+ecx*4-4]  // each 'unpack' only accesses 32 bits of memory.
							imul ecx

						psrlw      mm1, 8
						// multiply & add
						pmullw      mm1, m_old
							add eax, dword ptr [u0+4]
							// ---stall: 2 cycles---
						paddusw     mm0, mm1
							imul ecx
						
						// pack & store
						psrlw       mm0, 8		 // bytes: 0a0b0g0r
							add eax, dword ptr [u0]
						packuswb    mm0, mm0    // bytes: abgrabgr
							//mov ebx, eax
						movd        dword ptr [edi+ecx*4-4], mm0  // store 

					mov   ebx, ecx
					sub   ebx, _dx
					jl    LOOP1b

				EMMS
			}
			else    // paste (no blend)
            / *********************************** /
            / *           LOW QUALITY           * /
            / *           SOLID-PASTE           * /
            / *          (NON-256x256)          * /
            / *********************************** /
			__asm
			{
				xor ecx, ecx

				mov esi, tex
				mov edi, dest
				add edi, dest_pos

				movq mm2, m_new
				movq mm3, m_old
				pxor mm6, mm6		// sub-u coord
				pxor mm7, mm7		// sub-v coord

				mov eax, dword ptr [u0]

				ALIGN 8
				LOOP1d:

                    // [at this point, eax holds u0*DIVFACTOR]

                    xor  edx, edx
                    div  DIVFACTOR_U        
//////              mov  temp, edx    // preserve remainder
                    // modulate integer u-coord by texW
                    xor  edx, edx
                    div  texW
                    mov  ebx, edx
                     mov  temp_u, ebx        // u, in range [0..texW)
                        // bilinear interp: save sub-u coordinate (0..255)
//////                  mov  eax, temp
//////                  xor  edx, edx
//////                  div  DIVFACTOR_U_OVER_256 
//////                  movd mm6, eax 

					mov eax, dword ptr [v0+12]
					imul ecx
					add eax, dword ptr [v0+8]
					imul ecx
					add eax, dword ptr [v0+4]
					imul ecx
					add eax, dword ptr [v0]

                    inc ecx

                    
                    
                    // [at this point, eax holds v0*DIVFACTOR]

                    xor  edx, edx
                    div  DIVFACTOR_V
//////              mov  temp, edx        //  push remainder
                    // modulate integer v-coord by texH
                    xor  edx, edx
                    div  texH
                    mov  eax, edx
                    // multiply by texW for v-coord
                    xor  edx, edx
                     mov  temp_v, eax        // v, in range texW*[0..texH)
                    imul texW
                     mov  temp_v_times_texW, eax        // v, in range texW*[0..texH)
                    add  ebx, eax
                        // bilinear interp: save sub-u coordinate (0..255)
//////                  mov  eax, temp    // pop remainder
//////                  xor  edx, edx
//////                  div  DIVFACTOR_V_OVER_256 
//////                  movd mm7, eax 

					//mov ebx, [esi+eax*4]	// RGB
					//mov [edi], ebx
					 


						// put new pixel into mm0
						punpcklbw  mm0, dword ptr [esi+ebx*4]  // each 'unpack' only accesses 32 bits of memory and they go into the high bytes of the 4 words in the mmx register.
							mov eax, dword ptr [u0+12]
							imul ecx
						psrlw      mm0, 8
							add eax, dword ptr [u0+8]

									/ *
									//for alpha_src_color blending:
									movq    mm2, mm0
									psrlw   mm2, 2		// to intensify
									pcmpeqb mm3, mm3
									psrlw   mm3, 8
									psubb   mm3, mm2
									* /

						// put old pixel into mm1
//////					pmullw      mm0, m_new
						mov        edx, old_dest
//////					punpcklbw  mm1, dword ptr [edx+ecx*4-4]  // each 'unpack' only accesses 32 bits of memory.
							imul ecx

//////					psrlw      mm1, 8
						// multiply & add
//////					pmullw      mm1, m_old
							add eax, dword ptr [u0+4]
							// ---stall: 2 cycles---
//////					paddusw     mm0, mm1
							imul ecx
						
						// pack & store
//////					psrlw       mm0, 8		 // bytes: 0a0b0g0r
							add eax, dword ptr [u0]
						packuswb    mm0, mm0    // bytes: abgrabgr
							//mov ebx, eax
						movd        dword ptr [edi+ecx*4-4], mm0  // store 

					mov   ebx, ecx
					sub   ebx, _dx
					jl    LOOP1d

				EMMS
			}
		}

		dest_pos += W*4;
		old_dest += W*4;

	}

}

*/
















/*

void BlitWarp256(td_cellcornerinfo cell0, 
				 td_cellcornerinfo cell1, 
				 td_cellcornerinfo cell2, 
				 td_cellcornerinfo cell3, 
				 int x0, int y0, 
				 int x1, int y1, 
				 unsigned char *dest, int W, int H, 
				 unsigned char *tex)
{
	if (!tex) return;
	if (!dest) return;

	//  (x0y0)
	//   u0v0 ------ u1v1
	//    |            |
	//    |            |
	//   u2v2 ------->uv3 
	//				(x1y1)

	//unsigned __int16 texW16 = texW;
	//unsigned __int16 texH16 = texH;
	//int temp;

	unsigned int *dest32 = (unsigned int *)dest;
	unsigned int *tex32 = (unsigned int *)tex;

	int dx = x1 - x0;
	int _dx = x1 - x0;
	float fdy = (float)(y1 - y0);
	float fdx = (float)(x1 - x0);
	int y;
	int dest_pos;
	float min;

	min = (cell0.u < cell1.u) ? cell0.u : cell1.u;
	min = (min < cell2.u) ? min : cell2.u;
	min = (min < cell3.u) ? min : cell3.u;
	if (min < 0) 
	{
		float ufix = 2.0f - (int)min;
		cell0.u += ufix;
		cell1.u += ufix;
		cell2.u += ufix;
		cell3.u += ufix;
	}

	min = (cell0.v < cell1.v) ? cell0.v : cell1.v;
	min = (min < cell2.v) ? min : cell2.v;
	min = (min < cell3.v) ? min : cell3.v;
	if (min < 0) 
	{
		float vfix = 2.0f - (int)min;
		cell0.v += vfix;
		cell1.v += vfix;
		cell2.v += vfix;
		cell3.v += vfix;
	}

	cell0.u *= 256*INTFACTOR;
	cell1.u *= 256*INTFACTOR;
	cell2.u *= 256*INTFACTOR;
	cell3.u *= 256*INTFACTOR;
	cell0.r *= 256*INTFACTOR;
	cell1.r *= 256*INTFACTOR;
	cell2.r *= 256*INTFACTOR;
	cell3.r *= 256*INTFACTOR;
	cell0.dudy *= 256*INTFACTOR;
	cell1.dudy *= 256*INTFACTOR;
	cell2.dudy *= 256*INTFACTOR;
	cell3.dudy *= 256*INTFACTOR;
	cell0.drdy *= 256*INTFACTOR;
	cell1.drdy *= 256*INTFACTOR;
	cell2.drdy *= 256*INTFACTOR;
	cell3.drdy *= 256*INTFACTOR;
	cell0.v *= 256*INTFACTOR;
	cell1.v *= 256*INTFACTOR;
	cell2.v *= 256*INTFACTOR;
	cell3.v *= 256*INTFACTOR;
	cell0.s *= 256*INTFACTOR;
	cell1.s *= 256*INTFACTOR;
	cell2.s *= 256*INTFACTOR;
	cell3.s *= 256*INTFACTOR;
	cell0.dvdy *= 256*INTFACTOR;
	cell1.dvdy *= 256*INTFACTOR;
	cell2.dvdy *= 256*INTFACTOR;
	cell3.dvdy *= 256*INTFACTOR;
	cell0.dsdy *= 256*INTFACTOR;
	cell1.dsdy *= 256*INTFACTOR;
	cell2.dsdy *= 256*INTFACTOR;
	cell3.dsdy *= 256*INTFACTOR;

	dest_pos = (y0*W + x0)*4;

	float uL   ;
	float duL  ;
	float dduL ;
	float ddduL;
	float rL   ;
	float drL  ;
	float ddrL ;
	float dddrL;
	float vL   ;
	float dvL  ;
	float ddvL ;
	float dddvL;
	float sL   ;
	float dsL  ;
	float ddsL ;
	float dddsL;
	float uR   ;
	float duR  ;
	float dduR ;
	float ddduR;
	float rR   ;
	float drR  ;
	float ddrR ;
	float dddrR;
	float vR   ;
	float dvR  ;
	float ddvR ;
	float dddvR;
	float sR   ;
	float dsR  ;
	float ddsR ;
	float dddsR;
	float uL_now;
	float rL_now;
	float vL_now;
	float sL_now;
	float uR_now;
	float rR_now;
	float vR_now;
	float sR_now;

	// next 24 vars are with respect to y:
	fCubicInterp(cell0.u, cell2.u, cell0.dudy, cell2.dudy, &uL, &duL, &dduL, &ddduL, fdy);
	fCubicInterp(cell0.r, cell2.r, cell0.drdy, cell2.drdy, &rL, &drL, &ddrL, &dddrL, fdy);
	fCubicInterp(cell0.v, cell2.v, cell0.dvdy, cell2.dvdy, &vL, &dvL, &ddvL, &dddvL, fdy);
	fCubicInterp(cell0.s, cell2.s, cell0.dsdy, cell2.dsdy, &sL, &dsL, &ddsL, &dddsL, fdy);
	fCubicInterp(cell1.u, cell3.u, cell1.dudy, cell3.dudy, &uR, &duR, &dduR, &ddduR, fdy);
	fCubicInterp(cell1.r, cell3.r, cell1.drdy, cell3.drdy, &rR, &drR, &ddrR, &dddrR, fdy);
	fCubicInterp(cell1.v, cell3.v, cell1.dvdy, cell3.dvdy, &vR, &dvR, &ddvR, &dddvR, fdy);
	fCubicInterp(cell1.s, cell3.s, cell1.dsdy, cell3.dsdy, &sR, &dsR, &ddsR, &dddsR, fdy);

	signed __int32 u, du, ddu, dddu;
	signed __int32 v, dv, ddv, dddv;

	int i = 0;

	for (y=y0; y<y1; y++)
	{
		uL_now = uL + i*(duL + i*(dduL + i*(ddduL)));
		rL_now = rL + i*(drL + i*(ddrL + i*(dddrL)));
		vL_now = vL + i*(dvL + i*(ddvL + i*(dddvL)));
		sL_now = sL + i*(dsL + i*(ddsL + i*(dddsL)));
		uR_now = uR + i*(duR + i*(dduR + i*(ddduR)));
		rR_now = rR + i*(drR + i*(ddrR + i*(dddrR)));
		vR_now = vR + i*(dvR + i*(ddvR + i*(dddvR)));
		sR_now = sR + i*(dsR + i*(ddsR + i*(dddsR)));

		iCubicInterp(uL_now, uR_now, rL_now, rR_now, &u, &du, &ddu, &dddu, fdx);
		iCubicInterp(vL_now, vR_now, sL_now, sR_now, &v, &dv, &ddv, &dddv, fdx);
		
		__asm
		{
			xor ecx, ecx

			mov esi, tex
			mov edi, dest
			add edi, dest_pos

			LOOP1:

				mov eax, dddu
				imul ecx
				add eax, ddu
				imul ecx
				add eax, du
				imul ecx
				add eax, u
				mov ebx, eax

				mov eax, dddv
				imul ecx
				add eax, ddv
				imul ecx
				add eax, dv
				imul ecx
				add eax, v

				 inc   ecx
				shr eax, SHIFTFACTOR - 8
				shr ebx, SHIFTFACTOR
				and eax, 0xFF00
				and ebx, 0x00FF
				or  eax, ebx

				mov ebx, [esi+eax*4]	// RGB
				mov [edi], ebx

				mov   eax, ecx
				add   edi, 4
				
				sub   eax, _dx
				jnz   LOOP1
		}

		dest_pos += W*4;
		i++;

	}

}

*/



			/*
void BlitWarp(float u0, float v0, 
						  float u1, float v1, 
						  float u2, float v2, 
						  float u3, float v3, 
						  int x0, int y0, 
						  int x1, int y1, 
						  unsigned char *dest, int W, int H, 
						  unsigned char *tex, int texW, int texH)
			{
				if (!tex) return;
				if (!dest) return;

				//  (x0y0)
				//   u0v0 ------ u1v1
				//    |            |
				//    |            |
				//   u2v2 ------ u3v3 
				//				(x1y1)

				//unsigned __int16 texW16 = texW;
				//unsigned __int16 texH16 = texH;
				//int temp;

				unsigned int *dest32 = (unsigned int *)dest;
				unsigned int *tex32 = (unsigned int *)tex;

				int x, y;
				int u, du;
				int v, dv;
				int tex_offset;
				float u_start, u_end;
				float v_start, v_end;
				float u_min, v_min;
				int dest_pos;

				u0 *= texW;
				u1 *= texW;
				u2 *= texW;
				u3 *= texW;
				v0 *= texH;
				v1 *= texH;
				v2 *= texH;
				v3 *= texH;

				for (y=y0; y<y1; y++)
				{
					u_start = (u0*(y1-y) + u2*(y-y0))*INTFACTOR / (float)(y1-y0);
					u_end   = (u1*(y1-y) + u3*(y-y0))*INTFACTOR / (float)(y1-y0);
					v_start = (v0*(y1-y) + v2*(y-y0))*INTFACTOR / (float)(y1-y0);
					v_end   = (v1*(y1-y) + v3*(y-y0))*INTFACTOR / (float)(y1-y0);

					// temporary hack:
					//if (u_start < 0 || u_end < 0) u += texW*16*INTFACTOR;
					//if (v_start < 0 || v_end < 0) v += texH*16*INTFACTOR;
					u_min = (u_start < u_end) ? u_start : u_end;
					if (u_min < 0) 
					{
						int ufix = ((int)((-u_min / (texW*INTFACTOR)) + 1)) * (texW*INTFACTOR);
						u_start += ufix;
						u_end   += ufix;
					}

					v_min = (v_start < v_end) ? v_start : v_end;
					if (v_min < 0) 
					{
						int vfix = ((int)((-v_min / (texH*INTFACTOR)) + 1)) * (texH*INTFACTOR);
						v_start += vfix;
						v_end   += vfix;
					}

					du = (int)((u_end - u_start) / (x1-x0));
					dv = (int)((v_end - v_start) / (x1-x0));
					u = (int)u_start;
					v = (int)v_start;

					dest_pos = (y*W + x0);

					for (x=x0; x<x1; x++)
					{
						tex_offset = ((v >> SHIFTFACTOR) & 63)*texW + ((u >> SHIFTFACTOR) & 63);
						dest32[dest_pos++] = tex32[tex_offset];
						u += du;
						v += dv;
					}

					/ *
					__asm
					{
						mov ecx, x1
						sub ecx, x0
						
						mov esi, tex
						mov edi, dest
						add edi, dest_pos
						add edi, dest_pos
						add edi, dest_pos
						add edi, dest_pos

						LOOP1:
						
							// tex_offset = ...
							mov eax, v
							shr eax, SHIFTFACTOR
							 mov ebx, eax
							xor edx, edx
							div texH16
							mul texH16
							sub ebx, eax
							mov eax, ebx
							mul texW
							mov temp, eax

							mov eax, u
							shr eax, SHIFTFACTOR
							 mov ebx, eax
							xor edx, edx
							div texW16
							mul texW16
							sub ebx, eax
							mov eax, ebx
							add eax, temp

							mov edx, [esi+eax*4]	// RGB
							mov [edi], edx

							// u += du;
							mov   eax, u
							mov   ebx, du
							add   eax, ebx
							mov   u, eax

							// v += dv;
							mov   eax, v
							mov   ebx, dv
							add   eax, ebx
							mov   v, eax

							// dest_pos++
							add   edi, 4

							dec   ecx
							jnz   LOOP1


					}
					* /
					


				}






			}
*/


/*

			void BlitWarp256(float u0, float v0, 
						  float u1, float v1, 
						  float u2, float v2, 
						  float u3, float v3, 
						  int x0, int y0, 
						  int x1, int y1, 
						  unsigned char *dest, int W, int H, 
						  unsigned char *tex)
			{
				if (!tex) return;
				if (!dest) return;

				//  (x0y0)
				//   u0v0 ------ u1v1
				//    |            |
				//    |            |
				//   u2v2 ------ u3v3 
				//				(x1y1)

				//unsigned __int16 texW16 = texW;
				//unsigned __int16 texH16 = texH;
				//int temp;

				unsigned int *dest32 = (unsigned int *)dest;
				unsigned int *tex32 = (unsigned int *)tex;

				int dx = x1 - x0;
				int y;
				int dest_pos;
				float min;

				min = (u0 < u1) ? u0 : u1;
				min = (min < u2) ? min : u2;
				min = (min < u3) ? min : u3;
				if (min < 0) 
				{
					float ufix = 2.0f - (int)min;
					u0 += ufix;
					u1 += ufix;
					u2 += ufix;
					u3 += ufix;
				}

				min = (v0 < v1) ? v0 : v1;
				min = (min < v2) ? min : v2;
				min = (min < v3) ? min : v3;
				if (min < 0) 
				{
					float vfix = 2.0f - (int)min;
					v0 += vfix;
					v1 += vfix;
					v2 += vfix;
					v3 += vfix;
				}

				u0 *= 256*INTFACTOR;
				u1 *= 256*INTFACTOR;
				u2 *= 256*INTFACTOR;
				u3 *= 256*INTFACTOR;
				v0 *= 256*INTFACTOR;
				v1 *= 256*INTFACTOR;
				v2 *= 256*INTFACTOR;
				v3 *= 256*INTFACTOR;
				
				float duyL = (u2 - u0) / (y1 - y0);
				float duyR = (u3 - u1) / (y1 - y0);
				float dvyL = (v2 - v0) / (y1 - y0);
				float dvyR = (v3 - v1) / (y1 - y0);
				
				float uL = u0;
				float uR = u1;
				float vL = v0;
				float vR = v1;

				int du;
				int dv;
				int u_endi;
				int v_endi;

				dest_pos = (y0*W + x0);
				
				for (y=y0; y<y1; y++)
				{
					du = (int)(uL - uR) / dx;
					dv = (int)(vL - vR) / dx;
					u_endi = (int)uR;
					v_endi = (int)vR;
					
					/ *
					int u, du;
					int v, dv;
					int tex_offset;
					dest_pos = (y*W + x0);
					for (x=x0; x<x1; x++)
					{
						tex_offset = ((v >> SHIFTFACTOR) & mask)*256 + ((u >> SHIFTFACTOR) & mask);
						dest32[dest_pos++] = tex32[tex_offset];
						u += du;
						v += dv;
					}
					* /

					__asm
					{
						mov ecx, x1
						sub ecx, x0

						//mov edx, mask
						
						mov esi, tex
						mov edi, dest
						add edi, dest_pos
						add edi, dest_pos
						add edi, dest_pos
						add edi, dest_pos

						xor edx, edx

						LOOP1:
							
							mov eax, du
							mul ecx
							add eax, u_endi
							mov ebx, eax

							mov eax, dv
							mul ecx
							add eax, v_endi

							shr eax, SHIFTFACTOR - 8
							shr ebx, SHIFTFACTOR
							and eax, 0xFF00
							and ebx, 0x00FF
							or  eax, ebx

							mov ebx, [esi+eax*4]	// RGB
							mov [edi], ebx

							add   edi, 4

							dec   ecx
							jnz   LOOP1


					}

					uL += duyL;
					uR += duyR;
					vL += dvyL;
					vR += dvyR;

					dest_pos += W;

				}
			}


*/







