#include <stdio.h>
#include <memory.h>
#include <math.h>
#include "gym_play.h"
#include "psg.h"
#include "ym2612.h"

//int Seg_L[1600], Seg_R[1600];
int* Seg_L; int* Seg_R;
int Seg_Lenght;

#define CLOCK_NTSC 53700000 //53693175

unsigned int Sound_Extrapol[312][2];

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

int VDP_Current_Line = 0;
int GYM_Dumping = 0;

int Update_GYM_Dump(char v0, char v1, char v2)
{
	return 0;
}

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

unsigned char *jump_gym_time_pos(unsigned char *gym_start, unsigned int gym_size, unsigned int new_pos)
{
	unsigned int loop, num_zeros = 0;

	for (loop = 0; num_zeros < new_pos; loop++)
	{
		if (loop > gym_size)
		{	
			return 0;
		}

		switch(gym_start[loop])
		{
            case(0x00):
                num_zeros++;
                continue;
            case(0x01):
                loop += 2;
                continue;
            case(0x02):
                loop += 2;
                continue;
            case(0x03):
                loop += 1;
                continue;
		}
	}

	return (gym_start + loop);
}

void Write_Sound_Stereo(short *Dest, int lenght)
{
	int i, out_L, out_R;
	short *dest = Dest;
	
	for(i = 0; i < Seg_Lenght; i++)
	{
		out_L = Seg_L[i];
		Seg_L[i] = 0;

		if (out_L < -0x7FFF) *dest++ = -0x7FFF;
		else if (out_L > 0x7FFF) *dest++ = 0x7FFF;
		else *dest++ = (short) out_L;
						
		out_R = Seg_R[i];
		Seg_R[i] = 0;

		if (out_R < -0x7FFF) *dest++ = -0x7FFF;
		else if (out_R > 0x7FFF) *dest++ = 0x7FFF;
		else *dest++ = (short) out_R;
	}
}

void Start_Play_GYM(int sampleRate)
{
	Seg_Lenght = (int) ceil(sampleRate / 60.0);

/*	memset(Seg_L, 0, Seg_Lenght << 2);
	memset(Seg_R, 0, Seg_Lenght << 2);*/

	YM2612_Init(CLOCK_NTSC / 7, sampleRate, YM2612_Improv);
	PSG_Init(CLOCK_NTSC / 15, sampleRate);
}

unsigned char *GYM_Next(unsigned char *gym_start, unsigned char *gym_pos, unsigned int gym_size, unsigned int gym_loop)
{
	unsigned char c, c2;

    unsigned char dac_data[1600];

	int *buf[2];
	int dacMax = 0, i = 0;

	int oldPos = 0;
	double curPos = 0;
	double dacSize;
	int step;
	int *dacBuf[2];
	int retCode = 1;

	YM_Buf[0] = PSG_Buf[0] = buf[0] = Seg_L;
	YM_Buf[1] = PSG_Buf[1] = buf[1] = Seg_R;

	YM_Len = PSG_Len = 0;

	memset(dac_data, 0, sizeof(dac_data));

	if (!gym_pos)
	{
		return 0;
	}

	if ((unsigned int)(gym_pos - gym_start) >= gym_size)
	{
		if (gym_loop)
		{
			gym_pos = jump_gym_time_pos(gym_start, gym_size, gym_loop - 1);
		}
		else
		{
			return 0;
		}
	}
	
	do
	{
		c = *gym_pos++;

		switch(c)
		{
			case 0:
				if (YM2612_Enable)
				{
					// if dacMax is zero, dacSize will be NaN - so what, we won't
					// be using it in that case anyway :p
					dacSize = (double)Seg_Lenght / dacMax;

					for (i = 0; i < dacMax; i++)
					{
						oldPos = (int)curPos;

						YM2612_Write(0, 0x2A);
						YM2612_Write(1, dac_data[i]);

						if (i == dacMax - 1)
						{
							step = Seg_Lenght - oldPos;
						}
						else
						{
							curPos += dacSize;
							step = (int)curPos - oldPos;
						}

						dacBuf[0] = buf[0] + (int)oldPos;
						dacBuf[1] = buf[1] + (int)oldPos;

						YM2612_DacAndTimers_Update(dacBuf, step);
					}

					YM2612_Update(buf, Seg_Lenght);
				}
				if (PSG_Enable)
				{
					if (PSG_Improv)
					{
						PSG_Update_SIN(buf, Seg_Lenght);
					}
					else
					{
						PSG_Update(buf, Seg_Lenght);
					}
				}
				break;

			case 1:
				c2 = *gym_pos++;

				if (c2 == 0x2A)
				{
					c2 = *gym_pos++;
					dac_data[dacMax++] = c2;
				}
				else
				{
					YM2612_Write(0, c2);
					c2 = *gym_pos++;
					YM2612_Write(1, c2);
				}
				break;

			case 2:
				c2 = *gym_pos++;
				YM2612_Write(2, c2);

				c2 = *gym_pos++;
				YM2612_Write(3, c2);
				break;

			case 3:
				c2 = *gym_pos++;
				PSG_Write(c2);
				break;
		}

	} while (c);

	return gym_pos;
}

unsigned char *Play_GYM(void *Dump_Buf, unsigned char *gym_start, unsigned char *gym_pos, unsigned int gym_size, unsigned int gym_loop)
{
	unsigned char *new_gym_pos = GYM_Next(gym_start, gym_pos, gym_size, gym_loop);

	if (new_gym_pos == 0)
	{
		return 0;
	}

	Write_Sound_Stereo((short *)Dump_Buf, Seg_Lenght);

	return new_gym_pos;
}
