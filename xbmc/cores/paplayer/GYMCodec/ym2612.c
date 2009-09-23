/***********************************************************
 *                                                         *
 * YM2612.C : YM2612 emulator                              *
 *                                                         *
 * Almost constantes are taken from the MAME core          *
 *                                                         *
 * This source is a part of Gens project                   *
 * Written by Stéphane Dallongeville (gens@consolemul.com) *
 * Copyright (c) 2002 by Stéphane Dallongeville            *
 *                                                         *
 ***********************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "ym2612.h"


/********************************************
 *            Partie définition             *
 ********************************************/

#define YM_DEBUG_LEVEL 0

#ifndef PI
#define PI 3.14159265358979323846
#endif

#define ATTACK    0
#define DECAY     1
#define SUBSTAIN  2
#define RELEASE   3

// SIN_LBITS <= 16
// LFO_HBITS <= 16
// (SIN_LBITS + SIN_HBITS) <= 26
// (ENV_LBITS + ENV_HBITS) <= 28
// (LFO_LBITS + LFO_HBITS) <= 28

#define SIN_HBITS      12								// Sinus phase counter int part
#define SIN_LBITS      (26 - SIN_HBITS)					// Sinus phase counter float part (best setting)

#if (SIN_LBITS > 16)
#define SIN_LBITS      16								// Can't be greater than 16 bits
#endif

#define ENV_HBITS      12								// Env phase counter int part
#define ENV_LBITS      (28 - ENV_HBITS)					// Env phase counter float part (best setting)

#define LFO_HBITS      10								// LFO phase counter int part
#define LFO_LBITS      (28 - LFO_HBITS)					// LFO phase counter float part (best setting)

#define SIN_LENGHT     (1 << SIN_HBITS)
#define ENV_LENGHT     (1 << ENV_HBITS)
#define LFO_LENGHT     (1 << LFO_HBITS)

#define TL_LENGHT      (ENV_LENGHT * 3)					// Env + TL scaling + LFO

#define SIN_MASK       (SIN_LENGHT - 1)
#define ENV_MASK       (ENV_LENGHT - 1)
#define LFO_MASK       (LFO_LENGHT - 1)

#define ENV_STEP       (96.0 / ENV_LENGHT)				// ENV_MAX = 96 dB

#define ENV_ATTACK     ((ENV_LENGHT * 0) << ENV_LBITS)
#define ENV_DECAY      ((ENV_LENGHT * 1) << ENV_LBITS)
#define ENV_END        ((ENV_LENGHT * 2) << ENV_LBITS)

#define MAX_OUT_BITS   (SIN_HBITS + SIN_LBITS + 2)		// Modulation = -4 <--> +4
#define MAX_OUT        ((1 << MAX_OUT_BITS) - 1)

//Just for tests stuff...
//
//#define COEF_MOD       0.5
//#define MAX_OUT        ((int) (((1 << MAX_OUT_BITS) - 1) * COEF_MOD))

#define OUT_BITS       (OUTPUT_BITS - 2)
#define OUT_SHIFT      (MAX_OUT_BITS - OUT_BITS)
#define LIMIT_CH_OUT   ((int) (((1 << OUT_BITS) * 1.5) - 1))

#define PG_CUT_OFF     ((int) (78.0 / ENV_STEP))
#define ENV_CUT_OFF    ((int) (68.0 / ENV_STEP))

#define AR_RATE        399128
#define DR_RATE        5514396

//#define AR_RATE        426136
//#define DR_RATE        (AR_RATE * 12)

#define LFO_FMS_LBITS  9	// FIXED (LFO_FMS_BASE gives somethink as 1)
#define LFO_FMS_BASE   ((int) (0.05946309436 * 0.0338 * (double) (1 << LFO_FMS_LBITS)))

#define S0             0	// Stupid typo of the YM2612
#define S1             2
#define S2             1
#define S3             3


/********************************************
 *            Partie variables              *
 ********************************************/


struct ym2612__ YM2612;

int *SIN_TAB[SIN_LENGHT];					// SINUS TABLE (pointer on TL TABLE)
int TL_TAB[TL_LENGHT * 2];					// TOTAL LEVEL TABLE (positif and minus)
unsigned int ENV_TAB[2 * ENV_LENGHT + 8];	// ENV CURVE TABLE (attack & decay)

//unsigned int ATTACK_TO_DECAY[ENV_LENGHT];	// Conversion from attack to decay phase
unsigned int DECAY_TO_ATTACK[ENV_LENGHT];	// Conversion from decay to attack phase

unsigned int FINC_TAB[2048];				// Frequency step table

unsigned int AR_TAB[128];					// Attack rate table
unsigned int DR_TAB[96];					// Decay rate table
unsigned int DT_TAB[8][32];					// Detune table
unsigned int SL_TAB[16];					// Substain level table
unsigned int NULL_RATE[32];					// Table for NULL rate

int LFO_ENV_TAB[LFO_LENGHT];				// LFO AMS TABLE (adjusted for 11.8 dB)
int LFO_FREQ_TAB[LFO_LENGHT];				// LFO FMS TABLE
int LFO_ENV_UP[MAX_UPDATE_LENGHT];			// Temporary calculated LFO AMS (adjusted for 11.8 dB)
int LFO_FREQ_UP[MAX_UPDATE_LENGHT];			// Temporary calculated LFO FMS

int INTER_TAB[MAX_UPDATE_LENGHT];			// Interpolation table

int LFO_INC_TAB[8];							// LFO step table

int in0, in1, in2, in3;						// current phase calculation
int en0, en1, en2, en3;						// current enveloppe calculation

const void (*UPDATE_CHAN[8 * 8])(channel_ *CH, int **buf, int lenght) =		// Update Channel functions pointer table
{
	Update_Chan_Algo0,
	Update_Chan_Algo1,
	Update_Chan_Algo2,
	Update_Chan_Algo3,
	Update_Chan_Algo4,
	Update_Chan_Algo5,
	Update_Chan_Algo6,
	Update_Chan_Algo7,

	Update_Chan_Algo0_LFO,
	Update_Chan_Algo1_LFO,
	Update_Chan_Algo2_LFO,
	Update_Chan_Algo3_LFO,
	Update_Chan_Algo4_LFO,
	Update_Chan_Algo5_LFO,
	Update_Chan_Algo6_LFO,
	Update_Chan_Algo7_LFO,

	Update_Chan_Algo0_Int,
	Update_Chan_Algo1_Int,
	Update_Chan_Algo2_Int,
	Update_Chan_Algo3_Int,
	Update_Chan_Algo4_Int,
	Update_Chan_Algo5_Int,
	Update_Chan_Algo6_Int,
	Update_Chan_Algo7_Int,

	Update_Chan_Algo0_LFO_Int,
	Update_Chan_Algo1_LFO_Int,
	Update_Chan_Algo2_LFO_Int,
	Update_Chan_Algo3_LFO_Int,
	Update_Chan_Algo4_LFO_Int,
	Update_Chan_Algo5_LFO_Int,
	Update_Chan_Algo6_LFO_Int,
	Update_Chan_Algo7_LFO_Int
};

const void (*ENV_NEXT_EVENT[8])(slot_ *SL) =		// Next Enveloppe phase functions pointer table
{
	Env_Attack_Next,
	Env_Decay_Next,
	Env_Substain_Next,
	Env_Release_Next,
	Env_NULL_Next,
	Env_NULL_Next,
	Env_NULL_Next,
	Env_NULL_Next
};

const unsigned int DT_DEF_TAB[4 * 32] =
{
// FD = 0
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

// FD = 1
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
  2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,

// FD = 2
  1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
  5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 16, 16, 16, 16,

// FD = 3
  2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
  8 , 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 20, 22, 22, 22, 22
};

const unsigned int FKEY_TAB[16] =
{ 
	0, 0, 0, 0,
	0, 0, 0, 1,
	2, 3, 3, 3,
	3, 3, 3, 3
};

const unsigned int LFO_AMS_TAB[4] =
{
	31, 4, 1, 0
};

const unsigned int LFO_FMS_TAB[8] =
{
	LFO_FMS_BASE * 0, LFO_FMS_BASE * 1,
	LFO_FMS_BASE * 2, LFO_FMS_BASE * 3,
	LFO_FMS_BASE * 4, LFO_FMS_BASE * 6,
	LFO_FMS_BASE * 12, LFO_FMS_BASE * 24
};

int int_cnt;								// Interpolation calculation


#if YM_DEBUG_LEVEL > 0						// Debug
FILE *debug_file = NULL;
#endif


/* Gens */

extern unsigned int Sound_Extrapol[312][2];
//extern int Seg_L[882], Seg_R[882];
extern int *Seg_L, *Seg_R;
extern int VDP_Current_Line;
extern int GYM_Dumping;
extern int YM2612_Enable;
extern int DAC_Enable;

int Update_GYM_Dump(char v0, char v1, char v2);

int YM2612_Enable;
int YM2612_Improv;
int DAC_Enable;
int *YM_Buf[2];
int YM_Len = 0;

int Chan_Enable[6];

/* end */


/***********************************************
 *           fonctions calcul param            *
 ***********************************************/


