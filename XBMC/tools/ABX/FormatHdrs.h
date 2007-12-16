#ifndef FORMAT_HDRS_H
#define FORMAT_HDRS_H

#include "stdafx.h"

/******************************
*		FormatHdrs.h
******************************/

// Container Formats

extern struct
{
	unsigned int	index[4];
	unsigned int	fileNameIndex[4];
	unsigned int   	fileNameIndexEnd;
	unsigned char	fileName[80];
	unsigned int	fileNameLength[4];
	unsigned long	padding[4];
}rcf;


extern struct
{
	unsigned long  	hdrPos;
	unsigned short	ver[2];
	unsigned long	padding[4];
	unsigned int	headerSize[4];
	unsigned int   	filenameDir[4];
	unsigned char	bankType[2];
	unsigned char	waveEntries[2];
	unsigned char	loopZeroTail[2];
	unsigned char	convertSample[2];
	unsigned int 	detailsLength[4];
	unsigned int 	offset1[4];
	bool			x360;
}xwb;




// Audio Formats


////Microsoft Adpcm
//struct
//{
//u_char 	header1[4];
//u_long 	size;
//u_char 	header2[8];
//u_int	sizeofFmtBlock;
//u_short	format;
//u_short	channels;
//u_short	sampleRate;
//u_int	bitRate;
//u_short	blockAlign;
//u_short	bits;
//u_short	unknown1; //Error
//u_short	unknown2;
//u_int 	unknown3;
//u_short unknown4;
//u_short	unknown5;
//u_short	unknown6;
//u_int	unknown7;
//u_int	unknown8;
//u_int	unknown9;
//u_int	unknown10;
//u_short	unknown11;
////u_char 	fact[4];
////u_int	factUnknown;
////u_char factChunk[4];
//u_char	header3[4];
//u_long	subChunk2Size;
//};






// Xbox Adpcm
extern struct 
{
u_char	header1[4];
u_long 	size;
u_char 	header2[8];
u_int 	sizeofFmtBlock;
u_short	format;
u_short	channels;
u_short	sampleRate;
u_int	bitRate;
u_short	blockAlign;
u_short	bits;
u_int	unknown;
u_char	header3[4];
u_long	subChunk2Size;
}xadpcm;



// Pcm
extern struct
{
u_char 	header1[4];
u_long 	size;
u_char 	header2[8];
u_long 	sizeofFmtBlock;
u_short	format;
u_short	channels;
u_short	sampleRate;
u_long	bitRate;
u_short	blockSize;
u_short	bits;
u_char 	header3[4];
u_long	subChunk2Size;								
}pcm;




// Rsd4 Pcm
extern struct
{
u_char	header[7];
u_char	fullHeader[128];
u_short	channelsOffset;
u_short	bitsOffset;
u_short	sampleRateOffset;
u_char	pcmPadding[1920];// Not economical
u_short	channels[2];
u_short	bits[2];
u_int	sampleRate[4];
}rsd4;





// Wma
extern struct
{
u_char header[16];
} wma;



// Video Formats


extern struct
{
unsigned char header[4];
}gxmv;


extern struct
{
unsigned char header[4];
}m1v;


extern struct
{
unsigned char header[4];
}sfd;

//#pragma pack()	

//struct
//{
//	DWORD       format      : 2;	// 0 = Pcm, 1 = Xbadpcm, 2 = Wma
//	DWORD       channels	: 3;	// Channels (1 - 6)
//	DWORD       sampleRate  : 26;	// SamplesPerSec
//	DWORD       bits  		: 1;	// Bits per sample (8 / 16) [PCM Only]
//}xwbMagic;

#endif