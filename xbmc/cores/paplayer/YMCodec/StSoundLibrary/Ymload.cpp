/*-----------------------------------------------------------------------------

	ST-Sound ( YM files player library )

	Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )

	Manage YM file depacking and parsing

-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------

	This file is part of ST-Sound

	ST-Sound is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	ST-Sound is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ST-Sound; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "YmMusic.h"
#include "Depacker.h"

static	ymu16 ymVolumeTable[16] =
{	62,161,265,377,580,774,1155,1575,2260,3088,4570,6233,9330,13187,21220,32767};


static	void	signeSample(ymu8 *ptr,yms32 size)
{

		if (size>0)
		{
			do
			{
				*ptr++ ^= 0x80;
			}
			while (--size);
		}
}

char	*mstrdup(char *in)
{
		char *out = (char*)malloc(strlen(in)+1);
		if (out) strcpy(out,in);
		return out;
}

ymu32      readMotorolaDword(ymu8 **ptr)
{
ymu32 n;
ymu8 *p = *ptr;

        n = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
        p+=4;
        *ptr = p;
        return n;
}

ymu16      readMotorolaWord(ymu8 **ptr)
{
ymu16 n;
ymu8 *p = *ptr;

        n = (p[0]<<8)|p[1];
        p+=2;
        *ptr = p;
        return n;
}

ymchar    *readNtString(ymchar **ptr)
{
ymchar *p;

		p = mstrdup(*ptr);
		(*ptr) += strlen(*ptr)+1;
        return p;
}

yms32	ReadLittleEndian32(ymu8 *pLittle)
{
	yms32 v = ( (pLittle[0]<<0) |
				(pLittle[1]<<8) |
				(pLittle[2]<<16) |
				(pLittle[3]<<24));

	return v;
}

yms32	ReadBigEndian32(ymu8 *pBig)
{
	yms32 v = ( (pBig[0]<<24) |
				(pBig[1]<<16) |
				(pBig[2]<<8) |
				(pBig[3]<<0));

	return v;
}

unsigned char	*CYmMusic::depackFile(void)
 {
 lzhHeader_t *pHeader;
 ymu8	*pNew;
 ymu8	*pSrc;

		pHeader = (lzhHeader_t*)pBigMalloc;
		if ((pHeader->size==0) ||					// NOTE: Endianness works because value is 0
			(strncmp(pHeader->id,"-lh5-",5)))
		{ // Le fichier n'est pas compresse, on retourne l'original.
			return pBigMalloc;
		}

		fileSize = (ymu32)-1;

		if (pHeader->level != 0)					// NOTE: Endianness works because value is 0
		{ // Compression LH5, header !=0 : Error.
			free(pBigMalloc);
			pBigMalloc = NULL;
			setLastError("LHARC Header must be 0 !");
			return NULL;
		}

		fileSize = ReadLittleEndian32((ymu8*)&pHeader->original);
		pNew = (ymu8*)malloc(fileSize);
		if (!pNew)
		{
			setLastError("MALLOC Failed !");
			free(pBigMalloc);
			pBigMalloc = NULL;
			return NULL;
		}

		pSrc = pBigMalloc+sizeof(lzhHeader_t)+pHeader->name_lenght;			// NOTE: Endianness works because name_lenght is a byte

		pSrc += 2;		// skip CRC16

		if (!LzhDepackBlock(pSrc,pNew,fileSize))
		{
			setLastError("LH5 Depacking Error !");
			free(pNew);
			free(pBigMalloc);
			pNew = NULL;
			pBigMalloc = NULL;
			return NULL;
		}

		// Tout est bon, le fichier est depack‚, on free le bloque
		// pack‚ et on retourne le nouveau...
		free(pBigMalloc);
		return pNew;
 }





static ymint	fileSizeGet(FILE *h)
 {
 ymint size;
 ymint old;

		old = ftell(h);
		fseek(h,0,SEEK_END);
		size = ftell(h);
		fseek(h,old,SEEK_SET);
		return size;
 }


ymbool	CYmMusic::deInterleave(void)
 {
 yms32	nextPlane[32];
 ymu8	*pW,*tmpBuff;
 yms32	j,k;


		if (attrib&A_STREAMINTERLEAVED)
		{

			tmpBuff = (ymu8*)malloc(nbFrame*streamInc);
			if (!tmpBuff)
			{
				setLastError("Malloc error in deInterleave()\n");
				return YMFALSE;
			}

			// Precalcul les offsets.
			for (j=0;j<streamInc;j++) nextPlane[j] = nbFrame*j;

			pW = tmpBuff;
			for (j=0;j<nextPlane[1];j++)
			{
				for (k=0;k<streamInc;k++)
				{
					pW[k] = pDataStream[j + nextPlane[k]];
				}
				pW += streamInc;
			}

			free(pBigMalloc);
			pBigMalloc = tmpBuff;
			pDataStream = tmpBuff;

			attrib &= (~A_STREAMINTERLEAVED);
		}
		return YMTRUE;
 }



ymbool	CYmMusic::ymDecode(void)
 {
 ymu8 *pUD;
 ymu8	*ptr;
 ymint skip;
 ymint i;
 ymu32 sampleSize;
 yms32 tmp;
 ymu32 id;
 

		id = ReadBigEndian32((unsigned char*)pBigMalloc);
		switch (id)
		{
			case 'YM2!':		// MADMAX specific.
				songType = YM_V2;
				nbFrame = (fileSize-4)/14;
				loopFrame = 0;
				ymChip.setClock(ATARI_CLOCK);
				setPlayerRate(50);
				pDataStream = pBigMalloc+4;
				streamInc = 14;
				nbDrum = 0;
				setAttrib(A_STREAMINTERLEAVED|A_TIMECONTROL);
				pSongName = "";
				pSongAuthor = mstrdup("Unkonwn");
				pSongComment = mstrdup("Converted by Leonard.");
				pSongType = mstrdup("YM 2");
				pSongPlayer = mstrdup("YM-Chip driver.");
				break;

			case 'YM3!':		// Standart YM-Atari format.
				songType = YM_V3;
				nbFrame = (fileSize-4)/14;
				loopFrame = 0;
				ymChip.setClock(ATARI_CLOCK);
				setPlayerRate(50);
				pDataStream = pBigMalloc+4;
				streamInc = 14;
				nbDrum = 0;
				setAttrib(A_STREAMINTERLEAVED|A_TIMECONTROL);
				pSongName = ""; //mstrdup(tmpFName);
				pSongAuthor = mstrdup("Unkonwn");
				pSongComment = mstrdup("");
				pSongType = mstrdup("YM 3");
				pSongPlayer = mstrdup("YM-Chip driver.");
				break;

			case 'YM3b':		// Standart YM-Atari format + Loop info.
				pUD = (ymu8*)(pBigMalloc+fileSize-4);
				songType = YM_V3;
				nbFrame = (fileSize-4)/14;
				loopFrame = ReadLittleEndian32(pUD);
				ymChip.setClock(ATARI_CLOCK);
				setPlayerRate(50);
				pDataStream = pBigMalloc+4;
				streamInc = 14;
				nbDrum = 0;
				setAttrib(A_STREAMINTERLEAVED|A_TIMECONTROL);
				pSongName = ""; //mstrdup(tmpFName);
				pSongAuthor = mstrdup("Unkonwn");
				pSongComment = mstrdup("");
				pSongType = mstrdup("YM 3b (loop)");
				pSongPlayer = mstrdup("YM-Chip driver.");
				break;

			case 'YM4!':		// Extended ATARI format.
				setLastError("No more YM4! support. Use YM5! format.");
				return YMFALSE;
				break;

			case 'YM5!':		// Extended YM2149 format, all machines.
			case 'YM6!':		// Extended YM2149 format, all machines.
				if (strncmp((const char*)(pBigMalloc+4),"LeOnArD!",8))
				{
					setLastError("Not a valid YM format !");
					return YMFALSE;
				}
				ptr = pBigMalloc+12;
				nbFrame = readMotorolaDword(&ptr);
				setAttrib(readMotorolaDword(&ptr));
				nbDrum = readMotorolaWord(&ptr);
				ymChip.setClock(readMotorolaDword(&ptr));
				setPlayerRate(readMotorolaWord(&ptr));
				loopFrame = readMotorolaDword(&ptr);
				skip = readMotorolaWord(&ptr);
				ptr += skip;
				if (nbDrum>0)
				{
					pDrumTab=(digiDrum_t*)malloc(nbDrum*sizeof(digiDrum_t));
					for (i=0;i<nbDrum;i++)
					{
						pDrumTab[i].size = readMotorolaDword(&ptr);
						if (pDrumTab[i].size)
						{
							pDrumTab[i].pData = (ymu8*)malloc(pDrumTab[i].size);
							memcpy(pDrumTab[i].pData,ptr,pDrumTab[i].size);
							if (attrib&A_DRUM4BITS)
							{
								ymu32 j;
								ymu8 *pw = pDrumTab[i].pData;
								for (j=0;j<pDrumTab[i].size;j++)
								{
									*pw++ = ymVolumeTable[(*pw)&15]>>7;
								}
							}
							ptr += pDrumTab[i].size;
						}
						else
						{
							pDrumTab[i].pData = NULL;
						}
					}
					attrib &= (~A_DRUM4BITS);
				}
				pSongName = readNtString((char**)&ptr);
				pSongAuthor = readNtString((char**)&ptr);
				pSongComment = readNtString((char**)&ptr);
				songType = YM_V5;
				if (id=='YM6!')
				{
					songType = YM_V6;
					pSongType = mstrdup("YM 6");
				}
				else
				{
					pSongType = mstrdup("YM 5");
				}
				pDataStream = ptr;
				streamInc = 16;
				setAttrib(A_STREAMINTERLEAVED|A_TIMECONTROL);
				pSongPlayer = mstrdup("YM-Chip driver.");
				break;

			case 'MIX1':		// ATARI Remix digit format.

				if (strncmp((const char*)(pBigMalloc+4),"LeOnArD!",8))
				{
					setLastError("Not a valid YM format !");
					return YMFALSE;
				}
				ptr = pBigMalloc+12;
				songType = YM_MIX1;
				tmp = readMotorolaDword(&ptr);
				setAttrib(0);
				if (tmp&1) setAttrib(A_DRUMSIGNED);
				sampleSize = readMotorolaDword(&ptr);
				nbMixBlock = readMotorolaDword(&ptr);
				pMixBlock = (mixBlock_t*)malloc(nbMixBlock*sizeof(mixBlock_t));
				for (i=0;i<nbMixBlock;i++)
				{	// Lecture des block-infos.
					pMixBlock[i].sampleStart = readMotorolaDword(&ptr);
					pMixBlock[i].sampleLength = readMotorolaDword(&ptr);
					pMixBlock[i].nbRepeat = readMotorolaWord(&ptr);
					pMixBlock[i].replayFreq = readMotorolaWord(&ptr);
				}
				pSongName = readNtString((char**)&ptr);
				pSongAuthor = readNtString((char**)&ptr);
				pSongComment = readNtString((char**)&ptr);

				pBigSampleBuffer = (unsigned char*)malloc(sampleSize);
				memcpy(pBigSampleBuffer,ptr,sampleSize);

				if (!(attrib&A_DRUMSIGNED))
				{
					signeSample(pBigSampleBuffer,sampleSize);
					setAttrib(A_DRUMSIGNED);
				}

				mixPos = -1;		// numero du block info.
				pSongType = mstrdup("MIX1");
				pSongPlayer = mstrdup("Digi-Mix driver.");

				break;

			case 'YMT1':		// YM-Tracker
			case 'YMT2':		// YM-Tracker
/*;
; Format du YM-Tracker-1
;
; 4  YMT1
; 8  LeOnArD!
; 2  Nb voice
; 2  Player rate
; 4  Music lenght
; 4  Music loop
; 2  Nb digidrum
; 4  Flags		; Interlace, signed, 8 bits, etc...
; NT Music Name
; NT Music author
; NT Music comment
; nb digi *
*/
				if (strncmp((const char*)(pBigMalloc+4),"LeOnArD!",8))
				{
					setLastError("Not a valid YM format !");
					return YMFALSE;
				}
				ptr = pBigMalloc+12;
				songType = YM_TRACKER1;
				nbVoice = readMotorolaWord(&ptr);
				setPlayerRate(readMotorolaWord(&ptr));
				nbFrame= readMotorolaDword(&ptr);
				loopFrame = readMotorolaDword(&ptr);
				nbDrum = readMotorolaWord(&ptr);
				attrib = readMotorolaDword(&ptr);
				pSongName = readNtString((char**)&ptr);
				pSongAuthor = readNtString((char**)&ptr);
				pSongComment = readNtString((char**)&ptr);
				if (nbDrum>0)
				{
					pDrumTab=(digiDrum_t*)malloc(nbDrum*sizeof(digiDrum_t));
					for (i=0;i<(ymint)nbDrum;i++)
					{
						pDrumTab[i].size = readMotorolaWord(&ptr);
						pDrumTab[i].repLen = pDrumTab[i].size;
						if ('YMT2' == id)
						{
							pDrumTab[i].repLen = readMotorolaWord(&ptr);	// repLen
							readMotorolaWord(&ptr);		// flag
						}
						if (pDrumTab[i].repLen>pDrumTab[i].size)
						{
							pDrumTab[i].repLen = pDrumTab[i].size;
						}

						if (pDrumTab[i].size)
						{
							pDrumTab[i].pData = (ymu8*)malloc(pDrumTab[i].size);
							memcpy(pDrumTab[i].pData,ptr,pDrumTab[i].size);
							ptr += pDrumTab[i].size;
						}
						else
						{
							pDrumTab[i].pData = NULL;
						}
					}
				}

				ymTrackerFreqShift = 0;
				if ('YMT2' == id)
				{
					ymTrackerFreqShift = (attrib>>28)&15;
					attrib &= 0x0fffffff;
					pSongType = mstrdup("YM-T2");
				}
				else
				{
					pSongType = mstrdup("YM-T1");
				}


				pDataStream = ptr;
				ymChip.setClock(ATARI_CLOCK);

				ymTrackerInit(100);		// 80% de volume maxi.
				streamInc = 16;
				setTimeControl(YMTRUE);
				pSongPlayer = mstrdup("Universal Tracker");
				break;

			default:
				setLastError("Unknow YM format !");
				return YMFALSE;
				break;
		}

		if (!deInterleave())
		{
			return YMFALSE;
		}

		return YMTRUE;
 }

 
