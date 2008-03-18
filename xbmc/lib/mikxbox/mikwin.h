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
*	Mikwin unofficial mikmod port.
*
***********************************************************************************************************************************************************
*
*	Last Revision : Rev 2.0 01/06/2002
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
#ifndef _MIKWIN_H_
#define _MIKWIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_MIXFREQ 22500

BOOL	mikwinInit(UWORD mixfreq, BOOL stereo, BOOL bits16, BOOL interpolation,BYTE reserveMusicChannels,BYTE reservedSfxChannels, BYTE soundDelay);
void	mikwinExit(void);
void	mikwinSetSoundDelay(BYTE soundDelay);

void	mikwinSetErrno(int errno);
int		mikwinGetErrno(void);
void	mikwinSetMasterVolume(UBYTE vol);
UBYTE	mikwinGetMasterVolume(void);
void	mikwinSetMusicVolume(UBYTE vol);
UBYTE	mikwinGetMusicVolume(void);
void	mikwinSetSfxVolume(UBYTE vol);
UBYTE	mikwinGetSfxVolume(void);
void	mikwinSetMasterReverb(UBYTE rev);
UBYTE	mikwinGetMasterReverb(void);
void	mikwinSetPanning(UBYTE pan);
UBYTE	mikwinGetPanning(void);
void	mikwinSetMasterDevice(UWORD dev);
UWORD	mikwinGetMasterDevice(void);
void	mikwinSetMixFrequency(UWORD freq);
UWORD	mikwinGetMixFrequency(void);
void	mikwinSetMode(UWORD mode);
UWORD	mikwinGetMode(void);

__int64 mikxboxGetPTS();
void mikxboxSetCallback(void (*p)(unsigned char*, int));

SWORD mikwinPlaySfx(SAMPLE *pSample,ULONG flags,UWORD pan,UWORD vol,ULONG frequency);
void mikwinStopSfx(SWORD voice);
SAMPLE* mikwinGetSfx(SWORD voice);



#ifdef __cplusplus
}
#endif


#endif /* _MIKWIN_H */

