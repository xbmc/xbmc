/*	MikMod sound library
(c) 1998-2001 Miodrag Vallat and others - see file AUTHORS for
complete list.

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

Driver for output on xbox platform using DirectSound

==============================================================================*/

/*

XBOX Hardware mixing driver
by _Butcher_@EFnet for XBMC (http://www.xboxmediacenter.com)

This is (very) xbox specific!
But on the other hand, it allows modules with 100+ channels to actually run on an xbox...

*/

#include "xbsection_start.h"

#include "mikmod_internals.h"

#ifdef _XBOX

#include <memory.h>
#include <string.h>
#include <xtl.h>
#include <stdio.h>

// max of 127 because each channel takes up to 2 hardware voices (of which there are 255)
#define MAX_CHANS 127

static CRITICAL_SECTION DSCS;

static IDirectSound8* pDS;

// voice (aka channel) info, dual buffers to allow it to flip between them while still ramping out a sample
struct XB_VINFO
{
	IDirectSoundBuffer8* pdsb[2];
	int buf;
	int bufvol[2];
	int buffreq[2];
	int bufpan[2];
	int rampvol[2];
	int ramppan[2];
	bool bufplay[2];

	int setpos;
	int size;
	int handle;
	int chanvol;
	int chanfreq;
	int chanpan;

	bool active;
	bool kick;
	bool loop;
} VoiceInfo[MAX_CHANS];

// some state
static BOOL IsPaused;
static int OutputChans;
static BOOL ReverseStereo;
static int ActiveChans;
static LARGE_INTEGER LastUpdate;
static LARGE_INTEGER CounterFreq;
static __int64 CurrentPos;
static volatile __int64 PTS;

// sample, just data and format, all samples loaded as 48kHz then downsampled by the xbox mcp
struct XB_SAMPDATA
{
	SWORD* data;
	int chans;
	int bits;
};

// yeah we can have loads of these, it's just memory :p
static XB_SAMPDATA Samples[MAXSAMPLEHANDLES];

// volume lookup table - for doing linear volume to logarithic attenuation conversions.
// anything more than 4096 entries is pretty pointless as there are only 6400 discrete volume steps
#define VOLTABLESHIFT 4
#define VOLTABLEBITS  (VOLTABLESHIFT+8)
#define VOLTABLESIZE  (1 << VOLTABLEBITS)
static short VolumeTable[VOLTABLESIZE];

#ifdef XB_LOG
void XB_Log(const char* fmt, ...)
{
	char msg[1000] = "ModPlayer: ";
	va_list args;
	va_start(args, fmt);
	int n = vsprintf(msg + 11, fmt, args) + 11;
	va_end(args);
	msg[n] = '\n';
	msg[n+1] = '\0';
	OutputDebugString(msg);
}
#else
#define XB_Log 1 ? (void)0 : (void)
#endif

BOOL XB_IsThere(void)
{
	// I guess the fact that this module compiles at all proves it's on an xbox! :p
	return TRUE;
}

void XB_PlayStop(void);

void XB_Exit(void)
{
	XB_Log("XB_Exit");

	XB_PlayStop();

	if (pDS)
		pDS->Release();
	pDS = 0;
}

