


/******************************
*		FormatHdrs.cpp
******************************/

#include "stdafx.h"



// Container Formats

struct
{
	unsigned int	index[4];
	unsigned int	fileNameIndex[4];
	unsigned int   	fileNameIndexEnd;
	unsigned char	fileName[80];
	unsigned int	fileNameLength[4];
	unsigned long	padding[4];
}rcf;


struct
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
//} msadpcm = {
//"RIFF", 	// Header1
//0, 			// Size
//"WAVEfmt ", // Header2
//50, 		// SizeofFmtBlock
//0x02, 		// Format
//0, 			// Channels
//0, 			// SampleRate
//0, 			// BitRate
//0,			// BlockAlign
//4,			// Bits
//0x20,		// Unknown1  PROBLEM
//0,			// Unknown2
////0x07,		// Unknown3
////256,		// Unknown4
//16777223,
//0,
//512,	// Unknown5
//65280,		// Unknown6
//12582912,	// Unknown7
//15728704,	// Unknown8
//30146560,	// Unknown9
//25755440,	// Unknown10
//65304,	   	// Unknown11    PROBLEM
//
////"fact",	// Fact
////4,		// FactUnknown
////0,		// FactChunk
//
//"data", 	// Header3
//0			// SubChunk2Size
//};






// Xbox Adpcm
//#pragma pack(1)
struct 
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
} xadpcm = {
'R', 'I', 'F', 'F',						// Header1
0, 										// Size
'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', // Header2
20, 									// SizeofFmtBlock
0x69, 									// Format
0, 										// Channels
0, 										// SampleRate
0, 										// BitRate
0,										// BlockAlign
4,										// Bits
4194306,								// Unknown
'd', 'a', 't', 'a', 					// Header3
0										// SubChunk2Size
};
//#pragma pack()



// Pcm
struct
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
} pcm = {
'R', 'I', 'F', 'F',						// Header1
0,										// Size
'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', // Header2
16,										// SizeofFmtBlock
0x01,									// Format
0,										// Channels
0,										// SampleRate
0,										// BitRate
0,										// BlockSize
0,										// Bits
'd', 'a', 't', 'a', 					// Header3
0										// SubChunk2Size
};




// Rsd4 Pcm
struct
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
} rsd4 = {
'R', 'S', 'D', '4', 'P', 'C', 'M',// Header

// FullHeader
0x52, 0x53, 0x44, 0x34, 0x50, 0x43, 0x4D, 0x20, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
0xC0, 0x5D, 0x00, 0x00, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A,

8,    // ChannelsOffset
12,   // BitsOffset
16    // SampleRateOffset
};





// Wma
struct
{
u_char header[16];
} wma = {
0x30, 0x26,  0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6,
0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C
};



// Video Formats


struct
{
unsigned char header[4];
}gxmv={0x47,0x58,0x4D,0x56};


struct
{
unsigned char header[4];
}m1v={0x00, 0x00, 0x01, 0xB3};


struct
{
unsigned char header[4];
}sfd={0x00, 0x00, 0x01, 0xBA};

//#pragma pack()	

//struct
//{
//	DWORD       format      : 2;	// 0 = Pcm, 1 = Xbadpcm, 2 = Wma
//	DWORD       channels	: 3;	// Channels (1 - 6)
//	DWORD       sampleRate  : 26;	// SamplesPerSec
//	DWORD       bits  		: 1;	// Bits per sample (8 / 16) [PCM Only]
//}xwbMagic;

