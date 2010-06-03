/*
 *  LAME MP3 encoder for DirectShow
 *  LAME encoder wrapper
 *
 *  Copyright (c) 2000-2005 Marie Orlova, Peter Gubanov, Vitaly Ivanov, Elecard Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#if !defined(AFX_VITECENCODER_H__40DC8A44_B937_11D2_A381_A2FD7C37FA15__INCLUDED_)
#define AFX_VITECENCODER_H__40DC8A44_B937_11D2_A381_A2FD7C37FA15__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <lame.h>


const unsigned int dwBitRateValue[2][14] =
{
    {32,40,48,56,64,80,96,112,128,160,192,224,256,320},     // MPEG-1
    {8,16,24,32,40,48,56,64,80,96,112,128,144,160}          // MPEG-2/2.5
};
/*
#define STEREO           0
#define JOINT_STEREO     1
#define DUAL_CHANNEL     2
#define MONO             3
*/

#define OUT_BUFFER_SIZE             16384
#define OUT_BUFFER_GUARD            8192

#define OUT_BUFFER_MAX              (OUT_BUFFER_SIZE - OUT_BUFFER_GUARD)

typedef struct {
    DWORD   dwSampleRate;                   //SF in Hz
    DWORD   dwBitrate;                      //BR in bit per second
    vbr_mode    vmVariable;
    DWORD   dwVariableMin;                  //specify a minimum allowed bitrate
    DWORD   dwVariableMax;                  //specify a maximum allowed bitrate
    DWORD   dwQuality;                      //Encoding quality
    DWORD   dwVBRq;                         // VBR quality setting (0=highest quality, 9=lowest)                         
    long    lLayer;                         //Layer: 1 or 2

    MPEG_mode ChMode;                       //Channel coding mode: see doc
    DWORD   dwForceMS;

    DWORD   bCRCProtect;                    //Is CRC protection activated?
    DWORD   bForceMono;
    DWORD   bSetDuration;
    DWORD   bCopyright;                     //Is the stream protected by copyright?
    DWORD   bOriginal;                      //Is the stream an original?

    DWORD   dwPES;                          // PES header. Obsolete

    DWORD   dwEnforceVBRmin;
    DWORD   dwVoiceMode;
    DWORD   dwKeepAllFreq;
    DWORD   dwStrictISO;
    DWORD   dwNoShortBlock;
    DWORD   dwXingTag;
    DWORD   dwModeFixed;
    DWORD   bSampleOverlap;
} MPEG_ENCODER_CONFIG;


class CEncoder
{
public:

    CEncoder();
    virtual ~CEncoder();

    // Initialize encoder with PCM stream properties
    HRESULT SetInputType(LPWAVEFORMATEX lpwfex, bool bJustCheck = FALSE);   // returns E_INVALIDARG if not supported
    // GetInputType - returns current input type
    HRESULT GetInputType(WAVEFORMATEX *pwfex)
    {
        if(m_bInpuTypeSet)
        {
            memcpy(pwfex, &m_wfex, sizeof(WAVEFORMATEX));
            return S_OK;
        }
        else
            return E_UNEXPECTED;
    }

    // Set MPEG audio parameters
    HRESULT SetOutputType(MPEG_ENCODER_CONFIG &mabsi);      // returns E_INVALIDARG if not supported or
                                                            // not compatible with input type
    // Return current MPEG audio settings
    HRESULT GetOutputType(MPEG_ENCODER_CONFIG* pmabsi)
    {
        if (m_bOutpuTypeSet)
        {
            memcpy(pmabsi, &m_mabsi, sizeof(MPEG_ENCODER_CONFIG));
            return S_OK;
        }
        else
            return E_UNEXPECTED;
    }

    // Set if output stream is a PES. Obsolete
    void SetPES(bool bPES)
    {
        m_mabsi.dwPES = false;//bPES;
    }
    // Is output stream a PES. Obsolete
    BOOL IsPES() const
    {
        return (BOOL)m_mabsi.dwPES;
    }

    // Initialize encoder SDK
    HRESULT Init();
    // Close encoder SDK
    HRESULT Close(IStream* pStream);

    // Encode media sample data
    int Encode(const short * pdata, int data_size);
    int GetFrame(const unsigned char ** pframe);
	
	// Returns block of a mp3 file, witch size integer multiples of cbAlign
	int GetBlockAligned(const unsigned char ** pblock, int* piBufferSize, const long& cbAlign);

    HRESULT Finish();

protected:
	HRESULT updateLameTagFrame(IStream* pStream);
	HRESULT skipId3v2(IStream *pStream, size_t lametag_frame_size);
	HRESULT maybeSyncWord(IStream *pStream);
    HRESULT SetDefaultOutputType(LPWAVEFORMATEX lpwfex);

    // Input media type
    WAVEFORMATEX        m_wfex;

    // Output media type
    MPEG_ENCODER_CONFIG m_mabsi;

    // Compressor private data
    lame_global_flags * pgf;

    // Compressor miscelaneous state
    BOOL                m_bInpuTypeSet;
    BOOL                m_bOutpuTypeSet;

    BOOL                m_bFinished;
    int                 m_frameCount;

    unsigned char *     m_outFrameBuf;
    int                 m_outOffset;
    int                 m_outReadOffset;

    CCritSec            m_lock;
};

#endif // !defined(AFX_VITECENCODER_H__40DC8A44_B937_11D2_A381_A2FD7C37FA15__INCLUDED_)
