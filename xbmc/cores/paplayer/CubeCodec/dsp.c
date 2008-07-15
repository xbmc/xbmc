/*

  in_cube Gamecube Stream Player for Winamp
  by hcs

  includes work by Destop and bero

*/

// DSP family

#ifndef _LINUX
#include <windows.h>
#include <io.h>
#else
#include <string.h>
#include <stdlib.h>
#define MAX_PATH 256
#define strcmpi strcasecmp
#endif
#include "cube.h"
#include "wamain.h"

// standard devkit version
void get_dspheaderstd(CUBESTREAM *dsp,unsigned char *buf)
{
	int i;
	dsp->num_samples=get32bit(buf);
	dsp->num_adpcm_nibbles=get32bit(buf+4);
	dsp->sample_rate=get32bit(buf+8);
	dsp->loop_flag=get16bit(buf+0xC);
	dsp->format=get16bit(buf+0xE);
	dsp->sa=get32bit(buf+0x10);
	dsp->ea=get32bit(buf+0x14);
	dsp->ca=get32bit(buf+0x18);
	DisplayError("get_dspheaderstd:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
		"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
		dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x1C+i*2);
	dsp->gain=get16bit(buf+0x3C);
	dsp->ps=get16bit(buf+0x3E);
	dsp->yn1=get16bit(buf+0x40);
	dsp->yn2=get16bit(buf+0x42);
	dsp->lps=get16bit(buf+0x44);
	dsp->lyn1=get16bit(buf+0x46);
	dsp->lyn2=get16bit(buf+0x48);
}

// SF Assault version
void get_dspheadersfa(CUBESTREAM *dsp,unsigned char *buf)
{
	int i;
	buf+=0x20; // for SF Assault
	dsp->num_samples=get32bit(buf);
	dsp->num_adpcm_nibbles=get32bit(buf+4);
	dsp->sample_rate=get32bit(buf+8);
	dsp->loop_flag=get16bit(buf+0xC);
	dsp->format=get16bit(buf+0xE);
	dsp->sa=get32bit(buf+0x10);
	dsp->ea=get32bit(buf+0x14);
	dsp->ca=get32bit(buf+0x18);
	//DisplayError("get_dspheadersfa:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
	//"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
	//	dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x1C+i*2);
	dsp->gain=get16bit(buf+0x3C);
	dsp->ps=get16bit(buf+0x3E);
	dsp->yn1=get16bit(buf+0x40);
	dsp->yn2=get16bit(buf+0x42);
	dsp->lps=get16bit(buf+0x44);
	dsp->lyn1=get16bit(buf+0x46);
	dsp->lyn2=get16bit(buf+0x48);
}