BOOL XB_Init(void)
{
	XB_Log("XB_Init");

	if (pDS)
		XB_Exit();

	// switch off the modes that are not supported - software mixing and 16 bit selection are not available.
	// the xbox can only output 48kHz 20-bit audio, and always hardware mixes
	// there are no options for xbox that affect performance, all options change the output sound only.
	md_mode &= DMODE_STEREO | DMODE_SURROUND | DMODE_REVERSE;
	// always interpolate and always use high quality
	md_mode |= DMODE_HQMIXER | DMODE_INTERP;

	// fixed frequency mix, hardware limitation
	md_mixfreq = 48000;

	DWORD flags = XGetAudioFlags();

	switch (XC_AUDIO_FLAGS_BASIC(flags))
	{
	case XC_AUDIO_FLAGS_MONO:
		md_mode &= ~DMODE_STEREO;
	case XC_AUDIO_FLAGS_STEREO:
		md_mode &= ~DMODE_SURROUND;
		break;
	}

	if (md_mode & DMODE_STEREO)
	{
		if (md_mode & DMODE_SURROUND)
		{
			OutputChans = 4;
			DirectSoundOverrideSpeakerConfig(DSSPEAKER_SURROUND);
		}
		else
		{
			OutputChans = 2;
			DirectSoundOverrideSpeakerConfig(DSSPEAKER_STEREO);
		}
	}
	else
	{
		OutputChans = 2;
		DirectSoundOverrideSpeakerConfig(DSSPEAKER_MONO);
	}

	ReverseStereo = (md_mode & DMODE_REVERSE) ? TRUE : FALSE;

	DWORD flags = XGetAudioFlags();

	switch (XC_AUDIO_FLAGS_BASIC(flags))
	{
	case XC_AUDIO_FLAGS_MONO:
		md_mode &= ~DMODE_STEREO;
	case XC_AUDIO_FLAGS_STEREO:
		md_mode &= ~DMODE_SURROUND;
		break;
	}

	if (md_mode & DMODE_STEREO)
	{
		if (md_mode & DMODE_SURROUND)
		{
			OutputChans = 4;
			if (XC_AUDIO_FLAGS_BASIC(flags) != XC_AUDIO_FLAGS_SURROUND)
				DirectSoundOverrideSpeakerConfig(DSSPEAKER_SURROUND);
		}
		else
		{
			OutputChans = 2;
			if (XC_AUDIO_FLAGS_BASIC(flags) != XC_AUDIO_FLAGS_STEREO)
				DirectSoundOverrideSpeakerConfig(DSSPEAKER_STEREO);
		}
	}
	else
	{
		OutputChans = 2;
		if (XC_AUDIO_FLAGS_BASIC(flags) != XC_AUDIO_FLAGS_MONO)
			DirectSoundOverrideSpeakerConfig(DSSPEAKER_MONO);
	}

	ReverseStereo = (md_mode & DMODE_REVERSE) ? TRUE : FALSE;

	IsPaused = FALSE;
	ActiveChans = 0;

	if (FAILED(DirectSoundCreate(NULL, &pDS, NULL)))
		return 1;

	InitializeCriticalSection(&DSCS);

	QueryPerformanceFrequency(&CounterFreq);

	// setup the volume table, intel asm is fine - xboxes only have intel cpus
	for (int i = 0; i < VOLTABLESIZE; ++i)
	{
		float v;
		__asm {
			fld1
			fild i
			fyl2x
			fstp v
		}
		v = DSBVOLUME_HW_MIN * (VOLTABLEBITS - v) / VOLTABLEBITS;
		VolumeTable[i] = (short)v;
	}

	return 0;
}

BOOL XB_Reset(void)
{
	// always use 48kHz mixing
	return 1;
}

// PTS for xbmc
__int64 XB_GetPTS()
{
	return 10 * PTS / CounterFreq.QuadPart;
}

BOOL XB_PlayStart(void)
{
	XB_Log("XB_PlayStart");

	CurrentPos = -1;
	PTS = 0;
	IsPaused = FALSE;

	return 0;
}

void XB_PlayStop(void)
{
	XB_Log("XB_PlayStop");

	EnterCriticalSection(&DSCS);

	// stop then release after they've had time to actually stop to avoid a busy-wait
	for (int i = 0; i < MAX_CHANS; ++i)
	{
		if (VoiceInfo[i].pdsb[0])
			VoiceInfo[i].pdsb[0]->Stop();
		if (VoiceInfo[i].pdsb[1])
			VoiceInfo[i].pdsb[1]->Stop();
	}

	Sleep(10);

	for (int i = 0; i < MAX_CHANS; ++i)
	{
		if (VoiceInfo[i].pdsb[0])
		{
			VoiceInfo[i].pdsb[0]->Release();
			VoiceInfo[i].pdsb[0] = 0;
		}
		if (VoiceInfo[i].pdsb[1])
		{
			VoiceInfo[i].pdsb[1]->Release();
			VoiceInfo[i].pdsb[1] = 0;
		}
	}

	LeaveCriticalSection(&DSCS);
}

inline long ConvertVolume(UWORD vol)
{
	// do lookup, clamp values
	if (vol >= 255)
		return 0;
	if (!vol)
		return DSBVOLUME_MIN;
	return VolumeTable[vol << VOLTABLESHIFT];
}

#define RAMPSHIFT 6
#define RAMPSAMPLES (1 << RAMPSHIFT)

inline long ConvertRampVolume(UWORD vol, int ramp, int frame)
{
	// do interpolated lookup
	int v = (vol << VOLTABLESHIFT) + ((ramp * frame) >> RAMPSHIFT);
	return VolumeTable[v];
}

inline bool CheckBufStatus(XB_VINFO* pVI, int i)
{
	// check if the buffer has finished playing
	DWORD status;
	if (!pVI->pdsb[i])
		return pVI->bufplay[i] = false;
	if (!FAILED(pVI->pdsb[i]->GetStatus(&status)))
	{
		if (!(status & DSBSTATUS_PLAYING))
		{
			return pVI->bufplay[i] = false;
		}
	}
	return pVI->bufplay[i] = true;
}

