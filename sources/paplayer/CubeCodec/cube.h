/*

  in_cube Gamecube Stream Player for Winamp
  by hcs

  includes work by Destop and bero

*/

// CUBEFILE structure, prototypes for everything

#ifndef _CUBE_H

#define _CUBE_H

#define f32 float
#define u8	unsigned char
#define s16 signed short
#define u16 unsigned short
#define s32 signed long
#define u32 unsigned long

#define BPS 16

int get16bit(unsigned char* p);
int get32bit(unsigned char* p);
int get16bitL(unsigned char* p);
int get32bitL(unsigned char* p);

#include <stdio.h>

// both header and file type
typedef enum {
	type_std,   // devkit standard DSP
	type_sfass, // Star Fox Assault Cstr (DSP)
	type_mp2,   // Metroid Prime 2 RS03 (DSP)
	type_pm2,   // Paper Mario 2 STM (DSP)
	type_halp,  // HALPST (DSP)
	type_mp2d,  // Metroid Prime 2 Demo (DSP)
	type_idsp,  // IDSP (DSP)
	type_spt,   // SPT+SPD (DSP)
	type_mss,   // MSS (DSP)
	type_gcm,   // GCM (DSP)
	type_mpdsp, // Monopoly Party hack (DSP)
	type_ish,   // ISH+ISD (DSP)
	type_ymf,   // YMF (DSP)
	type_rsddsp,// RSD (DSP)
	type_idsp2, // IDSP (Harvest Moon - Another Wonderful Life) (DSP)
	type_gcub,  // GCub (DSP)
	type_wam,   // WAM (RIFF) (DSP)
	type_wvs,   // WVS (DSP)
	type_adx03, // ADX type 03
	type_adx04, // ADX type 04
	type_adp,   // ADP
	type_rsdpcm,// RSD (PCM)
	type_astpcm,// AST (PCM) (temporarily broken)
	type_whdpcm,// (from WHD, Hitman 2) (PCM)
	type_afc,   // AFC
	type_astafc, // AFC in an AST wrapper
	type_vag,   // VAG
	type_ads,   // ASD (VAG)
} filetype;

// structure for a single channel
typedef struct {
	FILE* infile; // processing on a single channel needs this,also this allows for seperate L/R files to be played at once
	
	// header data
	u32 num_samples;
	u32 num_adpcm_nibbles;
	u32 sample_rate;
	u16 loop_flag;
	u16 format;
	u32 sa,ea,ca;
	s16 coef[16];
	u16 gain; // never used anyway
	u16 ps,yn1,yn2;
	u16 lps,lyn1,lyn2;
	
	short chanbuf[0x8000/8*14];
	int readloc,writeloc; // offsets into the chanbuf
	filetype type;
	short bps; // bits per "size", 4 if offsets specified in nibbles, 8 if bytes
	u32 chanstart; // offset in file of start of this channel
	s32 offs; // current location
	s32 loopoffs;
	union { // sample history (either long or short, ADP uses long, others use short)
		long lhist1;
		short hist1;
	};
	union {
		long lhist2;
		short hist2;
	};
	long interleave; // _bytes_ of interleave, 0 if none
} CUBESTREAM;

// structure represents a DSP file
typedef struct {
	CUBESTREAM ch[2];
	int NCH;
	int ADXCH; // number of channels in an ADX (may be different from number being rendered)
	long nrsamples;
	long file_length;
	long nexthalp; // next HALPST header
	long halpsize;
	long samplesdone,loopsamplesdone,loopnexthalp,loophalpsize;
	int lastchunk;
	long startinterleave;
} CUBEFILE;

int InitDSPFILE(char * infile, CUBEFILE * dsp);
int InitADXFILE(char * infile, CUBEFILE * adx);
int InitADPFILE(char * infile, CUBEFILE * adp);
int InitPCMFILE(char * infile, CUBEFILE * pcm);
int InitAFCFILE(char * infile, CUBEFILE * afc);
int InitVAGFILE(char * infile, CUBEFILE * vag);

int InitCUBEFILE(char * fn, CUBEFILE * cf);
void CloseCUBEFILE(CUBEFILE * dsp);

int CheckSampleRate(int sr);

int DSPdecodebuffer
(
    u8			*input, // location of encoded source samples
    s16         *out,   // location of destination buffer (16 bits / sample)
    short		coef[16],   // location of decode coefficients
	short * histp,
	short * hist2p
);


int AFCdecodebuffer
(
    u8			*input, // location of encoded source samples
    s16         *out,   // location of destination buffer (16 bits / sample)
    short * histp,
	short * hist2p
);

int ADXdecodebuffer
(
    u8			*input, // location of encoded source samples
    s16         *out,   // location of destination buffer (16 bits / sample)
	short * histp,
	short * hist2p
);

int ADPdecodebuffer(
	unsigned char *input,
	short *outl,
	short *outr,
	long *histl1,
	long *histl2,
	long *histr1,
	long *histr2
);

int VAGdecodebuffer(
	unsigned char * input,
	short * out,
	short * hist1p,
	short * hist2p
);

void fillbuffers(CUBEFILE * dsp);

void fillbufferDSP(CUBESTREAM * stream);
void fillbufferDSPinterleave(CUBEFILE * stream);
void fillbufferHALP(CUBEFILE * stream);
void fillbufferADX(CUBEFILE * adx);
void fillbufferADP(CUBEFILE * adp);
void fillbufferPCM(CUBEFILE * pcm);
void fillbufferAFC(CUBEFILE * afc);
void fillbufferASTAFC(CUBEFILE * pcm);
void fillbufferASTPCM(CUBEFILE * pcm);
void fillbufferPCMinterleave(CUBEFILE * pcm);
void fillbufferVAG(CUBEFILE * afc);
void fillbufferVAGinterleave(CUBEFILE * afc);

// configurable ADX parameters
extern long BASE_VOL;
extern int adxonechan;

#endif

