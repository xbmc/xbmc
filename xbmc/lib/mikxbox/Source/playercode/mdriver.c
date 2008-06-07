/*	MikMod sound library
	(c) 1998, 1999, 2000, 2001 Miodrag Vallat and others - see file AUTHORS
	for complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id$

  These routines are used to access the available soundcard drivers.

==============================================================================*/

#include "xbsection_start.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined unix || (defined __APPLE__ && defined __MACH__)
#include <pwd.h>
#include <sys/stat.h>
#endif

#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include "mikmod.h"
#include "mikmod_internals.h"

#ifdef SUNOS
extern int fprintf(FILE *, const char *, ...);
#endif

static	MDRIVER *firstdriver=NULL;
MIKMODAPI	MDRIVER *md_driver=NULL;
extern	MODULE *pf; /* modfile being played */

/* Initial global settings */
MIKMODAPI	UWORD md_device         = 0;	/* autodetect */
MIKMODAPI	UWORD md_mixfreq        = 44100;
MIKMODAPI	UWORD md_mode           = DMODE_STEREO | DMODE_16BITS |
									  DMODE_SURROUND |DMODE_SOFT_MUSIC |
									  DMODE_SOFT_SNDFX;
MIKMODAPI	UBYTE md_pansep         = 128;	/* 128 == 100% (full left/right) */
MIKMODAPI	UBYTE md_reverb         = 0;	/* no reverb */
MIKMODAPI	UBYTE md_volume         = 128;	/* global sound volume (0-128) */
MIKMODAPI	UBYTE md_musicvolume    = 128;	/* volume of song */
MIKMODAPI	UBYTE md_sndfxvolume    = 128;	/* volume of sound effects */
			UWORD md_bpm            = 125;	/* tempo */

/* Do not modify the numchn variables yourself!  use MD_SetVoices() */
			UBYTE md_numchn=0,md_sngchn=0,md_sfxchn=0;
			UBYTE md_hardchn=0,md_softchn=0;

			void (*md_player)(void) = Mod_Player_HandleTick;
static		BOOL  isplaying=0;
BOOL		initialized = 0;
static		UBYTE *sfxinfo;
static		int sfxpool;

static		SAMPLE **md_sample = NULL;

/* Previous driver in use */
static		SWORD olddevice = -1;

/* Limits the number of hardware voices to the specified amount.
   This function should only be used by the low-level drivers. */
static	void LimitHardVoices(int limit)
{
	int t=0;

	if (!(md_mode & DMODE_SOFT_SNDFX) && (md_sfxchn>limit)) md_sfxchn=limit;
	if (!(md_mode & DMODE_SOFT_MUSIC) && (md_sngchn>limit)) md_sngchn=limit;

	if (!(md_mode & DMODE_SOFT_SNDFX))
		md_hardchn=md_sfxchn;
	else
		md_hardchn=0;

	if (!(md_mode & DMODE_SOFT_MUSIC)) md_hardchn += md_sngchn;

	while (md_hardchn>limit) {
		if (++t & 1) {
			if (!(md_mode & DMODE_SOFT_SNDFX) && (md_sfxchn>4)) md_sfxchn--;
		} else {
			if (!(md_mode & DMODE_SOFT_MUSIC) && (md_sngchn>8)) md_sngchn--;
		}

		if (!(md_mode & DMODE_SOFT_SNDFX))
			md_hardchn=md_sfxchn;
		else
			md_hardchn=0;

		if (!(md_mode & DMODE_SOFT_MUSIC))
			md_hardchn+=md_sngchn;
	}
	md_numchn=md_hardchn+md_softchn;
}

/* Limits the number of hardware voices to the specified amount.
   This function should only be used by the low-level drivers. */