extern "C" MODULE* pf;

void XB_VoiceStop(UBYTE voice);

void XB_Update(void)
{
	LARGE_INTEGER Time;
	int q, n = 0;
	bool doramp;

	EnterCriticalSection(&DSCS);

	if (pf->forbid)
	{
		LeaveCriticalSection(&DSCS);
		return;
	}

	if (pf->sngpos >= pf->numpos)
	{
		LeaveCriticalSection(&DSCS);
		return;
	}

	if (CurrentPos == -1)
	{
		QueryPerformanceCounter(&LastUpdate);
		CurrentPos = LastUpdate.QuadPart;
	}

	if (IsPaused)
	{
		// unpause all the buffers
		for (int i = 0; i < md_hardchn; ++i)
		{
			if (VoiceInfo[i].pdsb[0])
				VoiceInfo[i].pdsb[0]->Pause(DSBPAUSE_RESUME);
			if (VoiceInfo[i].pdsb[1])
				VoiceInfo[i].pdsb[1]->Pause(DSBPAUSE_RESUME);
		}
		XB_Log("Unpaused");
		IsPaused = FALSE;
		// reset sync
		QueryPerformanceCounter(&LastUpdate);
		CurrentPos = LastUpdate.QuadPart;
	}

	// Process mod tick
	Mod_Player_HandleTick();
	++n;

	if (pf->sngpos >= pf->numpos)
	{
		for (int i = 0; i < md_hardchn; ++i)
			XB_VoiceStop(i);
	}

	// go highest priority to stop things putting the timing out of whack
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	// sync to pts
	CurrentPos += (CounterFreq.QuadPart*125)/(md_bpm*50);
	PTS += (CounterFreq.QuadPart*125)/(md_bpm*50);
	QueryPerformanceCounter(&Time);
	int Proc = (int)(100000 * (Time.QuadPart - LastUpdate.QuadPart) / CounterFreq.QuadPart);
	// conservative guess of time to sleep - this seems to work quite well though it probably wastes a bit of cpu
	q = (1000*125)/(md_bpm*50) - (Proc + 99) / 100 - 1;

	// sanity check on q, to avoid lock ups while debugging
	if (q > 0 && q < 100)
		Sleep(q);

	// spin for more precise timing
	do {
		QueryPerformanceCounter(&LastUpdate);
	} while (LastUpdate.QuadPart < CurrentPos);

	// Don't do anything here - the update should be as fast as possible after the sync, ideally within the sample
	// realistically it takes a few samples to change all the settings

	doramp = false;
	for (int i = 0; i < md_hardchn; ++i)
	{
		if (VoiceInfo[i].active)
		{
			int b = VoiceInfo[i].buf;

			if (VoiceInfo[i].setpos != -1)
			{
				// set position, used for s3m Oxx offset effects
				VoiceInfo[i].pdsb[b]->SetCurrentPosition(VoiceInfo[i].setpos);
				VoiceInfo[i].setpos = -1;
			}

			if (VoiceInfo[i].kick)
			{
				// kick off a new voice
				VoiceInfo[i].kick = false;			
				VoiceInfo[i].bufplay[b] = true;
				VoiceInfo[i].pdsb[b]->PlayEx(0, (VoiceInfo[i].loop ? DSBPLAY_LOOPING : 0));
			}
			else if (!VoiceInfo[i].bufplay[b])
			{
				// check for buffers going silent after use
				if (VoiceInfo[i].rampvol[1-b])
				{
					VoiceInfo[i].pdsb[1-b]->SetVolume(ConvertRampVolume(VoiceInfo[i].bufvol[1-b], VoiceInfo[i].rampvol[1-b], 1));
					doramp = true;
				}
				// this voice has no active buffer so don't bother updating
				VoiceInfo[i].active = false;
				--ActiveChans;
				continue;
			}

			if (VoiceInfo[i].chanfreq != VoiceInfo[i].buffreq[b])
			{
				VoiceInfo[i].buffreq[b] = VoiceInfo[i].chanfreq;
				if (VoiceInfo[i].buffreq)
					VoiceInfo[i].pdsb[b]->SetFrequency(VoiceInfo[i].buffreq[b]);
				else // bad freq, mute the channel
					VoiceInfo[i].chanvol = 0;
			}

			if (VoiceInfo[i].chanvol != VoiceInfo[i].bufvol[b])
			{
				// calculate volume ramp if it's a large change to remove clicks
				if (abs((long)VoiceInfo[i].bufvol[b] - (long)VoiceInfo[i].chanvol) > 32)
					VoiceInfo[i].rampvol[b] = (VoiceInfo[i].chanvol - VoiceInfo[i].bufvol[b]) << VOLTABLESHIFT;

				if (!VoiceInfo[i].rampvol[b])
				{
					VoiceInfo[i].bufvol[b] = VoiceInfo[i].chanvol;
					VoiceInfo[i].pdsb[b]->SetVolume(ConvertVolume(VoiceInfo[i].bufvol[b]));
				}
				else
				{
					VoiceInfo[i].pdsb[b]->SetVolume(ConvertRampVolume(VoiceInfo[i].bufvol[b], VoiceInfo[i].rampvol[b], 1));
					doramp = true;
				}
			}

			// check for buffers going silent after use
			if (VoiceInfo[i].rampvol[1-b])
			{
				VoiceInfo[i].pdsb[1-b]->SetVolume(ConvertRampVolume(VoiceInfo[i].bufvol[1-b], VoiceInfo[i].rampvol[1-b], 1));
				doramp = true;
			}

			// panning, need to declick this
			if (VoiceInfo[i].chanpan != VoiceInfo[i].bufpan[b])
			{
				VoiceInfo[i].bufpan[b] = VoiceInfo[i].chanpan;
				DSMIXBINVOLUMEPAIR vols[] = { DSMIXBINVOLUMEPAIRS_DEFAULT_4CHANNEL };
				switch (OutputChans)
				{
				case 2:
					if (VoiceInfo[i].bufpan[b] == PAN_SURROUND)
					{
						vols[0].lVolume = vols[1].lVolume = ConvertVolume(255 * 256 / 480);
					}
					else
					{
						vols[0].lVolume = ConvertVolume(PAN_RIGHT - VoiceInfo[i].bufpan[b]);
						vols[1].lVolume = ConvertVolume(VoiceInfo[i].bufpan[b]);
					}
					break;
				case 4:
					if (VoiceInfo[i].bufpan[b] == PAN_SURROUND)
					{
						vols[0].lVolume = vols[1].lVolume = vols[2].lVolume = vols[3].lVolume = ConvertVolume(255 * 256 / 480);
					}
					else
					{
						vols[0].lVolume = ConvertVolume(PAN_RIGHT - VoiceInfo[i].bufpan[b]);
						vols[1].lVolume = ConvertVolume(VoiceInfo[i].bufpan[b]);
						vols[2].lVolume = vols[3].lVolume = DSBVOLUME_MIN;
					}
					break;
				}
				if (ReverseStereo)
				{
					// swap channels
					long t = vols[0].lVolume;
					vols[0].lVolume = vols[1].lVolume;
					vols[1].lVolume = t;
				}
				DSMIXBINS MixBins = { OutputChans, vols };
				VoiceInfo[i].pdsb[b]->SetMixBinVolumes(&MixBins);
			}
		}
		else
		{
			// check inactive voices for buffers going silent
			if (VoiceInfo[i].rampvol[0])
			{
				VoiceInfo[i].pdsb[0]->SetVolume(ConvertRampVolume(VoiceInfo[i].bufvol[0], VoiceInfo[i].rampvol[0], 1));
				doramp = true;
			}
			if (VoiceInfo[i].rampvol[1])
			{
				VoiceInfo[i].pdsb[1]->SetVolume(ConvertRampVolume(VoiceInfo[i].bufvol[1], VoiceInfo[i].rampvol[1], 1));
				doramp = true;
			}
		}
	}

	// go high priority while ramping
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	DWORD StartSamp = DirectSoundGetSampleTime()-1;
	if (doramp)
	{
		// volume ramp calculations for click removal
		DWORD l = 1, f = 1;
		while (true)
		{
			// busy wait as a sleep is way to long to wait for a single sample interval
			while ((f = DirectSoundGetSampleTime() - StartSamp + 1) == l)
				;
			if (f >= RAMPSAMPLES)
				break;
			for (int i = 0; i < md_hardchn; ++i)
			{
				if (VoiceInfo[i].rampvol[0])
					VoiceInfo[i].pdsb[0]->SetVolume(ConvertRampVolume(VoiceInfo[i].bufvol[0], VoiceInfo[i].rampvol[0], f));
				if (VoiceInfo[i].rampvol[1])
					VoiceInfo[i].pdsb[1]->SetVolume(ConvertRampVolume(VoiceInfo[i].bufvol[1], VoiceInfo[i].rampvol[1], f));
			}
			l = f;
		}

		// set final volumes after ramping
		for (int i = 0; i < md_hardchn; ++i)
		{
			if (VoiceInfo[i].rampvol[0])
			{
				VoiceInfo[i].bufvol[0] += (VoiceInfo[i].rampvol[0] >> VOLTABLESHIFT);
				VoiceInfo[i].pdsb[0]->SetVolume(ConvertVolume(VoiceInfo[i].bufvol[0]));
				VoiceInfo[i].rampvol[0] = 0;
				if (!VoiceInfo[i].bufvol[0] && (!VoiceInfo[i].active || VoiceInfo[i].buf != 0))
					VoiceInfo[i].pdsb[0]->Stop();
			}

			if (VoiceInfo[i].rampvol[1])
			{
				VoiceInfo[i].bufvol[1] += (VoiceInfo[i].rampvol[1] >> VOLTABLESHIFT);
				VoiceInfo[i].pdsb[1]->SetVolume(ConvertVolume(VoiceInfo[i].bufvol[1]));
				VoiceInfo[i].rampvol[1] = 0;
				if (!VoiceInfo[i].bufvol[1] && (!VoiceInfo[i].active || VoiceInfo[i].buf != 1))
					VoiceInfo[i].pdsb[1]->Stop();
			}
		}
	}

	// go normal priority while processing
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	// if any inactive buffers have been ramped to silence, stop them
	for (int i = 0; i < md_hardchn; ++i)
	{
		if (VoiceInfo[i].bufplay[0] && !VoiceInfo[i].bufvol[0] && (!VoiceInfo[i].active || VoiceInfo[i].buf != 0))
			VoiceInfo[i].pdsb[0]->Stop();

		if (VoiceInfo[i].bufplay[1] && !VoiceInfo[i].bufvol[1] && (!VoiceInfo[i].active || VoiceInfo[i].buf != 1))
			VoiceInfo[i].pdsb[1]->Stop();
	}

	// wait for buffers to stop
	Sleep(1);

	// check all buffers for completion (either vis explicit stop or because they have run to duration)
	for (int i = 0; i < md_hardchn; ++i)
	{
		CheckBufStatus(&VoiceInfo[i], 0);
		CheckBufStatus(&VoiceInfo[i], 1);
	}

	// check for overshoot on the sync (done here to avoid causing more desync with the check)
	if (LastUpdate.QuadPart > CurrentPos+733) // 733 = 1us (xbox has a 733mhz cpu)
	{
		XB_Log("Overshoot: %4dus (q=%2d; p=%d)", (int)((LastUpdate.QuadPart - CurrentPos) / 733), q, Proc);
	}

	static int LastChans = 0;
	if (ActiveChans != LastChans)
	{
		XB_Log("%02d:%02d: Active Chans: %03d", pf->positions[pf->sngpos], pf->patpos, LastChans = ActiveChans);
		if (pf->positions[pf->sngpos] == 14)
		{
			for (int i = 0; i < md_hardchn; ++i)
				XB_Log("Chn: %02d, act=%d, vol=%03d, pan=%03d, frq=%05d", i, VoiceInfo[i].active, VoiceInfo[i].chanvol, VoiceInfo[i].chanpan, VoiceInfo[i].chanfreq);
		}
	}

	LeaveCriticalSection(&DSCS);
}

