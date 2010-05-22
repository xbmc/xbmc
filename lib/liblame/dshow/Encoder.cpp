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

#include <streams.h>
#include "Encoder.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CEncoder::CEncoder() :
    m_bInpuTypeSet(FALSE),
    m_bOutpuTypeSet(FALSE),
    m_bFinished(FALSE),
    m_outOffset(0),
    m_outReadOffset(0),
    m_frameCount(0),
    pgf(NULL)
{
    m_outFrameBuf = new unsigned char[OUT_BUFFER_SIZE];
}

CEncoder::~CEncoder()
{
    Close(NULL);

    if (m_outFrameBuf)
        delete [] m_outFrameBuf;
}

//////////////////////////////////////////////////////////////////////
// SetInputType - check if given input type is supported
//////////////////////////////////////////////////////////////////////
HRESULT CEncoder::SetInputType(LPWAVEFORMATEX lpwfex, bool bJustCheck)
{
    CAutoLock l(&m_lock);

    if (lpwfex->wFormatTag == WAVE_FORMAT_PCM)
    {
        if (lpwfex->nChannels == 1 || lpwfex->nChannels == 2)
        {
            if (lpwfex->nSamplesPerSec  == 48000 ||
                lpwfex->nSamplesPerSec  == 44100 ||
                lpwfex->nSamplesPerSec  == 32000 ||
                lpwfex->nSamplesPerSec  == 24000 ||
                lpwfex->nSamplesPerSec  == 22050 ||
                lpwfex->nSamplesPerSec  == 16000 ||
                lpwfex->nSamplesPerSec  == 12000 ||
                lpwfex->nSamplesPerSec  == 11025 ||
                lpwfex->nSamplesPerSec  ==  8000)
            {
                if (lpwfex->wBitsPerSample == 16)
                {
                    if (!bJustCheck)
                    {
                        memcpy(&m_wfex, lpwfex, sizeof(WAVEFORMATEX));
                        m_bInpuTypeSet = true;
                    }

                    return S_OK;
                }
            }
        }
    }

    if (!bJustCheck)
        m_bInpuTypeSet = false;

    return E_INVALIDARG;
}

