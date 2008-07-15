/*

  in_cube Gamecube Stream Player for Winamp
  by hcs

  includes work by Destop and bero

*/

// DLS (a.k.a. DTK, TRK, ADP)
// uses same algorithm as XA, apparently

#ifndef _LINUX
#include <windows.h>
#else
#include <string.h>
#define strcmpi strcasecmp
#endif
#include "wamain.h"
#include "cube.h"

// inputfile == NULL means file is already opened, just reload
// return 1 if valid ADP not detected, 0 on success
int InitADPFILE(char * inputfile, CUBEFILE * adp) {
	int l;
	char readbuf[4];
	if (inputfile) {
		adp->ch[0].infile=adp->ch[1].infile=NULL;

		if (strcmpi(inputfile+strlen(inputfile)-4,".adp")) return 1;

		adp->ch[0].infile = fopen(inputfile,"rb");

		if (!adp->ch[0].infile) // error opening file
			return 1;

		// check for valid first frame
		fread(readbuf,4,1,adp->ch[0].infile);
		if (readbuf[0]!=readbuf[2] || readbuf[1]!=readbuf[3]) {
			fclose(adp->ch[0].infile);
			adp->ch[0].infile=NULL;
			return 1;
		}
	} else if (adp->ch[0].type!=type_adp) return 1; // we don't have the file name to recheck

	adp->ch[0].type=type_adp;

	
	adp->NCH = 2;
	adp->ch[0].sample_rate = 48000;
  fseek(adp->ch[0].infile,0,SEEK_END);
	adp->nrsamples = ftell(adp->ch[0].infile)*7/8;
	adp->ch[0].loop_flag=0;

  adp->file_length=ftell(adp->ch[0].infile);

	fseek(adp->ch[0].infile, adp->ch[0].chanstart, SEEK_SET);
	
	adp->ch[0].lhist1 = 0;
    adp->ch[0].lhist2 = 0;
    adp->ch[1].lhist1 = 0;
    adp->ch[1].lhist2 = 0;

	adp->ch[0].readloc=adp->ch[1].readloc=adp->ch[0].writeloc=adp->ch[1].writeloc=0;

	fseek(adp->ch[0].infile,0,SEEK_SET);

	return 0;
}

void fillbufferADP(CUBEFILE * adp) {
	int l;
	char ADPCMbuf[32];

	if ((signed long)ftell(adp->ch[0].infile) >= adp->file_length) {
		adp->ch[0].readloc=adp->ch[1].readloc=adp->ch[0].writeloc-1;
		return;
	}

	do {
		l = fread(ADPCMbuf, 1, 32, adp->ch[0].infile);
		if (l<32) return;
		ADPdecodebuffer(ADPCMbuf,adp->ch[0].chanbuf+adp->ch[0].writeloc,
								 adp->ch[1].chanbuf+adp->ch[1].writeloc,
					&adp->ch[0].lhist1, &adp->ch[0].lhist2, &adp->ch[1].lhist1, &adp->ch[1].lhist2);

		adp->ch[0].writeloc+=28;
		adp->ch[1].writeloc+=28;

		if (adp->ch[0].writeloc>=0x8000/8*14) adp->ch[0].writeloc=0;
		if (adp->ch[1].writeloc>=0x8000/8*14) adp->ch[1].writeloc=0;
	} while (adp->ch[0].writeloc != adp->ch[0].readloc);
}