void XB_Pause(void)
{
	if (!IsPaused)
	{
		EnterCriticalSection(&DSCS);
		// pause all the buffers
		for (int i = 0; i < md_hardchn; ++i)
		{
			if (VoiceInfo[i].pdsb[0])
				VoiceInfo[i].pdsb[0]->Pause(DSBPAUSE_PAUSE);
			if (VoiceInfo[i].pdsb[1])
				VoiceInfo[i].pdsb[1]->Pause(DSBPAUSE_PAUSE);
		}
		XB_Log("Paused");
		IsPaused = TRUE;
		LeaveCriticalSection(&DSCS);
	}
	Sleep(20);
}

extern "C" MODULE* pf;

SWORD XB_SampleLoad(SAMPLOAD* sload, int type)
{
	// sorry no software mixing
	if(type != MD_HARDWARE)
		return -1;

	SAMPLE *s = sload->sample;
	int handle;
	ULONG length,loopstart,loopend;

	for (handle = 0; handle < MAXSAMPLEHANDLES; ++handle)
		if(!Samples[handle].data)
			break;

	if (handle == MAXSAMPLEHANDLES)
	{
		_mm_errno = MMERR_OUT_OF_HANDLES;
		return -1;
	}

	/* Reality check for loop settings */
	if (s->loopend > s->length)
		s->loopend = s->length;
	if (s->loopstart >= s->loopend)
		s->flags &= ~SF_LOOP;

	length    = s->length;
	loopstart = s->loopstart;
	loopend   = s->loopend;

	// need to adjust the loader so it can load 8-bit and stereo without conversion but I'm feeling lazy
	// might not be a good idea as it'll cause more voice format changes
	SL_SampleSigned(sload);
	SL_Sample8to16(sload);

	XB_Log("XB_SampleLoad: %2d; %6d samples, %c%c%c", handle, s->length, (s->flags & SF_BIDI) ? 'B' : ' ', (s->flags & SF_LOOP) ? 'L' : ' ', (s->flags & SF_REVERSE) ? 'R' : ' ');

	if (!(Samples[handle].data = (SWORD*)malloc((length + 16) << (1 + ((s->flags & SF_BIDI) ? 1 : 0))))) {
		_mm_errno = MMERR_SAMPLE_TOO_BIG;
		return -1;
	}

	Samples[handle].bits = 16;
	Samples[handle].chans = 1;

	/* read sample into buffer */
	if (SL_Load(Samples[handle].data,sload,length))
	{
		free(Samples[handle].data);
		Samples[handle].data = NULL;
		return -1;
	}

	// mirror sample for bi-dir samples - can't do reverse on xbox hardware
	if (s->flags & SF_BIDI)
	{
		if (s->flags & SF_LOOP)
		{
			SWORD* src = Samples[handle].data + loopstart;
			SWORD* dst = Samples[handle].data + loopend + (loopend - loopstart) - 1;
			for (unsigned i = loopstart; i < loopend; ++i)
				*dst-- = *src++;
		}
		else
		{
			SWORD* src = Samples[handle].data;
			SWORD* dst = Samples[handle].data + length * 2 - 1;
			for (unsigned i = 0; i < length; ++i)
				*dst-- = *src++;
		}
	}

	// sample test

//	WAVEFORMATEX fmt;
//	XAudioCreatePcmFormat(Samples[handle].chans, 8363, Samples[handle].bits, &fmt);
//
//	DSBUFFERDESC bufdesc;
//	bufdesc.dwSize = sizeof(bufdesc);
//	bufdesc.dwFlags = 0;
//	bufdesc.dwBufferBytes = 0;
//	bufdesc.lpwfxFormat = &fmt;
//	DSMIXBINVOLUMEPAIR vols[] = { DSMIXBINVOLUMEPAIRS_DEFAULT_STEREO };
//	vols[0].lVolume = vols[1].lVolume = 0;
//	DSMIXBINS MixBins = { 2, vols };
//	bufdesc.lpMixBins = &MixBins;
//	bufdesc.dwInputMixBin = 0;
//
//	LPDIRECTSOUNDBUFFER8 pDSB;
//	HRESULT r = pDS->CreateSoundBuffer(&bufdesc, &pDSB, 0);
//	if (!FAILED(r))
//		pDSB->SetHeadroom(0);
//
//	if (!FAILED(r))
//	{
//		if ((s->flags & SF_LOOP) && (s->flags & SF_BIDI))
//			pDSB->SetBufferData(Samples[handle].data, (length + (loopend - loopstart)) * fmt.nBlockAlign);
//		else
//			pDSB->SetBufferData(Samples[handle].data, length * fmt.nBlockAlign);
//		if (s->flags & SF_LOOP)
//		{
//			if (s->flags & SF_BIDI)
//				pDSB->SetLoopRegion(loopstart * fmt.nBlockAlign, (loopend - loopstart) * 2 * fmt.nBlockAlign);
//			else
//				pDSB->SetLoopRegion(loopstart * fmt.nBlockAlign, (loopend - loopstart) * fmt.nBlockAlign);
//		}
//		pDSB->PlayEx(0, ((s->flags & SF_LOOP) ? DSBPLAY_LOOPING : 0));
//
//		Sleep(5000);
//		pDSB->Stop();
//		Sleep(100);
//		pDSB->Release();
//	}

	/* Unclick sample */
	// not sure if this is particularly effective tbh
	int t;
	if(s->flags & SF_LOOP) {
		if(s->flags & SF_BIDI)
			for(t=0;t<16;t++)
				Samples[handle].data[loopend+t]=Samples[handle].data[(loopend-t)-1];
		else
			for(t=0;t<16;t++)
				Samples[handle].data[loopend+t]=Samples[handle].data[t+loopstart];
	} else
	{
		for(t=-8;t<0;t++)
			Samples[handle].data[t+length] /= (8+t)+1;
		for(t=0;t<16;t++)
			Samples[handle].data[t+length]=0;
	}

	return handle;
}

