/**********************************************************************************************************************************************************	
*	MikMod sound library
*	(c) 1998, 1999 Miodrag Vallat and others - see file AUTHORS for
*	complete list.
*
*	This library is free software; you can redistribute it and/or modify
*	it under the terms of the GNU Library General Public License as
*	published by the Free Software Foundation; either version 2 of
*	the License, or (at your option) any later version.
* 
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU Library General Public License for more details.
* 
*	You should have received a copy of the GNU Library General Public
*	License along with this library; if not, write to the Free Software
*	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
*	02111-1307, USA.
*
*	Mikwin unofficial mikmod port. / XBOX PORT
*
***********************************************************************************************************************************************************
*   Last Revision : Rev 2.1 6/21/2003
*   PeteGabe <Peter Knepley> hacked this to work on XBOX
*
*   Rev 2.0 01/06/2002
*		Modified functions prototypes.
*		Added SFX management using a ring buffer.
*		Added compilation defines for all kind of supported tracker formats. (see mikmod.h)
*		Fixed bugs in libmikmod (probs with effects channels reinitialisation when module was wrapping)
*		Now modules are wrapping by default.
*		Ozzy <ozzy@orkysquad.org> -> http://www.orkysquad.org
*
*   Rev 1.2 23/08/2001
*		Modified DirectSound buffer management from double buffering to a 50Hz playing system.
*		This change prevent some rastertime overheads during the MikMod_update()..
*		Replay calls are now much more stable in this way! :)
*		Ozzy <ozzy@orkysquad.org> -> http://www.orkysquad.org
*		
*	Rev 1.1 15/02/1999	
*		first & nice DirectSound Double-buffered implementation.
*		Jörg Mensmann <joerg.mensmann@gmx.net>	
*
*
**TABULATION 4*******RESOLUTION : 1280*1024 ***************************************************************************************************************/
#include <mikmod.h>
#include "mikmod_internals.h"
#include "mikwin.h"

/*Variable export/import----------------------------------------------------------------------------------------------------------------------------------*/ 
static SAMPLE *sfxRing[UF_MAXCHAN];
static BYTE firstSfxVoice;
static BYTE currentSfxVoice;
static BYTE resMusicChannels;
static BYTE resSfxChannels;

#ifdef WIN32
extern void set_ds_hwnd(HWND wnd);
extern BOOL DS_Init(void);
extern void DS_Exit(void);
extern void set_ds_buffersize(int);
#endif
/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/