static	void LimitSoftVoices(int limit)
{
	int t=0;

	if ((md_mode & DMODE_SOFT_SNDFX) && (md_sfxchn>limit)) md_sfxchn=limit;
	if ((md_mode & DMODE_SOFT_MUSIC) && (md_sngchn>limit)) md_sngchn=limit;

	if (md_mode & DMODE_SOFT_SNDFX)
		md_softchn=md_sfxchn;
	else
		md_softchn=0;

	if (md_mode & DMODE_SOFT_MUSIC) md_softchn+=md_sngchn;

	while (md_softchn>limit) {
		if (++t & 1) {
			if ((md_mode & DMODE_SOFT_SNDFX) && (md_sfxchn>4)) md_sfxchn--;
		} else {
			if ((md_mode & DMODE_SOFT_MUSIC) && (md_sngchn>8)) md_sngchn--;
		}

		if (!(md_mode & DMODE_SOFT_SNDFX))
			md_softchn=md_sfxchn;
		else
			md_softchn=0;

		if (!(md_mode & DMODE_SOFT_MUSIC))
			md_softchn+=md_sngchn;
	}
	md_numchn=md_hardchn+md_softchn;
}

/* Note: 'type' indicates whether the returned value should be for music or for
   sound effects. */
ULONG MD_SampleSpace(int type)
{
	if(type==MD_MUSIC)
		type=(md_mode & DMODE_SOFT_MUSIC)?MD_SOFTWARE:MD_HARDWARE;
	else if(type==MD_SNDFX)
		type=(md_mode & DMODE_SOFT_SNDFX)?MD_SOFTWARE:MD_HARDWARE;

	return md_driver->FreeSampleSpace(type);
}

ULONG MD_SampleLength(int type,SAMPLE* s)
{
	if(type==MD_MUSIC)
		type=(md_mode & DMODE_SOFT_MUSIC)?MD_SOFTWARE:MD_HARDWARE;
	else
	  if(type==MD_SNDFX)
		type=(md_mode & DMODE_SOFT_SNDFX)?MD_SOFTWARE:MD_HARDWARE;

	return md_driver->RealSampleLength(type,s);
}

MIKMODAPI CHAR* MikMod_InfoDriver(void)
{
	int t,len=0;
	MDRIVER *l;
	CHAR *list=NULL;

	MUTEX_LOCK(lists);
	/* compute size of buffer */
	for(l=firstdriver;l;l=l->next)
		len+=4+(l->next?1:0)+strlen(l->Version);

	if(len)
		if((list=_mm_malloc(len*sizeof(CHAR)))) {
			list[0]=0;
			/* list all registered device drivers : */
			for(t=1,l=firstdriver;l;l=l->next,t++)
				sprintf(list,(l->next)?"%s%2d %s\n":"%s%2d %s",
				    list,t,l->Version);
		}
	MUTEX_UNLOCK(lists);
	return list;
}

void _mm_registerdriver(struct MDRIVER* drv)
{
	MDRIVER *cruise = firstdriver;

	if (firstdriver == drv)
		return;

	/* don't register a MISSING() driver */
	if ((drv->Name) && (drv->Version)) {
		if (cruise) {
			while (cruise->next)
				cruise = cruise->next;
			cruise->next = drv;
		} else
			firstdriver = drv; 
	}
}

MIKMODAPI void MikMod_RegisterDriver(struct MDRIVER* drv)
{
	/* if we try to register an invalid driver, or an already registered driver,
	   ignore this attempt */
	if ((!drv)||(drv->next)||(!drv->Name))
		return;

	MUTEX_LOCK(lists);
	_mm_registerdriver(drv);
	MUTEX_UNLOCK(lists);
}

MIKMODAPI int MikMod_DriverFromAlias(CHAR *alias)
{
	int rank=1;
	MDRIVER *cruise;

	MUTEX_LOCK(lists);
	cruise=firstdriver;
	while(cruise) {
		if (cruise->Alias) {
			if (!(strcasecmp(alias,cruise->Alias))) break;
			rank++;
		}
		cruise=cruise->next;
	}
	if(!cruise) rank=0;
	MUTEX_UNLOCK(lists);

	return rank;
}

SWORD MD_SampleLoad(SAMPLOAD* s, int type)
{
	SWORD result;

	if(type==MD_MUSIC)
		type=(md_mode & DMODE_SOFT_MUSIC)?MD_SOFTWARE:MD_HARDWARE;
	else if(type==MD_SNDFX)
		type=(md_mode & DMODE_SOFT_SNDFX)?MD_SOFTWARE:MD_HARDWARE;

	SL_Init(s);
	result=md_driver->SampleLoad(s,type);
	SL_Exit(s);

	return result;
}

