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
*	Windows DirectSound-Raw Driver
*	(Wrapper for use as a static library or DLL)
*
*/

#include "xbsection_start.h"

//HACKED FOR XBOX BY PETE KNEPLEY

#define INITGUID

#include <xtl.h>
#include "mikmod.h"
#include "mikmod_internals.h"

#define DEFAULT_DSBUFFERSIZE 42


LPDIRECTSOUND        lpds = 0;			    /* main DSound-object ========*/
LPDIRECTSOUNDBUFFER  lpdsbprimary = 0;		/* primary DSound-buffer =====*/

int		dsbuffersize = DEFAULT_DSBUFFERSIZE;/* size of sound buffer (ms) =*/

/*===== \end{Configurable stuff} ======*/

static LPDIRECTSOUNDBUFFER	 lpdsb;					/* streaming DSound-buffer ====*/
static DWORD				 dwPlay;				/* current Dsouns position ====*/
static DWORD				 dwMidBuffer;			/* middle of sound buffer =====*/
static DWORD				 dwSizeBuffer;			/* size of sound buffer =======*/
static DWORD				 dwTimeSize;			/* frequency/timer refresh 50Hz*/
static DWORD				 dwReplayPos;			/* replay's offset in sound buf*/ 
static BOOL					 HasNotify = FALSE;     /* Notification available? ====*/
static LPDIRECTSOUNDNOTIFY	 lpdsNotify;			/* Notify interface ===========*/
static int					 playingFinished = 0;	/* >= 2: playing is finished ==*/
static BYTE				*	 pSoundData=NULL;		/* Dsound buffer ptr ==========*/
static __int64 PTS;
static void (*pCallback)(unsigned char*, int) = 0;
static BOOL IsPaused = FALSE;

void set_ds(LPDIRECTSOUND ds) { lpds = ds; }
void set_dsbprimary(LPDIRECTSOUNDBUFFER b) { lpdsbprimary = b; }
void set_ds_buffersize(int size) { if (size <= 0) dsbuffersize = DEFAULT_DSBUFFERSIZE; 
                                     else dsbuffersize = size; }
LPDIRECTSOUNDBUFFER get_ds_dsbstream(void) { return lpdsb; }
BOOL get_ds_active() { return playingFinished < 2; }
__int64 get_ds_pts() { return PTS / 3; }
void set_ds_callback(void (*p)(unsigned char*, int)) { pCallback = p; }