void XB_SampleUnload(SWORD handle)
{
	XB_Log("XB_SampleUnload: %d", handle);

	if (handle<MAXSAMPLEHANDLES) {
		if (Samples[handle].data)
			free(Samples[handle].data);
		Samples[handle].data=NULL;
	}
}

ULONG XB_FreeSampleSpace(int type)
{
	// 8mb sound buffers max - the xbox only has scatter gather enteries for 2047 pages
	if (type == MD_HARDWARE)
		return 2047*4;
	else
		return 0;
}


BOOL XB_SetNumVoices(void)
{
	memset(VoiceInfo, 0, md_hardchn * sizeof(XB_VINFO));
	for (int i = 0; i < md_hardchn; ++i)
	{
		VoiceInfo[i].chanfreq = 10000;
		VoiceInfo[i].chanpan = (i&1)?PAN_LEFT:PAN_RIGHT;
	}

	return 0;
}

void XB_VoiceSetVolume(UBYTE voice, UWORD vol)
{
	if (vol > 255)
		vol = 255;
	VoiceInfo[voice].chanvol = vol;
}

UWORD XB_VoiceGetVolume(UBYTE voice)
{
	return VoiceInfo[voice].chanvol;
}

void XB_VoiceSetFrequency(UBYTE voice, ULONG freq)
{
	// I'm yet to see a module that exceeds these, it would be rather odd!
	if (freq < DSBFREQUENCY_MIN)
	{
		XB_Log("WARN: Frequency out of range (%d > %d), killing channel", freq, DSBFREQUENCY_MAX);
		freq = 0;
	}
	if (freq > DSBFREQUENCY_MAX)
	{
		XB_Log("WARN: Frequency out of range (%d > %d), clamping", freq, DSBFREQUENCY_MAX);
		freq = DSBFREQUENCY_MAX;
	}
	VoiceInfo[voice].chanfreq = freq;
}