void MD_SampleUnload(SWORD handle)
{
	md_driver->SampleUnload(handle);
}

MIKMODAPI MikMod_player_t MikMod_RegisterPlayer(MikMod_player_t player)
{
	MikMod_player_t result;

	MUTEX_LOCK(vars);
	result=md_player;
	md_player=player;
	MUTEX_UNLOCK(vars);

	return result;
}

MIKMODAPI void MikMod_Update(void)
{
	MUTEX_LOCK(vars);
	if(isplaying) {
		if((!pf)||(!pf->forbid))
			md_driver->Update();
		else {
			if (md_driver->Pause)
				md_driver->Pause();
		}
	}
	MUTEX_UNLOCK(vars);
}

void Voice_SetVolume_internal(SBYTE voice,UWORD vol)
{
	ULONG  tmp;

	if((voice<0)||(voice>=md_numchn)) return;

	/* range checks */
	if(md_musicvolume>128) md_musicvolume=128;
	if(md_sndfxvolume>128) md_sndfxvolume=128;
	if(md_volume>128) md_volume=128;

	tmp=(ULONG)vol*(ULONG)md_volume*
	     ((voice<md_sngchn)?(ULONG)md_musicvolume:(ULONG)md_sndfxvolume);
	md_driver->VoiceSetVolume(voice,tmp/16384UL);
}

MIKMODAPI void Voice_SetVolume(SBYTE voice,UWORD vol)
{
	MUTEX_LOCK(vars);
	Voice_SetVolume_internal(voice,vol);
	MUTEX_UNLOCK(vars);
}

MIKMODAPI UWORD Voice_GetVolume(SBYTE voice)
{
	UWORD result=0;

	MUTEX_LOCK(vars);
	if((voice>=0)&&(voice<md_numchn))
		result=md_driver->VoiceGetVolume(voice);
	MUTEX_UNLOCK(vars);

	return result;
}

void Voice_SetFrequency_internal(SBYTE voice,ULONG frq)
{
	if((voice<0)||(voice>=md_numchn)) return;
	if((md_sample[voice])&&(md_sample[voice]->divfactor))
		frq/=md_sample[voice]->divfactor;
	md_driver->VoiceSetFrequency(voice,frq);
}

MIKMODAPI void Voice_SetFrequency(SBYTE voice,ULONG frq)
{
	MUTEX_LOCK(vars);
	Voice_SetFrequency_internal(voice,frq);
	MUTEX_UNLOCK(vars);
}

MIKMODAPI ULONG Voice_GetFrequency(SBYTE voice)
{
	ULONG result=0;

	MUTEX_LOCK(vars);
	if((voice>=0)&&(voice<md_numchn))
		result=md_driver->VoiceGetFrequency(voice);
	MUTEX_UNLOCK(vars);

	return result;
}

void Voice_SetPanning_internal(SBYTE voice,ULONG pan)
{
	if((voice<0)||(voice>=md_numchn)) return;
	if(pan!=PAN_SURROUND) {
		if(md_pansep>128) md_pansep=128;
		if(md_mode & DMODE_REVERSE) pan=255-pan;
		pan = (((SWORD)(pan-128)*md_pansep)/128)+128;
	}
	md_driver->VoiceSetPanning(voice, pan);
}

MIKMODAPI void Voice_SetPanning(SBYTE voice,ULONG pan)
{
#ifdef MIKMOD_DEBUG
	if((pan!=PAN_SURROUND)&&((pan<0)||(pan>255)))
		fprintf(stderr,"\rVoice_SetPanning called with pan=%ld\n",(long)pan);
#endif

	MUTEX_LOCK(vars);
	Voice_SetPanning_internal(voice,pan);
	MUTEX_UNLOCK(vars);
}

MIKMODAPI ULONG Voice_GetPanning(SBYTE voice)
{
	ULONG result=PAN_CENTER;

	MUTEX_LOCK(vars);
	if((voice>=0)&&(voice<md_numchn))
		result=md_driver->VoiceGetPanning(voice);
	MUTEX_UNLOCK(vars);

	return result;
}

