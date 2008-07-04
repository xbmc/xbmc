/*

  in_cube Gamecube Stream Player for Winamp
  by hcs

  includes work by Destop and bero

*/

// decode functions (kept seperate since I didn't write the DSP or ADX decoders)

#ifndef _LINUX
#include <windows.h>
#endif

#include "cube.h"

//================ C R A Z Y N A T I O N ===================================
//==========================================================================
// ADPCM decoder for Nintendo GAMECUBE dsp format, w/ modifications by hcs
//==========================================================================
// decode 8 bytes of input (1 frame, 14 samples)
int DSPdecodebuffer
(
    u8			*input, // location of encoded source samples
    s16         *out,   // location of destination buffer (16 bits / sample)
    short		coef[16],   // location of adpcm info
	short * histp,
	short * hist2p
)
{
int sample;
short nibbles[14];
int index1;
int i,j;
char *src,*dst;
short delta;
short hist=*histp;
short hist2=*hist2p;

	dst = (char*)out;

	src=input;
    i = *src&0xFF;
    src++;
    delta = 1 << (i & 255 & 15);
    index1 = (i & 255) >> 4;
	
    for(i = 0; i < 14; i = i + 2) {
		j = ( *src & 255) >> 4;
		nibbles[i] = j;
		j = *src & 255 & 15;
		nibbles[i+1] = j;
		src++;
	}

     for(i = 0; i < 14; i = i + 1) {
		if(nibbles[i] >= 8) 
			nibbles[i] = nibbles[i] - 16;       
     }
     
	 for(i = 0; i<14 ; i = i + 1) {

		sample = (delta * nibbles[i])<<11;
		sample += coef[index1*2] * hist;
		sample += coef[index1*2+1] * hist2;
		sample = sample + 1024;
		sample = sample >> 11;
		if(sample > 32767) {
			sample = 32767;
		}
		if(sample < -32768) {
			sample = -32768;
		}

        *(short*)dst = (short)sample;
        dst = dst + 2;

		hist2 = hist;
        hist = (short)sample;
        
    }
	*histp=hist;
	*hist2p=hist2;

    return((int)src);
}



// AFC decoder (by hcs)

short afccoef[16][2] =
{{0,0},
{0x0800,0},
{0,0x0800},
{0x0400,0x0400},
{0x1000,0xf800},
{0x0e00,0xfa00},
{0x0c00,0xfc00},
{0x1200,0xf600},
{0x1068,0xf738},
{0x12c0,0xf704},
{0x1400,0xf400},
{0x0800,0xf800},
{0x0400,0xfc00},
{0xfc00,0x0400},
{0xfc00,0},
{0xf800,0}};

int AFCdecodebuffer
(
    u8			*input, // location of encoded source samples
    s16         *out,   // location of destination buffer (16 bits / sample)
    short * histp,
	short * hist2p
)
{
int sample;
short nibbles[16];
int i,j;
char *src,*dst;
short idx;
short delta;
short hist=*histp;
short hist2=*hist2p;

	dst = (char*)out;

	src=input;
    delta = 1<<(((*src)>>4)&0xf);
	idx = (*src)&0xf;

	src++;
    
    for(i = 0; i < 16; i = i + 2) {
		j = ( *src & 255) >> 4;
		nibbles[i] = j;
		j = *src & 255 & 15;
		nibbles[i+1] = j;
		src++;
	}

     for(i = 0; i < 16; i = i + 1) {
		if(nibbles[i] >= 8) 
			nibbles[i] = nibbles[i] - 16;
     }
     
	 for(i = 0; i<16 ; i = i + 1) {

		sample = (delta * nibbles[i])<<11;
		sample += ((long)hist * afccoef[idx][0]) + ((long)hist2 * afccoef[idx][1]);
		sample = sample >> 11;

		if(sample > 32767) {
			sample = 32767;
		}
		if(sample < -32768) {
			sample = -32768;
		}

        *(short*)dst = (short)sample;
        dst = dst + 2;

		hist2 = hist;
        hist = (short)sample;
        
    }
	*histp=hist;
	*hist2p=hist2;

    return((int)src);
}

// ADX decoder (from bero)
// decode 18 bytes of input (32 frames)