INLINE void CALC_FINC_SL(slot_ *SL, int finc, int kc)
{
	int ksr;
	
	SL->Finc = (finc + SL->DT[kc]) * SL->MUL;

	ksr = kc >> SL->KSR_S;	// keycode atténuation

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "FINC = %d  SL->Finc = %d\n", finc, SL->Finc);
#endif

	if (SL->KSR != ksr)			// si le KSR a changé alors
	{						// les différents taux pour l'enveloppe sont mis à jour
		SL->KSR = ksr;

		SL->EincA = SL->AR[ksr];
		SL->EincD = SL->DR[ksr];
		SL->EincS = SL->SR[ksr];
		SL->EincR = SL->RR[ksr];

		if (SL->Ecurp == ATTACK) SL->Einc = SL->EincA;
		else if (SL->Ecurp == DECAY) SL->Einc = SL->EincD;
		else if (SL->Ecnt < ENV_END)
		{
			if (SL->Ecurp == SUBSTAIN) SL->Einc = SL->EincS;
			else if (SL->Ecurp == RELEASE) SL->Einc = SL->EincR;
		}

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "KSR = %.4X  EincA = %.8X EincD = %.8X EincS = %.8X EincR = %.8X\n", ksr, SL->EincA, SL->EincD, SL->EincS, SL->EincR);
#endif
	}
}


INLINE void CALC_FINC_CH(channel_ *CH)
{
	int finc, kc;
	
	finc = FINC_TAB[CH->FNUM[0]] >> (7 - CH->FOCT[0]);
	kc = CH->KC[0];

	CALC_FINC_SL(&CH->SLOT[0], finc, kc);
	CALC_FINC_SL(&CH->SLOT[1], finc, kc);
	CALC_FINC_SL(&CH->SLOT[2], finc, kc);
	CALC_FINC_SL(&CH->SLOT[3], finc, kc);
}



/***********************************************
 *             fonctions setting               *
 ***********************************************/


INLINE void KEY_ON(channel_ *CH, int nsl)
{
	slot_ *SL = &(CH->SLOT[nsl]);	// on recupère le bon pointeur de slot
	
	if (SL->Ecurp == RELEASE)		// la touche est-elle relâchée ?
	{
		SL->Fcnt = 0;

		// Fix Ecco 2 splash sound
		
		SL->Ecnt = (DECAY_TO_ATTACK[ENV_TAB[SL->Ecnt >> ENV_LBITS]] + ENV_ATTACK) & SL->ChgEnM;
		SL->ChgEnM = 0xFFFFFFFF;

//		SL->Ecnt = DECAY_TO_ATTACK[ENV_TAB[SL->Ecnt >> ENV_LBITS]] + ENV_ATTACK;
//		SL->Ecnt = 0;

		SL->Einc = SL->EincA;
		SL->Ecmp = ENV_DECAY;
		SL->Ecurp = ATTACK;
	}
}


INLINE void KEY_OFF(channel_ *CH, int nsl)
{
	slot_ *SL = &(CH->SLOT[nsl]);	// on recupère le bon pointeur de slot
	
	if (SL->Ecurp != RELEASE)		// la touche est-elle appuyée ?
	{
		if (SL->Ecnt < ENV_DECAY)	// attack phase ?
		{
			SL->Ecnt = (ENV_TAB[SL->Ecnt >> ENV_LBITS] << ENV_LBITS) + ENV_DECAY;
		}

		SL->Einc = SL->EincR;
		SL->Ecmp = ENV_END;
		SL->Ecurp = RELEASE;
	}
}


INLINE void CSM_Key_Control()
{
	KEY_ON(&YM2612.CHANNEL[2], 0);
	KEY_ON(&YM2612.CHANNEL[2], 1);
	KEY_ON(&YM2612.CHANNEL[2], 2);
	KEY_ON(&YM2612.CHANNEL[2], 3);
}


int SLOT_SET(int Adr, unsigned char data)
{
	channel_ *CH;
	slot_ *SL;
	int nch, nsl;

	if ((nch = Adr & 3) == 3) return 1;
	nsl = (Adr >> 2) & 3;

	if (Adr & 0x100) nch += 3;

	CH = &(YM2612.CHANNEL[nch]);
	SL = &(CH->SLOT[nsl]);

	switch(Adr & 0xF0)
	{
		case 0x30:
			if ((SL->MUL = (data & 0x0F))) SL->MUL <<= 1;
			else SL->MUL = 1;

			SL->DT = DT_TAB[(data >> 4) & 7];

			CH->SLOT[0].Finc = -1;

#if YM_DEBUG_LEVEL > 1
			fprintf(debug_file, "CHANNEL[%d], SLOT[%d] DTMUL = %.2X\n", nch, nsl, data & 0x7F);
#endif
			break;

		case 0x40:
			SL->TL = data & 0x7F;

			// SOR2 do a lot of TL adjustement and this fix R.Shinobi jump sound...
			YM2612_Special_Update();

#if ((ENV_HBITS - 7) < 0)
			SL->TLL = SL->TL >> (7 - ENV_HBITS);
#else
			SL->TLL = SL->TL << (ENV_HBITS - 7);
#endif

#if YM_DEBUG_LEVEL > 1
			fprintf(debug_file, "CHANNEL[%d], SLOT[%d] TL = %.2X\n", nch, nsl, SL->TL);
#endif
			break;

		case 0x50:
			SL->KSR_S = 3 - (data >> 6);

			CH->SLOT[0].Finc = -1;

			if (data &= 0x1F) SL->AR = &AR_TAB[data << 1];
			else SL->AR = &NULL_RATE[0];

			SL->EincA = SL->AR[SL->KSR];
			if (SL->Ecurp == ATTACK) SL->Einc = SL->EincA;

#if YM_DEBUG_LEVEL > 1
			fprintf(debug_file, "CHANNEL[%d], SLOT[%d] AR = %.2X  EincA = %.6X\n", nch, nsl, data, SL->EincA);
#endif
			break;

		case 0x60:
			if ((SL->AMSon = (data & 0x80))) SL->AMS = CH->AMS;
			else SL->AMS = 31;

			if (data &= 0x1F) SL->DR = &DR_TAB[data << 1];
			else SL->DR = &NULL_RATE[0];

			SL->EincD = SL->DR[SL->KSR];
			if (SL->Ecurp == DECAY) SL->Einc = SL->EincD;

#if YM_DEBUG_LEVEL > 1
			fprintf(debug_file, "CHANNEL[%d], SLOT[%d] AMS = %d  DR = %.2X  EincD = %.6X\n", nch, nsl, SL->AMSon, data, SL->EincD);
#endif
			break;

		case 0x70:
			if (data &= 0x1F) SL->SR = &DR_TAB[data << 1];
			else SL->SR = &NULL_RATE[0];

			SL->EincS = SL->SR[SL->KSR];
			if ((SL->Ecurp == SUBSTAIN) && (SL->Ecnt < ENV_END)) SL->Einc = SL->EincS;

#if YM_DEBUG_LEVEL > 1
			fprintf(debug_file, "CHANNEL[%d], SLOT[%d] SR = %.2X  EincS = %.6X\n", nch, nsl, data, SL->EincS);
#endif
			break;

		case 0x80:
			SL->SLL = SL_TAB[data >> 4];

			SL->RR = &DR_TAB[((data & 0xF) << 2) + 2];

			SL->EincR = SL->RR[SL->KSR];
			if ((SL->Ecurp == RELEASE) && (SL->Ecnt < ENV_END)) SL->Einc = SL->EincR;

#if YM_DEBUG_LEVEL > 1
			fprintf(debug_file, "CHANNEL[%d], SLOT[%d] SL = %.8X\n", nch, nsl, SL->SLL);
			fprintf(debug_file, "CHANNEL[%d], SLOT[%d] RR = %.2X  EincR = %.2X\n", nch, nsl, ((data & 0xF) << 1) | 2, SL->EincR);
#endif
			break;

		case 0x90:
			// SSG-EG envelope shapes :
			/*
			   E  At Al H
			  
			   1  0  0  0  \\\\
			  
			   1  0  0  1  \___
			  
			   1  0  1  0  \/\/
			                ___
			   1  0  1  1  \
			  
			   1  1  0  0  ////
			                ___
			   1  1  0  1  /
			  
			   1  1  1  0  /\/\
			  
			   1  1  1  1  /___
			  
			   E  = SSG-EG enable
			   At = Start negate
			   Al = Altern
			   H  = Hold */

			if (data & 0x08) SL->SEG = data & 0x0F;
			else SL->SEG = 0;

#if YM_DEBUG_LEVEL > 1
			fprintf(debug_file, "CHANNEL[%d], SLOT[%d] SSG-EG = %.2X\n", nch, nsl, data);
#endif
			break;
	}

	return 0;
}


