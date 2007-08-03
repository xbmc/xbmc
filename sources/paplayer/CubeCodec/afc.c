/*

  in_cube Gamecube Stream Player for Winamp
  by hcs

  includes work by Destop and bero


*/

// experimental AFC support

#ifndef _LINUX
#include <windows.h>
#else
#include <string.h>
#define strcmpi strcasecmp
#endif
#include "wamain.h"
#include "cube.h"

int InitAFCFILE(char * inputfile, CUBEFILE * afc) {
	char readbuf[0x50], * ext;
	int l;
	
	ext=strrchr(inputfile,'.')+1;
	if (ext==(char*)1 || strcmpi(ext,"afc")) return 1; // only check for .afc ext

	if (inputfile) {
		afc->ch[0].infile=afc->ch[1].infile=NULL;

		afc->ch[0].infile = fopen(inputfile,"rb");

		if (!afc->ch[0].infile) // error opening file
			return 1;
	} else if (afc->ch[0].type!=type_afc) return 1; // we don't have the file name to recheck

	afc->ch[1].infile=afc->ch[0].infile;

	fread(readbuf,0x50,1,afc->ch[0].infile);

  fseek(afc->ch[0].infile,0,SEEK_END);
	afc->file_length=ftell(afc->ch[0].infile);

	if (!memcmp(readbuf, "STRM",4) && afc->file_length-0x40 == get32bit(readbuf+4) && !memcmp(readbuf+0x40,"BLCK",4)) {
		// AST (afc with interleaved blocks)
		afc->NCH = 2; // 'cause dat's all I gots
		afc->ch[0].sample_rate = get32bit(readbuf+0x10);
		afc->ch[0].chanstart = 0x40;
		afc->ch[1].chanstart = 0x40;
		afc->ch[0].num_samples = get32bit(readbuf+0x14)*16/8/2;
		afc->ch[0].loop_flag = get16bit(readbuf+0xe);
		afc->ch[0].sa = get32bit(readbuf+0x18);
		afc->ch[0].ea = get32bit(readbuf+0x1c);

		afc->samplesdone = 0;

		// uses a very HALP-like mechanism, BLCK
		afc->nexthalp=0x40;
		afc->halpsize=0;
		
		afc->ch[0].hist1=0;
		afc->ch[0].hist2=0;
		afc->ch[1].hist1=0;
		afc->ch[1].hist2=0;
		
		// invalidate loop context so it'll be remembered when we hit the start
		afc->loophalpsize=-1;
		
		afc->ch[0].type = type_astafc;
	} else {

		afc->NCH = 2;
		afc->ch[0].sample_rate = (unsigned short)get16bit(readbuf+8);
		if (!CheckSampleRate(afc->ch[0].sample_rate)) {
			CloseCUBEFILE(afc);
			return 1;
		}

		afc->ch[0].chanstart = 0x20;
		afc->ch[0].hist1=0;
		afc->ch[0].hist2=0;
		afc->ch[1].hist1=0;
		afc->ch[1].hist2=0;
		afc->ch[0].type = type_afc;

		afc->ch[0].offs=afc->ch[0].chanstart;
		afc->ch[0].num_samples = get32bit(readbuf+4); //(GetFileSize(afc->ch[0].infile,&l)-afc->ch[0].chanstart)*16/18;
		afc->ch[0].loop_flag = get32bit(readbuf+0x10);
		afc->ch[0].sa = get32bit(readbuf+0x14);
		afc->ch[0].ea = afc->ch[0].num_samples;

	}

	if (!afc->ch[0].loop_flag) afc->nrsamples = afc->ch[0].num_samples;
	else afc->nrsamples=afc->ch[0].sa+looptimes*(afc->ch[0].ea-afc->ch[0].sa)+(fadelength+fadedelay)*afc->ch[0].sample_rate;

	afc->ch[0].readloc=afc->ch[1].readloc=afc->ch[0].writeloc=afc->ch[1].writeloc=0;

	return 0;
}

