/*
 * XBoxMediaPlayer
 * Copyright (c) 2002 d7o3g4q and RUNTiME
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// AsyncAudioRenderer.h: interface for the CAc97DirectSound class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ASYNCAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_)
#define AFX_ASYNCAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif

#include "IAudioCallback.h"
extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();

class CAc97DirectSound 
{
public:
	CAc97DirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample);
	virtual ~CAc97DirectSound();

	void 							UnRegisterAudioCallback();
	void 							RegisterAudioCallback(IAudioCallback* pCallback);
	DWORD 						GetChunkLen();
	FLOAT 						GetDelay();
	DWORD 					AddPackets(unsigned char* data, DWORD len);
	DWORD						GetSpace();
	virtual HRESULT Deinitialize();
	virtual HRESULT Pause();
	virtual HRESULT Stop();
	HRESULT					Resume();
	DWORD						GetBytesInBuffer();
	virtual LONG		GetMinimumVolume() const;
	virtual LONG		GetMaximumVolume() const;
	virtual LONG		GetCurrentVolume() const;
	virtual HRESULT	SetCurrentVolume(LONG nVolume);
	virtual bool		SupportsSurroundSound()  const;
	static void CALLBACK StaticStreamCallback(LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus);
	void						StreamCallback(LPVOID pPacketContext, DWORD dwStatus);
	int							SetPlaySpeed(int iSpeed);

private:
	IAudioCallback* m_pCallback;
	LONG m_lFadeVolume;

	bool									FindFreePacket( DWORD& pdwIndex );

	LPAC97MEDIAOBJECT			m_pAnalogOutput;
	LPAC97MEDIAOBJECT			m_pDigitalOutput;
	WAVEFORMATEX				  m_wfx;
	LONG								  m_nCurrentVolume;
	DWORD									m_dwPacketSize;
	bool									m_AudioEOF;
	DWORD									m_dwNumPackets;
	PBYTE									m_pbSampleData[150];
	DWORD*								m_adwStatus;
	bool									m_bPause;
	bool									m_bIsAllocated;
	LONGLONG       				m_startTime;
	WAVEFORMATEXTENSIBLE	m_wfxex;
	LPDIRECTSOUND8        m_pDSound;
};

#endif // !defined(AFX_ASYNCAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_)