//////////////////////////////////////////////////////////////////////
// SetOutputType - try to initialize encoder with given output type
//////////////////////////////////////////////////////////////////////
HRESULT CEncoder::SetOutputType(MPEG_ENCODER_CONFIG &mabsi)
{
    CAutoLock l(&m_lock);

    m_mabsi = mabsi;
    m_bOutpuTypeSet = true;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// SetDefaultOutputType - sets default MPEG audio properties according
// to input type
//////////////////////////////////////////////////////////////////////
HRESULT CEncoder::SetDefaultOutputType(LPWAVEFORMATEX lpwfex)
{
    CAutoLock l(&m_lock);

    if(lpwfex->nChannels == 1 || m_mabsi.bForceMono)
        m_mabsi.ChMode = MONO;

    if((lpwfex->nSamplesPerSec < m_mabsi.dwSampleRate) || (lpwfex->nSamplesPerSec % m_mabsi.dwSampleRate != 0))
        m_mabsi.dwSampleRate = lpwfex->nSamplesPerSec;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Init - initialized or reiniyialized encoder SDK with given input 
// and output settings
//////////////////////////////////////////////////////////////////////
HRESULT CEncoder::Init()
{
    CAutoLock l(&m_lock);

    m_outOffset     = 0;
    m_outReadOffset = 0;

    m_bFinished     = FALSE;

    m_frameCount    = 0;

    if (!pgf)
    {
        if (!m_bInpuTypeSet || !m_bOutpuTypeSet)
            return E_UNEXPECTED;

        // Init Lame library
        // note: newer, safer interface which doesn't 
        // allow or require direct access to 'gf' struct is being written
        // see the file 'API' included with LAME.
        if (pgf = lame_init())
        {
            lame_set_num_channels(pgf, m_wfex.nChannels);
            lame_set_in_samplerate(pgf, m_wfex.nSamplesPerSec);
            lame_set_out_samplerate(pgf, m_mabsi.dwSampleRate);
            if ((lame_get_out_samplerate(pgf) >= 32000) && (m_mabsi.dwBitrate < 32))
                lame_set_brate(pgf, 32);
            else
                lame_set_brate(pgf, m_mabsi.dwBitrate);
            lame_set_VBR(pgf, m_mabsi.vmVariable);
            lame_set_VBR_min_bitrate_kbps(pgf, m_mabsi.dwVariableMin);
            lame_set_VBR_max_bitrate_kbps(pgf, m_mabsi.dwVariableMax);

            lame_set_copyright(pgf, m_mabsi.bCopyright);
            lame_set_original(pgf, m_mabsi.bOriginal);
            lame_set_error_protection(pgf, m_mabsi.bCRCProtect);

            lame_set_bWriteVbrTag(pgf, m_mabsi.dwXingTag);
            lame_set_strict_ISO(pgf, m_mabsi.dwStrictISO);
            lame_set_VBR_hard_min(pgf, m_mabsi.dwEnforceVBRmin);

            if (lame_get_num_channels(pgf) == 2 && !m_mabsi.bForceMono)
            {
                //int act_br = pgf->VBR ? pgf->VBR_min_bitrate_kbps + pgf->VBR_max_bitrate_kbps / 2 : pgf->brate;

                // Disabled. It's for user's consideration now
                //int rel = pgf->out_samplerate / (act_br + 1);
                //pgf->mode = rel < 200 ? m_mabsi.ChMode : JOINT_STEREO;

                lame_set_mode(pgf, m_mabsi.ChMode);
            }
            else
                lame_set_mode(pgf, MONO);

            if (lame_get_mode(pgf) == JOINT_STEREO)
                lame_set_force_ms(pgf, m_mabsi.dwForceMS);
            else
                lame_set_force_ms(pgf, 0);

//            pgf->mode_fixed = m_mabsi.dwModeFixed;

            if (m_mabsi.dwVoiceMode != 0)
            {
                lame_set_lowpassfreq(pgf,12000);
                ///pgf->VBR_max_bitrate_kbps = 160;
            }

            if (m_mabsi.dwKeepAllFreq != 0)
            {
                ///pgf->lowpassfreq = -1;
                ///pgf->highpassfreq = -1;
                /// not available anymore
            }

            lame_set_quality(pgf, m_mabsi.dwQuality);
            lame_set_VBR_q(pgf, m_mabsi.dwVBRq);

            lame_init_params(pgf);

            // encoder delay compensation
            {
                int const nch = lame_get_num_channels(pgf);
                short * start_padd = (short *)calloc(48, nch * sizeof(short));

				int out_bytes = 0;

                if (nch == 2)
                    out_bytes = lame_encode_buffer_interleaved(pgf, start_padd, 48, m_outFrameBuf, OUT_BUFFER_SIZE);
                else
                    out_bytes = lame_encode_buffer(pgf, start_padd, start_padd, 48, m_outFrameBuf, OUT_BUFFER_SIZE);

				if (out_bytes > 0)
					m_outOffset += out_bytes;

                free(start_padd);
            }

            return S_OK;
        }

        return E_FAIL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Close - closes encoder
//////////////////////////////////////////////////////////////////////
HRESULT CEncoder::Close(IStream* pStream)
{
	CAutoLock l(&m_lock);
    if (pgf)
    {
		if(lame_get_bWriteVbrTag(pgf) && pStream)
		{
			updateLameTagFrame(pStream);
		}

        lame_close(pgf);
        pgf = NULL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Encode - encodes data placed on pdata and returns
// the number of processed bytes
//////////////////////////////////////////////////////////////////////
int CEncoder::Encode(const short * pdata, int data_size)
{
    CAutoLock l(&m_lock);

    if (!pgf || !m_outFrameBuf || !pdata || data_size < 0 || (data_size & (sizeof(short) - 1)))
        return -1;

    // some data left in the buffer, shift to start
    if (m_outReadOffset > 0)
    {
        if (m_outOffset > m_outReadOffset)
            memmove(m_outFrameBuf, m_outFrameBuf + m_outReadOffset, m_outOffset - m_outReadOffset);

        m_outOffset -= m_outReadOffset;
    }

    m_outReadOffset = 0;



    m_bFinished = FALSE;

    int bytes_processed = 0;
    int const nch = lame_get_num_channels(pgf);

    while (1)
    {
        int nsamples = (data_size - bytes_processed) / (sizeof(short) * nch);

        if (nsamples <= 0)
            break;

        if (nsamples > 1152)
            nsamples = 1152;

        if (m_outOffset >= OUT_BUFFER_MAX)
            break;

        int out_bytes = 0;

        if (nch == 2)
            out_bytes = lame_encode_buffer_interleaved(
                                            pgf,
                                            (short *)(pdata + (bytes_processed / sizeof(short))),
                                            nsamples,
                                            m_outFrameBuf + m_outOffset,
                                            OUT_BUFFER_SIZE - m_outOffset);
        else
            out_bytes = lame_encode_buffer(
                                            pgf,
                                            pdata + (bytes_processed / sizeof(short)),
                                            pdata + (bytes_processed / sizeof(short)),
                                            nsamples,
                                            m_outFrameBuf + m_outOffset,
                                            OUT_BUFFER_SIZE - m_outOffset);

        if (out_bytes < 0)
            return -1;

        m_outOffset     += out_bytes;
        bytes_processed += nsamples * nch * sizeof(short);
    }

    return bytes_processed;
}

//
// Finsh - flush the buffered samples
//
HRESULT CEncoder::Finish()
{
    CAutoLock l(&m_lock);

    if (!pgf || !m_outFrameBuf || (m_outOffset >= OUT_BUFFER_MAX))
        return E_FAIL;

    m_outOffset += lame_encode_flush(pgf, m_outFrameBuf + m_outOffset, OUT_BUFFER_SIZE - m_outOffset);

    m_bFinished = TRUE;

    return S_OK;
}


int getFrameLength(const unsigned char * pdata)
{
    if (!pdata || pdata[0] != 0xff || (pdata[1] & 0xe0) != 0xe0)
        return -1;

    const int sample_rate_tab[4][4] =
    {
        {11025,12000,8000,1},
        {1,1,1,1},
        {22050,24000,16000,1},
        {44100,48000,32000,1}
    };

#define MPEG_VERSION_RESERVED   1
#define MPEG_VERSION_1          3

#define LAYER_III               1

#define BITRATE_FREE            0
#define BITRATE_RESERVED        15

#define SRATE_RESERVED          3

#define EMPHASIS_RESERVED       2

    int version_id      = (pdata[1] & 0x18) >> 3;
    int layer           = (pdata[1] & 0x06) >> 1;
    int bitrate_id      = (pdata[2] & 0xF0) >> 4;
    int sample_rate_id  = (pdata[2] & 0x0C) >> 2;
    int padding         = (pdata[2] & 0x02) >> 1;
    int emphasis        =  pdata[3] & 0x03;

    if (version_id      != MPEG_VERSION_RESERVED &&
        layer           == LAYER_III &&
        bitrate_id      != BITRATE_FREE &&
        bitrate_id      != BITRATE_RESERVED &&
        sample_rate_id  != SRATE_RESERVED &&
        emphasis        != EMPHASIS_RESERVED)
    {
        int spf         = (version_id == MPEG_VERSION_1) ? 1152 : 576;
        int sample_rate = sample_rate_tab[version_id][sample_rate_id];
        int bitrate     = dwBitRateValue[version_id != MPEG_VERSION_1][bitrate_id - 1] * 1000;

        return (bitrate * spf) / (8 * sample_rate) + padding;
    }

    return -1;
}


int CEncoder::GetFrame(const unsigned char ** pframe)
{
    if (!pgf || !m_outFrameBuf || !pframe)
        return -1;

	while ((m_outOffset - m_outReadOffset) > 4)
    {
        int frame_length = getFrameLength(m_outFrameBuf + m_outReadOffset);

        if (frame_length < 0)
        {
            m_outReadOffset++;
        }
        else if (frame_length <= (m_outOffset - m_outReadOffset))
        {
            *pframe = m_outFrameBuf + m_outReadOffset;
            m_outReadOffset += frame_length;

            m_frameCount++;

            // don't deliver the first and the last frames
            if (m_frameCount != 1 && !(m_bFinished && (m_outOffset - m_outReadOffset) < 5))
                return frame_length;
        }
        else
            break;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Returns block of a mp3 file, witch size integer multiples of cbAlign
// or not aligned if finished
////////////////////////////////////////////////////////////////////////////////
int CEncoder::GetBlockAligned(const unsigned char ** pblock, int* piBufferSize, const long& cbAlign)
{
	ASSERT(piBufferSize);
    if (!pgf || !m_outFrameBuf || !pblock)
        return -1;

	int iBlockLen = m_outOffset - m_outReadOffset;
	ASSERT(iBlockLen >= 0);
	
	if(!m_bFinished)
	{
		if(cbAlign > 0)
			iBlockLen-=iBlockLen%cbAlign;
		*piBufferSize = iBlockLen;
	}
	else
	{
		if(cbAlign && iBlockLen%cbAlign)
		{
			*piBufferSize = iBlockLen + cbAlign - iBlockLen%cbAlign;
		}
		else
		{
			*piBufferSize = iBlockLen;
		}
	}

	if(iBlockLen) {
		*pblock = m_outFrameBuf + m_outReadOffset;
		m_outReadOffset+=iBlockLen;
	}

	return iBlockLen;
}

HRESULT CEncoder::maybeSyncWord(IStream *pStream)
{
	HRESULT hr = S_OK;
    unsigned char mp3_frame_header[4];
	ULONG nbytes;
	if(FAILED(hr = pStream->Read(mp3_frame_header, sizeof(mp3_frame_header), &nbytes)))
		return hr;
	
    if ( nbytes != sizeof(mp3_frame_header) ) {
        return E_FAIL;
    }
    if ( mp3_frame_header[0] != 0xffu ) {
        return S_FALSE; /* doesn't look like a sync word */
    }
    if ( (mp3_frame_header[1] & 0xE0u) != 0xE0u ) {
		return S_FALSE; /* doesn't look like a sync word */
    }
    return S_OK;
}

HRESULT CEncoder::skipId3v2(IStream *pStream, size_t lametag_frame_size)
{
	HRESULT hr = S_OK;
    ULONG  nbytes;
    size_t  id3v2TagSize = 0;
    unsigned char id3v2Header[10];
	LARGE_INTEGER seekTo;

    /* seek to the beginning of the stream */
	seekTo.QuadPart = 0;
	if (FAILED(hr = pStream->Seek(seekTo,  STREAM_SEEK_SET, NULL))) {
        return hr;  /* not seekable, abort */
    }
    /* read 10 bytes in case there's an ID3 version 2 header here */
	hr = pStream->Read(id3v2Header, sizeof(id3v2Header), &nbytes);
    if (FAILED(hr))
		return hr;
	if(nbytes != sizeof(id3v2Header)) {
        return E_FAIL;  /* not readable, maybe opened Write-Only */
    }
    /* does the stream begin with the ID3 version 2 file identifier? */
    if (!strncmp((char *) id3v2Header, "ID3", 3)) {
        /* the tag size (minus the 10-byte header) is encoded into four
        * bytes where the most significant bit is clear in each byte
        */
        id3v2TagSize = (((id3v2Header[6] & 0x7f) << 21)
            | ((id3v2Header[7] & 0x7f) << 14)
            | ((id3v2Header[8] & 0x7f) << 7)
            | (id3v2Header[9] & 0x7f))
            + sizeof id3v2Header;
    }
    /* Seek to the beginning of the audio stream */
	seekTo.QuadPart = id3v2TagSize;
	if (FAILED(hr = pStream->Seek(seekTo, STREAM_SEEK_SET, NULL))) {
        return hr;
    }
    if (S_OK != (hr = maybeSyncWord(pStream))) {
		return SUCCEEDED(hr)?E_FAIL:hr;
    }
	seekTo.QuadPart = id3v2TagSize+lametag_frame_size;
	if (FAILED(hr = pStream->Seek(seekTo, STREAM_SEEK_SET, NULL))) {
        return hr;
    }
    if (S_OK != (hr = maybeSyncWord(pStream))) {
        return SUCCEEDED(hr)?E_FAIL:hr;
    }
    /* OK, it seems we found our LAME-Tag/Xing frame again */
    /* Seek to the beginning of the audio stream */
	seekTo.QuadPart = id3v2TagSize;
	if (FAILED(hr = pStream->Seek(seekTo, STREAM_SEEK_SET, NULL))) {
        return hr;
    }
    return S_OK;
}

// Updates VBR tag
HRESULT CEncoder::updateLameTagFrame(IStream* pStream)
{
	HRESULT hr = S_OK;
	size_t n = lame_get_lametag_frame( pgf, 0, 0 ); /* ask for bufer size */

    if ( n > 0 )
    {
        unsigned char* buffer = 0;
        ULONG m = n;

        if ( FAILED(hr = skipId3v2(pStream, n) )) 
        {
            /*DispErr( "Error updating LAME-tag frame:\n\n"
                     "can't locate old frame\n" );*/
            return hr;
        }

        buffer = (unsigned char*)malloc( n );

        if ( buffer == 0 ) 
        {
            /*DispErr( "Error updating LAME-tag frame:\n\n"
                     "can't allocate frame buffer\n" );*/
            return E_OUTOFMEMORY;
        }

        /* Put it all to disk again */
        n = lame_get_lametag_frame( pgf, buffer, n );
        if ( n > 0 ) 
        {
			hr = pStream->Write(buffer, n, &m);        
        }
        free( buffer );

        if ( m != n ) 
        {
            /*DispErr( "Error updating LAME-tag frame:\n\n"
                     "couldn't write frame into file\n" );*/
			return E_FAIL;
        }
    }
    return hr;
}
