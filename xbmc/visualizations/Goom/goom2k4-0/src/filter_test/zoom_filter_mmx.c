
#define BUFFPOINTNB 16
#define BUFFPOINTMASK 0xffff
#define BUFFINCR 0xff

#define sqrtperte 16
// faire : a % sqrtperte <=> a & pertemask
#define PERTEMASK 0xf
// faire : a / sqrtperte <=> a >> PERTEDEC
#define PERTEDEC 4

//#define MMX_TRACE
#include "mmx.h"

void zoom_filter_mmx (int prevX, int prevY,
                      unsigned int *expix1, unsigned int *expix2,
                      int *lbruS, int *lbruD, int buffratio,
                      int precalCoef[16][16])
{
  int bufsize = prevX * prevY; /* taille du buffer */
  volatile int loop;                    /* variable de boucle */

	mmx_t *brutS = (mmx_t*)lbruS; /* buffer de transformation source */
	mmx_t *brutD = (mmx_t*)lbruD; /* buffer de transformation dest */

	int pos;

  volatile mmx_t prevXY;
	volatile mmx_t ratiox;
	volatile mmx_t interpix;

	volatile mmx_t mask;	        /* masque des nombres a virgules */
	mask.ud[0] = BUFFPOINTMASK;
	mask.ud[1] = BUFFPOINTMASK;

	prevXY.ud[0] = (prevX-1)<<PERTEDEC;
	prevXY.ud[1] = (prevY-1)<<PERTEDEC;

  pxor_r2r (mm7,mm7);

	ratiox.d[0] = buffratio;
	ratiox.d[1] = buffratio;
	movq_m2r (ratiox, mm6);
	pslld_i2r (16,mm6);
		
  for (loop=0; loop<bufsize; loop++)
		{
			/*
			 * pre : mm6 = [buffratio<<16|buffratio<<16]
			 * post : mm0 = S + ((D-S)*buffratio)>>16 format [X|Y]
			 * modified = mm0,mm1,mm2
			 */
			
			movq_m2r (brutS[loop],mm0);               /* mm0 = S */
			movq_m2r (brutD[loop],mm1);               /* mm1 = D */
			psubd_r2r (mm0,mm1);           /* mm1 = D - S */
			movq_r2r (mm1, mm2);           /* mm2 = D - S */
			
			pslld_i2r (16,mm1);
			mmx_r2r (pmulhuw, mm6, mm1);   /* mm1 = ?? */
			pmullw_r2r (mm6, mm2);
		
			paddd_r2r (mm2, mm1);     /* mm1 = (D - S) * buffratio >> 16 */
			pslld_i2r (16,mm0);
			
			paddd_r2r (mm1, mm0);     /* mm0 = S + mm1 */
			psrld_i2r (16, mm0);
			
			/*
			 * pre : mm0 : position vector on screen
			 *       prevXY : coordinate of the lower-right point on screen
			 * post : clipped mm0
			 * modified : mm0,mm1
			 */
			movq_m2r (prevXY,mm1);
			pcmpgtd_r2r (mm0, mm1); /* mm0 en X contient :
																        1111 si prevXY > px
																				0000 si prevXY <= px
																 (idem pour y) */
			pand_r2r (mm1, mm0);    /* on met a zero la partie qui deborde */
			

			/*
			 * pre : mm0 : clipped position on screen
			 * post : mm6 : coefs for this position
			 *        mm1 : X vector [0|X]
			 * modif : eax,ebx
			 */
			__asm__ __volatile__ (
				"movq %%mm0,%%mm1\n"
				"movd %%mm0,%%eax\n"
				"psrlq $32,%%mm1\n"
				"movd %%mm1,%%ebx\n"
				"and $15,%%eax\n"
				"and $15,%%ebx\n"
				"add %0,%%eax\n"
				"movd (%%eax,%%ebx,$16),%%mm6\n"
				::"X"(precalCoef):"eax","ebx");

			/*
			 * pre : mm0 : Y pos [*|Y]
			 *       mm1 : X pos [*|X]
			 * post : eax : absolute position of the source pixel.
			 * modif : ebx
			 */
			psrld_i2r (PERTEDEC,mm0);
			psrld_i2r (PERTEDEC,mm1);
			__asm__ __volatile__ (
				"movd %%mm1,%%eax\n"
//				"movl %1,%%ebx\n"
				"mull %1\n"
				"movd %%mm0,%%ebx\n"
				"addl %%ebx,%%eax\n"
				"movl %%eax,%0\n"
				:"=X"(pos):"X"(prevX):"eax","ebx"
				);
			 
			expix2[loop] = expix1[pos];
			/*
			 * coeffs = precalCoef [px & PERTEMASK][py & PERTEMASK]; 
			 * coef en modulo 15 *
			 * pos = ((px>>PERTEMASK) + prevX * (py>>PERTEMASK));
			 */
//			precal + eax + ebx * 16

//			movd_m2r (precalCoef[interpix.d[0]][interpix.d[1]],mm6);

			/* recuperation des deux premiers pixels dans mm0 et mm1 */
//			movq_m2r (/*expix1[pos]*/a, mm0);     /* b1-v1-r1-a1-b2-v2-r2-a2 */
//			movq_r2r (mm0, mm1);            /* b1-v1-r1-a1-b2-v2-r2-a2 */
				
			/* depackage du premier pixel */
//			punpcklbw_r2r (mm7, mm0);       /* 00-b2-00-v2-00-r2-00-a2 */
			
//			movq_r2r (mm6, mm5);            /* ??-??-??-??-c4-c3-c2-c1 */
			/* depackage du 2ieme pixel */
//			punpckhbw_r2r (mm7, mm1);       /* 00-b1-00-v1-00-r1-00-a1 */
			
			/* extraction des coefficients... */
//			punpcklbw_r2r (mm5, mm6);       /* c4-c4-c3-c3-c2-c2-c1-c1 */
//			movq_r2r (mm6, mm4);            /* c4-c4-c3-c3-c2-c2-c1-c1 */
//			movq_r2r (mm6, mm5);            /* c4-c4-c3-c3-c2-c2-c1-c1 */
				
//			punpcklbw_r2r (mm5, mm6);       /* c2-c2-c2-c2-c1-c1-c1-c1 */
//			punpckhbw_r2r (mm5, mm4);       /* c4-c4-c4-c4-c3-c3-c3-c3 */
				
//			movq_r2r (mm6, mm3);			/* c2-c2-c2-c2-c1-c1-c1-c1 */
//			punpcklbw_r2r (mm7, mm6);	/* 00-c1-00-c1-00-c1-00-c1 */
//			punpckhbw_r2r (mm7, mm3);	/* 00-c2-00-c2-00-c2-00-c2 */
			
			/* multiplication des pixels par les coefficients */
//			pmullw_r2r (mm6, mm0);		/* c1*b2-c1*v2-c1*r2-c1*a2 */
//			pmullw_r2r (mm3, mm1);		/* c2*b1-c2*v1-c2*r1-c2*a1 */
//			paddw_r2r (mm1, mm0);
				
			/* ...extraction des 2 derniers coefficients */
//			movq_r2r (mm4, mm5);			/* c4-c4-c4-c4-c3-c3-c3-c3 */
//			punpcklbw_r2r (mm7, mm4);	/* 00-c3-00-c3-00-c3-00-c3 */
//			punpckhbw_r2r (mm7, mm5);	/* 00-c4-00-c4-00-c4-00-c4 */
			
				/* recuperation des 2 derniers pixels */
//			movq_m2r (a/*expix1[pos+largeur]*/, mm1);
//			movq_r2r (mm1, mm2);
			
			/* depackage des pixels */
//			punpcklbw_r2r (mm7, mm1);
//			punpckhbw_r2r (mm7, mm2);
			
			/* multiplication pas les coeffs */
//			pmullw_r2r (mm4, mm1);
//			pmullw_r2r (mm5, mm2);
			   
			/* ajout des valeurs obtenues à la valeur finale */
//			paddw_r2r (mm1, mm0);
//			paddw_r2r (mm2, mm0);
			   
			/* division par 256 = 16+16+16+16, puis repackage du pixel final */
//			psrlw_i2r (8, mm0);
//			packuswb_r2r (mm7, mm0);

//			movd_r2m (mm0,expix2[loop]);
			
			//	  expix2[loop] = couleur;
		}
	emms(); /* __asm__ __volatile__ ("emms"); */
}