int CHANNEL_SET(int Adr, unsigned char data)
{
	channel_ *CH;
	int num;
	
	if ((num = Adr & 3) == 3) return 1;
		
	switch(Adr & 0xFC)
	{
		case 0xA0:
			if (Adr & 0x100) num += 3;
			CH = &(YM2612.CHANNEL[num]);

			YM2612_Special_Update();

			CH->FNUM[0] = (CH->FNUM[0] & 0x700) + data;
			CH->KC[0] = (CH->FOCT[0] << 2) | FKEY_TAB[CH->FNUM[0] >> 7];

			CH->SLOT[0].Finc = -1;

#if YM_DEBUG_LEVEL > 1
			fprintf(debug_file, "CHANNEL[%d] part1 FNUM = %d  KC = %d\n", num, CH->FNUM[0], CH->KC[0]);
#endif
			break;

		case 0xA4:
			if (Adr & 0x100) num += 3;
			CH = &(YM2612.CHANNEL[num]);

			YM2612_Special_Update();

			CH->FNUM[0] = (CH->FNUM[0] & 0x0FF) + ((int) (data & 0x07) << 8);
			CH->FOCT[0] = (data & 0x38) >> 3;
			CH->KC[0] = (CH->FOCT[0] << 2) | FKEY_TAB[CH->FNUM[0] >> 7];

			CH->SLOT[0].Finc = -1;

#if YM_DEBUG_LEVEL > 1
			fprintf(debug_file, "CHANNEL[%d] part2 FNUM = %d  FOCT = %d  KC = %d\n", num, CH->FNUM[0], CH->FOCT[0], CH->KC[0]);
#endif
			break;

		case 0xA8:
			if (Adr < 0x100)
			{
				num++;

				YM2612_Special_Update();

				YM2612.CHANNEL[2].FNUM[num] = (YM2612.CHANNEL[2].FNUM[num] & 0x700) + data;
				YM2612.CHANNEL[2].KC[num] = (YM2612.CHANNEL[2].FOCT[num] << 2) | FKEY_TAB[YM2612.CHANNEL[2].FNUM[num] >> 7];

				YM2612.CHANNEL[2].SLOT[0].Finc = -1;

#if YM_DEBUG_LEVEL > 1
				fprintf(debug_file, "CHANNEL[2] part1 FNUM[%d] = %d  KC[%d] = %d\n", num, YM2612.CHANNEL[2].FNUM[num], num, YM2612.CHANNEL[2].KC[num]);
#endif
			}
			break;

		case 0xAC:
			if (Adr < 0x100)
			{
				num++;

				YM2612_Special_Update();

				YM2612.CHANNEL[2].FNUM[num] = (YM2612.CHANNEL[2].FNUM[num] & 0x0FF) + ((int) (data & 0x07) << 8);
				YM2612.CHANNEL[2].FOCT[num] = (data & 0x38) >> 3;
				YM2612.CHANNEL[2].KC[num] = (YM2612.CHANNEL[2].FOCT[num] << 2) | FKEY_TAB[YM2612.CHANNEL[2].FNUM[num] >> 7];

				YM2612.CHANNEL[2].SLOT[0].Finc = -1;

#if YM_DEBUG_LEVEL > 1
				fprintf(debug_file, "CHANNEL[2] part2 FNUM[%d] = %d  FOCT[%d] = %d  KC[%d] = %d\n", num, YM2612.CHANNEL[2].FNUM[num], num, YM2612.CHANNEL[2].FOCT[num], num, YM2612.CHANNEL[2].KC[num]);
#endif
			}
			break;

		case 0xB0:
			if (Adr & 0x100) num += 3;
			CH = &(YM2612.CHANNEL[num]);

			if (CH->ALGO != (data & 7))
			{
				// Fix VectorMan 2 heli sound (level 1)
				YM2612_Special_Update();

				CH->ALGO = data & 7;
				
				CH->SLOT[0].ChgEnM = 0;
				CH->SLOT[1].ChgEnM = 0;
				CH->SLOT[2].ChgEnM = 0;
				CH->SLOT[3].ChgEnM = 0;
			}

			CH->FB = 9 - ((data >> 3) & 7);								// Real thing ?

//			if (CH->FB = ((data >> 3) & 7)) CH->FB = 9 - CH->FB;		// Thunder force 4 (music stage 8), Gynoug, Aladdin bug sound...
//			else CH->FB = 31;

#if YM_DEBUG_LEVEL > 1
			fprintf(debug_file, "CHANNEL[%d] ALGO = %d  FB = %d\n", num, CH->ALGO, CH->FB);
#endif
			break;

		case 0xB4:
			if (Adr & 0x100) num += 3;
			CH = &(YM2612.CHANNEL[num]);

			YM2612_Special_Update();

			if (data & 0x80) CH->LEFT = 0xFFFFFFFF;
			else CH->LEFT = 0;
			
			if (data & 0x40) CH->RIGHT = 0xFFFFFFFF;
			else CH->RIGHT = 0;
				
			CH->AMS = LFO_AMS_TAB[(data >> 4) & 3];
			CH->FMS = LFO_FMS_TAB[data & 7];

			if (CH->SLOT[0].AMSon) CH->SLOT[0].AMS = CH->AMS;
			else CH->SLOT[0].AMS = 31;
			if (CH->SLOT[1].AMSon) CH->SLOT[1].AMS = CH->AMS;
			else CH->SLOT[1].AMS = 31;
			if (CH->SLOT[2].AMSon) CH->SLOT[2].AMS = CH->AMS;
			else CH->SLOT[2].AMS = 31;
			if (CH->SLOT[3].AMSon) CH->SLOT[3].AMS = CH->AMS;
			else CH->SLOT[3].AMS = 31;

#if YM_DEBUG_LEVEL > 0
			fprintf(debug_file, "CHANNEL[%d] AMS = %d  FMS = %d\n", num, CH->AMS, CH->FMS);
#endif
			break;
	}
	
	return 0;
}


int YM_SET(int Adr, unsigned char data)
{
	channel_ *CH;
	int nch;

	switch(Adr)
	{
		case 0x22:
			if (data & 8)
			{
				// Cool Spot music 1, LFO modified severals time which
				// distord the sound, have to check that on a real genesis...

				YM2612.LFOinc = LFO_INC_TAB[data & 7];

#if YM_DEBUG_LEVEL > 0
				fprintf(debug_file, "\nLFO Enable, LFOinc = %.8X   %d\n", YM2612.LFOinc, data & 7);
#endif
			}
			else
			{
				YM2612.LFOinc = YM2612.LFOcnt = 0;

#if YM_DEBUG_LEVEL > 0
				fprintf(debug_file, "\nLFO Disable\n");
#endif
			}
			break;

		case 0x24:
			YM2612.TimerA = (YM2612.TimerA & 0x003) | (((int) data) << 2);

			if (YM2612.TimerAL != (1024 - YM2612.TimerA) << 12)
			{
				YM2612.TimerAcnt = YM2612.TimerAL = (1024 - YM2612.TimerA) << 12;

#if YM_DEBUG_LEVEL > 1
				fprintf(debug_file, "Timer A Set = %.8X\n", YM2612.TimerAcnt);
#endif
			}
			break;

		case 0x25:
			YM2612.TimerA = (YM2612.TimerA & 0x3fc) | (data & 3);

			if (YM2612.TimerAL != (1024 - YM2612.TimerA) << 12)
			{
				YM2612.TimerAcnt = YM2612.TimerAL = (1024 - YM2612.TimerA) << 12;

#if YM_DEBUG_LEVEL > 1
				fprintf(debug_file, "Timer A Set = %.8X\n", YM2612.TimerAcnt);
#endif
			}
			break;

		case 0x26:
			YM2612.TimerB = data;

			if (YM2612.TimerBL != (256 - YM2612.TimerB) << (4 + 12))
			{
				YM2612.TimerBcnt = YM2612.TimerBL = (256 - YM2612.TimerB) << (4 + 12);

#if YM_DEBUG_LEVEL > 1
				fprintf(debug_file, "Timer B Set = %.8X\n", YM2612.TimerBcnt);
#endif
			}
			break;

		case 0x27:
			// Paramètre divers
			// b7 = CSM MODE
			// b6 = 3 slot mode
			// b5 = reset b
			// b4 = reset a
			// b3 = timer enable b
			// b2 = timer enable a
			// b1 = load b
			// b0 = load a

			if ((data ^ YM2612.Mode) & 0x40)
			{
				// We changed the channel 2 mode, so recalculate phase step
				// This fix the punch sound in Street of Rage 2

				YM2612_Special_Update();

				YM2612.CHANNEL[2].SLOT[0].Finc = -1;		// recalculate phase step
			}

//			if ((data & 2) && (YM2612.Status & 2)) YM2612.TimerBcnt = YM2612.TimerBL;
//			if ((data & 1) && (YM2612.Status & 1)) YM2612.TimerAcnt = YM2612.TimerAL;

//			YM2612.Status &= (~data >> 4);					// Reset du Status au cas ou c'est demandé
			YM2612.Status &= (~data >> 4) & (data >> 2);	// Reset Status

			YM2612.Mode = data;

#if YM_DEBUG_LEVEL > 0
			fprintf(debug_file, "Mode reg = %.2X\n", data);
#endif
			break;

		case 0x28:
			if ((nch = data & 3) == 3) return 1;

			if (data & 4) nch += 3;
			CH = &(YM2612.CHANNEL[nch]);

			YM2612_Special_Update();

			if (data & 0x10) KEY_ON(CH, S0);	// On appuie sur la touche pour le slot 1
			else KEY_OFF(CH, S0);				// On relâche la touche pour le slot 1
			if (data & 0x20) KEY_ON(CH, S1);	// On appuie sur la touche pour le slot 3
			else KEY_OFF(CH, S1);				// On relâche la touche pour le slot 3
			if (data & 0x40) KEY_ON(CH, S2);	// On appuie sur la touche pour le slot 2
			else KEY_OFF(CH, S2);				// On relâche la touche pour le slot 2
			if (data & 0x80) KEY_ON(CH, S3);	// On appuie sur la touche pour le slot 4
			else KEY_OFF(CH, S3);				// On relâche la touche pour le slot 4

#if YM_DEBUG_LEVEL > 0
			fprintf(debug_file, "CHANNEL[%d]  KEY %.1X\n", nch, ((data & 0xf0) >> 4));
#endif
			break;

		case 0x2A:
			YM2612.DACdata = ((int) data - 0x80) << 7;	// donnée du DAC
			break;

		case 0x2B:
			if (YM2612.DAC ^ (data & 0x80)) YM2612_Special_Update();

			YM2612.DAC = data & 0x80;	// activation/désactivation du DAC
			break;
	}
	
	return 0;
}



