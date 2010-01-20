/*-----------------------------------------------------------------------------

	ST-Sound ( YM files player library )

	Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )

	Extended YM-2149 Emulator, with ATARI music demos effects.
	(SID-Like, Digidrum, Sync Buzzer, Sinus SID and Pattern SID)

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


#ifndef __YM2149EX__
#define __YM2149EX__

#define	AMSTRAD_CLOCK	1000000L
#define	ATARI_CLOCK		2000000L
#define	SPECTRUM_CLOCK	1773400L
#define	MFP_CLOCK		2457600L
#define	NOISESIZE		16384
#define	MAX_NBSAMPLE	1024
#define	DRUM_PREC		15

#define	PI				3.1415926
#define	SIDSINPOWER		0.7

enum
{
	VOICE_A=0,
	VOICE_B=1,
	VOICE_C=2,
};

struct	YmSpecialEffect
{

		ymbool		bDrum;
		ymu32		drumSize;
		ymu8	*	drumData;
		ymu32		drumPos;
		ymu32		drumStep;

		ymbool		bSid;
		ymu32		sidPos;
		ymu32		sidStep;
		ymint		sidVol;

};

static	const	ymint		DC_ADJUST_BUFFERLEN		=	512;

class	CDcAdjuster
{
public:
	CDcAdjuster();
	
	void	AddSample(ymint sample);
	ymint	GetDcLevel(void)			{ return m_sum / DC_ADJUST_BUFFERLEN; }
	void	Reset(void);

private:
	ymint	m_buffer[DC_ADJUST_BUFFERLEN];
	ymint		m_pos;
	ymint		m_sum;
};


class CYm2149Ex
{
public:
		CYm2149Ex(ymu32 masterClock=ATARI_CLOCK,ymint prediv=1,ymu32 playRate=44100);
		~CYm2149Ex();

		void	reset(void);
		void	update(signed short *pSampleBuffer,ymint nbSample);

		void	setClock(ymu32 _clock);
		void	writeRegister(ymint reg,ymint value);
		ymint	readRegister(ymint reg);
		void	drumStart(ymint voice,ymu8 *drumBuffer,ymu32 drumSize,ymint drumFreq);
		void	drumStop(ymint voice);
		void	sidStart(ymint voice,ymint freq,ymint vol);
		void	sidSinStart(ymint voice,ymint timerFreq,ymint sinPattern);
		void	sidStop(ymint voice);
		void	syncBuzzerStart(ymint freq,ymint envShape);
		void	syncBuzzerStop(void);


private:

		CDcAdjuster		m_dcAdjust;

		ymu32	frameCycle;
		ymu32	cyclePerSample;
		inline	ymsample nextSample(void);
		ymu32 toneStepCompute(ymint rHigh,ymint rLow);
		ymu32 noiseStepCompute(ymint rNoise);
		ymu32 envStepCompute(ymint rHigh,ymint rLow);
		void	updateEnvGen(ymint nbSample);
		void	updateNoiseGen(ymint nbSample);
		void	updateToneGen(ymint voice,ymint nbSample);
		ymu32	rndCompute(void);

		void	sidVolumeCompute(ymint voice,ymint *pVol);

		ymint	replayFrequency;
		ymu32	internalClock;
		ymint	registers[14];

		ymu32	cycleSample;
		ymu32	stepA,stepB,stepC;
		ymu32	posA,posB,posC;
		ymint				volA,volB,volC,volE;
		ymu32	mixerTA,mixerTB,mixerTC;
		ymu32	mixerNA,mixerNB,mixerNC;
		ymint				*pVolA,*pVolB,*pVolC;

		ymu32	noiseStep;
		ymu32 noisePos;
		ymu32	rndRack;
		ymu32	currentNoise;
		ymu32	bWrite13;

		ymu32	envStep;
		ymu32 envPos;
		ymint		envPhase;
		ymint		envShape;
		ymu8	envData[16][2][16*2];
		ymint	globalVolume;

		struct	YmSpecialEffect	specialEffect[3];
		ymbool	bSyncBuzzer;
		ymu32	syncBuzzerStep;
		ymu32	syncBuzzerPhase;
		ymint	syncBuzzerShape;


};

#endif