void fillbufferAFC(CUBEFILE * afc) {
	int l,i;
	char AFCbuf[18];
	short wavbuf[32];

	fseek(afc->ch[0].infile,afc->ch[0].offs,SEEK_SET);

	do {
		l = fread(AFCbuf, 1, 18, afc->ch[0].infile);
		if (l<18) {
			// only seems to support loop from end
			if (afc->ch[0].loop_flag) {
				afc->ch[0].offs=afc->ch[0].chanstart+afc->ch[0].sa/16*18;
				fseek(afc->ch[0].infile,afc->ch[0].offs,SEEK_SET);
				continue;
			}
			return;
		}
		afc->ch[0].offs+=18;

		AFCdecodebuffer(AFCbuf,wavbuf,&afc->ch[0].hist1,&afc->ch[0].hist2);
		AFCdecodebuffer(AFCbuf+9,wavbuf+16,&afc->ch[1].hist1,&afc->ch[1].hist2);

		for (i=0;i<16;i++) {
			afc->ch[0].chanbuf[afc->ch[0].writeloc++]=wavbuf[i];
			if (afc->ch[0].writeloc>=0x8000/8*14) afc->ch[0].writeloc=0;
			afc->ch[1].chanbuf[afc->ch[1].writeloc++]=wavbuf[i+16];
			if (afc->ch[1].writeloc>=0x8000/8*14) afc->ch[1].writeloc=0;
		}
	} while (afc->ch[0].writeloc != afc->ch[0].readloc);

}

void fillbufferASTAFC(CUBEFILE * afc) {
	int l,i;
	char AFCbuf1[9],AFCbuf2[9];
	short wavbuf1[16],wavbuf2[16];

	if (afc->halpsize==0 && (long)afc->nexthalp == 0) afc->ch[0].readloc=afc->ch[1].readloc=afc->ch[0].writeloc-1;
	
	do {
			// handle BLCK headers
			if (afc->halpsize==0) {
				if ((long)afc->nexthalp == 0) return;
				afc->ch[0].offs=afc->nexthalp+0x20;
				fseek(afc->ch[0].infile, afc->nexthalp,SEEK_SET);
				l = fread(AFCbuf1, 1, 8, afc->ch[0].infile);
				if (l<8) return;
				afc->halpsize=get32bit(AFCbuf1+4)*2; // interleave amount (this whole BLCK is 2x)
								
				afc->ch[1].offs=afc->nexthalp+0x20+get32bit(AFCbuf1+4);
				afc->nexthalp+=get32bit(AFCbuf1+4)*2+0x20;
			}

			fseek(afc->ch[0].infile, afc->ch[0].offs,SEEK_SET);
			fread(AFCbuf1, 9, 1, afc->ch[0].infile);

			fseek(afc->ch[0].infile, afc->ch[1].offs,SEEK_SET);
			fread(AFCbuf2, 9, 1, afc->ch[0].infile);			

			afc->ch[0].offs+=9;
			afc->ch[1].offs+=9;
			
			afc->halpsize-=18;
			if (afc->halpsize<18) afc->halpsize=0;

			AFCdecodebuffer(AFCbuf1,wavbuf1,&afc->ch[0].hist1,&afc->ch[0].hist2);
			AFCdecodebuffer(AFCbuf2,wavbuf2,&afc->ch[1].hist1,&afc->ch[1].hist2);

			afc->samplesdone+=16;
			if (afc->loophalpsize < 0 && afc->samplesdone >= (int)afc->ch[0].sa) {
				// a lot of these values could probably just be recalculated, but it's easy to save them
				afc->loophalpsize = afc->halpsize;
				afc->loopnexthalp = afc->nexthalp;
				afc->ch[0].loopoffs = afc->ch[0].offs;
				afc->ch[1].loopoffs = afc->ch[1].offs;
				afc->ch[0].sa = afc->samplesdone;
			}
			if (afc->samplesdone >= (int)afc->ch[0].ea) {
				afc->halpsize = afc->loophalpsize;
				afc->nexthalp = afc->loopnexthalp;
				afc->ch[0].offs = afc->ch[0].loopoffs;
				afc->ch[1].offs = afc->ch[1].loopoffs;
				afc->samplesdone = afc->ch[0].sa;
			}
		
			for (i=0;i<16;i++) {
				afc->ch[0].chanbuf[afc->ch[0].writeloc++]=wavbuf1[i];
				afc->ch[1].chanbuf[afc->ch[1].writeloc++]=wavbuf2[i];
			
				if (afc->ch[0].writeloc>=0x8000/8*14) afc->ch[0].writeloc=0;
				if (afc->ch[1].writeloc>=0x8000/8*14) afc->ch[1].writeloc=0;
			}
	} while (afc->ch[0].writeloc != afc->ch[0].readloc);
}