long BASE_VOL;
int ADXdecodebuffer(unsigned char *in, short *out, short *hist1, short *hist2)
{

  int scale = ( (in[0] << 8) | (in[1]) ) * BASE_VOL;
  int i;
  int s0, s1, s2, d;

  in += 2;


  s1 = *hist1;
  s2 = *hist2;

  for (i = 0; i < 16; i++)
  {
    d = in[i] >> 4;

    if (d & 8)
    {
      d -= 16;
    }

    s0 = (d*scale + 0x7298*s1 - 0x3350*s2) >> 14;
	
    if (s0 > 32767)
    {
      s0 = 32767;
    }
    else if (s0 < -32768)
    {
      s0 = -32768;
    }

	*out++ = s0;
  
    s2 = s1;
    s1 = s0;

    d = in[i] & 15;

    if (d & 8)
    {
      d -= 16;
    }

    s0 = (d*scale + 0x7298*s1 - 0x3350*s2) >> 14;

    if (s0 > 32767)
    {
      s0 = 32767;
    }
    else if (s0 < -32768)
    {
      s0 = -32768;
    }

  	*out++ = s0;
	
    s2 = s1;
    s1 = s0;

  }

  *hist1 = s1;
  *hist2 = s2;

  return 0;
   
}

// ADP decoder function by hcs, reversed from dtkmake (trkmake v1.4)
// this is pretty much XA, isn't it?

#define ONE_BLOCK_SIZE		32
#define SAMPLES_PER_BLOCK	28

short ADPDecodeSample( int bits, int q, long * hist1p, long * hist2p) {
	long hist,cur;
	long hist1=*hist1p,hist2=*hist2p;
	
	switch( q >> 4 )
	{
	case 0:
		hist = 0;
		break;
	case 1:
		hist = (hist1 * 0x3c);
		break;
	case 2:
		hist = (hist1 * 0x73) - (hist2 * 0x34);
		break;
	case 3:
		hist = (hist1 * 0x62) - (hist2 * 0x37);
		break;
	//default:
	//	hist = (q>>4)*hist1+(q>>4)*hist2; // a bit weird but it's in the code, never used
	}
	hist=(hist+0x20)>>6;
	if (hist >  0x1fffff) hist= 0x1fffff;
	if (hist < -0x200000) hist=-0x200000;

	cur = ( ( (short)(bits << 12) >> (q & 0xf)) << 6) + hist;
	
	*hist2p = *hist1p;
	*hist1p = cur;

	cur>>=6;

	if ( cur < -0x8000 ) return -0x8000;
	if ( cur >  0x7fff ) return  0x7fff;

	return (short)cur;
}

// decode 32 bytes of input (28 samples), assume stereo
int ADPdecodebuffer(unsigned char *input, short *outl, short * outr, long *histl1, long *histl2, long *histr1, long *histr2) {
	int i;
	for( i = 0; i < SAMPLES_PER_BLOCK; i++ )
	{
		outl[i] = ADPDecodeSample( input[i + (ONE_BLOCK_SIZE - SAMPLES_PER_BLOCK)] & 0xf, input[0], histl1, histl2 );
		outr[i] = ADPDecodeSample( input[i + (ONE_BLOCK_SIZE - SAMPLES_PER_BLOCK)] >> 4, input[1], histr1, histr2 );
	}
	return 0;
}

double VAG_f[5][2] = { { 0.0, 0.0 },
                   { 60.0 / 64.0, 0.0 },
		   { 115.0 / 64.0, -52.0 / 64.0 },
		   { 98.0 / 64.0, -55.0 / 64.0 } ,
		   { 122.0 / 64.00, -60.0 / 64.0 } } ;

int VAGdecodebuffer(unsigned char * input, short * out, short * hist1p, short * hist2p) {
    int predict_nr, shift_factor, flags;
    short hist1=*hist1p, hist2=*hist2p;

    int s;
    int i;
    predict_nr = input[0] >> 4;
    shift_factor = input[0] & 0xf;
    flags = input[1];
    for (i=0;i<28;i+=2) {
	s = (short)((input[i/2+2]&0xf)<<12);
	out[i]=(s >> shift_factor)+hist1*VAG_f[predict_nr][0]+hist2*VAG_f[predict_nr][1];
	hist2=hist1;
	hist1=out[i];
	s = (short)((input[i/2+2]>>4)<<12);
	out[i+1]=(s >> shift_factor)+hist1*VAG_f[predict_nr][0]+hist2*VAG_f[predict_nr][1];
	hist2=hist1;
	hist1=out[i+1];
    }

    *hist1p=hist1;
    *hist2p=hist2;

    return 0;
}