/**********************************************************************************************************************************************************

																	...FillBufferWithSilence() ...

***********************************************************************************************************************************************************/
static void FillBufferWithSilence()
{
	if (pSoundData != NULL)
		memset(pSoundData,0,dwSizeBuffer);
} 
/**********************************************************************************************************************************************************

																	... FillMusicBuffer(...) ...											

***********************************************************************************************************************************************************/
static void FillMusicBuffer(DWORD size) 
{
	if (pSoundData != NULL){
		if (!Mod_Player_Active() && playingFinished < 2) 
			playingFinished++; 

		VC_WriteBytes((SBYTE*)pSoundData+dwReplayPos, size);
		if (pCallback)
			pCallback(pSoundData+dwReplayPos, size);
	}
}
/**********************************************************************************************************************************************************

																		... DS_Init(...) ...											
	
***********************************************************************************************************************************************************/
BOOL DS_Init(void)
{
    HRESULT				hr;
    DSBUFFERDESC        dsbd;
    WAVEFORMATEX        wfm;
    DWORD				dwBytesLocked;
	VOID				*lpvData;

//xbox has no foreground win
//    if (hwnd == 0) hwnd = GetForegroundWindow();  /* not such a good way! */
   
	if (lpds == 0) {
		/* Create DirectSound object */
		if FAILED(hr = DirectSoundCreate(NULL, &lpds, NULL)) return FALSE;
	}

    /* Set buffer format. */
	memset(&wfm, 0, sizeof(WAVEFORMATEX)); 
    wfm.wFormatTag = WAVE_FORMAT_PCM; 
    wfm.nChannels = (md_mode & DMODE_STEREO) ? 2 : 1; 
    wfm.nSamplesPerSec = md_mixfreq; 
	wfm.wBitsPerSample = (md_mode & DMODE_16BITS) ? 16 : 8; 
    wfm.nBlockAlign = wfm.wBitsPerSample / 8 * wfm.nChannels;
    wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;
	
    if (lpdsbprimary == 0) {
		/* Create primary buffer */
		memset(&dsbd, 0, sizeof(DSBUFFERDESC)); 
		dsbd.dwSize = sizeof(DSBUFFERDESC); 
	    //dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
		//xbox can't lock software, get position, etc...
		dsbd.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY; 
	    dsbd.dwBufferBytes = 0;  
		dsbd.lpwfxFormat = &wfm; 
		if FAILED(hr = lpds->CreateSoundBuffer(&dsbd, &lpdsbprimary, NULL)) 
				return FALSE;

		
		/* Start looping (this doesn't play anything as long as no secondary buffers are active */
		/* but will prevent time-consuming DMA reprogramming on some ISA-soundcards.        	*/
		//if FAILED(hr = lpdsbprimary->Play(0, 0, DSBPLAY_LOOPING)) 
		//	return FALSE;
	}

	/* Create secondary buffer */
	memset(&dsbd, 0, sizeof(DSBUFFERDESC)); 
    dsbd.dwSize = sizeof(DSBUFFERDESC); 
	dsbd.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY;
	/* time size correspond to frequency / timer refresh (50Hz) */
	dwTimeSize = wfm.nAvgBytesPerSec / 30;
    dsbd.dwBufferBytes = dwTimeSize * dsbuffersize;  
    dsbd.lpwfxFormat = &wfm; 
 
    if FAILED(hr = lpds->CreateSoundBuffer(&dsbd, &lpdsb, NULL))
	{
		dsbd.dwFlags = dsbd.dwFlags & (!DSBCAPS_CTRLPOSITIONNOTIFY);
		if FAILED(hr = lpds->CreateSoundBuffer(&dsbd, &lpdsb, NULL))
			return FALSE; 
	}
	
    dwMidBuffer = dsbd.dwBufferBytes / 2;	
	dwSizeBuffer = dsbd.dwBufferBytes;

	if FAILED(lpdsb->Lock(
			  0,					/* Start = 0 oder middle of buffer =*/
			  dwSizeBuffer,         /* size = half of the buffer =======*/
			  &lpvData,             /* pointer to the locked buffer ====*/
			  &dwBytesLocked,       /* bytes to be locked ==============*/
			  NULL, NULL, 0)) return FALSE;


	lpdsb->Unlock(lpvData, dwBytesLocked, NULL, 0);
	pSoundData = (BYTE*) lpvData;
		
	memset(lpvData,0,dwSizeBuffer); 
	
	BYTE* peteisapimp=NULL;

	IsPaused = FALSE;

	dwReplayPos = dwMidBuffer;
    return VC_Init();
}
/**********************************************************************************************************************************************************

																	... DS_Exit(...) ...											

***********************************************************************************************************************************************************/
void DS_Exit(void)
{    
  if (lpds)
	{
		lpdsbprimary->StopEx( 0, DSBSTOPEX_IMMEDIATE );
		lpdsb->StopEx( 0, DSBSTOPEX_IMMEDIATE );
		lpdsbprimary->Release(); lpdsbprimary = 0;
		lpdsb->Release(); lpdsb = 0;
		lpds->Release(); lpds = 0;
	}
	VC_Exit();
}
/**********************************************************************************************************************************************************

																	... DS_IsThere(...) ...											

***********************************************************************************************************************************************************/
static BOOL DS_IsThere(void)
{
    if SUCCEEDED(DirectSoundCreate(NULL, &lpds, NULL)) {
		lpds->Release();
		lpds = 0;
		return TRUE;
    } else return FALSE;
}
/**********************************************************************************************************************************************************

																	... DS_PlayStart(...) ...											

***********************************************************************************************************************************************************/
static BOOL DS_PlayStart(void)
{
	BOOL r = VC_PlayStart();
	dwReplayPos = 0;
	PTS = 0;
	FillMusicBuffer(dwMidBuffer);
	dwReplayPos = dwMidBuffer;
  lpdsb->Play(0, 0, DSBPLAY_LOOPING);
	playingFinished = 0;
	return r;
}
/**********************************************************************************************************************************************************

																	... DS_PlayStop(...) ...											

***********************************************************************************************************************************************************/
void DS_PlayStop(void)
{

	FillBufferWithSilence();
	lpdsb->SetCurrentPosition(0);
	lpdsb->Stop();

	playingFinished = 1000;
	VC_PlayStop();
}
/**********************************************************************************************************************************************************

																	... DS_Update(...) ...											

  Note: as far as refresh periods on PC are faster than 50Hz (also the good old 800*600 53Hz on some VGA) 
		the Dsound buffer update can easily catch again the DMA.

		the updated size is now constant until u change frequency -> dwTimeSize = frequency / 50Hz. 

***********************************************************************************************************************************************************/

