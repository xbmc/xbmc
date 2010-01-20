/***********************************************************/
/*                                                         */
/* PSG.C : SN76489 emulator                                */
/*                                                         */
/* Noise define constantes taken from MAME                 */
/*                                                         */
/* This source is a part of Gens project                   */
/* Written by Stéphane Dallongeville (gens@consolemul.com) */
/* Copyright (c) 2002 by Stéphane Dallongeville            */
/*                                                         */
/***********************************************************/

#include <stdio.h>
#include <math.h>
#include "psg.h"


/* Defines */

#ifndef PI
#define PI 3.14159265358979323846
#endif

// Change MAX_OUTPUT to change PSG volume (default = 0x7FFF)

#define MAX_OUTPUT 0x4FFF

#define W_NOISE 0x12000
#define P_NOISE 0x08000

//#define NOISE_DEF 0x0f35
//#define NOISE_DEF 0x0001
#define NOISE_DEF 0x4000

#define PSG_DEBUG_LEVEL 0

#if PSG_DEBUG_LEVEL > 0

#define PSG_DEBUG_0(x)								\
fprintf(psg_debug_file, (x));
#define PSG_DEBUG_1(x, a)							\
fprintf(psg_debug_file, (x), (a));
#define PSG_DEBUG_2(x, a, b)						\
fprintf(psg_debug_file, (x), (a), (b));
#define PSG_DEBUG_3(x, a, b, c)						\
fprintf(psg_debug_file, (x), (a), (b), (c));
#define PSG_DEBUG_4(x, a, b, c, d)					\
fprintf(psg_debug_file, (x), (a), (b), (c), (d));

#else

#define PSG_DEBUG_0(x)
#define PSG_DEBUG_1(x, a)
#define PSG_DEBUG_2(x, a, b)
#define PSG_DEBUG_3(x, a, b, c)
#define PSG_DEBUG_4(x, a, b, c, d)

#endif


/* Variables */

unsigned int PSG_SIN_Table[16][512];
unsigned int PSG_Step_Table[1024];
unsigned int PSG_Volume_Table[16];
unsigned int PSG_Noise_Step_Table[4];
unsigned int PSG_Save[8];

struct _psg PSG;

#if PSG_DEBUG_LEVEL > 0
FILE *psg_debug_file = NULL;
#endif


/* Gens specific extern and variables */

extern unsigned int Sound_Extrapol[312][2];
extern int Seg_L[882], Seg_R[882];
extern int VDP_Current_Line;
extern int GYM_Dumping;

int Update_GYM_Dump(char v0, char v1, char v2);

int PSG_Enable;
int PSG_Improv;
int *PSG_Buf[2];
int PSG_Len = 0;

int PSG_Chan_Enable[4];

/* Functions */

void PSG_Write(int data)
{
	if (GYM_Dumping) Update_GYM_Dump((unsigned char) 3, (unsigned char) data, (unsigned char) 0);

	if (data & 0x80)
	{
		PSG.Current_Register = (data & 0x70) >> 4;
		PSG.Current_Channel = PSG.Current_Register >> 1;

		data &= 0x0F;
		
		PSG.Register[PSG.Current_Register] = (PSG.Register[PSG.Current_Register] & 0x3F0) | data;

		if (PSG.Current_Register & 1)
		{
			// Volume

			PSG_Special_Update();

			PSG.Volume[PSG.Current_Channel] = PSG_Volume_Table[data];

			PSG_DEBUG_2("channel %d    volume = %.8X\n", PSG.Current_Channel, PSG.Volume[PSG.Current_Channel]);
		}
		else
		{
			// Frequency

			PSG_Special_Update();

			if (PSG.Current_Channel != 3)
			{
				// Normal channel

				PSG.CntStep[PSG.Current_Channel] = PSG_Step_Table[PSG.Register[PSG.Current_Register]];

				if ((PSG.Current_Channel == 2) && ((PSG.Register[6] & 3) == 3))
				{
					PSG.CntStep[3] = PSG.CntStep[2] >> 1;
				}

				PSG_DEBUG_2("channel %d    step = %.8X\n", PSG.Current_Channel, PSG.CntStep[PSG.Current_Channel]);
			}
			else
			{
				// Noise channel

				PSG.Noise = NOISE_DEF;
				PSG_Noise_Step_Table[3] = PSG.CntStep[2] >> 1;
				PSG.CntStep[3] = PSG_Noise_Step_Table[data & 3];

				if (data & 4) PSG.Noise_Type = W_NOISE;
				else PSG.Noise_Type = P_NOISE;

				PSG_DEBUG_1("channel N    type = %.2X\n", data);
			}
		}
	}
	else
	{
		if (!(PSG.Current_Register & 1))
		{
			// Frequency 

			if (PSG.Current_Channel != 3)
			{
				PSG_Special_Update();

				PSG.Register[PSG.Current_Register] = (PSG.Register[PSG.Current_Register] & 0x0F) | ((data & 0x3F) << 4);

				PSG.CntStep[PSG.Current_Channel] = PSG_Step_Table[PSG.Register[PSG.Current_Register]];

				if ((PSG.Current_Channel == 2) && ((PSG.Register[6] & 3) == 3))
				{
					PSG.CntStep[3] = PSG.CntStep[2] >> 1;
				}

				PSG_DEBUG_2("channel %d    step = %.8X\n", PSG.Current_Channel, PSG.CntStep[PSG.Current_Channel]);
			}
		}
	}
}