void Voice_Play_internal(SBYTE voice,SAMPLE* s,ULONG start)
{
	ULONG  repend;

	if((voice<0)||(voice>=md_numchn)) return;

	md_sample[voice]=s;
	repend=s->loopend;

	if(s->flags&SF_LOOP)
		/* repend can't be bigger than size */
		if(repend>s->length) repend=s->length;

	md_driver->VoicePlay(voice,s->handle,start,s->length,s->loopstart,repend,s->flags);
}

MIKMODAPI void Voice_Play(SBYTE voice,SAMPLE* s,ULONG start)
{
	if(start>s->length) return;

	MUTEX_LOCK(vars);
	Voice_Play_internal(voice,s,start);
	MUTEX_UNLOCK(vars);
}

void Voice_Stop_internal(SBYTE voice)
{
	if((voice<0)||(voice>=md_numchn)) return;
	if(voice>=md_sngchn)
		/* It is a sound effects channel, so flag the voice as non-critical! */
		sfxinfo[voice-md_sngchn]=0;
	md_driver->VoiceStop(voice);
}

MIKMODAPI void Voice_Stop(SBYTE voice)
{
	MUTEX_LOCK(vars);
	Voice_Stop_internal(voice);
	MUTEX_UNLOCK(vars);
}

BOOL Voice_Stopped_internal(SBYTE voice)
{
	if((voice<0)||(voice>=md_numchn)) return 0;
	return(md_driver->VoiceStopped(voice));
}

MIKMODAPI BOOL Voice_Stopped(SBYTE voice)
{
	BOOL result;

	MUTEX_LOCK(vars);
	result=Voice_Stopped_internal(voice);
	MUTEX_UNLOCK(vars);

	return result;
}

MIKMODAPI SLONG Voice_GetPosition(SBYTE voice)
{
	SLONG result=0;

	MUTEX_LOCK(vars);
	if((voice>=0)&&(voice<md_numchn)) {
		if (md_driver->VoiceGetPosition)
			result=(md_driver->VoiceGetPosition(voice));
		else
			result=-1;
	}
	MUTEX_UNLOCK(vars);

	return result;
}

MIKMODAPI ULONG Voice_RealVolume(SBYTE voice)
{
	ULONG result=0;

	MUTEX_LOCK(vars);
	if((voice>=0)&&(voice<md_numchn)&& md_driver->VoiceRealVolume)
		result=(md_driver->VoiceRealVolume(voice));
	MUTEX_UNLOCK(vars);

	return result;
}

static BOOL _mm_init(CHAR *cmdline)
{
	UWORD t;

	_mm_critical = 1;

	/* if md_device==0, try to find a device number */
	if(!md_device) {
		cmdline=NULL;

		for(t=1,md_driver=firstdriver;md_driver;md_driver=md_driver->next,t++)
			if(md_driver->IsPresent()) break;

		if(!md_driver) {
			_mm_errno = MMERR_DETECTING_DEVICE;
			if(_mm_errorhandler) _mm_errorhandler();
			md_driver = &drv_nos;
			return 1;
		}

		md_device = t;
	} else {
		/* if n>0, use that driver */
		for(t=1,md_driver=firstdriver;(md_driver)&&(t!=md_device);md_driver=md_driver->next)
			t++;

		if(!md_driver) {
			_mm_errno = MMERR_INVALID_DEVICE;
			if(_mm_errorhandler) _mm_errorhandler();
			md_driver = &drv_nos;
			return 1;
		}

		/* arguments here might be necessary for the presence check to succeed */
		if(cmdline&&(md_driver->CommandLine))
			md_driver->CommandLine(cmdline);

		if(!md_driver->IsPresent()) {
			_mm_errno = MMERR_DETECTING_DEVICE;
			if(_mm_errorhandler) _mm_errorhandler();
			md_driver = &drv_nos;
			return 1;
		}
	}

	olddevice = md_device;
	if(md_driver->Init()) {
		MikMod_Exit_internal();
		if(_mm_errorhandler) _mm_errorhandler();
		return 1;
	}

	initialized=1;
	_mm_critical=0;

	return 0;
}