static void DS_Update(void)
{
	if (IsPaused)
		lpdsb->Pause(DSBPAUSE_RESUME);
	IsPaused = FALSE;

	static DWORD dwWrite=0;
	
	if FAILED(lpdsb->GetCurrentPosition(&dwPlay, &dwWrite)){
		return;
		//goto trythis;
	}

	if (dwPlay <= dwReplayPos){
		if ( dwReplayPos < (dwPlay+dwMidBuffer)){
//trythis:
			FillMusicBuffer(dwTimeSize);
			dwReplayPos += dwTimeSize;
			if (dwReplayPos >= dwSizeBuffer){
				dwReplayPos = 0;
			}
			++PTS;
		}
	}
	else{
		if ( dwPlay > (dwReplayPos+dwMidBuffer)){
			FillMusicBuffer(dwTimeSize);
			dwReplayPos += dwTimeSize;
			++PTS;
		}

	}
}

/**********************************************************************************************************************************************************


... DS_Pause(...) ...											

***********************************************************************************************************************************************************/
static void DS_Pause(void)
{
	if (!IsPaused)
		lpdsb->Pause(DSBPAUSE_PAUSE);
	IsPaused = TRUE;
}

/**********************************************************************************************************************************************************


																	... DS_Reset(...) ...											

***********************************************************************************************************************************************************/
static BOOL DS_Reset(void)
{
	/* can't change mixfreq at runtime (yet!) */
	return 1;
}
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#ifdef _DEBUG
DWORD DSgetPlayPos()
{
	return dwPlay;	
}
DWORD DSgetReplayPos()
{
	return dwReplayPos;
}
#endif // _DEBUG
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
MIKMODAPI MDRIVER drv_ds_raw = {
        NULL,
        "DirectSound Raw",
        "DirectSound Raw Driver v1.2 - by Ozzy@Orkysquad <ozzy@orkysquad.org> ",
		0, 255,
		"dsRaw",
		NULL,
        DS_IsThere,
        VC_SampleLoad,
        VC_SampleUnload,
		VC_SampleSpace,
		VC_SampleLength,
        DS_Init,
        DS_Exit,
     	DS_Reset,
		VC_SetNumVoices,
        DS_PlayStart,
        DS_PlayStop,
        DS_Update,
		DS_Pause,
        VC_VoiceSetVolume,
		VC_VoiceGetVolume,
		VC_VoiceSetFrequency,
		VC_VoiceGetFrequency,
        VC_VoiceSetPanning,
		VC_VoiceGetPanning,
        VC_VoicePlay,
		VC_VoiceStop,
		VC_VoiceStopped,
		VC_VoiceGetPosition,
		VC_VoiceRealVolume
};

