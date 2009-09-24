/*

  in_cube Gamecube Stream Player for Winamp
  by hcs

  includes work by Destop and bero

*/

// VAG (Sony "Very Audio Good")

#ifndef _LINUX
#include <windows.h>
#else
#include <memory.h>
#define INVALID_HANDLE_VALUE NULL
#endif
#include "wamain.h"
#include "cube.h"

// inputfile == NULL means file is already opened, just reload
// return 1 if valid VAG not detected, 0 on success
int InitVAGFILE(char * inputfile, CUBEFILE * vag) {
	int l;
	char readbuf[0x40];
	if (inputfile) {
		vag->ch[0].infile=vag->ch[1].infile=INVALID_HANDLE_VALUE;

		//if (strcmpi(inputfile+strlen(inputfile)-4,".vag")) return 1;

		vag->ch[0].infile = fopen(inputfile,"rb");

		if (!vag->ch[0].infile) // error opening file
			return 1;

	} else if (vag->ch[0].type!=type_vag) return 1; // we don't have the file name to recheck


	fread(readbuf,0x40,1,vag->ch[0].infile);

	if (!memcmp(readbuf,"VAGp",4)) {
	    vag->ch[0].type=type_vag;
	    vag->NCH = 1;
	    vag->ch[0].sample_rate = get32bit(readbuf+0x10);
	    vag->ch[0].loop_flag=0;
	    vag->nrsamples = get32bit(readbuf+0xc)*28/16;
	    vag->ch[0].chanstart=0x40;

	    vag->ch[0].hist1 = 0;
	    vag->ch[0].hist2 = 0;
	} else if (!memcmp(readbuf,"SShd",4) && !memcmp(readbuf+0x20,"SSbd",4)) {
	    vag->ch[0].type=type_ads;
	    vag->NCH = get32bitL(readbuf+0x10);
	    vag->ch[1].sample_rate = vag->ch[0].sample_rate = get32bitL(readbuf+0xc);
	    
	    vag->nrsamples = get32bitL(readbuf+0x24)*28/16/vag->NCH;
	    vag->ch[1].interleave=vag->ch[0].interleave=get32bitL(readbuf+0x14);
	    vag->ch[0].chanstart=0x28;
	    vag->ch[1].chanstart=0x28+vag->ch[0].interleave;

	    vag->ch[0].hist1=vag->ch[1].hist1=vag->ch[0].hist2=vag->ch[1].hist2=0;
	    vag->ch[0].sa=vag->ch[1].sa=(get32bitL(readbuf+0x18)<<4)+vag->ch[0].chanstart;
	    vag->ch[0].ea=vag->ch[1].ea=get32bitL(readbuf+0x24);
	    vag->ch[0].bps=vag->ch[1].bps=8;

		vag->ch[0].loop_flag = vag->ch[1].loop_flag = (vag->ch[0].sa<vag->ch[0].ea);
	} else return 1;

  fseek(vag->ch[0].infile,0,SEEK_END);
  vag->file_length=ftell(vag->ch[0].infile);

	vag->ch[0].readloc=vag->ch[1].readloc=vag->ch[0].writeloc=vag->ch[1].writeloc=0;

	vag->ch[0].offs=vag->ch[0].chanstart;
	vag->ch[1].offs=vag->ch[1].chanstart;
	fseek(vag->ch[0].infile,vag->ch[0].chanstart,SEEK_SET);

	if (vag->ch[0].loop_flag)
	vag->nrsamples=(vag->ch[0].sa-vag->ch[0].chanstart+(vag->ch[0].ea-vag->ch[0].sa)*looptimes)*28/16/vag->NCH+(fadelength+fadedelay)*vag->ch[0].sample_rate;
	return 0;
}

void fillbufferVAG(CUBEFILE * vag) {
	int l;
	char ADPCMbuf[16];

	if ((signed long)ftell(vag->ch[0].infile) >= vag->file_length) {
		vag->ch[0].readloc=vag->ch[0].writeloc-1;
		return;
	}

	do {
		l = fread(ADPCMbuf, 1, 16, vag->ch[0].infile);
		if (l<16) return;
		VAGdecodebuffer(ADPCMbuf,vag->ch[0].chanbuf+vag->ch[0].writeloc,
					&vag->ch[0].hist1, &vag->ch[0].hist2);

		vag->ch[0].writeloc+=28;

		if (vag->ch[0].writeloc>=0x8000/8*14) vag->ch[0].writeloc=0;
	} while (vag->ch[0].writeloc != vag->ch[0].readloc);
}
void fillbufferVAGinterleave(CUBEFILE * vag) {
    int l;
    char ADPCMbuf[16];

    do {
	if (vag->ch[0].offs >= vag->ch[0].ea) {
		if (!vag->ch[0].loop_flag) {
			vag->ch[0].readloc=vag->ch[0].writeloc-1;
			vag->ch[1].readloc=vag->ch[1].writeloc-1;
			return;
		}

	    vag->ch[0].offs = vag->ch[0].sa;
	    vag->ch[1].offs = vag->ch[0].sa+vag->ch[0].interleave;
	}
	    fseek(vag->ch[0].infile, vag->ch[0].offs,SEEK_SET);
	    fread(ADPCMbuf,16,1,vag->ch[0].infile);
	    VAGdecodebuffer(ADPCMbuf,vag->ch[0].chanbuf+vag->ch[0].writeloc,&vag->ch[0].hist1,&vag->ch[0].hist2);
	    vag->ch[0].offs+=16;
	    vag->ch[0].writeloc+=28;
	    if (vag->ch[0].writeloc>=0x8000/8*14) vag->ch[0].writeloc=0;
	    if ((vag->ch[0].offs-vag->ch[0].chanstart)%vag->ch[0].interleave==0) vag->ch[0].offs+=vag->ch[0].interleave;

	    fseek(vag->ch[0].infile, vag->ch[1].offs,SEEK_SET);
	    fread(ADPCMbuf,16,1,vag->ch[0].infile);
	    VAGdecodebuffer(ADPCMbuf,vag->ch[1].chanbuf+vag->ch[1].writeloc,&vag->ch[1].hist1,&vag->ch[1].hist2);
	    vag->ch[1].offs+=16;
	    vag->ch[1].writeloc+=28;
	    if (vag->ch[1].writeloc>=0x8000/8*14) vag->ch[1].writeloc=0;
	    if ((vag->ch[1].offs-vag->ch[1].chanstart)%vag->ch[1].interleave==0) vag->ch[1].offs+=vag->ch[1].interleave;
    } while (vag->ch[0].writeloc != vag->ch[0].readloc);
}