MIKMODAPI BOOL MikMod_Init(CHAR *cmdline)
{
	BOOL result;

	MUTEX_LOCK(vars);
	MUTEX_LOCK(lists);
	result=_mm_init(cmdline);
	MUTEX_UNLOCK(lists);
	MUTEX_UNLOCK(vars);

	return result;
}

void MikMod_Exit_internal(void)
{
	MikMod_DisableOutput_internal();
	md_driver->Exit();
	md_numchn = md_sfxchn = md_sngchn = 0;
	md_driver = &drv_nos;

	if(sfxinfo) free(sfxinfo);
	if(md_sample) free(md_sample);
	md_sample  = NULL;
	sfxinfo    = NULL;

	initialized = 0;
}

MIKMODAPI void MikMod_Exit(void)
{
	MUTEX_LOCK(vars);
	MUTEX_LOCK(lists);
	MikMod_Exit_internal();
	MUTEX_UNLOCK(lists);
	MUTEX_UNLOCK(vars);
}

/* Reset the driver using the new global variable settings. 
   If the driver has not been initialized, it will be now. */
static BOOL _mm_reset(CHAR *cmdline)
{
	BOOL wasplaying = 0;

	if(!initialized) return _mm_init(cmdline);
	
	if (isplaying) {
		wasplaying = 1;
		md_driver->PlayStop();
	}

	if((!md_driver->Reset)||(md_device != olddevice)) {
		/* md_driver->Reset was NULL, or md_device was changed, so do a full
		   reset of the driver. */
		md_driver->Exit();
		if(_mm_init(cmdline)) {
			MikMod_Exit_internal();
			if(_mm_errno)
				if(_mm_errorhandler) _mm_errorhandler();
			return 1;
		}
	} else {
		if(md_driver->Reset()) {
			MikMod_Exit_internal();
			if(_mm_errno)
				if(_mm_errorhandler) _mm_errorhandler();
			return 1;
		}
	}
	
	if (wasplaying) md_driver->PlayStart();
	return 0;
}

MIKMODAPI BOOL MikMod_Reset(CHAR *cmdline)
{
	BOOL result;

	MUTEX_LOCK(vars);
	MUTEX_LOCK(lists);
	result=_mm_reset(cmdline);
	MUTEX_UNLOCK(lists);
	MUTEX_UNLOCK(vars);

	return result;
}

/* If either parameter is -1, the current set value will be retained. */
BOOL MikMod_SetNumVoices_internal(int music, int sfx)
{
	BOOL resume = 0;
	int t, oldchn = 0;

	if((!music)&&(!sfx)) return 1;
	_mm_critical = 1;
	if(isplaying) {
		MikMod_DisableOutput_internal();
		oldchn = md_numchn;
		resume = 1;
	}

	if(sfxinfo) free(sfxinfo);
	if(md_sample) free(md_sample);
	md_sample  = NULL;
	sfxinfo    = NULL;

	if(music!=-1) md_sngchn = music;
	if(sfx!=-1)   md_sfxchn = sfx;
	md_numchn = md_sngchn + md_sfxchn;

	LimitHardVoices(md_driver->HardVoiceLimit);
	LimitSoftVoices(md_driver->SoftVoiceLimit);

	if(md_driver->SetNumVoices()) {
		MikMod_Exit_internal();
		if(_mm_errno)
			if(_mm_errorhandler!=NULL) _mm_errorhandler();
		md_numchn = md_softchn = md_hardchn = md_sfxchn = md_sngchn = 0;
		return 1;
	}

	if(md_sngchn+md_sfxchn)
		md_sample=(SAMPLE**)_mm_calloc(md_sngchn+md_sfxchn,sizeof(SAMPLE*));
	if(md_sfxchn)
		sfxinfo = (UBYTE *)_mm_calloc(md_sfxchn,sizeof(UBYTE));

	/* make sure the player doesn't start with garbage */
	for(t=oldchn;t<md_numchn;t++)  Voice_Stop_internal(t);

	sfxpool = 0;
	if(resume) MikMod_EnableOutput_internal();
	_mm_critical = 0;

	return 0;
}

