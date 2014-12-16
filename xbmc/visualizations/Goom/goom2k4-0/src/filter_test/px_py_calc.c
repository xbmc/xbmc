#include "mmx.h"

int testD [] = {0x1200, 0x2011, 0, 0x12, 0x5331, 0x8000};
int testS [] = {0x1205, 0x11, 0x4210, 0x412, 0x121, 0x1211};

int ratios [] = {0x8000, 0x4000, 0x1234, 0x6141, 0xffff, 0};

int main () {
	int nbERROR = 0;
	int i,j;
	volatile mmx_t ratiox;

	volatile mmx_t sortie;

	/* creation des variables de test */
	volatile int buffratio = 0x8000;
	volatile int loop = 0;
	volatile int *buffS;
	volatile int *buffD;
	buffS = malloc (256);
	buffD = malloc (256);

	buffS = buffS + (unsigned)buffS % 64;
	buffD = buffD + (unsigned)buffD % 64;

	for (j=0;j<6;j++)
	for (i=0;i<3;i++) {
		
		buffratio = ratios[j];

		buffS[0] = testS[i<<1];
		buffS[1] = testS[(i<<1)+1];
		
		buffD[0] = testD[i*2];
		buffD[1] = testD[i*2+1];

		/* code */
		
		ratiox.d[0] = buffratio;
		ratiox.d[1] = buffratio;
		movq_m2r (ratiox, mm6);
		pslld_i2r (16,mm6);
		
		/*     |0hhhhhhh|llllvvvv|
				 x |00000000|bbbbbbbb|
				    =================
				   |.bl.high|..bl.low|
				 + |..bh.low|00000000|
				    =================
				          result1
		*/
	
		/*
		 * pre : mm6 = [buffratio<<16|buffratio<<16]
		 */

		movq_m2r (buffS[loop],mm0);               /* mm0 = S */
		movq_m2r (buffD[loop],mm1);               /* mm1 = D */
		psubd_r2r (mm0,mm1);           /* mm1 = D - S */
		movq_r2r (mm1, mm2);           /* mm2 = D - S */
		
		pslld_i2r (16,mm1);
		mmx_r2r (pmulhuw, mm6, mm1);    /* mm1 = ?? */
		pmullw_r2r (mm6, mm2);
		
		paddd_r2r (mm2, mm1);     /* mm1 = (D - S) * buffratio >> 16 */
		pslld_i2r (16,mm0);
		
		paddd_r2r (mm1, mm0);     /* mm0 = S + mm1 */
		psrld_i2r (16, mm0);
		movq_r2m (mm0, sortie);
		
		/*
		 * post : mm0 = S + ((D-S)*buffratio)>>16
		 * modified = mm0,mm1,mm2
		 */

		if (
			(buffS[0] + (((buffD[0]-buffS[0]) * buffratio)>>16) != sortie.d[0])
			| (buffS[1] + (((buffD[1]-buffS[1]) * buffratio)>>16) != sortie.d[1]))
			{
				nbERROR++;
				printf ("\ns : (0x%08x,0x%08x)\n", buffS[0], buffS[1]);
				printf ("d : (0x%08x,0x%08x)\n", buffD[0], buffD[1]);		
				printf ("ratio : (0x%08x,0x%08x)\n", buffratio, buffratio);
				
				printf ("en mmx : (0x%08x,0x%08x)\n", sortie.d[0], sortie.d[1]);
				printf ("en c : (0x%08x,0x%08x)\n",
								buffS[0] + (((buffD[0]-buffS[0]) * buffratio)>>16),
								buffS[1] + (((buffD[1]-buffS[1]) * buffratio)>>16));
			}
	}
	printf ("%d errors\n",nbERROR);
}