/***********************************************
 *          fonctions de génération            *
 ***********************************************/


void Env_NULL_Next(slot_ *SL)
{
}


void Env_Attack_Next(slot_ *SL)
{
	// Verified with Gynoug even in HQ (explode SFX)
	SL->Ecnt = ENV_DECAY;

	SL->Einc = SL->EincD;
	SL->Ecmp = SL->SLL;
	SL->Ecurp = DECAY;
}


void Env_Decay_Next(slot_ *SL)
{
	// Verified with Gynoug even in HQ (explode SFX)
	SL->Ecnt = SL->SLL;

	SL->Einc = SL->EincS;
	SL->Ecmp = ENV_END;
	SL->Ecurp = SUBSTAIN;
}


void Env_Substain_Next(slot_ *SL)
{
	if (SL->SEG & 8)	// SSG envelope type
	{
		if (SL->SEG & 1)
		{
			SL->Ecnt = ENV_END;
			SL->Einc = 0;
			SL->Ecmp = ENV_END + 1;
		}
		else
		{
			// re KEY ON

			// SL->Fcnt = 0;
			// SL->ChgEnM = 0xFFFFFFFF;

			SL->Ecnt = 0;
			SL->Einc = SL->EincA;
			SL->Ecmp = ENV_DECAY;
			SL->Ecurp = ATTACK;
		}

		SL->SEG ^= (SL->SEG & 2) << 1;
	}
	else
	{
		SL->Ecnt = ENV_END;
		SL->Einc = 0;
		SL->Ecmp = ENV_END + 1;
	}
}


void Env_Release_Next(slot_ *SL)
{
	SL->Ecnt = ENV_END;
	SL->Einc = 0;
	SL->Ecmp = ENV_END + 1;
}


#define GET_CURRENT_PHASE	\
in0 = CH->SLOT[S0].Fcnt;	\
in1 = CH->SLOT[S1].Fcnt;	\
in2 = CH->SLOT[S2].Fcnt;	\
in3 = CH->SLOT[S3].Fcnt;


#define UPDATE_PHASE					\
CH->SLOT[S0].Fcnt += CH->SLOT[S0].Finc;	\
CH->SLOT[S1].Fcnt += CH->SLOT[S1].Finc;	\
CH->SLOT[S2].Fcnt += CH->SLOT[S2].Finc;	\
CH->SLOT[S3].Fcnt += CH->SLOT[S3].Finc;


#define UPDATE_PHASE_LFO																		\
if ((freq_LFO = (CH->FMS * LFO_FREQ_UP[i]) >> (LFO_HBITS - 1)))									\
{																								\
	CH->SLOT[S0].Fcnt += CH->SLOT[S0].Finc + ((CH->SLOT[S0].Finc * freq_LFO) >> LFO_FMS_LBITS);	\
	CH->SLOT[S1].Fcnt += CH->SLOT[S1].Finc + ((CH->SLOT[S1].Finc * freq_LFO) >> LFO_FMS_LBITS);	\
	CH->SLOT[S2].Fcnt += CH->SLOT[S2].Finc + ((CH->SLOT[S2].Finc * freq_LFO) >> LFO_FMS_LBITS);	\
	CH->SLOT[S3].Fcnt += CH->SLOT[S3].Finc + ((CH->SLOT[S3].Finc * freq_LFO) >> LFO_FMS_LBITS);	\
}																								\
else																							\
{																								\
	CH->SLOT[S0].Fcnt += CH->SLOT[S0].Finc;														\
	CH->SLOT[S1].Fcnt += CH->SLOT[S1].Finc;														\
	CH->SLOT[S2].Fcnt += CH->SLOT[S2].Finc;														\
	CH->SLOT[S3].Fcnt += CH->SLOT[S3].Finc;														\
}


#define GET_CURRENT_ENV																				\
if (CH->SLOT[S0].SEG & 4)																			\
{																									\
	if ((en0 = ENV_TAB[(CH->SLOT[S0].Ecnt >> ENV_LBITS)] + CH->SLOT[S0].TLL) > ENV_MASK) en0 = 0;	\
	else en0 ^= ENV_MASK;																			\
}																									\
else en0 = ENV_TAB[(CH->SLOT[S0].Ecnt >> ENV_LBITS)] + CH->SLOT[S0].TLL;							\
if (CH->SLOT[S1].SEG & 4)																			\
{																									\
	if ((en1 = ENV_TAB[(CH->SLOT[S1].Ecnt >> ENV_LBITS)] + CH->SLOT[S1].TLL) > ENV_MASK) en1 = 0;	\
	else en1 ^= ENV_MASK;																			\
}																									\
else en1 = ENV_TAB[(CH->SLOT[S1].Ecnt >> ENV_LBITS)] + CH->SLOT[S1].TLL;							\
if (CH->SLOT[S2].SEG & 4)																			\
{																									\
	if ((en2 = ENV_TAB[(CH->SLOT[S2].Ecnt >> ENV_LBITS)] + CH->SLOT[S2].TLL) > ENV_MASK) en2 = 0;	\
	else en2 ^= ENV_MASK;																			\
}																									\
else en2 = ENV_TAB[(CH->SLOT[S2].Ecnt >> ENV_LBITS)] + CH->SLOT[S2].TLL;							\
if (CH->SLOT[S3].SEG & 4)																			\
{																									\
	if ((en3 = ENV_TAB[(CH->SLOT[S3].Ecnt >> ENV_LBITS)] + CH->SLOT[S3].TLL) > ENV_MASK) en3 = 0;	\
	else en3 ^= ENV_MASK;																			\
}																									\
else en3 = ENV_TAB[(CH->SLOT[S3].Ecnt >> ENV_LBITS)] + CH->SLOT[S3].TLL;


#define GET_CURRENT_ENV_LFO																					\
env_LFO = LFO_ENV_UP[i];																					\
																											\
if (CH->SLOT[S0].SEG & 4)																					\
{																											\
	if ((en0 = ENV_TAB[(CH->SLOT[S0].Ecnt >> ENV_LBITS)] + CH->SLOT[S0].TLL) > ENV_MASK) en0 = 0;			\
	else en0 = (en0 ^ ENV_MASK) + (env_LFO >> CH->SLOT[S0].AMS);											\
}																											\
else en0 = ENV_TAB[(CH->SLOT[S0].Ecnt >> ENV_LBITS)] + CH->SLOT[S0].TLL + (env_LFO >> CH->SLOT[S0].AMS);	\
if (CH->SLOT[S1].SEG & 4)																					\
{																											\
	if ((en1 = ENV_TAB[(CH->SLOT[S1].Ecnt >> ENV_LBITS)] + CH->SLOT[S1].TLL) > ENV_MASK) en1 = 0;			\
	else en1 = (en1 ^ ENV_MASK) + (env_LFO >> CH->SLOT[S1].AMS);											\
}																											\
else en1 = ENV_TAB[(CH->SLOT[S1].Ecnt >> ENV_LBITS)] + CH->SLOT[S1].TLL + (env_LFO >> CH->SLOT[S1].AMS);	\
if (CH->SLOT[S2].SEG & 4)																					\
{																											\
	if ((en2 = ENV_TAB[(CH->SLOT[S2].Ecnt >> ENV_LBITS)] + CH->SLOT[S2].TLL) > ENV_MASK) en2 = 0;			\
	else en2 = (en2 ^ ENV_MASK) + (env_LFO >> CH->SLOT[S2].AMS);											\
}																											\
else en2 = ENV_TAB[(CH->SLOT[S2].Ecnt >> ENV_LBITS)] + CH->SLOT[S2].TLL + (env_LFO >> CH->SLOT[S2].AMS);	\
if (CH->SLOT[S3].SEG & 4)																					\
{																											\
	if ((en3 = ENV_TAB[(CH->SLOT[S3].Ecnt >> ENV_LBITS)] + CH->SLOT[S3].TLL) > ENV_MASK) en3 = 0;			\
	else en3 = (en3 ^ ENV_MASK) + (env_LFO >> CH->SLOT[S3].AMS);											\
}																											\
else en3 = ENV_TAB[(CH->SLOT[S3].Ecnt >> ENV_LBITS)] + CH->SLOT[S3].TLL + (env_LFO >> CH->SLOT[S3].AMS);


#define UPDATE_ENV														\
if ((CH->SLOT[S0].Ecnt += CH->SLOT[S0].Einc) >= CH->SLOT[S0].Ecmp)		\
	ENV_NEXT_EVENT[CH->SLOT[S0].Ecurp](&(CH->SLOT[S0]));				\
if ((CH->SLOT[S1].Ecnt += CH->SLOT[S1].Einc) >= CH->SLOT[S1].Ecmp)		\
	ENV_NEXT_EVENT[CH->SLOT[S1].Ecurp](&(CH->SLOT[S1]));				\
if ((CH->SLOT[S2].Ecnt += CH->SLOT[S2].Einc) >= CH->SLOT[S2].Ecmp)		\
	ENV_NEXT_EVENT[CH->SLOT[S2].Ecurp](&(CH->SLOT[S2]));				\
if ((CH->SLOT[S3].Ecnt += CH->SLOT[S3].Einc) >= CH->SLOT[S3].Ecmp)		\
	ENV_NEXT_EVENT[CH->SLOT[S3].Ecurp](&(CH->SLOT[S3]));