MIKMODAPI BOOL MikMod_SetNumVoices(int music, int sfx)
{
	BOOL result;

	MUTEX_LOCK(vars);
	result=MikMod_SetNumVoices_internal(music,sfx);
	MUTEX_UNLOCK(vars);

	return result;
}

BOOL MikMod_EnableOutput_internal(void)
{
	_mm_critical = 1;
	if(!isplaying) {
		if(md_driver->PlayStart()) return 1;
		isplaying = 1;
	}
	_mm_critical = 0;
	return 0;
}

MIKMODAPI BOOL MikMod_EnableOutput(void)
{
	BOOL result;

	MUTEX_LOCK(vars);
	result=MikMod_EnableOutput_internal();
	MUTEX_UNLOCK(vars);

	return result;
}

void MikMod_DisableOutput_internal(void)
{
	if(isplaying && md_driver) {
		isplaying = 0;
		md_driver->PlayStop();
	}
}

MIKMODAPI void MikMod_DisableOutput(void)
{
	MUTEX_LOCK(vars);
	MikMod_DisableOutput_internal();
	MUTEX_UNLOCK(vars);
}

BOOL MikMod_Active_internal(void)
{
	return isplaying;
}

MIKMODAPI BOOL MikMod_Active(void)
{
	BOOL result;

	MUTEX_LOCK(vars);
	result=MikMod_Active_internal();
	MUTEX_UNLOCK(vars);

	return result;
}

/* Plays a sound effects sample.  Picks a voice from the number of voices
   allocated for use as sound effects (loops through voices, skipping all active
   criticals).

   Returns the voice that the sound is being played on.                       */
SBYTE Sample_Play_internal(SAMPLE *s,ULONG start,UBYTE flags)
{
	int orig=sfxpool;/* for cases where all channels are critical */
	int c;

	if(!md_sfxchn) return -1;
	if(s->volume>64) s->volume = 64;

	/* check the first location after sfxpool */
	do {
		if(sfxinfo[sfxpool]&SFX_CRITICAL) {
			if(md_driver->VoiceStopped(c=sfxpool+md_sngchn)) {
				sfxinfo[sfxpool]=flags;
				Voice_Play_internal(c,s,start);
				md_driver->VoiceSetVolume(c,s->volume<<2);
				Voice_SetPanning_internal(c,s->panning);
				md_driver->VoiceSetFrequency(c,s->speed);
				sfxpool++;
				if(sfxpool>=md_sfxchn) sfxpool=0;
				return c;
			}
		} else {
			sfxinfo[sfxpool]=flags;
			Voice_Play_internal(c=sfxpool+md_sngchn,s,start);
			md_driver->VoiceSetVolume(c,s->volume<<2);
			Voice_SetPanning_internal(c,s->panning);
			md_driver->VoiceSetFrequency(c,s->speed);
			sfxpool++;
			if(sfxpool>=md_sfxchn) sfxpool=0;
			return c;
		}

		sfxpool++;
		if(sfxpool>=md_sfxchn) sfxpool = 0;
	} while(sfxpool!=orig);

	return -1;
}

MIKMODAPI SBYTE Sample_Play(SAMPLE *s,ULONG start,UBYTE flags)
{
	SBYTE result;

	MUTEX_LOCK(vars);
	result=Sample_Play_internal(s,start,flags);
	MUTEX_UNLOCK(vars);

	return result;
}

MIKMODAPI long MikMod_GetVersion(void)
{
	return LIBMIKMOD_VERSION;
}

/*========== MT-safe stuff */

#ifdef HAVE_PTHREAD
#define INIT_MUTEX(name) \
	pthread_mutex_t _mm_mutex_##name=PTHREAD_MUTEX_INITIALIZER
#elif defined(__OS2__)||defined(__EMX__)
#define INIT_MUTEX(name) \
	HMTX _mm_mutex_##name
#elif defined(WIN32)
#define INIT_MUTEX(name) \
	HANDLE _mm_mutex_##name