// Metroid Prime 2 version
void get_dspheadermp2(CUBESTREAM *dsp,unsigned char *buf)
{
	int i;
	dsp->num_samples=get32bit(buf+0x8);
	dsp->num_adpcm_nibbles=get32bit(buf+0x10);
	dsp->sample_rate=get32bit(buf+0x0c);
	dsp->loop_flag=get16bit(buf+0x14);
	dsp->format=get16bit(buf+0xE);
	dsp->sa=get32bit(buf+0x18);
	dsp->ea=get32bit(buf+0x1c);
	DisplayError("get_dspheadermp2:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
		"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
		dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x20+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// Metroid Prime 2 version (second channel)
void get_dspheadermp22(CUBESTREAM *dsp,unsigned char *buf)
{
	int i;
	dsp->num_samples=get32bit(buf+0x8);
	dsp->num_adpcm_nibbles=get32bit(buf+0x10);
	dsp->sample_rate=get32bit(buf+0x0c);
	dsp->loop_flag=get16bit(buf+0x14);
	dsp->format=get16bit(buf+0xE);
	dsp->sa=get32bit(buf+0x18);
	dsp->ea=get32bit(buf+0x1c);
	DisplayError("get_dspheadermp22:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
		"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
		dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x40+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// SSB:M HALPST version
void get_dspheaderhalp(CUBESTREAM *dsp,unsigned char *buf)
{
	int i;
	dsp->num_samples=get32bit(buf+0x18)*14/16; // I'm using the same as the loop endpoint...
	dsp->num_adpcm_nibbles=get32bit(buf+0x18)*2; // ditto
	dsp->sample_rate=get32bit(buf+0x08);
	dsp->sa=get32bit(buf+0x14);
	dsp->ea=get32bit(buf+0x18);
	//DisplayError("get_dspheaderhalp:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
	//	"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
	//	dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x20+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// SSB:M HALPST version (second channel)
void get_dspheaderhalp2(CUBESTREAM *dsp,unsigned char *buf)
{
	int i;
	dsp->num_samples=get32bit(buf+0x50)*14/16; // I'm using the same as the loop endpoint...
	dsp->num_adpcm_nibbles=get32bit(buf+0x50)*2; // ditto
	dsp->sample_rate=get32bit(buf+0x08);
	dsp->sa=get32bit(buf+0x4c);
	dsp->ea=get32bit(buf+0x50);
	//DisplayError("get_dspheaderhalp2:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
	//	"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
	//	dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x58+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// Metroid Prime 2 demo's stereo fmt
void get_dspheadermp2d(CUBESTREAM *dsp,unsigned char *buf) {
	int i;
	dsp->num_samples=get32bit(buf+0xc);
	dsp->num_adpcm_nibbles=get32bit(buf+0xc)*2;
	dsp->sample_rate=get32bit(buf+0x8);
	dsp->loop_flag=get16bit(buf+0x10);
	dsp->format=get16bit(buf+0x12);
	dsp->sa=get32bit(buf+0);
	dsp->ea=get32bit(buf+4);
	//DisplayError("get_dspheadermp2d:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
	//	"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
	//	dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x1c+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// Metroid Prime 2 demo's stereo fmt (chan 2)
void get_dspheadermp2d2(CUBESTREAM *dsp,unsigned char *buf) {
	int i;
	dsp->num_samples=get32bit(buf+0x18);
	dsp->num_adpcm_nibbles=get32bit(buf+0x18)*2;
	dsp->sample_rate=get32bit(buf+0x8);
	dsp->loop_flag=get16bit(buf+0x10);
	dsp->format=get16bit(buf+0x12);
	dsp->sa=get32bit(buf+0);
	dsp->ea=get32bit(buf+4);
	//DisplayError("get_dspheadermp2d2:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
	//	"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
	//	dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x3c+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// spt header (seperate file)
void get_dspheaderspt(CUBESTREAM *dsp,unsigned char *buf)
{
	int i;
	//dsp->num_samples=get32bit(buf);
	//dsp->num_adpcm_nibbles=get32bit(buf+4);
	dsp->sample_rate=get32bit(buf+8);
	dsp->loop_flag=get32bit(buf+4); // "type" field, 0=nonlooped ADPCM, 1=looped ADPCM
	dsp->format=0; //get16bit(buf+0xE);
	dsp->sa=get32bit(buf+0x0C);
	dsp->ea=get32bit(buf+0x10);
	//dsp->ea=get32bit(buf+0x14);
	
	dsp->ca=get32bit(buf+0x18);
	DisplayError("get_dspheaderspt:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
		"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
		dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x20+i*2);
	dsp->gain=get16bit(buf+0x40);
	dsp->ps=get16bit(buf+0x42);
	dsp->yn1=get16bit(buf+0x44);
	dsp->yn2=get16bit(buf+0x46);
	dsp->lps=get16bit(buf+0x48);
	dsp->lyn1=get16bit(buf+0x4A);
	dsp->lyn2=get16bit(buf+0x4C);
}

// ish (I_SF) header (no point in two fcns...)
void get_dspheaderish(CUBESTREAM *dsp1, CUBESTREAM *dsp2, unsigned char * buf) {
	int i;
	dsp1->sample_rate=get32bit(buf+0x08);
	dsp1->num_samples=get32bit(buf+0x0c);
	dsp1->num_adpcm_nibbles=get32bit(buf+0x10);
	dsp1->ca=get32bit(buf+0x14);
	// 0x00008000 at 0x18 might be interleave
	dsp1->loop_flag=get16bit(buf+0x1E);
	dsp1->sa=get32bit(buf+0x20);
	dsp1->ea=get32bit(buf+0x24);
	//dsp1->ea=get32bit(buf+0x28);
	memcpy(dsp2,dsp1,sizeof(CUBESTREAM));

	for (i=0;i<16;i++)
	dsp1->coef[i]=get16bit(buf+0x40+i*2);
	dsp1->ps=get16bit(buf+0x62);
	dsp1->yn1=get16bit(buf+0x64);
	dsp1->yn2=get16bit(buf+0x66);
	dsp1->lps=get16bit(buf+0x68);
	dsp1->lyn1=get16bit(buf+0x6A);
	dsp1->lyn2=get16bit(buf+0x6C);

	for (i=0;i<16;i++)
	dsp2->coef[i]=get16bit(buf+0x80+i*2);
	dsp2->ps=get16bit(buf+0xA2);
	dsp2->yn1=get16bit(buf+0xA4);
	dsp2->yn2=get16bit(buf+0xA6);
	dsp2->lps=get16bit(buf+0xA8);
	dsp2->lyn1=get16bit(buf+0xAA);
	dsp2->lyn2=get16bit(buf+0xAC);
}

// ymf

void get_dspheaderymf(CUBESTREAM * dsp, unsigned char * buf) {
	int i;
	
	//memset(dsp,0,sizeof(CUBESTREAM));
	//dsp->
	dsp->loop_flag=0;
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
	dsp->sample_rate=get32bit(buf+0x08);
	dsp->num_samples=get32bit(buf+0x3c);
	dsp->num_adpcm_nibbles=get32bit(buf+0x40);
	
	for (i=0;i<16;i++) dsp->coef[i]=get16bit(buf + 0x0e + i * 2);
}

// rsd (GC ADPCM)
void get_dspheaderrsd(CUBESTREAM * dsp, unsigned char * buf) {
	int i;
	
	dsp->loop_flag=0;

	dsp->sample_rate=get32bitL(buf+0x10);
	
	for (i=0;i<16;i++) dsp->coef[i]=get16bitL(buf+0x1c+2*i);
	
	//gain=get16bitL(buf+0x3c);
	dsp->ps=get16bitL(buf+0x3e);
	dsp->yn1=get16bitL(buf+0x40);
	dsp->yn2=get16bitL(buf+0x42);

	dsp->lps=get16bitL(buf+0x44);
	dsp->lyn1=get16bitL(buf+0x46);
	dsp->lyn2=get16bitL(buf+0x48);
}

// GCub
void get_dspheadergcub(CUBESTREAM * dsp1, CUBESTREAM * dsp2, unsigned char * buf) {
    int i;
    dsp1->loop_flag=dsp2->loop_flag=0;
    dsp1->sample_rate=dsp2->sample_rate=get32bit(buf+8);
    dsp1->num_adpcm_nibbles=dsp2->num_adpcm_nibbles=get32bit(buf+12)/2*2;
    dsp1->num_samples=dsp2->num_samples=get32bit(buf+12)/2*14/8;
    for (i=0;i<16;i++) dsp1->coef[i]=get16bit(buf+0x10+(i*2));
    for (i=0;i<16;i++) dsp2->coef[i]=get16bit(buf+0x30+(i*2));
}

long mp2round(long addr) {
	return (addr%0x8f00)+(addr/0x8f00*2*0x8f00);
}

long mp2roundup(long addr) {
	return (0x8f00+addr%0x8f00)+(addr/0x8f00*2*0x8f00);
}

// call with infile = NULL if file is already opened
// return 1 on failure, 0 on success
int InitDSPFILE(char * inputfile, CUBEFILE * dsp) {
	unsigned char readbuf[0x180];
	char infile[MAX_PATH*2],infile2[MAX_PATH*2]; // file name, second file name (for dual-file stereo)
	char * ext, * ext2; // file extension
	int l;
	int IDSP=0; // IDSP header detected?
	int IDSP2=0; // another file called IDSP w/ variable interleave
	int MSS=0; // MSS (standard stereo, 0x1000 interleave)
	int MPDSP=0; // MPDSP (Monopoly Party, single header stereo)
	int GCM=0; // GCM (standard stereo, 0x8000 interleave)
	int SPT=0; // seperate header file (SPT) detected
	int YMF=0; // YMF
	int WVS=0; // WVS
	int dfs=-1; // dual-file stereo (0 if we have the name of left channel, 1 if right)

	dsp->ch[0].interleave=dsp->ch[1].interleave=0;

	if (inputfile) {
		strcpy(infile,inputfile);
		//dsp->ch[0].infile=dsp->ch[1].infile=INVALID_HANDLE_VALUE;

		ext=strrchr(infile,'.')+1;
		if (ext!=(char*)1) { // extension-specific handling
			//DisplayError("ex=%s",ext);
			if (!strcmpi(ext,"spt")) { // we've been passed the header
				dsp->ch[0].infile=fopen(infile,"rb");
				if (!dsp->ch[0].infile) return 1;
				
				fread(readbuf, 0x4e, 1, dsp->ch[0].infile);
				get_dspheaderspt(&dsp->ch[0],readbuf);

        fclose(dsp->ch[0].infile);

				ext[2]='d'; // open the SPD next
				SPT=1;
			} else if (!strcmpi(ext,"spd")) {
				ext[2]='t';

				dsp->ch[0].infile=fopen(infile,"rb");
				if (!dsp->ch[0].infile) return 1;
				
				fread(readbuf, 0x4e, 1, dsp->ch[0].infile);
				get_dspheaderspt(&dsp->ch[0],readbuf);

				fclose(dsp->ch[0].infile);

				ext[2]='d';

				SPT=1;
			} else if (!strcmpi(ext,"mss")) {
				MSS=1;
			} else if (!strcmpi(ext,"gcm")) {
				GCM=1;
			} else if (!strcmpi(ext,"mpdsp")) {
				MPDSP=1;
			} else if (!strcmpi(ext,"ymf")) {
				YMF=1;
			} else if (!strcmpi(ext,"wvs")) {
				WVS=1;
			}
		}

		dsp->ch[0].infile=fopen(infile,"rb");

		if (!dsp->ch[0].infile) return 1;

		// Dual file stereo
		strcpy(infile2,infile);
		ext2=ext-infile+infile2;
		ext2[-1]='\0';
		if (!strcmpi(ext2-2,"L")) { // L/R, Metroid Prime, etc
			ext2[-2]='R';
			dfs=0;
		} else if (!strcmpi(ext2-2,"R")) {
			ext2[-2]='L';
			dfs=1;
		} else if (!strcmpi(ext2-3,"_0")) { // _0/_1, Wario World
			ext2[-2]='1';
			dfs=0;
		} else if (!strcmpi(ext2-3,"_1")) {
			ext2[-2]='0';
			dfs=1;
		} else if (!strcmpi(ext2-5,"left")) { // left/right, NFL Blitz 2003
			strcpy(ext2-5,"right");
			strcpy(ext2+1,ext);
			dfs=0;
		} else if (!strcmpi(ext2-6,"right")) {
			strcpy(ext2-6,"left");
			strcpy(ext2-1,ext);
			dfs=1;
		}
		infile2[strlen(infile2)]='.';
		
		if (dfs==0) { // left channel already opened, load right
			dsp->ch[1].infile=fopen(infile2,"rb");
			
			if (!dsp->ch[1].infile) dsp->ch[1].infile=dsp->ch[0].infile;
		} else if (dfs==1) { // right channel already opened, load left
			dsp->ch[1].infile=dsp->ch[0].infile;
			dsp->ch[0].infile=fopen(infile2,"rb");
			
			if (!dsp->ch[0].infile) dsp->ch[0].infile=dsp->ch[1].infile;
		} else dsp->ch[1].infile=dsp->ch[0].infile;

	}// else return 1;

	fseek(dsp->ch[0].infile,0,SEEK_SET);

	if (!SPT) fread(readbuf, 0x180, 1, dsp->ch[0].infile);

	if (SPT) {
		// SPT+SPD
		
		// only play single archives.
		if (get32bit(readbuf)!=1) {
			CloseCUBEFILE(dsp);
			return 1;
		}

    fseek(dsp->ch[0].infile,0,SEEK_END);
		dsp->ch[0].num_adpcm_nibbles=dsp->ch[1].num_adpcm_nibbles=
			ftell(dsp->ch[0].infile)*2;
		dsp->ch[0].num_samples=dsp->ch[1].num_samples=
			ftell(dsp->ch[0].infile)*14/8;

		dsp->NCH=1;
		dsp->ch[0].chanstart=0;
		dsp->ch[0].type=type_spt;
		dsp->ch[0].bps=4;
	} else if (YMF) {
		// YMF

		get_dspheaderymf(&dsp->ch[0],readbuf+get32bit(readbuf+0x34));
		get_dspheaderymf(&dsp->ch[1],readbuf+get32bit(readbuf+0x34)+0x60);

		dsp->NCH=2;
		dsp->ch[0].interleave=dsp->ch[1].interleave=0x20000;
		dsp->ch[0].chanstart=get32bit(readbuf+0);
		dsp->ch[1].chanstart=get32bit(readbuf+0)+dsp->ch[0].interleave;
		dsp->ch[0].bps=dsp->ch[1].bps=8;
		dsp->ch[0].type=dsp->ch[1].type=type_ymf;
    dsp->ch[1].infile = fopen(inputfile,"rb");
	} else if (WVS && !memcmp("\0\0\0\x2",readbuf,4)) {
		// WVS

		int i;
		dsp->ch[1].sample_rate=dsp->ch[0].sample_rate=32000;
		dsp->NCH=get32bit(readbuf);
		dsp->ch[0].interleave=dsp->ch[1].interleave=get32bit(readbuf+0xc);
		dsp->ch[0].chanstart=0x60;
		dsp->ch[1].chanstart=0x60+dsp->ch[0].interleave;

		dsp->ch[1].loop_flag=dsp->ch[0].loop_flag=0;

		dsp->ch[1].num_adpcm_nibbles=dsp->ch[0].num_adpcm_nibbles=get32bit(readbuf+0x14);
		dsp->ch[1].num_samples=dsp->ch[0].num_samples=dsp->ch[0].num_adpcm_nibbles*14/8;

		for (i=0;i<16;i++) dsp->ch[0].coef[i]=get16bit(readbuf+(0x18)+i*2);
		for (i=0;i<16;i++) dsp->ch[1].coef[i]=get16bit(readbuf+(0x38)+i*2);

		dsp->ch[0].bps=dsp->ch[1].bps=8;
		dsp->ch[0].type=dsp->ch[1].type=type_wvs;
	} else if (!memcmp("RS\x00\x03",readbuf,4)) {
		// Metroid Prime 2 "RS03"
		if (get16bit(readbuf+6)==2) { // channel count
			get_dspheadermp2(&dsp->ch[0],readbuf);
			get_dspheadermp22(&dsp->ch[1],readbuf);

			dsp->ch[0].sa=mp2round(dsp->ch[0].sa);
			dsp->ch[1].sa=mp2round(dsp->ch[1].sa);

			dsp->ch[0].ea*=2;
			dsp->ch[1].ea*=2;

      fseek(dsp->ch[0].infile,0,SEEK_END);
			if (ftell(dsp->ch[0].infile)-dsp->ch[0].ea > 0x8f00/2)
				dsp->ch[0].ea=dsp->ch[1].ea=mp2roundup(dsp->ch[0].ea/2);

			dsp->NCH=2;
			dsp->ch[0].interleave=dsp->ch[1].interleave=0x8f00;

			dsp->ch[0].chanstart=0x60;
			dsp->ch[1].chanstart=0x8f60;

			dsp->ch[0].type=dsp->ch[1].type=type_mp2;
			dsp->ch[0].bps=dsp->ch[1].bps=8;
      dsp->ch[1].infile = fopen(inputfile,"rb");
		} else {
			// mono variant, haven't seen any of these loop
			get_dspheadermp2(&dsp->ch[0],readbuf);
			
			dsp->ch[0].sa*=2;
			
			dsp->NCH=1;
			dsp->ch[0].interleave=0;

			dsp->ch[0].chanstart=0x60;

			dsp->ch[0].type=type_mp2;
			dsp->ch[0].bps=dsp->ch[1].bps=4;
		}
	} else if (!memcmp("Cstr",readbuf,4)) {
		// Star Fox Assault "Cstr"
		get_dspheadersfa(&dsp->ch[0],readbuf);
		get_dspheadersfa(&dsp->ch[1],readbuf+0x60);

		// weird but needed
		dsp->ch[0].sa*=2;

		// second header has odd values for sa and ea
		dsp->ch[1].ea=dsp->ch[0].ea;
		dsp->ch[1].sa=dsp->ch[0].sa;

		dsp->NCH=2;
		dsp->ch[0].interleave=dsp->ch[1].interleave=0x800;

		dsp->ch[0].chanstart=0xe0;
		dsp->ch[1].chanstart=0x8e0;

		dsp->ch[0].type=dsp->ch[1].type=type_sfass;
		dsp->ch[0].bps=dsp->ch[1].bps=8;
	} else if (!memcmp("\x02\x00",readbuf,2) && !memcmp(readbuf+2,readbuf+0x4a,2)) { // srate is in STM header
		// Paper Mario 2 "STM"
		get_dspheaderstd(&dsp->ch[0],readbuf+0x40);
		get_dspheaderstd(&dsp->ch[1],readbuf+0xa0);
		
		dsp->NCH=2;
		dsp->ch[0].interleave=dsp->ch[1].interleave=0;

		dsp->ch[0].chanstart=0x100;
		dsp->ch[1].chanstart=0x100+get32bit(readbuf+8); // the offset of chan2 is in several places...

		// easy way to detect mono form
		if (dsp->ch[0].num_adpcm_nibbles!=dsp->ch[1].num_adpcm_nibbles ||
			dsp->ch[0].num_samples!=dsp->ch[1].num_samples) {
			dsp->NCH=1;
			dsp->ch[0].chanstart=0xa0;
		}

		dsp->ch[0].type=dsp->ch[1].type=type_pm2;
		dsp->ch[0].bps=dsp->ch[1].bps=4;
	} else if (!memcmp(" HALPST",readbuf,7)) {
		// Super Smash Bros. Melee "HALPST"
		get_dspheaderhalp(&dsp->ch[0],readbuf);
		get_dspheaderhalp2(&dsp->ch[1],readbuf);

		dsp->NCH=2;
		dsp->ch[0].interleave=dsp->ch[1].interleave=0x8000;
		
		dsp->ch[0].chanstart=0x80;
		dsp->ch[1].chanstart=0x80;
		
		dsp->ch[0].type=dsp->ch[1].type=type_halp;
		dsp->ch[0].bps=dsp->ch[1].bps=8;

		dsp->nexthalp=0x80;
		dsp->halpsize=0;

		// determine if a HALPST file loops
		{
			long c=0x80,lastc=0;
			while (c > lastc) {
				lastc=c;
				fseek(dsp->ch[0].infile,c+8,SEEK_SET);
				fread(&c,4,1,dsp->ch[0].infile);
				c=get32bit((char*)&c);
			}
			dsp->ch[0].loop_flag=dsp->ch[1].loop_flag=((c<0)?0:1);
      dsp->ch[1].infile = fopen(infile,"rb");
    }

	} else if (!memcmp("I_SF ",readbuf,4)) {
#ifndef _LINUX
		// ISH+ISD (I_SF in header)
		struct _finddata_t finddata;
		long findhandle;
		int isdfound;
		char *t;

		fclose(dsp->ch[0].infile);
		dsp->ch[0].infile=dsp->ch[1].infile=NULL;

		// read header

		get_dspheaderish(&dsp->ch[0],&dsp->ch[1],readbuf);

		dsp->NCH=2;
		dsp->ch[0].interleave=dsp->ch[1].interleave=0x8000;
		dsp->ch[0].chanstart=0;
		dsp->ch[1].chanstart=0x8000;

		dsp->ch[0].type=dsp->ch[1].type=type_ish;
		dsp->ch[0].bps=dsp->ch[1].bps=8;

		// have to screw with the loop start point
		dsp->ch[0].sa=(dsp->ch[0].sa&0xffff)/2 | (dsp->ch[0].sa&0xffff0000);
								
		dsp->ch[1].sa=dsp->ch[0].sa;

		// attempt to open data (ISD) file, should have same name, diff ext.

		strcpy(infile2,infile);
		ext=strrchr(infile2,(char)'/');
		ext[1]='*';
    ext[2]='.';
    ext[3]='*';
		ext[4]='\0';

		findhandle=_findfirst(infile2,&finddata);

		if (findhandle == -1) {
			CloseCUBEFILE(dsp);
			return 1;
		}

		isdfound=0;
		do {
      if (finddata.size==(dsp->ch[0].num_adpcm_nibbles+0xffff)/0x10000*0x10000) {isdfound=1; break;}
		} while (_findnext(findhandle,&finddata)==0);

		if (!isdfound) {
			CloseCUBEFILE(dsp);
			return 1;
		}

		t=strrchr(infile2,'\\');
		if (!t) 
      t=strrchr(infile2,'/');
    if (!t) 
      t=infile2;
		else t++;
		strcpy(t,finddata.name);

    printf("%s",infile2);
		dsp->ch[0].infile=dsp->ch[1].infile=fopen(infile2,"rb");

		if (!dsp->ch[0].infile) {
			CloseCUBEFILE(dsp);
			return 1;
		}
#else
	return 1;
#endif
	} else if (!memcmp("RSD3GADP",readbuf,8)) {
		// RSD (ADPCM type)

		get_dspheaderrsd(&dsp->ch[0],readbuf);

		dsp->ch[0].chanstart=get32bitL(readbuf+0x18);

    fseek(dsp->ch[0].infile,0,SEEK_END);
		dsp->ch[0].num_adpcm_nibbles=dsp->ch[1].num_adpcm_nibbles=
			(ftell(dsp->ch[0].infile)-dsp->ch[0].chanstart)*2;
		dsp->ch[0].num_samples=dsp->ch[1].num_samples=
			(ftell(dsp->ch[0].infile)-dsp->ch[0].chanstart)*7/4;

		dsp->ch[0].bps = dsp->ch[1].bps = 4;

		dsp->NCH=1;
		dsp->ch[0].interleave=0;

		dsp->ch[0].type=type_rsddsp;
	} else if (!memcmp("GCub",readbuf,4)) {
		// GCub

        get_dspheadergcub(&dsp->ch[0],&dsp->ch[1],readbuf);

        dsp->ch[0].chanstart=dsp->ch[1].chanstart=0x60;
        dsp->ch[0].bps=dsp->ch[1].bps=4;
        dsp->NCH=2;
        dsp->ch[0].interleave=dsp->ch[1].interleave=0x8000;


        dsp->ch[0].type=dsp->ch[1].type=type_gcub;
	} else if (!memcmp("RIFF",readbuf,4) && !memcmp("WAVEfmt ",readbuf+8,8) && !memcmp("\xfe\xff",readbuf+0x14,2)) {
		int i,l;

		dsp->NCH=get16bitL(readbuf+0x16);
		dsp->ch[0].sample_rate=get32bitL(readbuf+0x18);
		dsp->ch[0].chanstart=0x5c;
		dsp->ch[0].num_adpcm_nibbles=get32bitL(readbuf+0x2a)/dsp->NCH-0x2a;
		dsp->ch[0].num_samples=get32bitL(readbuf+0x2a)/dsp->NCH*14/8;
		dsp->ch[0].loop_flag=dsp->ch[1].loop_flag=1;
		dsp->ch[1].ea=dsp->ch[0].ea=dsp->ch[0].num_adpcm_nibbles;
		dsp->ch[1].sa=dsp->ch[0].sa=0;

		for (i=0;i<16;i++) dsp->ch[0].coef[i]=get16bit(readbuf+(0x2e)+i*2);

		if (dsp->NCH==2) {
			char coeffs[0x20];

			dsp->ch[1].num_adpcm_nibbles=dsp->ch[0].num_adpcm_nibbles;
			dsp->ch[1].num_samples=dsp->ch[0].num_samples;
			
			dsp->ch[1].sample_rate=dsp->ch[0].sample_rate;
			dsp->ch[1].chanstart=0x58+get32bitL(readbuf+0x2a)/2+0x32;

			fseek(dsp->ch[0].infile,0x58+get32bitL(readbuf+0x2a)/2+4,SEEK_SET);
      fread(coeffs,0x20,1,dsp->ch[0].infile);

			for (i=0;i<16;i++) dsp->ch[1].coef[i]=get16bit(coeffs+i*2);

		}

		dsp->ch[0].bps=8;
		dsp->ch[1].bps=8;

		dsp->ch[0].type=type_wam;
		
	} else {
		// assume standard devkit (or other formats without signature)

		// check for MP2 demo stereo format
		/*get_dspheadermp2d(&dsp->ch[0],readbuf);
		get_dspheadermp2d2(&dsp->ch[1],readbuf);
		if (dsp->ch[0].num_samples==dsp->ch[1].num_samples) {
				DisplayError("MP2 demo");
				// stereo
				dsp->NCH=2;
				dsp->ch[0].interleave=dsp->ch[1].interleave=0x8000;

				dsp->ch[0].chanstart=0x68;
				dsp->ch[1].chanstart=0x8068;

				dsp->ch[0].type=dsp->ch[1].type=type_mp2d;
				dsp->ch[0].bps=dsp->ch[1].bps=8;
		} else { // if MP2 demo*/

			// IDSP in Mario Smash Football
			if (!memcmp("IDSP",readbuf,4)) IDSP=1;
			// IDSP (no relation) in Harvest Moon - Another Wonderful Life
			if (!memcmp("\x00\x00\x00\x00\x00\x01\x00\x02",readbuf,8))  IDSP2=1;
			
			if (IDSP) {
				get_dspheaderstd(&dsp->ch[0],readbuf+0xC);
				get_dspheaderstd(&dsp->ch[1],readbuf+0x6C); // header for channel 2
			} else if (IDSP2) {
				get_dspheaderstd(&dsp->ch[0],readbuf+0x40);
				get_dspheaderstd(&dsp->ch[1],readbuf+0xa0);
			} else {
				get_dspheaderstd(&dsp->ch[0],readbuf);
				get_dspheaderstd(&dsp->ch[1],readbuf+0x60); // header for channel 2
			}
			
			// if a valid second header (agrees with first)
			if (abs(dsp->ch[0].num_adpcm_nibbles-dsp->ch[1].num_adpcm_nibbles)<=1 &&
				abs(dsp->ch[0].num_samples-dsp->ch[1].num_samples)<=1) {

				// stereo
				dsp->NCH=2;

				if (IDSP) {
					dsp->ch[0].interleave=dsp->ch[1].interleave=get32bit(readbuf+4); //0x6b40;
					dsp->ch[0].chanstart=0xcc;
					dsp->ch[1].chanstart=0xcc+dsp->ch[0].interleave;
          dsp->ch[1].infile = fopen(inputfile,"rb");
				} else if (IDSP2) {
					dsp->ch[0].interleave=dsp->ch[1].interleave=get32bit(readbuf+8);
					dsp->ch[0].chanstart=0x100;
					dsp->ch[1].chanstart=0x100+dsp->ch[0].interleave;
          dsp->ch[1].infile = fopen(inputfile,"rb");
				} else if (MSS) {
					dsp->ch[0].interleave=dsp->ch[1].interleave=0x1000;
					dsp->ch[0].chanstart=0xc0;
					dsp->ch[1].chanstart=0x10c0;
          dsp->ch[1].infile = fopen(inputfile,"rb");
				} else if (GCM) {
					dsp->ch[0].interleave=dsp->ch[1].interleave=0x8000;
					dsp->ch[0].chanstart=0xc0;
					dsp->ch[1].chanstart=0x80c0;
          dsp->ch[1].infile = fopen(inputfile,"rb");
				} else {
					dsp->ch[0].interleave=dsp->ch[1].interleave=0x14180;
					dsp->ch[0].chanstart=0xc0;
					dsp->ch[1].chanstart=0x14180+0xc0;
				}					

				dsp->ch[0].type=dsp->ch[1].type=type_std;
				dsp->ch[0].bps=dsp->ch[1].bps=8;
			} else { // if valid second header (standard)
				// mono
				dsp->ch[0].interleave=0;

				dsp->ch[0].type=type_std;

				dsp->ch[0].bps=4;

				if (IDSP) dsp->ch[0].chanstart=0x6c;
				else dsp->ch[0].chanstart=0x60;

				if (dsp->ch[0].infile != dsp->ch[1].infile) { // dual-file stereo
					//DisplayError("stereo");

					fseek(dsp->ch[1].infile,0,SEEK_SET);

          fread(&readbuf,0x100,1,dsp->ch[1].infile);
					
					get_dspheaderstd(&dsp->ch[1],readbuf);

					dsp->NCH=2;
					dsp->ch[1].interleave=0;

					dsp->ch[1].type=type_std;

					dsp->ch[1].bps=4;

					dsp->ch[1].chanstart=0x60;

				} else dsp->NCH=1; // if dual-file stereo

				// Single header stereo (Monopoly Party)
				if (MPDSP) {
					DisplayError("monopoly");
					dsp->NCH=2;

					dsp->ch[0].interleave=0xf000;
					dsp->ch[0].num_samples/=2;
					memcpy(&dsp->ch[1],&dsp->ch[0],sizeof(CUBESTREAM));
					dsp->ch[0].chanstart=0x60;
					dsp->ch[1].chanstart=0x60+0xf000;

					dsp->ch[0].type=dsp->ch[1].type=type_mpdsp;
				  dsp->ch[1].infile = fopen(inputfile,"rb");
        }


			} // if valid second header (standard)

			if (IDSP) dsp->ch[0].type=dsp->ch[1].type=type_idsp;
			if (IDSP2) dsp->ch[0].type=dsp->ch[1].type=type_idsp2;
			if (MSS) dsp->ch[0].type=dsp->ch[1].type=type_mss;
	}

	// initialize
	dsp->ch[0].offs=dsp->ch[0].chanstart;
	dsp->ch[1].offs=dsp->ch[1].chanstart;
	dsp->startinterleave=dsp->ch[0].interleave;
	dsp->lastchunk=0;

	dsp->ch[0].hist1=dsp->ch[0].yn1;
	dsp->ch[0].hist2=dsp->ch[0].yn2;
	dsp->ch[1].hist1=dsp->ch[1].yn1;
	dsp->ch[1].hist2=dsp->ch[1].yn2;

	dsp->ch[0].readloc=dsp->ch[1].readloc=dsp->ch[0].writeloc=dsp->ch[1].writeloc=0;

	// check for Metroid Prime over-world (a special case, to remove the blip at the beginning
	// (which seems to have been an error in the source WAV))
	{
		unsigned char data[28] = {
		0x00, 0x73, 0x09, 0xBD, 0x00, 0x83, 0x78, 0xD9, 0x00, 0x00, 0x7D, 0x00, 0x00, 0x01, 0x00,
		0x00, 0x00, 0x01, 0x3A, 0x6E, 0x00, 0x83, 0x78, 0xD8, 0x00, 0x00, 0x00, 0x02
		};

		unsigned char data2[19] = {
		0xD8, 0x21, 0x13, 0x00, 0x05, 0x00, 0x00, 0x00, 0x78, 0x6F, 0xE8, 0x77, 0xD8, 0xFE, 0x12,
		0x00, 0x0C, 0x00, 0x63
		};

		fseek(dsp->ch[0].infile,0,SEEK_SET);

		if (!memcmp(data,readbuf,28) && !memcmp(data2,readbuf+0x50,19)) {
			dsp->ch[0].offs+=0x38;
			dsp->ch[1].offs+=0x38;
		}
	}

	// Disney's Magical Mirror loop oddity (samples instead of nibbles)
	if (dsp->ch[0].type==type_std && dsp->NCH==2 && !(dsp->ch[0].sa&0xf) && !(dsp->ch[0].ea&0xf) && !(dsp->ch[1].sa&0xf) && !(dsp->ch[1].ea&0xf))
	{
		dsp->ch[0].sa=dsp->ch[0].sa*16/14;
		dsp->ch[0].ea=dsp->ch[0].ea*16/14;
		dsp->ch[1].sa=dsp->ch[1].sa*16/14;
		dsp->ch[1].ea=dsp->ch[1].ea*16/14;

		//MessageBox(NULL,"disney","yo",MB_OK);
	}

	// start and end points equal...
	// why would they do this? to screw with you
	if (dsp->ch[0].sa==dsp->ch[0].ea) dsp->ch[0].loop_flag=0;
	if (dsp->ch[1].sa==dsp->ch[1].ea) dsp->ch[1].loop_flag=0;

	//check sample rate
	if (!CheckSampleRate(dsp->ch[0].sample_rate)) {
		CloseCUBEFILE(dsp);
		return 1;
	}

	// get file size
  fseek(dsp->ch[0].infile,0,SEEK_END);
  dsp->file_length=ftell(dsp->ch[0].infile);

	// Adjust file size, except for IDSP (has strange file sizes)
	if (!IDSP) dsp->file_length=(dsp->file_length+0xf)&(~0xf);

	// in case loop end offset is beyond EOF... (MMX:CM)
	// breaks Metroid Prime 2 looping
	//if (dsp->ch[0].ea*dsp->ch[0].bps/8>dsp->file_length-dsp->ch[0].chanstart) dsp->ch[0].ea=(dsp->file_length-dsp->ch[0].chanstart)*8/dsp->ch[0].bps;
	//if (dsp->NCH==2 && dsp->ch[1].ea*dsp->ch[1].bps/8>dsp->file_length-dsp->ch[1].chanstart) dsp->ch[1].ea=(dsp->file_length-dsp->ch[1].chanstart)*8/dsp->ch[1].bps;
	
	// calculate how long to play
	if (!dsp->ch[0].loop_flag) dsp->nrsamples = dsp->ch[0].num_samples;
	else if (dsp->ch[0].interleave)
		dsp->nrsamples=(dsp->ch[0].sa+looptimes*(dsp->ch[0].ea-dsp->ch[0].sa))*14/(8*8/dsp->ch[0].bps)/dsp->NCH+(fadelength+fadedelay)*dsp->ch[0].sample_rate;
	else
		dsp->nrsamples=(dsp->ch[0].sa+looptimes*(dsp->ch[0].ea-dsp->ch[0].sa))*14/(8*8/dsp->ch[0].bps)+(fadelength+fadedelay)*dsp->ch[0].sample_rate;

	return 0;
}

// Note that for both of these I don't bother loading the loop context. Sounds great the
// way it is, why mess with a good thing?

// for noninterleaved files (mono, STM)
// also for reading two mono Metroid Prime files simultaneously as stereo
void fillbufferDSP(CUBESTREAM * stream) {
	int i,j;
	short decodebuf[14];
	char ADPCMbuf[8];

	fseek(stream->infile,stream->offs,SEEK_SET);

	i=0;
	do {
		if (i==0) {
			fread(ADPCMbuf, 8, 1,stream->infile);
			DSPdecodebuffer(ADPCMbuf,decodebuf,stream->coef,&stream->hist1,&stream->hist2);
			i=14;
			j=14;
			stream->offs+=8;
						
			if (stream->loop_flag && (stream->offs-stream->chanstart+8)>=((stream->ea*stream->bps/8)&(~7))) {
				DisplayError("loop from %08x to %08x",stream->offs,stream->chanstart+((stream->sa*stream->bps/8)&(~7)));
        stream->offs=fseek(stream->infile,stream->chanstart+((stream->sa*stream->bps/8)&(~7)),SEEK_SET);
			}

			/*if (stream->loop_flag && (stream->offs-stream->chanstart)>=stream->ea*stream->bps/8) {
				stream->offs=SetFilePointer(stream->infile,stream->chanstart+(stream->sa&(~0xf))*stream->bps/8,0,FILE_BEGIN);
				DisplayError("loop");
			}*/
		}
		stream->chanbuf[stream->writeloc++]=decodebuf[j-i];
		i--;
		if (stream->writeloc>=0x8000/8*14) stream->writeloc=0;
	} while (stream->writeloc != stream->readloc);
}

// each HALP block contains the address of the next one and the size of the current one
void fillbufferHALP(CUBEFILE * dsp) {
	int c,i,l;
	short decodebuf1[28];
	short decodebuf2[28];
	char ADPCMbuf[16];

	if (dsp->halpsize==0 && (long)dsp->nexthalp < 0) dsp->ch[0].readloc=dsp->ch[1].readloc=dsp->ch[0].writeloc-1;
	
	i=0;
	do {
		if (i==0) {
			
			// handle HALPST headers
			if (dsp->halpsize==0) {
				if ((long)dsp->nexthalp < 0) {
					//for (c=0;c<0x8000/8*14;c++) dsp->ch[0].chanbuf[c]=dsp->ch[1].chanbuf[c]=0;
					//dsp->ch[0].writeloc=dsp->ch[1].writeloc=0;
					//dsp->ch[0].readloc=dsp->ch[1].readloc=dsp->ch[0].writeloc-1;
					return;
				}
				dsp->ch[0].offs=dsp->nexthalp+0x20;
				fseek(dsp->ch[0].infile, dsp->nexthalp, SEEK_SET);
				fread(ADPCMbuf, 16, 1, dsp->ch[0].infile);
				dsp->halpsize=get32bit(ADPCMbuf+4)+1; // size to read?
				
				dsp->ch[1].offs=dsp->nexthalp+0x20+get32bit(ADPCMbuf)/2;
				dsp->nexthalp=get32bit(ADPCMbuf+8);
			}

			fseek(dsp->ch[0].infile, dsp->ch[0].offs,SEEK_SET);
			fread(ADPCMbuf, 16, 1, dsp->ch[0].infile);
			DSPdecodebuffer(ADPCMbuf,decodebuf1,dsp->ch[0].coef,&dsp->ch[0].hist1,&dsp->ch[0].hist2);
			DSPdecodebuffer(ADPCMbuf+8,decodebuf1+14,dsp->ch[0].coef,&dsp->ch[0].hist1,&dsp->ch[0].hist2);

			fseek(dsp->ch[1].infile, dsp->ch[1].offs,SEEK_SET);
			fread(ADPCMbuf, 16, 1, dsp->ch[1].infile);
			DSPdecodebuffer(ADPCMbuf,decodebuf2,dsp->ch[1].coef,&dsp->ch[1].hist1,&dsp->ch[1].hist2);
			DSPdecodebuffer(ADPCMbuf+8,decodebuf2+14,dsp->ch[1].coef,&dsp->ch[1].hist1,&dsp->ch[1].hist2);

			i=28;
			c=0;
			dsp->ch[0].offs+=0x10;
			dsp->ch[1].offs+=0x10;
			
			dsp->halpsize-=0x20;
			if (dsp->halpsize<0x20) dsp->halpsize=0;
		}
		dsp->ch[0].chanbuf[dsp->ch[0].writeloc++]=decodebuf1[c];
		dsp->ch[1].chanbuf[dsp->ch[1].writeloc++]=decodebuf2[c];
		c++; i--;
		if (dsp->ch[0].writeloc>=0x8000/8*14) dsp->ch[0].writeloc=0;
		if (dsp->ch[1].writeloc>=0x8000/8*14) dsp->ch[1].writeloc=0;
	} while (dsp->ch[0].writeloc != dsp->ch[0].readloc);
}

// interleaved files requires streams with knowledge of each other (for proper looping)
void fillbufferDSPinterleave(CUBEFILE * dsp) {
	int i,l;
	short decodebuf1[14];
	short decodebuf2[14];
	char ADPCMbuf[8];

	i=0;
	do {
		if (i==0) {

			fseek(dsp->ch[0].infile, dsp->ch[0].offs,SEEK_SET);
			fread(ADPCMbuf, 8, 1, dsp->ch[0].infile);
			DSPdecodebuffer(ADPCMbuf,decodebuf1,dsp->ch[0].coef,&dsp->ch[0].hist1,&dsp->ch[0].hist2);

			fseek(dsp->ch[1].infile, dsp->ch[1].offs,SEEK_SET);
			fread(ADPCMbuf, 8, 1, dsp->ch[1].infile);
			DSPdecodebuffer(ADPCMbuf,decodebuf2,dsp->ch[1].coef,&dsp->ch[1].hist1,&dsp->ch[1].hist2);

			i=14;
			dsp->ch[0].offs+=8;
			dsp->ch[1].offs+=8;

			// handle interleave
			if (!dsp->lastchunk && (dsp->ch[0].offs-dsp->ch[0].chanstart)%dsp->ch[0].interleave==0) {
				dsp->ch[0].offs+=dsp->ch[0].interleave;
				//if (dsp->lastchunk) DisplayError("chanstart with lastchunk\noffset=%08x",dsp->ch[0].offs);
			}
			if (!dsp->lastchunk && (dsp->ch[1].offs-dsp->ch[1].chanstart)%dsp->ch[1].interleave==0) {
				dsp->ch[1].offs+=dsp->ch[1].interleave;

				// metroid prime 2, IDSP has smaller interleave for last chunk
				if (!dsp->lastchunk &&
					(dsp->ch[0].type==type_mp2 || dsp->ch[0].type==type_idsp) && 
					dsp->ch[1].offs+dsp->ch[1].interleave>dsp->file_length) {
					
					dsp->ch[0].interleave=dsp->ch[1].interleave=
						(dsp->file_length-dsp->ch[0].offs)/2;
					dsp->ch[1].offs=dsp->ch[0].offs+dsp->ch[1].interleave;

					dsp->lastchunk=1;

					DisplayError("smallchunk, ch[0].offs=%08x ch[0].interleave=%08x\nfilesize=%08x",dsp->ch[0].offs,dsp->ch[0].interleave,dsp->file_length);
				}
			}
						
			if (dsp->ch[0].loop_flag && (
				(dsp->ch[0].offs-dsp->ch[0].chanstart)>=dsp->ch[0].ea*dsp->ch[0].bps/8 ||
				(dsp->ch[1].offs-dsp->ch[0].chanstart)>=dsp->ch[1].ea*dsp->ch[1].bps/8 
				) ) {
			
				if (dsp->ch[0].type==type_sfass && (dsp->ch[0].sa/dsp->ch[0].interleave)%2 == 1) {
					dsp->ch[1].offs=dsp->ch[0].chanstart+(dsp->ch[0].sa&(~7))*dsp->ch[0].bps/8;
					dsp->ch[0].offs=dsp->ch[1].offs-dsp->ch[0].interleave;
				} else {
					dsp->ch[0].offs=dsp->ch[0].chanstart+(dsp->ch[0].sa&(~7))*dsp->ch[0].bps/8;
					dsp->ch[1].offs=dsp->ch[1].chanstart+(dsp->ch[1].sa&(~7))*dsp->ch[1].bps/8;
				}

				DisplayError("loop\nch[1].offs=%08x",dsp->ch[1].offs);

				dsp->ch[0].interleave=dsp->ch[1].interleave=dsp->startinterleave;
				dsp->lastchunk=0;
			}
		}
		dsp->ch[0].chanbuf[dsp->ch[0].writeloc++]=decodebuf1[14-i];
		dsp->ch[1].chanbuf[dsp->ch[1].writeloc++]=decodebuf2[14-i];
		i--;
		if (dsp->ch[0].writeloc>=0x8000/8*14) dsp->ch[0].writeloc=0;
		if (dsp->ch[1].writeloc>=0x8000/8*14) dsp->ch[1].writeloc=0;
	} while (dsp->ch[0].writeloc != dsp->ch[0].readloc);
}