#define DO_LIMIT												\
if (CH->OUTd > LIMIT_CH_OUT) CH->OUTd = LIMIT_CH_OUT;			\
else if (CH->OUTd < -LIMIT_CH_OUT) CH->OUTd = -LIMIT_CH_OUT;


#define DO_FEEDBACK0											\
in0 += CH->S0_OUT[0] >> CH->FB;									\
CH->S0_OUT[0] = SIN_TAB[(in0 >> SIN_LBITS) & SIN_MASK][en0];


#define DO_FEEDBACK												\
in0 += (CH->S0_OUT[0] + CH->S0_OUT[1]) >> CH->FB;				\
CH->S0_OUT[1] = CH->S0_OUT[0];									\
CH->S0_OUT[0] = SIN_TAB[(in0 >> SIN_LBITS) & SIN_MASK][en0];


#define DO_FEEDBACK2														\
in0 += (CH->S0_OUT[0] + (CH->S0_OUT[0] >> 2) + CH->S0_OUT[1]) >> CH->FB;	\
CH->S0_OUT[1] = CH->S0_OUT[0] >> 2;											\
CH->S0_OUT[0] = SIN_TAB[(in0 >> SIN_LBITS) & SIN_MASK][en0];
		

#define DO_FEEDBACK3																	\
in0 += (CH->S0_OUT[0] + CH->S0_OUT[1] + CH->S0_OUT[2] + CH->S0_OUT[3]) >> CH->FB;		\
CH->S0_OUT[3] = CH->S0_OUT[2] >> 1;														\
CH->S0_OUT[2] = CH->S0_OUT[1] >> 1;														\
CH->S0_OUT[1] = CH->S0_OUT[0] >> 1;														\
CH->S0_OUT[0] = SIN_TAB[(in0 >> SIN_LBITS) & SIN_MASK][en0];


#define DO_ALGO_0															\
DO_FEEDBACK																	\
in1 += CH->S0_OUT[1];														\
in2 += SIN_TAB[(in1 >> SIN_LBITS) & SIN_MASK][en1];							\
in3 += SIN_TAB[(in2 >> SIN_LBITS) & SIN_MASK][en2];							\
CH->OUTd = (SIN_TAB[(in3 >> SIN_LBITS) & SIN_MASK][en3]) >> OUT_SHIFT;

#define DO_ALGO_1															\
DO_FEEDBACK																	\
in2 += CH->S0_OUT[1] + SIN_TAB[(in1 >> SIN_LBITS) & SIN_MASK][en1];			\
in3 += SIN_TAB[(in2 >> SIN_LBITS) & SIN_MASK][en2];							\
CH->OUTd = (SIN_TAB[(in3 >> SIN_LBITS) & SIN_MASK][en3]) >> OUT_SHIFT;

#define DO_ALGO_2															\
DO_FEEDBACK																	\
in2 += SIN_TAB[(in1 >> SIN_LBITS) & SIN_MASK][en1];							\
in3 += CH->S0_OUT[1] + SIN_TAB[(in2 >> SIN_LBITS) & SIN_MASK][en2];			\
CH->OUTd = (SIN_TAB[(in3 >> SIN_LBITS) & SIN_MASK][en3]) >> OUT_SHIFT;

#define DO_ALGO_3															\
DO_FEEDBACK																	\
in1 += CH->S0_OUT[1];														\
in3 += SIN_TAB[(in1 >> SIN_LBITS) & SIN_MASK][en1] + SIN_TAB[(in2 >> SIN_LBITS) & SIN_MASK][en2];	\
CH->OUTd = (SIN_TAB[(in3 >> SIN_LBITS) & SIN_MASK][en3]) >> OUT_SHIFT;

#define DO_ALGO_4															\
DO_FEEDBACK																	\
in1 += CH->S0_OUT[1];														\
in3 += SIN_TAB[(in2 >> SIN_LBITS) & SIN_MASK][en2];							\
CH->OUTd = ((int) SIN_TAB[(in3 >> SIN_LBITS) & SIN_MASK][en3] + (int) SIN_TAB[(in1 >> SIN_LBITS) & SIN_MASK][en1]) >> OUT_SHIFT;	\
DO_LIMIT

#define DO_ALGO_5															\
DO_FEEDBACK																	\
in1 += CH->S0_OUT[1];														\
in2 += CH->S0_OUT[1];														\
in3 += CH->S0_OUT[1];														\
CH->OUTd = ((int) SIN_TAB[(in3 >> SIN_LBITS) & SIN_MASK][en3] + (int) SIN_TAB[(in1 >> SIN_LBITS) & SIN_MASK][en1] + (int) SIN_TAB[(in2 >> SIN_LBITS) & SIN_MASK][en2]) >> OUT_SHIFT;	\
DO_LIMIT

#define DO_ALGO_6															\
DO_FEEDBACK																	\
in1 += CH->S0_OUT[1];														\
CH->OUTd = ((int) SIN_TAB[(in3 >> SIN_LBITS) & SIN_MASK][en3] + (int) SIN_TAB[(in1 >> SIN_LBITS) & SIN_MASK][en1] + (int) SIN_TAB[(in2 >> SIN_LBITS) & SIN_MASK][en2]) >> OUT_SHIFT;	\
DO_LIMIT

#define DO_ALGO_7															\
DO_FEEDBACK																	\
CH->OUTd = ((int) SIN_TAB[(in3 >> SIN_LBITS) & SIN_MASK][en3] + (int) SIN_TAB[(in1 >> SIN_LBITS) & SIN_MASK][en1] + (int) SIN_TAB[(in2 >> SIN_LBITS) & SIN_MASK][en2] + CH->S0_OUT[1]) >> OUT_SHIFT;	\
DO_LIMIT


#define DO_OUTPUT						\
buf[0][i] += CH->OUTd & CH->LEFT;		\
buf[1][i] += CH->OUTd & CH->RIGHT;


#define DO_OUTPUT_INT0							\
if ((int_cnt += YM2612.Inter_Step) & 0x04000)	\
{												\
	int_cnt &= 0x3FFF;							\
	buf[0][i] += CH->OUTd & CH->LEFT;			\
	buf[1][i] += CH->OUTd & CH->RIGHT;			\
}												\
else i--;


#define DO_OUTPUT_INT1							\
CH->Old_OUTd = (CH->OUTd + CH->Old_OUTd) >> 1;	\
if ((int_cnt += YM2612.Inter_Step) & 0x04000)	\
{												\
	int_cnt &= 0x3FFF;							\
	buf[0][i] += CH->Old_OUTd & CH->LEFT;		\
	buf[1][i] += CH->Old_OUTd & CH->RIGHT;		\
}												\
else i--;


#define DO_OUTPUT_INT2								\
if ((int_cnt += YM2612.Inter_Step) & 0x04000)		\
{													\
	int_cnt &= 0x3FFF;								\
	CH->Old_OUTd = (CH->OUTd + CH->Old_OUTd) >> 1;	\
	buf[0][i] += CH->Old_OUTd & CH->LEFT;			\
	buf[1][i] += CH->Old_OUTd & CH->RIGHT;			\
}													\
else i--;											\
CH->Old_OUTd = CH->OUTd;


#define DO_OUTPUT_INT							\
if ((int_cnt += YM2612.Inter_Step) & 0x04000)	\
{												\
	int_cnt &= 0x3FFF;							\
	CH->Old_OUTd = (((int_cnt ^ 0x3FFF) * CH->OUTd) + (int_cnt * CH->Old_OUTd)) >> 14;	\
	buf[0][i] += CH->Old_OUTd & CH->LEFT;		\
	buf[1][i] += CH->Old_OUTd & CH->RIGHT;		\
}												\
else i--;										\
CH->Old_OUTd = CH->OUTd;


void Update_Chan_Algo0(channel_ *CH, int **buf, int lenght)
{
	int i;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 0 len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_0
		DO_OUTPUT
	}
}


void Update_Chan_Algo1(channel_ *CH, int **buf, int lenght)
{
	int i;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 1 len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_1
		DO_OUTPUT
	}
}


void Update_Chan_Algo2(channel_ *CH, int **buf, int lenght)
{
	int i;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 2 len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_2
		DO_OUTPUT
	}
}


void Update_Chan_Algo3(channel_ *CH, int **buf, int lenght)
{
	int i;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 3 len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_3
		DO_OUTPUT
	}
}


void Update_Chan_Algo4(channel_ *CH, int **buf, int lenght)
{
	int i;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 4 len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_4
		DO_OUTPUT
	}
}


void Update_Chan_Algo5(channel_ *CH, int **buf, int lenght)
{
	int i;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 5 len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_5
		DO_OUTPUT
	}
}


void Update_Chan_Algo6(channel_ *CH, int **buf, int lenght)
{
	int i;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 6 len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_6
		DO_OUTPUT
	}
}


void Update_Chan_Algo7(channel_ *CH, int **buf, int lenght)
{
	int i;

	if ((CH->SLOT[S0].Ecnt == ENV_END) && (CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 7 len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_7
		DO_OUTPUT
	}
}


void Update_Chan_Algo0_LFO(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 0 LFO len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_0
		DO_OUTPUT
	}
}


void Update_Chan_Algo1_LFO(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 1 LFO len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_1
		DO_OUTPUT
	}
}


void Update_Chan_Algo2_LFO(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 2 LFO len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_2
		DO_OUTPUT
	}
}