ymbool	CYmMusic::checkCompilerTypes()
{
	setLastError("Basic types size are not correct (check ymTypes.h)");

	if (1 != sizeof(ymu8)) return YMFALSE;
	if (1 != sizeof(yms8)) return YMFALSE;
	if (1 != sizeof(ymchar)) return YMFALSE;

	if (2 != sizeof(ymu16)) return YMFALSE;
	if (2 != sizeof(yms16)) return YMFALSE;
	if (4 != sizeof(ymu32)) return YMFALSE;
	if (4 != sizeof(yms32)) return YMFALSE;

	if (2 != sizeof(ymsample)) return YMFALSE;

#ifdef YM_INTEGER_ONLY
	if (8 != sizeof(yms64)) return YMFALSE;
#endif	

	if (sizeof(ymint) < 4) return YMFALSE;		// ymint should be at least 32bits

	setLastError("");
	return YMTRUE;
}


ymbool	CYmMusic::load(const char *fileName)
{
FILE	*in;


		stop();
		unLoad();

		if (!checkCompilerTypes())
			return YMFALSE;

		in = fopen(fileName,"rb");
		if (!in)
		{
			setLastError("File not Found");
			return YMFALSE;
		}

		//---------------------------------------------------
		// Allocation d'un buffer pour lire le fichier.
		//---------------------------------------------------
		fileSize = fileSizeGet(in);
		pBigMalloc = (unsigned char*)malloc(fileSize);
		if (!pBigMalloc)
		{
			setLastError("MALLOC Error");
			fclose(in);
			return YMFALSE;
		}

		//---------------------------------------------------
		// Chargement du fichier complet.
		//---------------------------------------------------
		if (fread(pBigMalloc,fileSize,1,in)!=(size_t)1)
		{
			free(pBigMalloc);
			setLastError("File is corrupted.");
			fclose(in);
			return YMFALSE;
		}
		fclose(in);

		//---------------------------------------------------
		// Transforme les donn‚es en donn‚es valides.
		//---------------------------------------------------
		pBigMalloc = depackFile();
		if (!pBigMalloc)
		{
			return YMFALSE;
		}

		//---------------------------------------------------
		// Lecture des donn‚es YM:
		//---------------------------------------------------
		if (!ymDecode())
		{
			free(pBigMalloc);
			pBigMalloc = NULL;
			return YMFALSE;
		}

		ymChip.reset();
		bMusicOk = YMTRUE;
		bPause = YMFALSE;
		return YMTRUE;
 }

