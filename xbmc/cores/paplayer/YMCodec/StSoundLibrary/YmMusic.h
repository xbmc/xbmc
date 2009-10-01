/*-----------------------------------------------------------------------------

	ST-Sound ( YM files player library )

	Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )

	YM Music Driver

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


#ifndef __YMMUSIC__
#define __YMMUSIC__

#include "YmTypes.h"
#include "StSoundLibrary.h"
#include "Ym2149Ex.h"
#include "Ymload.h"
#include "digidrum.h"

#define	MAX_DIGIDRUM	128

#define	YMTPREC		16
#define	MAX_VOICE	8
#define	PC_DAC_FREQ	44100
#define	YMTNBSRATE	(PC_DAC_FREQ/50)

typedef enum
{
	YM_V2,
	YM_V3,
	YM_V4,
	YM_V5,
	YM_V6,
	YM_VMAX,

	YM_TRACKER1=32,
	YM_TRACKER2,
	YM_TRACKERMAX,

	YM_MIX1=64,
	YM_MIX2,
	YM_MIXMAX,
} ymFile_t;

typedef struct
{
	ymu32	sampleStart;
	ymu32	sampleLength;
	ymu16	nbRepeat;
	ymu16	replayFreq;
} mixBlock_t;

typedef struct
{
	ymu32		size;
	ymu8	*	pData;
	ymu32		repLen;
} digiDrum_t;

typedef struct
{
	ymint		nbVoice;
	ymu32		nbVbl;
	ymu8	*	pDataBufer;
	ymu32		currentVbl;
	ymu32		flags;
	ymbool		bLoop;
} ymTrackerPartoche_t;


typedef struct
{
	ymu8	*	pSample;
	ymu32		sampleSize;
	ymu32		samplePos;
	ymu32		repLen;
	yms32		sampleVolume;
	ymu32		sampleFreq;
	ymbool		bLoop;
	ymbool		bRunning;
} ymTrackerVoice_t;

typedef struct
{
	ymu8 noteOn;
	ymu8 volume;
	ymu8 freqHigh;
	ymu8 freqLow;
} ymTrackerLine_t;


enum
{
	A_STREAMINTERLEAVED = 1,
	A_DRUMSIGNED = 2,
	A_DRUM4BITS = 4,
	A_TIMECONTROL = 8,
	A_LOOPMODE = 16,
};


class	CYmMusic
{

public:
	CYmMusic(ymint _replayRate=44100);
	~CYmMusic();

	ymbool	load(const char *pName);
	ymbool	loadMemory(void *pBlock,ymu32 size);

	void	unLoad(void);
	ymbool	isSeekable(void);
	ymbool	update(ymsample *pBuffer,ymint nbSample);
	ymu32	getPos(void);
	ymu32	getMusicTime(void);
	ymu32	setMusicTime(ymu32 time);
	void	play(void);
	void	pause(void);
	void	stop(void);
	void	setVolume(ymint volume);
	int		getAttrib(void);
	void	getMusicInfo(ymMusicInfo_t *pInfo);
	void	setLoopMode(ymbool bLoop);
	char	*getLastError(void);
	int		 readYmRegister(ymint reg)		{ return ymChip.readRegister(reg); }

//-------------------------------------------------------------
// WAVE Generator
//-------------------------------------------------------------
	int		waveCreate(char *fName);

	ymbool	bMusicOver;

private:

	ymbool	checkCompilerTypes();

	void	setPlayerRate(int rate);
	void	setAttrib(int _attrib);
	void	setLastError(char *pError);
	ymu8 *depackFile(void);
	ymbool	deInterleave(void);
	void	readYm6Effect(ymu8 *pReg,int code,int prediv,int count);
	void	player(void);
	void	setTimeControl(ymbool bFlag);


	CYm2149Ex	ymChip;
	char	*pLastError;
	ymFile_t	songType;
	int		nbFrame;
	int		loopFrame;
	int		currentFrame;
	int		nbDrum;
	digiDrum_t *pDrumTab;
	int		musicTime;
	ymu8 *pBigMalloc;
	ymu8 *pDataStream;
	ymbool	bLoop;
	ymint	fileSize;
	ymbool	ymDecode(void);
	ymint		playerRate;
	ymint		attrib;
	volatile	ymbool	bMusicOk;
	volatile	ymbool	bPause;
	ymint		streamInc;
	ymint		innerSamplePos;
	ymint		replayRate;

	ymchar	*pSongName;
	ymchar	*pSongAuthor;
	ymchar	*pSongComment;
	ymchar	*pSongType;
	ymchar	*pSongPlayer;

//-------------------------------------------------------------
// ATARI Digi Mix Music.
//-------------------------------------------------------------
	void	readNextBlockInfo(void);
	void	stDigitMix(signed short *pWrite16,int nbs);
	ymint	nbRepeat;
	ymint	nbMixBlock;
	mixBlock_t *pMixBlock;
	ymint	mixPos;
	ymu8 *pBigSampleBuffer;
	ymu8	*pCurrentMixSample;
	ymu32	currentSampleLength;
	ymu32	currentPente;
	ymu32	currentPos;

//-------------------------------------------------------------
// YM-Universal-Tracker
//-------------------------------------------------------------
	void	ymTrackerInit(int volMaxPercent);
	void	ymTrackerUpdate(signed short *pBuffer,int nbSample);
	void	ymTrackerDesInterleave(void);
	void	ymTrackerPlayer(ymTrackerVoice_t *pVoice);
	void	ymTrackerVoiceAdd(ymTrackerVoice_t *pVoice,signed short *pBuffer,int nbs);

	int			nbVoice;
	ymTrackerVoice_t	ymTrackerVoice[MAX_VOICE];
	int					ymTrackerNbSampleBefore;
	signed short		ymTrackerVolumeTable[256*64];
	int					ymTrackerFreqShift;


};

/*
		int	version;
		SD	pos;
		UD	inc;
		UD	timeSec;
		UD	timeMin;
		UD	loopSec;
		UD	loopMin;
		UD	nbVbl;
		UD	vblRestart;
		UD	ymFreq;
		UD	playerFreq;
		UB	*pRegister;
		UB	*pFileBuffer;
		UD	fileSize;
		UD	attrib;
		char *pSongName;
		char *pSongComment;
		char *pSongAuthor;
		mixBlock_t *pMixBlock;
		long	nbMixBlock;
		UD	nbDrum;
		digiDrum_t *pDrumTab;
		int	nbVoice;
		ymu32 currentVbl;
		int	bTimeControl;
		int	timeTotal;
		int	ymtFreqShift;
*/

#endif
