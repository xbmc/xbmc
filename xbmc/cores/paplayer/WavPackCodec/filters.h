//////////////////////////////////////////////////////
// GENERAL PURPOSE DEFS FOR CREATING CUSTOM FILTERS //
//////////////////////////////////////////////////////


typedef struct riffspecialdata_t
{	HANDLE hSpecialData;	
	HANDLE hData;			// Actual data handle
	DWORD  dwSize;			// size of data in handle
	DWORD  dwExtra;			// optional extra data (usually a count)
	char   szListType[8];	// Parent list type (usually "WAVE" or "INFO", or "adtl")
	char   szType[8];		// Usually a four character code for data, but can be up to 7 chars
} SPECIALDATA;

// "CUE " dwExtra=number of cues, each cue is 8 bytes		([4] name [4] sample offset)
// "LTXT" dwExtra=number of items, each one is 8 bytes		([4] ltxt len [4] name [4] cue length [4] purpose [n] data)
// "NOTE" dwExtra=number of strings, each one is n bytes	([4] name [n-4] length zero term)
// "LABL" dwExtra=number of strings, each one is n bytes	([4] name [n-4] length zero term)
// "PLST" dwExtra=number if items, each one is 16 bytes		([4] name [4] dwLen [4] dwLoops [4] dwMode)


// For special data, .FLT must implement FilterGetFirstSpecialData and FilterGetNextSpecialData


typedef DWORD           FOURCC;         // a four character code

struct cue_type { DWORD dwName;
				  DWORD dwPosition;
				  FOURCC fccChunk;
				  DWORD dwChunkStart;
				  DWORD dwBlockStart;
				  DWORD dwSampleOffset;
				 };

struct play_type {DWORD dwName;
					 DWORD dwLength;
					 DWORD dwLoops;
					};


typedef struct coolquery_tag
	{char szName[24];
	 char szCopyright[80];
	 
	 // rate table, bits are set for modes that can be handled
	 WORD Quad32;  // Quads are 3-D encoded
	 WORD Quad16;
	 WORD Quad8;
	 WORD Stereo8;    		// rates are from lowest bit:
	 WORD Stereo12;   		// bit 0 set: 5500 (5512.5)
	 WORD Stereo16;   		// bit 1 set: 11025 (11K)
	 WORD Stereo24;   		// bit 2 set: 22050 (22K)
	 WORD Stereo32;   		// bit 3 set: 32075 (32K, or 32000)
	 WORD Mono8;      		// bit 4 set: 44100 (44K)
	 WORD Mono12;	  		// bit 5 set: 48000 (48K)
	 WORD Mono16;	  		// bit 6 set: 88200 (88K)   (future ultra-sonic rates?)
	 WORD Mono24;	  		// bit 7 set: 96000 (96K)
	 WORD Mono32;     		// bit 8 set: 132300 (132K)
	 				  		// bit 9 set: 176400 (176K)
	 DWORD dwFlags;
	 char szExt[4];
	 long lChunkSize;
	 char szExt2[4];
	 char szExt3[4];
	 char szExt4[4];
	} COOLQUERY;

#define R_5500   1
#define R_11025  2
#define R_22050  4
#define R_32075  8
#define R_44100  16
#define R_48000  32
#define R_88200  64
#define R_96000  128
#define R_132300 256
#define R_176400 512

#define C_VALIDLIBRARY 1154

#define QF_RATEADJUSTABLE		0x001   // if can handle non-standard sample rates
										// if not, only rates in bit rate table understood
#define QF_CANSAVE				0x002		  
#define QF_CANLOAD				0x004
#define QF_UNDERSTANDSALL		0x008   // will read ANYTHING, so it is the last resort if no other
										// formats match
#define QF_READSPECIALFIRST		0x010	// read special info before trying to read data
#define QF_READSPECIALLAST		0x020	// read special info after reading data
#define QF_WRITESPECIALFIRST	0x040	// when writing a file, special info is sent to DLL before data
#define QF_WRITESPECIALLAST		0x080	// when writing, special info is sent to DLL after data
#define QF_HASOPTIONSBOX		0x100	// set if options box implemented
#define QF_NOASKFORCONVERT		0x200	// set to bypass asking for conversion if original in different rate, auto convert
#define QF_NOHEADER				0x400	// set if this is a raw data format with no header
#define QF_CANDO32BITFLOATS		0x800	// set if file format can handle 32-bit sample data for input
#define QF_CANOPENVIRTUAL		0x1000	// Set if data is in Intel 8-bit or 16-bit sample format, or floats
										// and the GetDataOffset() function is implemented

// special types are read from and written to DLL in the order below
/*
// special types (particular to Windows waveforms)
#define SP_IART  20
#define SP_ICMT  21
#define SP_ICOP  22
#define SP_ICRD  23
#define SP_IENG  24
#define SP_IGNR  25
#define SP_IKEY  26
#define SP_IMED  27
#define SP_INAM  28
#define SP_ISFT  29
#define SP_ISRC  30
#define SP_ITCH  31
#define SP_ISBJ  32
#define SP_ISRF  33
#define SP_DISP  34
#define SP_CUE   40 // returns number of cues of size cue_type 
#define SP_LTXT  41 // returns number of adtl texts of size 8 (4,id and 4,len)
#define SP_NOTE  42 // returns LO=size, HI=number of strings (sz sz sz...)
#define SP_LABL	 43 // returns LO=size, HI=number of strings (sz sz sz...)
#define SP_PLST  44 // returns number of playlist entries size play_type 
*/