ymbool	CYmMusic::loadMemory(void *pBlock,ymu32 size)
{


		stop();
		unLoad();

		if (!checkCompilerTypes())
			return YMFALSE;

		//---------------------------------------------------
		// Allocation d'un buffer pour lire le fichier.
		//---------------------------------------------------
		fileSize = size;
		pBigMalloc = (unsigned char*)malloc(fileSize);
		if (!pBigMalloc)
		{
			setLastError("MALLOC Error");
			return YMFALSE;
		}

		//---------------------------------------------------
		// Chargement du fichier complet.
		//---------------------------------------------------
		memcpy(pBigMalloc,pBlock,size);

		//---------------------------------------------------
		// Transforme les donn‚es en donn‚es valides.
		//---------------------------------------------------
		pBigMalloc = depackFile();
		if (!pBigMalloc)
		{
			return YMFALSE;
		}

		//---------------------------------------------------
		// Lecture des donn‚es YM:
		//---------------------------------------------------
		if (!ymDecode())
		{
			free(pBigMalloc);
			pBigMalloc = NULL;
			return YMFALSE;
		}

		ymChip.reset();
		bMusicOk = YMTRUE;
		bPause = YMFALSE;
		return YMTRUE;
 }

void	myFree(void **pPtr)
{
		if (*pPtr) free(*pPtr);
		*pPtr = NULL;
}

void	CYmMusic::unLoad(void)
{

		bMusicOk = YMFALSE;
		bPause = YMTRUE;
		bMusicOver = YMFALSE;
//		myFree((void**)&pSongName);
		myFree((void**)&pSongAuthor);
		myFree((void**)&pSongComment);
		myFree((void**)&pSongType);
		myFree((void**)&pSongPlayer);
		myFree((void**)&pBigMalloc);
		if (nbDrum>0)
		{
			for (ymint i=0;i<nbDrum;i++)
			{
				myFree((void**)&pDrumTab[i].pData);
			}
			nbDrum = 0;
			myFree((void**)&pDrumTab);
		}
		myFree((void**)&pBigSampleBuffer);
		myFree((void**)&pMixBlock);

}

void	CYmMusic::stop(void)
{
	bPause = YMTRUE;
	currentFrame = 0;
	mixPos = -1;
}

void	CYmMusic::play(void)
{
	bPause = YMFALSE;
}

void	CYmMusic::pause(void)
{
	bPause = YMTRUE;
}