void PSG_Update_SIN(int **buffer, int length)
{
	int i, j, out;
	int cur_cnt, cur_step, cur_vol;
	unsigned int *sin_t;

	for(j = 2; j >= 0; j--)
	{
		if (PSG.Volume[j])
		{
			cur_cnt = PSG.Counter[j];
			cur_step = PSG.CntStep[j];
			sin_t = PSG_SIN_Table[PSG.Register[(j << 1) + 1]];

			for(i = 0; i < length; i++)
			{
				out = sin_t[(cur_cnt = (cur_cnt + cur_step) & 0x1FFFF) >> 8];

				if (PSG_Chan_Enable[j])
				{
					buffer[0][i] += out;
					buffer[1][i] += out;
				}
			}

			PSG.Counter[j] = cur_cnt;
		}
		else PSG.Counter[j] += PSG.CntStep[j] * length;
	}


	// Channel 3 - Noise

	if ((cur_vol = PSG.Volume[3]))
	{
		cur_cnt = PSG.Counter[3];
		cur_step = PSG.CntStep[3];

		for(i = 0; i < length; i++)
		{
			cur_cnt += cur_step;

			if (PSG.Noise & 1)
			{
				if (PSG_Chan_Enable[3])
				{
					buffer[0][i] += cur_vol;
					buffer[1][i] += cur_vol;
				}

				if (cur_cnt & 0x10000)
				{
					cur_cnt &= 0xFFFF;
					PSG.Noise = (PSG.Noise ^ PSG.Noise_Type) >> 1;
				}
			}
			else if (cur_cnt & 0x10000)
			{
				cur_cnt &= 0xFFFF;
				PSG.Noise >>= 1;
			}
		}

		PSG.Counter[3] = cur_cnt;
	}
	else PSG.Counter[3] += PSG.CntStep[3] * length;
}


void PSG_Update(int **buffer, int length)
{
	int i, j;
	int cur_cnt, cur_step, cur_vol;

	for(j = 2; j >= 0; j--)
	{
		if ((cur_vol = PSG.Volume[j]))
		{
			if ((cur_step = PSG.CntStep[j]) < 0x10000)
			{
				cur_cnt = PSG.Counter[j];

				for(i = 0; i < length; i++)
				{
					if ((cur_cnt += cur_step) & 0x10000)
					{
						if (PSG_Chan_Enable[j])
						{
							buffer[0][i] += cur_vol;
							buffer[1][i] += cur_vol;
						}
					}
				}

				PSG.Counter[j] = cur_cnt;
			}
			else
			{
				for(i = 0; i < length; i++)
				{
					if (PSG_Chan_Enable[j])
					{
						buffer[0][i] += cur_vol;
						buffer[1][i] += cur_vol;
					}
				}
			}
		}
		else
		{
			PSG.Counter[j] += PSG.CntStep[j] * length;
		}
	}

	// Channel 3 - Noise
	
	if ((cur_vol = PSG.Volume[3]))
	{
		cur_cnt = PSG.Counter[3];
		cur_step = PSG.CntStep[3];

		for(i = 0; i < length; i++)
		{
			cur_cnt += cur_step;

			if (PSG.Noise & 1)
			{
				if (PSG_Chan_Enable[3])
				{
					buffer[0][i] += cur_vol;
					buffer[1][i] += cur_vol;
				}

				if (cur_cnt & 0x10000)
				{
					cur_cnt &= 0xFFFF;
					PSG.Noise = (PSG.Noise ^ PSG.Noise_Type) >> 1;
				}
			}
			else if (cur_cnt & 0x10000)
			{
				cur_cnt &= 0xFFFF;
				PSG.Noise >>= 1;
			}
		}

		PSG.Counter[3] = cur_cnt;
	}
	else PSG.Counter[3] += PSG.CntStep[3] * length;
}