ULONG XB_VoiceGetFrequency(UBYTE voice)
{
	return VoiceInfo[voice].chanfreq;
}


void XB_VoiceSetPanning(UBYTE voice, ULONG pan)
{
	VoiceInfo[voice].chanpan = pan;
}

ULONG XB_VoiceGetPanning(UBYTE voice)
{
	return VoiceInfo[voice].chanpan;
}

void XB_VoicePlay(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags)
{
	if (!VoiceInfo[voice].active)
	{
		VoiceInfo[voice].active = true;
		++ActiveChans;
	}

	int i = VoiceInfo[voice].buf;

	WAVEFORMATEX fmt;
	XAudioCreatePcmFormat(Samples[handle].chans, 48000, Samples[handle].bits, &fmt);

	// check to see if it's a s3m Oxx offset effect - if so reuse the buffer
	// only reuse for offsets not new notes so that declicking works
	if (VoiceInfo[voice].handle == handle && start && CheckBufStatus(&VoiceInfo[voice], i))
	{
		VoiceInfo[voice].setpos = start * fmt.nBlockAlign;
		VoiceInfo[voice].kick = true; // kick it in case the buffer ends before the offset can be reset
		return;
	}

	XB_Log("chn: %02d; New instrument %02d", voice, handle);

	// check to see if the current buffer is playing
	if (CheckBufStatus(&VoiceInfo[voice], i))
	{
		// it is, fade to silence and use the other one
		VoiceInfo[voice].rampvol[i] = -VoiceInfo[voice].bufvol[i] << VOLTABLESHIFT;
		i = 1 - i;
	}

	HRESULT r;
	if (VoiceInfo[voice].pdsb[i])
	{
		// check for sample format change
		int h = VoiceInfo[voice].handle;
		if ((h != handle) && (Samples[h].bits != Samples[handle].bits || Samples[h].chans != Samples[handle].chans))
			r = VoiceInfo[voice].pdsb[i]->SetFormat(&fmt);
		else
			r = S_OK;
	}
	else
	{
		// nice shiny new buffer
		DSBUFFERDESC bufdesc;
		bufdesc.dwSize = sizeof(bufdesc);
		bufdesc.dwFlags = 0;
		bufdesc.dwBufferBytes = 0;
		bufdesc.lpwfxFormat = &fmt;
		DSMIXBINVOLUMEPAIR vols[] = { DSMIXBINVOLUMEPAIRS_DEFAULT_4CHANNEL };
		vols[0].lVolume = vols[1].lVolume = vols[2].lVolume = vols[3].lVolume = DSBVOLUME_MIN;
		DSMIXBINS MixBins = { OutputChans, vols };
		bufdesc.lpMixBins = &MixBins;
		bufdesc.dwInputMixBin = 0;

		r = pDS->CreateSoundBuffer(&bufdesc, &VoiceInfo[voice].pdsb[i], 0);
		// 3dB headroom to reduce distortion from mods with a lot of loud samples
		// not entirely faithful to the original, but distortion sucks more than slightly quieter mods.
		if (!FAILED(r))
			VoiceInfo[voice].pdsb[i]->SetHeadroom(300);

		// set buffer defaults
		VoiceInfo[voice].bufvol[i] = 0;
		VoiceInfo[voice].buffreq[i] = 48000;
		VoiceInfo[voice].bufpan[i] = -1;
		VoiceInfo[voice].rampvol[i] = 0;
		VoiceInfo[voice].ramppan[i] = 0;
	}

	if (!FAILED(r))
	{
		VoiceInfo[voice].buf = i;
		VoiceInfo[voice].size = size;
		VoiceInfo[voice].handle = handle;
		if ((flags & SF_LOOP) && (flags & SF_BIDI))
			VoiceInfo[voice].pdsb[i]->SetBufferData(Samples[handle].data, (size + (repend - reppos)) * fmt.nBlockAlign);
		else
			VoiceInfo[voice].pdsb[i]->SetBufferData(Samples[handle].data, size * fmt.nBlockAlign);
		VoiceInfo[voice].pdsb[i]->SetCurrentPosition(start * fmt.nBlockAlign);
		if (flags & SF_LOOP)
		{
			VoiceInfo[voice].loop = true;
			if (flags & SF_BIDI)
				VoiceInfo[voice].pdsb[i]->SetLoopRegion(reppos * fmt.nBlockAlign, (repend - reppos) * 2 * fmt.nBlockAlign);
			else
				VoiceInfo[voice].pdsb[i]->SetLoopRegion(reppos * fmt.nBlockAlign, (repend - reppos) * fmt.nBlockAlign);
		}
		else
			VoiceInfo[voice].loop = false;
		VoiceInfo[voice].kick = true;
		VoiceInfo[voice].setpos = -1;
		VoiceInfo[voice].bufvol[i] = 0;
		VoiceInfo[voice].pdsb[i]->SetVolume(DSBVOLUME_MIN);
	}
	else
	{
		// if this fails then the module will randomly drop a voice, should probably handle this in some fashion
		XB_Log("WARN: Failed to setup buffer (c=%02d)", voice);
	}
}