#define DEFAULT_MIXFREQ 22500
/********************************************************************************************************************************************************** 

																	... mikwinInit(...) ...

***********************************************************************************************************************************************************/
BOOL mikwinInit(UWORD mixfreq, BOOL stereo, BOOL bits16, BOOL interpolation,BYTE reservedMusicChannels,BYTE reservedSfxChannels,BYTE soundDelay )
{
	UWORD options;
	char *pInfos;
	BOOL initialised;

	/* register all the drivers */
	#ifdef WIN32
	set_ds_buffersize(soundDelay);
	MikMod_RegisterDriver(&drv_ds_raw);
	#endif

    /* register all the module loaders */
	MikMod_RegisterAllLoaders();

	/* initialize the library */
	if (mixfreq == 0) mixfreq = DEFAULT_MIXFREQ;
	md_mixfreq = mixfreq;
	options = DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX;
	if (stereo) options |= DMODE_STEREO;
	if (bits16) options |= DMODE_16BITS;
	if (interpolation) options |= DMODE_INTERP;

	md_mode = options;
    
	
	#ifndef _XBOX
	set_ds_hwnd(GetActiveWindow());
	#endif
	
	pInfos = MikMod_InfoDriver();
	initialised = !MikMod_Init(pInfos);

	if (reservedMusicChannels > UF_MAXCHAN)
		reservedMusicChannels = UF_MAXCHAN;
	if (reservedSfxChannels > UF_MAXCHAN)
		reservedSfxChannels = UF_MAXCHAN;

	resMusicChannels = reservedMusicChannels;
	resSfxChannels = reservedSfxChannels;

	firstSfxVoice = reservedMusicChannels;
	currentSfxVoice = 0;
	memset(sfxRing,0,sizeof(SAMPLE*)*UF_MAXCHAN);

	if (initialised){
		MikMod_SetNumVoices(reservedMusicChannels,reservedSfxChannels);          
		MikMod_EnableOutput();
	}
	return initialised;
}
/********************************************************************************************************************************************************** 

																		... mikwinExit(...) ...

***********************************************************************************************************************************************************/
void mikwinExit()
{
	MikMod_Exit();
}
/********************************************************************************************************************************************************** 

																	... mikwinSetSoundDelay(...) ...

  Note: Depending on the system it is sometime preferable to allow delay (bigger sound buffer size) to avoid glitches.
		For instance 8 is a value for good configs and 14 works nice almost everywhere.
		For Ultrasounds running with latest beta drivers (WIN) that value should be 40! :( 
		really big delay there but that works.

***********************************************************************************************************************************************************/
void	mikwinSetSoundDelay(BYTE soundDelay)
{
	DS_Exit();
	set_ds_buffersize(soundDelay);
	DS_Init();
	
}
/********************************************************************************************************************************************************** 

																	... mikwinSetErrno(...) ...

***********************************************************************************************************************************************************/
void	mikwinSetErrno(int errno)
{
	MikMod_errno = errno;
}
/********************************************************************************************************************************************************** 

																	... mikwinGetErrno(...) ...

***********************************************************************************************************************************************************/
int		mikwinGetErrno(void)
{
	return MikMod_errno;
}
/********************************************************************************************************************************************************** 

																	... mikwinSetMasterVolume(...) ...

***********************************************************************************************************************************************************/
void	mikwinSetMasterVolume(UBYTE vol)
{
	md_volume = vol;
}
/********************************************************************************************************************************************************** 

																	... mikwinGetMasterVolume(...) ...

***********************************************************************************************************************************************************/
UBYTE	mikwinGetMasterVolume(void)
{
	return md_volume;
}
/********************************************************************************************************************************************************** 

																	... mikwinSetMusicVolume(...) ...

  note: when playing music + sfx that's better to use 75% volume onto music coz sfx volume is a bit low by default.

***********************************************************************************************************************************************************/
void	mikwinSetMusicVolume(UBYTE vol)
{
	md_musicvolume = vol;
}
/********************************************************************************************************************************************************** 

																	... mikwinGetMusicVolume(...) ...

***********************************************************************************************************************************************************/
UBYTE	mikwinGetMusicVolume(void)
{
	return md_musicvolume;
}
/********************************************************************************************************************************************************** 

																	... mikwinSetSfxVolume(...) ...

***********************************************************************************************************************************************************/
void	mikwinSetSfxVolume(UBYTE vol)
{
	md_sndfxvolume = vol;
}
/********************************************************************************************************************************************************** 

																	... mikwinGetSfxVolume(...) ...

***********************************************************************************************************************************************************/
UBYTE	mikwinGetSfxVolume(void)
{
	return md_sndfxvolume;
}
/********************************************************************************************************************************************************** 

																	... mikwinSetMasterReverb(...) ...

***********************************************************************************************************************************************************/
void	mikwinSetMasterReverb(UBYTE rev)
{
	md_reverb = rev;
}
/********************************************************************************************************************************************************** 

																	... mikwinGetMasterReverb(...) ...

***********************************************************************************************************************************************************/
UBYTE	mikwinGetMasterReverb(void)
{
	return md_reverb;
}
/********************************************************************************************************************************************************** 

																	... mikwinSetPanning(...) ...

***********************************************************************************************************************************************************/
void	mikwinSetPanning(UBYTE pan)
{
	md_pansep = pan;
}
/********************************************************************************************************************************************************** 

																	... mikwinGetPanning(...) ...

***********************************************************************************************************************************************************/
UBYTE	mikwinGetPanning(void)
{
	return md_pansep;
}
/********************************************************************************************************************************************************** 

																	... mikwinSetMasterDevice(...) ...

***********************************************************************************************************************************************************/
void	mikwinSetMasterDevice(UWORD dev)
{
	md_device = dev;
}
/********************************************************************************************************************************************************** 

																	... mikwinGetMasterDevice(...) ...

***********************************************************************************************************************************************************/
UWORD	mikwinGetMasterDevice(void)
{
	return md_device;
}
/********************************************************************************************************************************************************** 

																	... mikwinSetMixFrequency(...) ...

***********************************************************************************************************************************************************/
void	mikwinSetMixFrequency(UWORD freq)
{
	md_mixfreq = freq;
}
/********************************************************************************************************************************************************** 

																	... mikwinGetMixFrequency(...) ...

***********************************************************************************************************************************************************/
UWORD	mikwinGetMixFrequency(void)
{
	return md_mixfreq;
}
/********************************************************************************************************************************************************** 

																		... mikwinSetMode(...) ...

***********************************************************************************************************************************************************/
void	mikwinSetMode(UWORD mode)
{
	md_mode = mode;
}
/********************************************************************************************************************************************************** 

																		... mikwinGetMode(...) ...

***********************************************************************************************************************************************************/
UWORD	mikwinGetMode(void)
{
	return md_mode;
}
/********************************************************************************************************************************************************** 

																	... mikwinPlaySfx(...) ...

  Note: Unfortunately, effects are applyed on the sample itself and not on the voice where it is played.
		Thus, multiple instances of a sfx with different types of effects should sound eh... weird.

***********************************************************************************************************************************************************/
SWORD mikwinPlaySfx(SAMPLE *pSample,ULONG flags,UWORD pan,UWORD vol,ULONG frequency)
{
	SWORD voice; 
	ULONG startPos=0;
	BYTE i;

	if (pSample == NULL)
		return -1;

	voice = firstSfxVoice+currentSfxVoice;

	for (i=0; i < resSfxChannels;i++){
		voice = firstSfxVoice+i;
		if(sfxRing[i] == NULL){
			sfxRing[i] = pSample;
			goto skipVoiceInc;
		}
		else{
			if (Voice_Stopped((CHAR)voice)){
				sfxRing[i] = pSample;
				goto skipVoiceInc;
			}
		}
	}

	sfxRing[currentSfxVoice++] = pSample;
	currentSfxVoice %= resSfxChannels;

skipVoiceInc:

	pSample->loopstart = 0;
	pSample->loopend = pSample->length;

	if (pSample->flags&SF_16BITS)
		pSample->loopend <<=1;

	if (flags&SF_LOOP){
		pSample->flags |= SF_LOOP;
	}
	if (flags&SF_BIDI){
		pSample->flags |= SF_BIDI;
	}
	if (flags&SF_REVERSE){
		pSample->flags |= SF_REVERSE;
		startPos = pSample->loopend;
	}
	
	Voice_Play((CHAR)voice,pSample,startPos);
	Voice_SetPanning((CHAR)voice,pan);
	Voice_SetVolume((CHAR)voice,vol);
	if (frequency)
		Voice_SetFrequency((CHAR)voice,frequency);

	return voice;
}
/********************************************************************************************************************************************************** 

																		... mikwinStopSfx(...) ...

***********************************************************************************************************************************************************/
void mikwinStopSfx(SWORD voice)
{
	if ((voice < resMusicChannels) || (voice >= (firstSfxVoice+resSfxChannels)))
		return;

	sfxRing[voice-firstSfxVoice] = NULL;
	Voice_Stop((CHAR)voice);
}
/********************************************************************************************************************************************************** 

																		... mikwinGetSfx(...) ...

***********************************************************************************************************************************************************/
SAMPLE* mikwinGetSfx(SWORD voice)
{
	if ((voice < resMusicChannels) || (voice >= (firstSfxVoice+resSfxChannels)))
		return NULL;

	return sfxRing[voice-firstSfxVoice];
}