void PSG_Init(int clock, int rate)
{
	int i, j;
	double out;

#if PSG_DEBUG_LEVEL > 0
	if (psg_debug_file == NULL)
	{
		psg_debug_file = fopen("psg.log", "w");
		fprintf(psg_debug_file, "PSG logging :\n\n");
	}
#endif

	for(i = 1; i < 1024; i++)
	{
		// Step calculation

		out = (double) (clock) / (double) (i << 4);		// out = frequency
		out /= (double) (rate);
		out *= 65536.0;

		PSG_Step_Table[i] = (unsigned int) out;
	}

	PSG_Step_Table[0] = PSG_Step_Table[1];
		
	for(i = 0; i < 3; i++)
	{
		out = (double) (clock) / (double) (1 << (9 + i));
		out /= (double) (rate);
		out *= 65536.0;
			
		PSG_Noise_Step_Table[i] = (unsigned int) out;
	}

	PSG_Noise_Step_Table[3] = 0;

	out = (double) MAX_OUTPUT / 3.0;

	for (i = 0; i < 15; i++)
	{
		PSG_Volume_Table[i] = (unsigned int) out;
		out /= 1.258925412;		// = 10 ^ (2/20) = 2dB
	}

	PSG_Volume_Table[15] = 0;

/*
	for(i = 0; i < 256; i++)
	{
		out = (i + 1.0) / 256.0;

		for(j = 0; j < 16; j++)
		{
			PSG_SIN_Table[j][i] = (unsigned int) (out * (double) PSG_Volume_Table[j]);
		}
	}

	for(i = 0; i < 256; i++)
	{
		out = 1.0 - ((i + 1.0) / 256.0);

		for(j = 0; j < 16; j++)
		{
			PSG_SIN_Table[j][i + 256] = (unsigned int) (out * (double) PSG_Volume_Table[j]);
		}
	}
*/
	for(i = 0; i < 512; i++)
	{
		out = sin((2.0 * PI) * ((double) (i) / 512));
		out = sin((2.0 * PI) * ((double) (i) / 512));

		for(j = 0; j < 16; j++)
		{
			PSG_SIN_Table[j][i] = (unsigned int) (out * (double) PSG_Volume_Table[j]);
		}
	}

	PSG.Current_Register = 0;
	PSG.Current_Channel = 0;
	PSG.Noise = 0;
	PSG.Noise_Type = 0;

	for (i = 0; i < 4; i++)
	{
		PSG.Volume[i] = 0;
		PSG.Counter[i] = 0;
		PSG.CntStep[i] = 0;
	}

	for (i = 0; i < 8; i += 2)
	{
		PSG_Save[i] = 0;
		PSG_Save[i + 1] = 0x0F;			// volume = OFF
	}

	PSG_Restore_State();				// Reset
}


void PSG_Save_State(void)
{
	int i;
	
	for(i = 0; i < 8; i++) PSG_Save[i] = PSG.Register[i];
}


void PSG_Restore_State(void)
{
	int i;
	
	for(i = 0; i < 8; i++)
	{
		PSG_Write(0x80 | (i << 4) | (PSG_Save[i] & 0xF));
		PSG_Write((PSG_Save[i] >> 4) & 0x3F);
	}
}


/* Gens */

void PSG_Special_Update(void)
{
	if (PSG_Len && PSG_Enable)
	{
		if (PSG_Improv) PSG_Update_SIN(PSG_Buf, PSG_Len);
		else PSG_Update(PSG_Buf, PSG_Len);

		PSG_Buf[0] = Seg_L + Sound_Extrapol[VDP_Current_Line + 1][0];
		PSG_Buf[1] = Seg_R + Sound_Extrapol[VDP_Current_Line + 1][0];
		PSG_Len = 0;
	}
}

#ifdef __PORT__
void _PSG_Write(int data) __attribute__ ((alias ("PSG_Write")));
#endif

/* end */