#else
#define INIT_MUTEX(name)
#endif

INIT_MUTEX(vars);
INIT_MUTEX(lists);

MIKMODAPI BOOL MikMod_InitThreads(void)
{
	static int firstcall=1;
	static int result=0;
	
	if (firstcall) {
		firstcall=0;
#ifdef HAVE_PTHREAD
		result=1;
#elif defined(__OS2__)||defined(__EMX__)
		if(DosCreateMutexSem((PSZ)NULL,&_mm_mutex_lists,0,0) ||
		   DosCreateMutexSem((PSZ)NULL,&_mm_mutex_vars,0,0)) {
			_mm_mutex_lists=_mm_mutex_vars=(HMTX)NULL;
			result=0;
		} else
			result=1;
#elif defined(WIN32)
		if((!(_mm_mutex_lists=CreateMutex(NULL,FALSE,"libmikmod(lists)")))||
		   (!(_mm_mutex_vars=CreateMutex(NULL,FALSE,"libmikmod(vars)"))))
			result=0;
		else
			result=1;
#endif
	}
	return result;
}

MIKMODAPI void MikMod_Unlock(void)
{
	MUTEX_UNLOCK(lists);
	MUTEX_UNLOCK(vars);
}

MIKMODAPI void MikMod_Lock(void)
{
	MUTEX_LOCK(vars);
	MUTEX_LOCK(lists);
}

/*========== Parameter extraction helper */

CHAR *MD_GetAtom(CHAR *atomname,CHAR *cmdline,BOOL implicit)
{
	CHAR *ret=NULL;

	if(cmdline) {
		CHAR *buf=strstr(cmdline,atomname);

		if((buf)&&((buf==cmdline)||(*(buf-1)==','))) {
			CHAR *ptr=buf+strlen(atomname);

			if(*ptr=='=') {
				for(buf=++ptr;(*ptr)&&((*ptr)!=',');ptr++);
				ret=_mm_malloc((1+ptr-buf)*sizeof(CHAR));
				if(ret)
					strncpy(ret,buf,ptr-buf);
			} else if((*ptr==',')||(!*ptr)) {
				if(implicit) {
					ret=_mm_malloc((1+ptr-buf)*sizeof(CHAR));
					if(ret)
						strncpy(ret,buf,ptr-buf);
				}
			}
		}
	}
	return ret;
}

#if defined unix || (defined __APPLE__ && defined __MACH__)

/*========== Posix helper functions */

/* Check if the file is a regular or nonexistant file (or a link to a such a
   file), and that, should the calling program be setuid, the access rights are
   reasonable. Returns 1 if it is safe to rewrite the file, 0 otherwise.
   The goal is to prevent a setuid root libmikmod application from overriding
   files like /etc/passwd with digital sound... */
BOOL MD_Access(CHAR *filename)
{
	struct stat buf;

	if(!stat(filename,&buf)) {
		/* not a regular file ? */
		if(!S_ISREG(buf.st_mode)) return 0;
		/* more than one hard link to the file ? */
		if(buf.st_nlink>1) return 0;
		/* check access rights with the real user and group id */
		if(getuid()==buf.st_uid) {
			if(!(buf.st_mode&S_IWUSR)) return 0;
		} else if(getgid()==buf.st_gid) {
			if(!(buf.st_mode&S_IWGRP)) return 0;
		} else
			if(!(buf.st_mode&S_IWOTH)) return 0;
	}
	
	return 1;
}

/* Drop all root privileges we might have */
BOOL MD_DropPrivileges(void)
{
	if(!geteuid()) {
		if(getuid()) {
			/* we are setuid root -> drop setuid to become the real user */
			if(setuid(getuid())) return 1;
		} else {
			/* we are run as root -> drop all and become user 'nobody' */
			struct passwd *nobody;
			int uid;

			if(!(nobody=getpwnam("nobody"))) return 1; /* no such user ? */
			uid=nobody->pw_uid;
			if (!uid) /* user 'nobody' has root privileges ? weird... */
				return 1;
			if (setuid(uid)) return 1;
		}
	}
	return 0;
}

#endif

/* ex:set ts=4: */
