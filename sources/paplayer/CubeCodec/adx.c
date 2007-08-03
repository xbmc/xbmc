/*

  in_cube Gamecube Stream Player for Winamp
  by hcs

  includes work by Destop and bero

*/

// ADX (headered CRI stream)
#ifndef _LINUX
#include <windows.h>
#else
#include <memory.h>
#endif
#include "cube.h"
#include "wamain.h"

int adxonechan; // == 0 if not in onechan mode, otherwise is 1-based channel number

// inputfile == NULL means file is already opened, just reload
// return 1 if valid ADX not detected, 0 on success
int InitADXFILE(char * inputfile, CUBEFILE * adx) {
	unsigned char readbuffer[4096],*preadbuf;
	int l;
	int offs;

	if (inputfile) {
		adx->ch[0].infile=adx->ch[1].infile=NULL;
		adx->ch[0].infile = fopen(inputfile,"rb");

		if (!adx->ch[0].infile) // error opening file
		{
			return 1;
		}
	}

	fseek(adx->ch[0].infile,0,SEEK_SET);
	
	fread(readbuffer, 4096, 1, adx->ch[0].infile);

	if(readbuffer[0] != 0x80) 
	{
		// check for valid ADX at 0x20 (Sonic Mega Collection)

		if (readbuffer[0x20] != 0x80) {

			// not ADX
			if (inputfile) {
				fclose(adx->ch[0].infile);
				adx->ch[0].infile=NULL;
			}
			return 1;
		} else preadbuf=readbuffer+(offs=0x20);
	} else preadbuf=readbuffer+(offs=0);

	adx->ch[0].chanstart = get16bit(&preadbuf[2])+4;
	adx->ch[1].chanstart = get16bit(&preadbuf[2])+4+18;

 	if(adx->ch[0].chanstart < 0 || adx->ch[0].chanstart > 4096 || memcmp(preadbuf+adx->ch[0].chanstart-6,"(c)CRI",6)) 
	{
		// not ADX
		if (inputfile) {
			fclose(adx->ch[0].infile);
			adx->ch[0].infile=NULL;
		}
		return 1;
	}

	adx->ADXCH = adx->NCH = (int)preadbuf[7];

	if (adxonechan) adx->NCH=1;
	else if (adx->NCH>2) {
		if (inputfile) {
			fclose(adx->ch[0].infile);
			adx->ch[0].infile=NULL;
		}
		return 1;
	}
	adx->ch[0].sample_rate = get32bit(&preadbuf[8]);
	adx->nrsamples = get32bit(&preadbuf[12]);
		
	// check version code, set up looping appropriately
	if (get32bit(&preadbuf[0x10])==0x01F40300) { // Soul Calibur 2
			if (adx->ch[0].chanstart-6 < 0x2c) adx->ch[0].loop_flag=0; // if header is too small for loop data...
			else {
				adx->ch[0].loop_flag = get32bit(&preadbuf[0x18]);
				adx->ch[0].ea = get32bit(&preadbuf[0x28]);
				adx->ch[0].sa = get32bit(&preadbuf[0x1c])*adx->NCH*18/32+adx->ch[0].chanstart;
			}
			adx->ch[0].type = type_adx03;
	} else if (get32bit(&preadbuf[0x10])==0x01F40400) {
			if (adx->ch[0].chanstart-6 < 0x38) adx->ch[0].loop_flag=0; // if header is too small for loop data...
			else {
				adx->ch[0].loop_flag = get32bit(&preadbuf[0x24]);
				adx->ch[0].ea = get32bit(&preadbuf[0x34]);
				adx->ch[0].sa = get32bit(&preadbuf[0x28])*adx->NCH*18/32+adx->ch[0].chanstart;
			}
			adx->ch[0].type = type_adx04;
	} else {
		if (inputfile) {
			fclose(adx->ch[0].infile);
			adx->ch[0].infile=NULL;
		}
		return 1;
	}

	adx->ch[0].sa+=offs;
	adx->ch[0].ea+=offs;
	adx->ch[0].chanstart+=offs;
	adx->ch[1].chanstart+=offs;

	if (adx->ch[0].loop_flag) adx->nrsamples=((adx->ch[0].sa-adx->ch[0].chanstart)+(adx->ch[0].ea-adx->ch[0].sa)*looptimes)*32/adx->NCH/18+(fadelength+fadedelay)*adx->ch[0].sample_rate;

    fseek(adx->ch[0].infile, 0, SEEK_END);
    adx->file_length=ftell(adx->ch[0].infile);

    fseek(adx->ch[0].infile, adx->ch[0].chanstart, SEEK_SET);
	
	  adx->ch[0].hist1 = 0;
    adx->ch[0].hist2 = 0;
    adx->ch[1].hist1 = 0;
    adx->ch[1].hist2 = 0;

	adx->ch[0].readloc=adx->ch[1].readloc=adx->ch[0].writeloc=adx->ch[1].writeloc=0;

	return 0;
}

void fillbufferADX(CUBEFILE * adx) {
	int i,j,l;
	short decodebuf[32];
	char ADPCMbuf[18];

	if ((signed long)ftell(adx->ch[0].infile) >= adx->file_length && !adx->ch[0].loop_flag) {
		adx->ch[0].readloc=adx->ch[1].readloc=adx->ch[0].writeloc-1;
		return;
	}

	do {
		if (adx->ch[0].loop_flag && ftell(adx->ch[0].infile) >= adx->ch[0].ea) {
			DisplayError("loop");
			fseek(adx->ch[0].infile,adx->ch[0].sa,SEEK_SET);
		}

		l = 0;

		for (j=0;j<adx->ADXCH;j++) {
#ifndef _LINUX
			if (adxonechan && j+1!=adxonechan) SetFilePointer(adx->ch[0].infile,18,0,FILE_CURRENT);
#else
			if (adxonechan && j+1!=adxonechan) fseek(adx->ch[0].infile,18,SEEK_CUR);
#endif
			else {
				l = fread(ADPCMbuf, 1, 18, adx->ch[0].infile);
				if (l<18) return;

				if (adxonechan) {
					ADXdecodebuffer(ADPCMbuf,decodebuf, &adx->ch[0].hist1, &adx->ch[0].hist2);
					for(i = 0; i < 32; i++)
						adx->ch[0].chanbuf[adx->ch[0].writeloc+i] = decodebuf[i];
				} else {
					ADXdecodebuffer(ADPCMbuf,decodebuf, &adx->ch[j].hist1, &adx->ch[j].hist2);
					for(i = 0; i < 32; i++)
						adx->ch[j].chanbuf[adx->ch[j].writeloc+i] = decodebuf[i];
				}
			}
		}
		adx->ch[0].writeloc+=32;
		if (adx->ch[0].writeloc>=0x8000/8*14) adx->ch[0].writeloc=0;
		
		if (adx->NCH==2) {
			adx->ch[1].writeloc+=32;
			if (adx->ch[1].writeloc>=0x8000/8*14) adx->ch[1].writeloc=0;
		}
	} while (adx->ch[0].writeloc != adx->ch[0].readloc);
} 