void Update_Chan_Algo3_LFO(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 3 LFO len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_3
		DO_OUTPUT
	}
}


void Update_Chan_Algo4_LFO(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 4 LFO len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_4
		DO_OUTPUT
	}
}


void Update_Chan_Algo5_LFO(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 5 LFO len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_5
		DO_OUTPUT
	}
}


void Update_Chan_Algo6_LFO(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 6 LFO len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_6
		DO_OUTPUT
	}
}


void Update_Chan_Algo7_LFO(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if ((CH->SLOT[S0].Ecnt == ENV_END) && (CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 7 LFO len = %d\n\n", lenght);
#endif

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_7
		DO_OUTPUT
	}
}


/******************************************************
 *          Interpolated output                       *
 *****************************************************/


void Update_Chan_Algo0_Int(channel_ *CH, int **buf, int lenght)
{
	int i;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 0 len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_0
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo1_Int(channel_ *CH, int **buf, int lenght)
{
	int i;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 1 len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_1
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo2_Int(channel_ *CH, int **buf, int lenght)
{
	int i;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 2 len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_2
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo3_Int(channel_ *CH, int **buf, int lenght)
{
	int i;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 3 len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_3
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo4_Int(channel_ *CH, int **buf, int lenght)
{
	int i;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 4 len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_4
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo5_Int(channel_ *CH, int **buf, int lenght)
{
	int i;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 5 len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_5
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo6_Int(channel_ *CH, int **buf, int lenght)
{
	int i;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 6 len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_6
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo7_Int(channel_ *CH, int **buf, int lenght)
{
	int i;

	if ((CH->SLOT[S0].Ecnt == ENV_END) && (CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 7 len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE
		GET_CURRENT_ENV
		UPDATE_ENV
		DO_ALGO_7
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo0_LFO_Int(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 0 LFO len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_0
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo1_LFO_Int(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 1 LFO len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_1
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo2_LFO_Int(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 2 LFO len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_2
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo3_LFO_Int(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if (CH->SLOT[S3].Ecnt == ENV_END) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 3 LFO len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_3
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo4_LFO_Int(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 4 LFO len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_4
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo5_LFO_Int(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 5 LFO len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_5
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo6_LFO_Int(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if ((CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 6 LFO len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_6
		DO_OUTPUT_INT
	}
}


void Update_Chan_Algo7_LFO_Int(channel_ *CH, int **buf, int lenght)
{
	int i, env_LFO, freq_LFO;

	if ((CH->SLOT[S0].Ecnt == ENV_END) && (CH->SLOT[S1].Ecnt == ENV_END) && (CH->SLOT[S2].Ecnt == ENV_END) && (CH->SLOT[S3].Ecnt == ENV_END)) return;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nAlgo 7 LFO len = %d\n\n", lenght);
#endif

	int_cnt = YM2612.Inter_Cnt;

	for(i = 0; i < lenght; i++)
	{
		GET_CURRENT_PHASE
		UPDATE_PHASE_LFO
		GET_CURRENT_ENV_LFO
		UPDATE_ENV
		DO_ALGO_7
		DO_OUTPUT_INT
	}
}



/***********************************************
 *            fonctions publiques              *
 ***********************************************/


// Initialisation de l'émulateur YM2612
int YM2612_Init(int Clock, int Rate, int Interpolation)
{
	int i, j;
	double x;

	if ((Rate == 0) || (Clock == 0)) return 1;

	memset(&YM2612, 0, sizeof(YM2612));

#if YM_DEBUG_LEVEL > 0
	if (debug_file == NULL)
	{
		debug_file = fopen("ym2612.log", "w");
		fprintf(debug_file, "YM2612 logging :\n\n");
	}
#endif

	YM2612.Clock = Clock;
	YM2612.Rate = Rate;

	// 144 = 12 * (prescale * 2) = 12 * 6 * 2
	// prescale set to 6 by default

	YM2612.Frequence = ((double) YM2612.Clock / (double) YM2612.Rate) / 144.0;
	YM2612.TimerBase = (int) (YM2612.Frequence * 4096.0);

	if ((Interpolation) && (YM2612.Frequence > 1.0))
	{
		YM2612.Inter_Step = (unsigned int) ((1.0 / YM2612.Frequence) * (double) (0x4000));
		YM2612.Inter_Cnt = 0;

		// We recalculate rate and frequence after interpolation
			
		YM2612.Rate = YM2612.Clock / 144;
		YM2612.Frequence = 1.0;
	}
	else
	{
		YM2612.Inter_Step = 0x4000;
		YM2612.Inter_Cnt = 0;
	}

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "YM2612 frequence = %g rate = %d  interp step = %.8X\n\n", YM2612.Frequence, YM2612.Rate, YM2612.Inter_Step);
#endif

	// Tableau TL :
	// [0     -  4095] = +output  [4095  - ...] = +output overflow (fill with 0)
	// [12288 - 16383] = -output  [16384 - ...] = -output overflow (fill with 0)

	for(i = 0; i < TL_LENGHT; i++)
	{
		if (i >= PG_CUT_OFF)	// YM2612 cut off sound after 78 dB (14 bits output ?)
		{
			TL_TAB[TL_LENGHT + i] = TL_TAB[i] = 0;
		}
		else
		{
			x = MAX_OUT;								// Max output
			x /= pow(10, (ENV_STEP * i) / 20);			// Decibel -> Voltage

			TL_TAB[i] = (int) x;
			TL_TAB[TL_LENGHT + i] = -TL_TAB[i];
		}

#if YM_DEBUG_LEVEL > 2
		fprintf(debug_file, "TL_TAB[%d] = %.8X    TL_TAB[%d] = %.8X\n", i, TL_TAB[i], TL_LENGHT + i, TL_TAB[TL_LENGHT + i]);
#endif
	}
	
#if YM_DEBUG_LEVEL > 2
	fprintf(debug_file, "\n\n\n\n");
#endif

	// Tableau SIN :
	// SIN_TAB[x][y] = sin(x) * y; 
	// x = phase and y = volume

	SIN_TAB[0] = SIN_TAB[SIN_LENGHT / 2] = &TL_TAB[(int)PG_CUT_OFF];

	for(i = 1; i <= SIN_LENGHT / 4; i++)
	{
		x = sin(2.0 * PI * (double) (i) / (double) (SIN_LENGHT));	// Sinus
		x = 20 * log10(1 / x);										// convert to dB

		j = (int) (x / ENV_STEP);						// Get TL range

		if (j > PG_CUT_OFF) j = (int) PG_CUT_OFF;

		SIN_TAB[i] = SIN_TAB[(SIN_LENGHT / 2) - i] = &TL_TAB[j];
		SIN_TAB[(SIN_LENGHT / 2) + i] = SIN_TAB[SIN_LENGHT - i] = &TL_TAB[TL_LENGHT + j];

#if YM_DEBUG_LEVEL > 2
		fprintf(debug_file, "SIN[%d][0] = %.8X    SIN[%d][0] = %.8X    SIN[%d][0] = %.8X    SIN[%d][0] = %.8X\n", i, SIN_TAB[i][0], (SIN_LENGHT / 2) - i, SIN_TAB[(SIN_LENGHT / 2) - i][0], (SIN_LENGHT / 2) + i, SIN_TAB[(SIN_LENGHT / 2) + i][0], SIN_LENGHT - i, SIN_TAB[SIN_LENGHT - i][0]);
#endif
	}

#if YM_DEBUG_LEVEL > 2
	fprintf(debug_file, "\n\n\n\n");
#endif

	// Tableau LFO (LFO wav) :

	for(i = 0; i < LFO_LENGHT; i++)
	{
		x = sin(2.0 * PI * (double) (i) / (double) (LFO_LENGHT));	// Sinus
		x += 1.0;
		x /= 2.0;					// positive only
		x *= 11.8 / ENV_STEP;		// ajusted to MAX enveloppe modulation

		LFO_ENV_TAB[i] = (int) x;

		x = sin(2.0 * PI * (double) (i) / (double) (LFO_LENGHT));	// Sinus
		x *= (double) ((1 << (LFO_HBITS - 1)) - 1);

		LFO_FREQ_TAB[i] = (int) x;

#if YM_DEBUG_LEVEL > 2
		fprintf(debug_file, "LFO[%d] = %.8X\n", i, LFO_ENV_TAB[i]);
#endif
	}

#if YM_DEBUG_LEVEL > 2
	fprintf(debug_file, "\n\n\n\n");
#endif

	// Tableau Enveloppe :
	// ENV_TAB[0] -> ENV_TAB[ENV_LENGHT - 1]				= attack curve
	// ENV_TAB[ENV_LENGHT] -> ENV_TAB[2 * ENV_LENGHT - 1]	= decay curve

	for(i = 0; i < ENV_LENGHT; i++)
	{
		// Attack curve (x^8 - music level 2 Vectorman 2)
		x = pow(((double) ((ENV_LENGHT - 1) - i) / (double) (ENV_LENGHT)), 8);
		x *= ENV_LENGHT;

		ENV_TAB[i] = (int) x;

		// Decay curve (just linear)
		x = pow(((double) (i) / (double) (ENV_LENGHT)), 1);
		x *= ENV_LENGHT;

		ENV_TAB[ENV_LENGHT + i] = (int) x;

#if YM_DEBUG_LEVEL > 2
		fprintf(debug_file, "ATTACK[%d] = %d   DECAY[%d] = %d\n", i, ENV_TAB[i], i, ENV_TAB[ENV_LENGHT + i]);
#endif
	}

	ENV_TAB[ENV_END >> ENV_LBITS] = ENV_LENGHT - 1;		// for the stopped state

	// Tableau pour la conversion Attack -> Decay and Decay -> Attack
	
	for(i = 0, j = ENV_LENGHT - 1; i < ENV_LENGHT; i++)
	{
		while (j && (ENV_TAB[j] < (unsigned) i)) j--;

		DECAY_TO_ATTACK[i] = j << ENV_LBITS;
	}

	// Tableau pour le Substain Level
	
	for(i = 0; i < 15; i++)
	{
		x = i * 3;					// 3 and not 6 (Mickey Mania first music for test)
		x /= ENV_STEP;

		j = (int) x;
		j <<= ENV_LBITS;

		SL_TAB[i] = j + ENV_DECAY;
	}

	j = ENV_LENGHT - 1;				// special case : volume off
	j <<= ENV_LBITS;
	SL_TAB[15] = j + ENV_DECAY;

	// Tableau Frequency Step

	for(i = 0; i < 2048; i++)
	{
		x = (double) (i) * YM2612.Frequence;

#if ((SIN_LBITS + SIN_HBITS - (21 - 7)) < 0)
		x /= (double) (1 << ((21 - 7) - SIN_LBITS - SIN_HBITS));
#else
		x *= (double) (1 << (SIN_LBITS + SIN_HBITS - (21 - 7)));
#endif

		x /= 2.0;	// because MUL = value * 2

		FINC_TAB[i] = (unsigned int) x;
	}

	// Tableaux Attack & Decay Rate

	for(i = 0; i < 4; i++)
	{
		AR_TAB[i] = 0;
		DR_TAB[i] = 0;
	}

	for(i = 0; i < 60; i++)
	{
		x = YM2612.Frequence;

		x *= 1.0 + ((i & 3) * 0.25);					// bits 0-1 : x1.00, x1.25, x1.50, x1.75
		x *= (double) (1 << ((i >> 2)));				// bits 2-5 : shift bits (x2^0 - x2^15)
		x *= (double) (ENV_LENGHT << ENV_LBITS);		// on ajuste pour le tableau ENV_TAB

		AR_TAB[i + 4] = (unsigned int) (x / AR_RATE);
		DR_TAB[i + 4] = (unsigned int) (x / DR_RATE);
	}

	for(i = 64; i < 96; i++)
	{
		AR_TAB[i] = AR_TAB[63];
		DR_TAB[i] = DR_TAB[63];

		NULL_RATE[i - 64] = 0;
	}

	// Tableau Detune

	for(i = 0; i < 4; i++)
	{
		for (j = 0; j < 32; j++)
		{
#if ((SIN_LBITS + SIN_HBITS - 21) < 0)
			x = (double) DT_DEF_TAB[(i << 5) + j] * YM2612.Frequence / (double) (1 << (21 - SIN_LBITS - SIN_HBITS));
#else
			x = (double) DT_DEF_TAB[(i << 5) + j] * YM2612.Frequence * (double) (1 << (SIN_LBITS + SIN_HBITS - 21));
#endif

			DT_TAB[i + 0][j] = (int) x;
			DT_TAB[i + 4][j] = (int) -x;
		}
	}

	// Tableau LFO

	j = (YM2612.Rate * YM2612.Inter_Step) / 0x4000;

	LFO_INC_TAB[0] = (unsigned int) (3.98 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
	LFO_INC_TAB[1] = (unsigned int) (5.56 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
	LFO_INC_TAB[2] = (unsigned int) (6.02 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
	LFO_INC_TAB[3] = (unsigned int) (6.37 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
	LFO_INC_TAB[4] = (unsigned int) (6.88 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
	LFO_INC_TAB[5] = (unsigned int) (9.63 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
	LFO_INC_TAB[6] = (unsigned int) (48.1 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
	LFO_INC_TAB[7] = (unsigned int) (72.2 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);

	YM2612_Reset();

	return 0;
}


int YM2612_End(void)
{

#if YM_DEBUG_LEVEL > 0
	if (debug_file) fclose(debug_file);
	debug_file = NULL;
#endif

	return 0;
}


int YM2612_Reset(void)
{
	int i, j;
  
#if YM_DEBUG_LEVEL > 0
	fprintf(debug_file, "\n\nStarting reseting YM2612 ...\n\n");
#endif

	YM2612.LFOcnt = 0;
	YM2612.TimerA = 0;
	YM2612.TimerAL = 0;
	YM2612.TimerAcnt = 0;
	YM2612.TimerB = 0;
	YM2612.TimerBL = 0;
	YM2612.TimerBcnt = 0;
	YM2612.DAC = 0;
	YM2612.DACdata = 0;

	YM2612.Status = 0;

	YM2612.OPNAadr = 0;
	YM2612.OPNBadr = 0;
	YM2612.Inter_Cnt = 0;

	for(i = 0; i < 6; i++)
	{
		YM2612.CHANNEL[i].Old_OUTd = 0;
		YM2612.CHANNEL[i].OUTd = 0;
		YM2612.CHANNEL[i].LEFT = 0xFFFFFFFF;
		YM2612.CHANNEL[i].RIGHT = 0xFFFFFFFF;
		YM2612.CHANNEL[i].ALGO = 0;;
		YM2612.CHANNEL[i].FB = 31;
		YM2612.CHANNEL[i].FMS = 0;
		YM2612.CHANNEL[i].AMS = 0;

		for(j = 0 ;j < 4 ; j++)
		{
			YM2612.CHANNEL[i].S0_OUT[j] = 0;
			YM2612.CHANNEL[i].FNUM[j] = 0;
			YM2612.CHANNEL[i].FOCT[j] = 0;
			YM2612.CHANNEL[i].KC[j] = 0;

			YM2612.CHANNEL[i].SLOT[j].Fcnt = 0;
			YM2612.CHANNEL[i].SLOT[j].Finc = 0;
			YM2612.CHANNEL[i].SLOT[j].Ecnt = ENV_END;		// Put it at the end of Decay phase...
			YM2612.CHANNEL[i].SLOT[j].Einc = 0;
			YM2612.CHANNEL[i].SLOT[j].Ecmp = 0;
			YM2612.CHANNEL[i].SLOT[j].Ecurp = RELEASE;

			YM2612.CHANNEL[i].SLOT[j].ChgEnM = 0;
		}
	}

	for(i = 0; i < 0x100; i++)
	{
		YM2612.REG[0][i] = -1;
		YM2612.REG[1][i] = -1;
	}

	for(i = 0xB6; i >= 0xB4; i--)
	{
		YM2612_Write(0, (unsigned char) i);
		YM2612_Write(2, (unsigned char) i);
		YM2612_Write(1, 0xC0);
		YM2612_Write(3, 0xC0);
	}

	for(i = 0xB2; i >= 0x22; i--)
	{
		YM2612_Write(0, (unsigned char) i);
		YM2612_Write(2, (unsigned char) i);
		YM2612_Write(1, 0);
		YM2612_Write(3, 0);
	}

	YM2612_Write(0, 0x2A);
	YM2612_Write(1, 0x80);

#if YM_DEBUG_LEVEL > 0
	fprintf(debug_file, "\n\nFinishing reseting YM2612 ...\n\n");
#endif

	return 0;
}


int YM2612_Read(void)
{
/*	static int cnt = 0;

	if (cnt++ == 50)
	{
		cnt = 0;
		return YM2612.Status;
	}
	else return YM2612.Status | 0x80;
*/
	return YM2612.Status;
}


int YM2612_Write(unsigned char adr, unsigned char data)
{
	int d;
	
	data &= 0xFF;
	adr &= 0x3;
	
	switch(adr)
	{
		case 0:
			YM2612.OPNAadr = data;
			break;

		case 1:
			// Trivial optimisation

			if (YM2612.OPNAadr == 0x2A)
			{
				YM2612.DACdata = ((int) data - 0x80) << 7;
				return 0;
			}

			d = YM2612.OPNAadr & 0xF0;

			if (d >= 0x30)
			{
				if (YM2612.REG[0][YM2612.OPNAadr] == data) return 2;
				YM2612.REG[0][YM2612.OPNAadr] = data;

				if (GYM_Dumping) Update_GYM_Dump(1, YM2612.OPNAadr, data);

				if (d < 0xA0)		// SLOT
				{
					SLOT_SET(YM2612.OPNAadr, data);
				}
				else				// CHANNEL
				{
					CHANNEL_SET(YM2612.OPNAadr, data);
				}
			}
			else					// YM2612
			{
				YM2612.REG[0][YM2612.OPNAadr] = data;

				if ((GYM_Dumping) && ((YM2612.OPNAadr == 0x22) || (YM2612.OPNAadr == 0x27) || (YM2612.OPNAadr == 0x28))) Update_GYM_Dump(1, YM2612.OPNAadr, data);

				YM_SET(YM2612.OPNAadr, data);
			}
			break;

		case 2:
			YM2612.OPNBadr = data;
			break;

		case 3:
			d = YM2612.OPNBadr & 0xF0;

			if (d >= 0x30)
			{
				if (YM2612.REG[1][YM2612.OPNBadr] == data) return 2;
				YM2612.REG[1][YM2612.OPNBadr] = data;

				if (GYM_Dumping) Update_GYM_Dump(2, YM2612.OPNBadr, data);

				if (d < 0xA0)		// SLOT
				{
					SLOT_SET(YM2612.OPNBadr + 0x100, data);
				}
				else				// CHANNEL
				{
					CHANNEL_SET(YM2612.OPNBadr + 0x100, data);
				}
			}
			else return 1;
			break;
	}

	return 0;
}


void YM2612_Update(int **buf, int length)
{
	int i, j, algo_type;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nStarting generating sound...\n\n");
#endif

	// Mise à jour des pas des compteurs-fréquences s'ils ont été modifiés

	if (YM2612.CHANNEL[0].SLOT[0].Finc == -1) CALC_FINC_CH(&YM2612.CHANNEL[0]);
	if (YM2612.CHANNEL[1].SLOT[0].Finc == -1) CALC_FINC_CH(&YM2612.CHANNEL[1]);
	if (YM2612.CHANNEL[2].SLOT[0].Finc == -1)
	{
		if (YM2612.Mode & 0x40)
		{
			CALC_FINC_SL(&(YM2612.CHANNEL[2].SLOT[S0]), FINC_TAB[YM2612.CHANNEL[2].FNUM[2]] >> (7 - YM2612.CHANNEL[2].FOCT[2]), YM2612.CHANNEL[2].KC[2]);
			CALC_FINC_SL(&(YM2612.CHANNEL[2].SLOT[S1]), FINC_TAB[YM2612.CHANNEL[2].FNUM[3]] >> (7 - YM2612.CHANNEL[2].FOCT[3]), YM2612.CHANNEL[2].KC[3]);
			CALC_FINC_SL(&(YM2612.CHANNEL[2].SLOT[S2]), FINC_TAB[YM2612.CHANNEL[2].FNUM[1]] >> (7 - YM2612.CHANNEL[2].FOCT[1]), YM2612.CHANNEL[2].KC[1]);
			CALC_FINC_SL(&(YM2612.CHANNEL[2].SLOT[S3]), FINC_TAB[YM2612.CHANNEL[2].FNUM[0]] >> (7 - YM2612.CHANNEL[2].FOCT[0]), YM2612.CHANNEL[2].KC[0]);
		}
		else
		{
			CALC_FINC_CH(&YM2612.CHANNEL[2]);
		}
	}
	if (YM2612.CHANNEL[3].SLOT[0].Finc == -1) CALC_FINC_CH(&YM2612.CHANNEL[3]);
	if (YM2612.CHANNEL[4].SLOT[0].Finc == -1) CALC_FINC_CH(&YM2612.CHANNEL[4]);
	if (YM2612.CHANNEL[5].SLOT[0].Finc == -1) CALC_FINC_CH(&YM2612.CHANNEL[5]);

/*
	CALC_FINC_CH(&YM2612.CHANNEL[0]);
	CALC_FINC_CH(&YM2612.CHANNEL[1]);
	if (YM2612.Mode & 0x40)
	{
		CALC_FINC_SL(&(YM2612.CHANNEL[2].SLOT[0]), FINC_TAB[YM2612.CHANNEL[2].FNUM[2]] >> (7 - YM2612.CHANNEL[2].FOCT[2]), YM2612.CHANNEL[2].KC[2]);
		CALC_FINC_SL(&(YM2612.CHANNEL[2].SLOT[1]), FINC_TAB[YM2612.CHANNEL[2].FNUM[3]] >> (7 - YM2612.CHANNEL[2].FOCT[3]), YM2612.CHANNEL[2].KC[3]);
		CALC_FINC_SL(&(YM2612.CHANNEL[2].SLOT[2]), FINC_TAB[YM2612.CHANNEL[2].FNUM[1]] >> (7 - YM2612.CHANNEL[2].FOCT[1]), YM2612.CHANNEL[2].KC[1]);
		CALC_FINC_SL(&(YM2612.CHANNEL[2].SLOT[3]), FINC_TAB[YM2612.CHANNEL[2].FNUM[0]] >> (7 - YM2612.CHANNEL[2].FOCT[0]), YM2612.CHANNEL[2].KC[0]);
	}
	else
	{
		CALC_FINC_CH(&YM2612.CHANNEL[2]);
	}
	CALC_FINC_CH(&YM2612.CHANNEL[3]);
	CALC_FINC_CH(&YM2612.CHANNEL[4]);
	CALC_FINC_CH(&YM2612.CHANNEL[5]);
*/

	if (YM2612.Inter_Step & 0x04000) algo_type = 0;
	else algo_type = 16;

	if (YM2612.LFOinc)
	{
		// Precalcul LFO wav
		
		for(i = 0; i < length; i++)
		{
			j = ((YM2612.LFOcnt += YM2612.LFOinc) >> LFO_LBITS) & LFO_MASK;

			LFO_ENV_UP[i] = LFO_ENV_TAB[j];
			LFO_FREQ_UP[i] = LFO_FREQ_TAB[j];

#if YM_DEBUG_LEVEL > 3
			fprintf(debug_file, "LFO_ENV_UP[%d] = %d   LFO_FREQ_UP[%d] = %d\n", i, LFO_ENV_UP[i], i, LFO_FREQ_UP[i]);
#endif
		}

		algo_type |= 8;
	}

	if (Chan_Enable[0]) UPDATE_CHAN[YM2612.CHANNEL[0].ALGO + algo_type](&(YM2612.CHANNEL[0]), buf, length);
	if (Chan_Enable[1]) UPDATE_CHAN[YM2612.CHANNEL[1].ALGO + algo_type](&(YM2612.CHANNEL[1]), buf, length);
	if (Chan_Enable[2]) UPDATE_CHAN[YM2612.CHANNEL[2].ALGO + algo_type](&(YM2612.CHANNEL[2]), buf, length);
	if (Chan_Enable[3]) UPDATE_CHAN[YM2612.CHANNEL[3].ALGO + algo_type](&(YM2612.CHANNEL[3]), buf, length);
	if (Chan_Enable[4]) UPDATE_CHAN[YM2612.CHANNEL[4].ALGO + algo_type](&(YM2612.CHANNEL[4]), buf, length);
	if (Chan_Enable[5]) if (!(YM2612.DAC)) UPDATE_CHAN[YM2612.CHANNEL[5].ALGO + algo_type](&(YM2612.CHANNEL[5]), buf, length);

	YM2612.Inter_Cnt = int_cnt;

#if YM_DEBUG_LEVEL > 1
	fprintf(debug_file, "\n\nFinishing generating sound...\n\n");
#endif

}


int YM2612_Save(unsigned char SAVE[0x200])
{
	int i;

	for(i = 0; i < 0x100; i++)
	{
		SAVE[0x000 + i] = YM2612.REG[0][i];
		SAVE[0x100 + i] = YM2612.REG[1][i];
	}

	return 0;
}


int YM2612_Restore(unsigned char SAVE[0x200])
{
	int i;

	YM2612_Reset();

	for(i = 0; i < 0x100; i++)
	{
		YM2612_Write(0, (unsigned char) i);
		YM2612_Write(1, SAVE[0x000 + i]);
		YM2612_Write(2, (unsigned char) i);
		YM2612_Write(3, SAVE[0x100 + i]);
	}

	return 0;
}

/* Gens */

void YM2612_DacAndTimers_Update(int **buffer, int length)
{
	int *bufL, *bufR;
	int i;

	if (YM2612.DAC && YM2612.DACdata && DAC_Enable)
	{
		bufL = buffer[0];
		bufR = buffer[1];

		for(i = 0; i < length; i++)
		{
			bufL[i] += YM2612.DACdata & YM2612.CHANNEL[5].LEFT;
			bufR[i] += YM2612.DACdata & YM2612.CHANNEL[5].RIGHT;
		}
	}

	i = YM2612.TimerBase * length;

	if (YM2612.Mode & 1)							// Timer A ON ?
	{
//		if ((YM2612.TimerAcnt -= 14073) <= 0)		// 13879=NTSC (old: 14475=NTSC  14586=PAL)
		if ((YM2612.TimerAcnt -= i) <= 0)
		{
			YM2612.Status |= (YM2612.Mode & 0x04) >> 2;
			YM2612.TimerAcnt += YM2612.TimerAL;

#if YM_DEBUG_LEVEL > 0
			fprintf(debug_file, "Counter A overflow\n");
#endif

			if (YM2612.Mode & 0x80) CSM_Key_Control();
		}
	}

	if (YM2612.Mode & 2)							// Timer B ON ?
	{
//		if ((YM2612.TimerBcnt -= 14073) <= 0)		// 13879=NTSC (old: 14475=NTSC  14586=PAL)
		if ((YM2612.TimerBcnt -= i) <= 0)
		{
			YM2612.Status |= (YM2612.Mode & 0x08) >> 2;
			YM2612.TimerBcnt += YM2612.TimerBL;

#if YM_DEBUG_LEVEL > 0
			fprintf(debug_file, "Counter B overflow\n");
#endif
		}
	}
}


void YM2612_Special_Update(void)
{
	if (YM_Len && YM2612_Enable)
	{
		YM2612_Update(YM_Buf, YM_Len);

		YM_Buf[0] = Seg_L + Sound_Extrapol[VDP_Current_Line + 1][0];
		YM_Buf[1] = Seg_R + Sound_Extrapol[VDP_Current_Line + 1][0];
		YM_Len = 0;
	}
}

#ifdef __PORT__
int _YM2612_Reset(void) __attribute__ (( alias ("YM2612_Reset")));
int _YM2612_Read(void) __attribute__ ((alias ("YM2612_Read")));
int _YM2612_Write(unsigned char adr, unsigned char data) __attribute__ ((alias ("YM2612_Write")));
#endif

/* end */