void XB_VoiceStop(UBYTE voice)
{
	if (VoiceInfo[voice].active)
	{
		// fade to silence, buffers will stop automatically in update
		VoiceInfo[voice].rampvol[0] = -VoiceInfo[voice].bufvol[0] << VOLTABLESHIFT;
		VoiceInfo[voice].rampvol[1] = -VoiceInfo[voice].bufvol[1] << VOLTABLESHIFT;
		--ActiveChans;
		VoiceInfo[voice].active = false;
	}
}

BOOL XB_VoiceStopped(UBYTE voice)
{
	return !VoiceInfo[voice].active;
}

SLONG XB_VoiceGetPosition(UBYTE voice)
{
	DWORD Pos;
	VoiceInfo[voice].pdsb[VoiceInfo[voice].buf]->GetCurrentPosition(&Pos, NULL);
	return Pos / 2;
}

ULONG XB_VoiceRealVolume(UBYTE voice)
{
	if (XB_VoiceStopped(voice))
		return 0;

	int t = XB_VoiceGetPosition(voice) - 64;
	int i=64, k=0, j=0;
	if(i>VoiceInfo[voice].size) i = VoiceInfo[voice].size;
	if(t<0) t = 0;
	if(t+i > VoiceInfo[voice].size) t = VoiceInfo[voice].size-i;

	i &= ~1;  /* make sure it's EVEN. */

	SWORD* smp = &Samples[VoiceInfo[voice].handle].data[t];
	for(;i;i--,smp++) {
		if(k<*smp) k = *smp;
		if(j>*smp) j = *smp;
	}
	return abs(k-j);
}

extern "C" ULONG VC1_SampleLength(int type,SAMPLE* s);

MIKMODAPI MDRIVER drv_xbox = {
	NULL,
		"XBox",
		"XBox hardware driver",
		255, 0,
		"xbox",
		NULL,					// nope, no command lines
		XB_IsThere,
		XB_SampleLoad,
		XB_SampleUnload,
		XB_FreeSampleSpace,
		VC1_SampleLength,
		XB_Init,
		XB_Exit,
		XB_Reset,
		XB_SetNumVoices,
		XB_PlayStart,
		XB_PlayStop,
		XB_Update,
		XB_Pause,
		XB_VoiceSetVolume,
		XB_VoiceGetVolume,
		XB_VoiceSetFrequency,
		XB_VoiceGetFrequency,
		XB_VoiceSetPanning,
		XB_VoiceGetPanning,
		XB_VoicePlay,
		XB_VoiceStop,
		XB_VoiceStopped,
		XB_VoiceGetPosition,
		XB_VoiceRealVolume
};

#endif