/*
 *  LAME MP3 encoder for DirectShow
 *  DirectShow filter implementation
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
#include <olectl.h>
#include <initguid.h>
//#include <olectlid.h>
#include "uids.h"
#include "iaudioprops.h"
#include "mpegac.h"
#include "resource.h"

#include "PropPage.h"
#include "PropPage_adv.h"
#include "aboutprp.h"

#include "Encoder.h"
#include "Reg.h"

#ifndef _INC_MMREG
#include <mmreg.h>
#endif

// default parameters
#define         DEFAULT_LAYER               3
#define         DEFAULT_STEREO_MODE         JOINT_STEREO
#define         DEFAULT_FORCE_MS            0
#define         DEFAULT_MODE_FIXED          0
#define         DEFAULT_ENFORCE_MIN         0
#define         DEFAULT_VOICE               0
#define         DEFAULT_KEEP_ALL_FREQ       0
#define         DEFAULT_STRICT_ISO          0
#define         DEFAULT_DISABLE_SHORT_BLOCK 0
#define         DEFAULT_XING_TAG            0
#define         DEFAULT_SAMPLE_RATE         44100
#define         DEFAULT_BITRATE             128
#define         DEFAULT_VARIABLE            0
#define         DEFAULT_CRC                 0
#define         DEFAULT_FORCE_MONO          0
#define         DEFAULT_SET_DURATION        1
#define         DEFAULT_SAMPLE_OVERLAP      1
#define         DEFAULT_COPYRIGHT           0
#define         DEFAULT_ORIGINAL            0
#define         DEFAULT_VARIABLEMIN         80
#define         DEFAULT_VARIABLEMAX         160
#define         DEFAULT_ENCODING_QUALITY    5
#define         DEFAULT_VBR_QUALITY         4
#define         DEFAULT_PES                 0

#define         DEFAULT_FILTER_MERIT        MERIT_DO_NOT_USE                // Standard compressor merit value

#define GET_DATARATE(kbps) (kbps * 1000 / 8)
#define GET_FRAMELENGTH(bitrate, sample_rate) ((WORD)(((sample_rate < 32000 ? 72000 : 144000) * (bitrate))/(sample_rate)))
#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

// Create a list of all (or mostly all) of the encoder CBR output capabilities which
// will be parsed into a list of capabilities used by the IAMStreamConfig Interface
output_caps_t OutputCapabilities[] = 
{ // {SampleRate, BitRate}
    { 48000, 320 },{ 48000, 256 },{ 48000, 224 },{ 48000, 192 },            // MPEG 1.0 Spec @ 48KHz
    { 48000, 160 },{ 48000, 128 },{ 48000, 112 },{ 48000, 96 },
    { 48000, 80 },{ 48000, 64 },{ 48000, 56 },{ 48000, 48 },
    { 48000, 40 },{ 48000, 32 },

    { 24000, 160 },{ 24000, 144 },{ 24000, 128 },{ 24000, 112 },            // MPEG 2.0 Spec @ 24KHz
    { 24000, 96 },{ 24000, 80 },{ 24000, 64 },{ 24000, 56 },
    { 24000, 48 },{ 24000, 40 },{ 24000, 32 },{ 24000, 24 },
    { 24000, 16 },{ 24000, 8 },

    { 12000, 64 },{ 12000, 56 },{ 12000, 48 },{ 12000, 40 },                // MPEG 2.5 Spec @ 12KHz
    { 12000, 32 },{ 12000, 24 },{ 12000, 16 },{ 12000, 8 },
    // ---------------------------                                          --------------------------
    { 44100, 320 },{ 44100, 256 },{ 44100, 224 },{ 44100, 192 },            // MPEG 1.0 Spec @ 44.1KHz
    { 44100, 160 },{ 44100, 128 },{ 44100, 112 },{ 44100, 96 },
    { 44100, 80 },{ 44100, 64 },{ 44100, 56 },{ 44100, 48 },
    { 44100, 40 },{ 44100, 32 },

    { 22050, 160 },{ 22050, 144 },{ 22050, 128 },{ 22050, 112 },            // MPEG 2.0 Spec @ 22.05KHz
    { 22050, 96 },{ 22050, 80 },{ 22050, 64 },{ 22050, 56 },
    { 22050, 48 },{ 22050, 40 },{ 22050, 32 },{ 22050, 24 },
    { 22050, 16 },{ 22050, 8 },

    { 11025, 64 },{ 11025, 56 },{ 11025, 48 },{ 11025, 40 },                // MPEG 2.5 Spec @ 11.025KHz
    { 11025, 32 },{ 11025, 24 },{ 11025, 16 },{ 11025, 8 },
    // ---------------------------                                          --------------------------
    { 32000, 320 },{ 32000, 256 },{ 32000, 224 },{ 32000, 192 },            // MPEG 1.0 Spec @ 32KHz
    { 32000, 160 },{ 32000, 128 },{ 32000, 112 },{ 32000, 96 },
    { 32000, 80 },{ 32000, 64 },{ 32000, 56 },{ 32000, 48 },
    { 32000, 40 },{ 32000, 32 },

    { 16000, 160 },{ 16000, 144 },{ 16000, 128 },{ 16000, 112 },            // MPEG 2.0 Spec @ 16KHz
    { 16000, 96 },{ 16000, 80 },{ 16000, 64 },{ 16000, 56 },
    { 16000, 48 },{ 16000, 40 },{ 16000, 32 },{ 16000, 24 },
    { 16000, 16 },{ 16000, 8 },

    { 8000, 64 },{ 8000, 56 },{ 8000, 48 },{ 8000, 40 },                    // MPEG 2.5 Spec @ 8KHz
    { 8000, 32 },{ 8000, 24 },{ 8000, 16 },{ 8000, 8 }
};


/*  Registration setup stuff */
//  Setup data


AMOVIESETUP_MEDIATYPE sudMpgInputType[] =
{
    { &MEDIATYPE_Audio, &MEDIASUBTYPE_PCM }
};
AMOVIESETUP_MEDIATYPE sudMpgOutputType[] =
{
    { &MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload },
    { &MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG2_AUDIO },
    { &MEDIATYPE_Audio, &MEDIASUBTYPE_MP3 },
    { &MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Audio }
};

AMOVIESETUP_PIN sudMpgPins[] =
{
    { L"PCM Input",
      FALSE,                               // bRendered
      FALSE,                               // bOutput
      FALSE,                               // bZero
      FALSE,                               // bMany
      &CLSID_NULL,                         // clsConnectsToFilter
      NULL,                                // ConnectsToPin
      NUMELMS(sudMpgInputType),            // Number of media types
      sudMpgInputType
    },
    { L"MPEG Output",
      FALSE,                               // bRendered
      TRUE,                                // bOutput
      FALSE,                               // bZero
      FALSE,                               // bMany
      &CLSID_NULL,                         // clsConnectsToFilter
      NULL,                                // ConnectsToPin
      NUMELMS(sudMpgOutputType),           // Number of media types
      sudMpgOutputType
    }
};

AMOVIESETUP_FILTER sudMpgAEnc =
{
	&CLSID_LAMEDShowFilter,
    L"LAME Audio Encoder",
    DEFAULT_FILTER_MERIT,                  // Standard compressor merit value
    NUMELMS(sudMpgPins),                   // 2 pins
    sudMpgPins
};

/*****************************************************************************/
// COM Global table of objects in this dll
static WCHAR g_wszName[] = L"LAME Audio Encoder";
CFactoryTemplate g_Templates[] = 
{
  { g_wszName, &CLSID_LAMEDShowFilter, CMpegAudEnc::CreateInstance, NULL, &sudMpgAEnc },
  { L"LAME Audio Encoder Property Page", &CLSID_LAMEDShow_PropertyPage, CMpegAudEncPropertyPage::CreateInstance},
  { L"LAME Audio Encoder Property Page", &CLSID_LAMEDShow_PropertyPageAdv, CMpegAudEncPropertyPageAdv::CreateInstance},
  { L"LAME Audio Encoder About", &CLSID_LAMEDShow_About, CMAEAbout::CreateInstance}
};
// Count of objects listed in g_cTemplates
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);



////////////////////////////////////////////
// Declare the DirectShow filter information.

// Used by IFilterMapper2() in the call to DllRegisterServer()
// to register the filter in the CLSID_AudioCompressorCategory.
REGFILTER2 rf2FilterReg = {
    1,                     // Version number.
    DEFAULT_FILTER_MERIT,  // Merit. This should match the merit specified in the AMOVIESETUP_FILTER definition
    NUMELMS(sudMpgPins),   // Number of pins.
    sudMpgPins             // Pointer to pin information.
};

STDAPI DllRegisterServer(void)
{
    HRESULT hr = AMovieDllRegisterServer2(TRUE);
    if (FAILED(hr)) {
        return hr;
    }

    IFilterMapper2 *pFM2 = NULL;
    hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, IID_IFilterMapper2, (void **)&pFM2);
    if (SUCCEEDED(hr)) {
        hr = pFM2->RegisterFilter(
            CLSID_LAMEDShowFilter,           // Filter CLSID. 
            g_wszName,                       // Filter name.
            NULL,                            // Device moniker. 
            &CLSID_AudioCompressorCategory,  // Audio compressor category.
            g_wszName,                       // Instance data.
            &rf2FilterReg                    // Filter information.
            );
        pFM2->Release();
    }
    return hr;
}

STDAPI DllUnregisterServer()
{
    HRESULT hr = AMovieDllRegisterServer2(FALSE);
    if (FAILED(hr)) {
        return hr;
    }

    IFilterMapper2 *pFM2 = NULL;
    hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, IID_IFilterMapper2, (void **)&pFM2);
    if (SUCCEEDED(hr)) {
        hr = pFM2->UnregisterFilter(&CLSID_AudioCompressorCategory, g_wszName, CLSID_LAMEDShowFilter);
        pFM2->Release();
    }
    return hr;
}



CUnknown *CMpegAudEnc::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) 
{
    CMpegAudEnc *punk = new CMpegAudEnc(lpunk, phr);
    if (punk == NULL) 
        *phr = E_OUTOFMEMORY;
    return punk;
}

CMpegAudEnc::CMpegAudEnc(LPUNKNOWN lpunk, HRESULT *phr)
 :  CTransformFilter(NAME("LAME Audio Encoder"), lpunk, CLSID_LAMEDShowFilter),
    CPersistStream(lpunk, phr)
{
    // ENCODER OUTPUT PIN
    // Override the output pin with our own which will implement the IAMStreamConfig Interface
    CTransformOutputPin *pOut = new CMpegAudEncOutPin( this, phr );
    if (pOut == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }
    else if (FAILED(*phr)) {             // A failed return code should delete the object
        delete pOut;
        return;
    }
    m_pOutput = pOut;

    // ENCODER INPUT PIN
    // Since we've created our own output pin we must also create
    // the input pin ourselves because the CTransformFilter base class 
    // will create an extra output pin if the input pin wasn't created.        
    CTransformInputPin *pIn = new CTransformInputPin(NAME("LameEncoderInputPin"),
                                                     this,              // Owner filter
                                                     phr,               // Result code
                                                     L"Input");         // Pin name

    if (pIn == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }
    else if (FAILED(*phr)) {             // A failed return code should delete the object
        delete pIn;
        return;
    }
    m_pInput = pIn;


    MPEG_ENCODER_CONFIG mec;
    ReadPresetSettings(&mec);
    m_Encoder.SetOutputType(mec);

    m_CapsNum = 0;
    m_hasFinished = TRUE;
    m_bStreamOutput = FALSE;
    m_currentMediaTypeIndex = 0;
}

CMpegAudEnc::~CMpegAudEnc(void)
{
}

LPAMOVIESETUP_FILTER CMpegAudEnc::GetSetupData()
{
    return &sudMpgAEnc;
}


HRESULT CMpegAudEnc::Receive(IMediaSample * pSample)
{
    CAutoLock lock(&m_cs);

    if (!pSample)
        return S_OK;

    BYTE * pSourceBuffer = NULL;

    if (pSample->GetPointer(&pSourceBuffer) != S_OK || !pSourceBuffer)
        return S_OK;

    long sample_size = pSample->GetActualDataLength();

    REFERENCE_TIME rtStart, rtStop;
    BOOL gotValidTime = (pSample->GetTime(&rtStart, &rtStop) != VFW_E_SAMPLE_TIME_NOT_SET);

    if (sample_size <= 0 || pSourceBuffer == NULL || m_hasFinished || (gotValidTime && rtStart < 0))
        return S_OK;

    if (gotValidTime)
    {
        if (m_rtStreamTime < 0)
        {
            m_rtStreamTime = rtStart;
            m_rtEstimated = rtStart;
        }
        else
        {
            resync_point_t * sync = m_sync + m_sync_in_idx;

            if (sync->applied)
            {
                REFERENCE_TIME rtGap = rtStart - m_rtEstimated;

                // if old sync data is applied and gap is greater than 1 ms
                // then make a new synchronization point
                if (rtGap > 10000 || (m_allowOverlap && rtGap < -10000))
                {
                    sync->sample    = m_samplesIn;
                    sync->delta     = rtGap;
                    sync->applied   = FALSE;

                    m_rtEstimated  += sync->delta;

                    if (m_sync_in_idx < (RESYNC_COUNT - 1))
                        m_sync_in_idx++;
                    else
                        m_sync_in_idx = 0;
                }
            }
        }
    }

    m_rtEstimated   += (LONGLONG)(m_bytesToDuration * sample_size);
    m_samplesIn     += sample_size / m_bytesPerSample;

    while (sample_size > 0)
    {
        int bytes_processed = m_Encoder.Encode((short *)pSourceBuffer, sample_size);

        if (bytes_processed <= 0)
            return S_OK;

        FlushEncodedSamples();

        sample_size     -= bytes_processed;
        pSourceBuffer   += bytes_processed;
    }

    return S_OK;
}




HRESULT CMpegAudEnc::FlushEncodedSamples()
{
    IMediaSample * pOutSample = NULL;
    BYTE * pDst = NULL;

	if(m_bStreamOutput)
	{
		HRESULT hr = S_OK;
		const unsigned char *   pblock      = NULL;
		int iBufferSize;
		int iBlockLength = m_Encoder.GetBlockAligned(&pblock, &iBufferSize, m_cbStreamAlignment);
		
		if(!iBlockLength)
			return S_OK;

		hr = m_pOutput->GetDeliveryBuffer(&pOutSample, NULL, NULL, 0);
		if (hr == S_OK && pOutSample)
		{
			hr = pOutSample->GetPointer(&pDst);
			if (hr == S_OK && pDst)
			{
				CopyMemory(pDst, pblock, iBlockLength);
				REFERENCE_TIME rtEndPos = m_rtBytePos + iBufferSize;
				EXECUTE_ASSERT(S_OK == pOutSample->SetTime(&m_rtBytePos, &rtEndPos));
				pOutSample->SetActualDataLength(iBufferSize);
				m_rtBytePos += iBlockLength;
				m_pOutput->Deliver(pOutSample);
			}

			pOutSample->Release();
		}
		return S_OK;
	}

    if (m_rtStreamTime < 0)
        m_rtStreamTime = 0;

    while (1)
    {
        const unsigned char *   pframe      = NULL;
        int                     frame_size  = m_Encoder.GetFrame(&pframe);

        if (frame_size <= 0 || !pframe)
            break;

        if (!m_sync[m_sync_out_idx].applied && m_sync[m_sync_out_idx].sample <= m_samplesOut)
        {
            m_rtStreamTime += m_sync[m_sync_out_idx].delta;
            m_sync[m_sync_out_idx].applied = TRUE;

            if (m_sync_out_idx < (RESYNC_COUNT - 1))
                m_sync_out_idx++;
            else
                m_sync_out_idx = 0;
        }

        REFERENCE_TIME rtStart = m_rtStreamTime;
        REFERENCE_TIME rtStop = rtStart + m_rtFrameTime;

		HRESULT hr = S_OK;
		
		hr = m_pOutput->GetDeliveryBuffer(&pOutSample, NULL, NULL, 0);
		if (hr == S_OK && pOutSample)
		{
			hr = pOutSample->GetPointer(&pDst);
			if (hr == S_OK && pDst)
			{
				CopyMemory(pDst, pframe, frame_size);
				pOutSample->SetActualDataLength(frame_size);
			
				pOutSample->SetSyncPoint(TRUE);
				pOutSample->SetTime(&rtStart, m_setDuration ? &rtStop : NULL);
			

				m_pOutput->Deliver(pOutSample);
			}

			pOutSample->Release();
		}
	

        m_samplesOut += m_samplesPerFrame;
        m_rtStreamTime = rtStop;
    }

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////
//  StartStreaming - prepare to receive new data
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::StartStreaming()
{
    WAVEFORMATEX * pwfxIn  = (WAVEFORMATEX *) m_pInput->CurrentMediaType().Format();

    m_bytesPerSample    = pwfxIn->nChannels * sizeof(short);
	DWORD dwOutSampleRate;
	if(MEDIATYPE_Stream == m_pOutput->CurrentMediaType().majortype)
	{
		MPEG_ENCODER_CONFIG mcfg;
		if(FAILED(m_Encoder.GetOutputType(&mcfg)))
			return E_FAIL;
		
		dwOutSampleRate = mcfg.dwSampleRate;
	}
	else
	{
		dwOutSampleRate = ((WAVEFORMATEX *) m_pOutput->CurrentMediaType().Format())->nSamplesPerSec;
	}
	m_samplesPerFrame   = (dwOutSampleRate >= 32000) ? 1152 : 576;

    m_rtFrameTime = MulDiv(10000000, m_samplesPerFrame, dwOutSampleRate);

    m_samplesIn = m_samplesOut = 0;
    m_rtStreamTime = -1;
	m_rtBytePos = 0;

    // initialize encoder
    m_Encoder.Init();

    m_hasFinished   = FALSE;

    for (int i = 0; i < RESYNC_COUNT; i++)
    {
        m_sync[i].sample   = 0;
        m_sync[i].delta    = 0;
        m_sync[i].applied  = TRUE;
    }

    m_sync_in_idx = 0;
    m_sync_out_idx = 0;

    get_SetDuration(&m_setDuration);
    get_SampleOverlap(&m_allowOverlap);

	return S_OK;
}


HRESULT CMpegAudEnc::StopStreaming()
{
	IStream *pStream = NULL;
	if(m_bStreamOutput && m_pOutput->IsConnected() != FALSE)
	{
		IPin * pDwnstrmInputPin = m_pOutput->GetConnected();
		if(pDwnstrmInputPin && FAILED(pDwnstrmInputPin->QueryInterface(IID_IStream, (LPVOID*)(&pStream))))
		{
			pStream = NULL;
		}
	}
	

	m_Encoder.Close(pStream);

	if(pStream)
		pStream->Release();

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////
//  EndOfStream - stop data processing 
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::EndOfStream()
{
    CAutoLock lock(&m_cs);

    // Flush data
    m_Encoder.Finish();
    FlushEncodedSamples();

	IStream *pStream = NULL;
    if(m_bStreamOutput && m_pOutput->IsConnected() != FALSE)
	{
		IPin * pDwnstrmInputPin = m_pOutput->GetConnected();
		if(pDwnstrmInputPin)
		{
			if(FAILED(pDwnstrmInputPin->QueryInterface(IID_IStream, (LPVOID*)(&pStream))))
			{
				pStream = NULL;	
			}
		}
	}

	if(pStream)
	{
		ULARGE_INTEGER size;
		size.QuadPart = m_rtBytePos;
		pStream->SetSize(size);	
	}

	m_Encoder.Close(pStream);

	if(pStream)
		pStream->Release();

    m_hasFinished = TRUE;

    return CTransformFilter::EndOfStream();
}


////////////////////////////////////////////////////////////////////////////
//  BeginFlush  - stop data processing 
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::BeginFlush()
{
    HRESULT hr = CTransformFilter::BeginFlush();

    if (SUCCEEDED(hr))
    {
        CAutoLock lock(&m_cs);

        DWORD dwDstSize = 0;

        // Flush data
        m_Encoder.Finish();
        FlushEncodedSamples();

		IStream *pStream = NULL;
		if(m_bStreamOutput && m_pOutput->IsConnected() != FALSE)
		{
			IPin * pDwnstrmInputPin = m_pOutput->GetConnected();
			if(pDwnstrmInputPin && SUCCEEDED(pDwnstrmInputPin->QueryInterface(IID_IStream, (LPVOID*)(&pStream))))
			{
				ULARGE_INTEGER size;
				size.QuadPart = m_rtBytePos;
				pStream->SetSize(size);	
				pStream->Release();
			}
		}
	    m_rtStreamTime = -1;
		m_rtBytePos = 0;
    }

    return hr;
}



////////////////////////////////////////////////////////////////////////////
//	SetMediaType - called when filters are connecting
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::SetMediaType(PIN_DIRECTION direction, const CMediaType * pmt)
{
    HRESULT hr = S_OK;

    if (direction == PINDIR_INPUT)
    {
		if (*pmt->FormatType() != FORMAT_WaveFormatEx)
        return VFW_E_INVALIDMEDIATYPE;

		if (pmt->FormatLength() < sizeof(WAVEFORMATEX))
			return VFW_E_INVALIDMEDIATYPE;

        DbgLog((LOG_TRACE,1,TEXT("CMpegAudEnc::SetMediaType(), direction = PINDIR_INPUT")));

        // Pass input media type to encoder
        m_Encoder.SetInputType((LPWAVEFORMATEX)pmt->Format());

        WAVEFORMATEX * pwfx = (WAVEFORMATEX *)pmt->Format();

        if (pwfx)
            m_bytesToDuration = (float)1.e7 / (float)(pwfx->nChannels * sizeof(short) * pwfx->nSamplesPerSec);
        else
            m_bytesToDuration = 0.0;

        // Parse the encoder output capabilities into the subset of capabilities that are supported 
        // for the current input format. This listing will be utilized by the IAMStreamConfig Interface.
        LoadOutputCapabilities(pwfx->nSamplesPerSec);

        Reconnect();
    }
    else if (direction == PINDIR_OUTPUT)
    {
        // Before we set the output type, we might need to reconnect 
        // the input pin with a new type.
        if (m_pInput && m_pInput->IsConnected()) 
        {
            // Check if the current input type is compatible.
            hr = CheckTransform(&m_pInput->CurrentMediaType(), &m_pOutput->CurrentMediaType());
            if (FAILED(hr)) {
                // We need to reconnect the input pin. 
                // Note: The CheckMediaType method has already called QueryAccept on the upstream filter. 
                hr = m_pGraph->Reconnect(m_pInput);
                return hr;
            }
        }

//        WAVEFORMATEX wfIn;
//        m_Encoder.GetInputType(&wfIn);

//        if (wfIn.nSamplesPerSec %
//            ((LPWAVEFORMATEX)pmt->Format())->nSamplesPerSec != 0)
//            return VFW_E_TYPE_NOT_ACCEPTED;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// CheckInputType - check if you can support mtIn
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::CheckInputType(const CMediaType* mtIn)
{
    if (*mtIn->Type() == MEDIATYPE_Audio && *mtIn->FormatType() == FORMAT_WaveFormatEx)
        if (mtIn->FormatLength() >= sizeof(WAVEFORMATEX))
            if (mtIn->IsTemporalCompressed() == FALSE)
                return m_Encoder.SetInputType((LPWAVEFORMATEX)mtIn->Format(), true);

    return E_INVALIDARG;
}

////////////////////////////////////////////////////////////////////////////
// CheckTransform - checks if we can support the transform from this input to this output
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	if(MEDIATYPE_Stream != mtOut->majortype)
	{
		if (*mtOut->FormatType() != FORMAT_WaveFormatEx)
			return VFW_E_INVALIDMEDIATYPE;

		if (mtOut->FormatLength() < sizeof(WAVEFORMATEX))
			return VFW_E_INVALIDMEDIATYPE;

		MPEG_ENCODER_CONFIG	mec;
		if(FAILED(m_Encoder.GetOutputType(&mec)))
			return S_OK;

		if (((LPWAVEFORMATEX)mtIn->Format())->nSamplesPerSec % mec.dwSampleRate != 0)
			return S_OK;

		if (mec.dwSampleRate != ((LPWAVEFORMATEX)mtOut->Format())->nSamplesPerSec)
			return VFW_E_TYPE_NOT_ACCEPTED;

		return S_OK;
	}
	else if(mtOut->subtype == MEDIASUBTYPE_MPEG1Audio)
		return S_OK;

	return VFW_E_TYPE_NOT_ACCEPTED;
}

////////////////////////////////////////////////////////////////////////////
// DecideBufferSize - sets output buffers number and size
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::DecideBufferSize(
                        IMemAllocator*		  pAllocator,
                        ALLOCATOR_PROPERTIES* pProperties)
{
    HRESULT hr = S_OK;

	if(m_bStreamOutput)
		m_cbStreamAlignment = pProperties->cbAlign;

    ///
    if (pProperties->cBuffers == 0) pProperties->cBuffers = 1;  // If downstream filter didn't suggest a buffer count then default to 1
    pProperties->cbBuffer = OUT_BUFFER_SIZE;
	//
	
	ASSERT(pProperties->cbBuffer);
	
    ALLOCATOR_PROPERTIES Actual;
    hr = pAllocator->SetProperties(pProperties,&Actual);
    if(FAILED(hr))
        return hr;

    if (Actual.cbBuffer < pProperties->cbBuffer ||
        Actual.cBuffers < pProperties->cBuffers) 
    {// can't use this allocator
        return E_INVALIDARG;
    }
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// GetMediaType - overrideable for suggesting output pin media types
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    DbgLog((LOG_TRACE,1,TEXT("CMpegAudEnc::GetMediaType()")));

    return m_pOutput->GetMediaType(iPosition, pMediaType);
}

////////////////////////////////////////////////////////////////////////////
//  Reconnect - called after a manual change has been made to the 
//  encoder parameters to reset the filter output media type structure
//  to match the current encoder out MPEG audio properties 
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::Reconnect()
{
    HRESULT hr = S_FALSE;

    if (m_pOutput && m_pOutput->IsConnected() && m_State == State_Stopped)
    {
        MPEG_ENCODER_CONFIG mec;
        hr = m_Encoder.GetOutputType(&mec);

        if ((hr = m_Encoder.SetOutputType(mec)) == S_OK)
        {
            // Create an updated output MediaType using the current encoder settings
            CMediaType cmt;
			cmt.InitMediaType();
            m_pOutput->GetMediaType(m_currentMediaTypeIndex, &cmt);

            // If the updated MediaType matches the current output MediaType no reconnect is needed
            if (m_pOutput->CurrentMediaType() == cmt) return S_OK;

            // Attempt to reconnect the output pin using the updated MediaType
            if (S_OK == (hr = m_pOutput->GetConnected()->QueryAccept(&cmt))) {
                hr = m_pOutput->SetMediaType(&cmt);
                if ( FAILED(hr) ) { return(hr); }

                hr = m_pGraph->Reconnect(m_pOutput);
            }
            else
                hr = m_pOutput->SetMediaType(&cmt);
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////
//  LoadOutputCapabilities - create a list of the currently supported output 
//  format capabilities which will be used by the IAMStreamConfig Interface
////////////////////////////////////////////////////////////////////////////
void CMpegAudEnc::LoadOutputCapabilities(DWORD sample_rate)
{
    m_CapsNum = 0;

    // Clear out any existing output capabilities
    ZeroMemory(OutputCaps, sizeof(OutputCaps));

    // Create the set of Constant Bit Rate output capabilities that are
    // supported for the current input pin sampling rate.
    for (int i = 0;  i < NUMELMS(OutputCapabilities); i++) {
        if (0 == sample_rate % OutputCapabilities[i].nSampleRate) {

            // Add this output capability to the OutputCaps list
            OutputCaps[m_CapsNum] = OutputCapabilities[i];
            m_CapsNum++;

            // Don't overrun the hard-coded capabilities array limit
            if (m_CapsNum > (int)MAX_IAMSTREAMCONFIG_CAPS) break;
        }
    }
}


//
// Read persistent configuration from Registry
//
void CMpegAudEnc::ReadPresetSettings(MPEG_ENCODER_CONFIG * pmec)
{
    DbgLog((LOG_TRACE,1,TEXT("CMpegAudEnc::ReadPresetSettings()")));

    Lame::CRegKey rk(HKEY_CURRENT_USER, KEY_LAME_ENCODER);

    pmec->dwBitrate         = rk.getDWORD(VALUE_BITRATE,DEFAULT_BITRATE);
    pmec->dwVariableMin     = rk.getDWORD(VALUE_VARIABLEMIN,DEFAULT_VARIABLEMIN);
    pmec->dwVariableMax     = rk.getDWORD(VALUE_VARIABLEMAX,DEFAULT_VARIABLEMAX);
    pmec->vmVariable        = rk.getDWORD(VALUE_VARIABLE, DEFAULT_VARIABLE) ? vbr_rh : vbr_off;
    pmec->dwQuality         = rk.getDWORD(VALUE_QUALITY,DEFAULT_ENCODING_QUALITY);
    pmec->dwVBRq            = rk.getDWORD(VALUE_VBR_QUALITY,DEFAULT_VBR_QUALITY);
    pmec->lLayer            = rk.getDWORD(VALUE_LAYER, DEFAULT_LAYER);
    pmec->bCRCProtect       = rk.getDWORD(VALUE_CRC, DEFAULT_CRC);
    pmec->bForceMono        = rk.getDWORD(VALUE_FORCE_MONO, DEFAULT_FORCE_MONO);
    pmec->bSetDuration      = rk.getDWORD(VALUE_SET_DURATION, DEFAULT_SET_DURATION);
    pmec->bSampleOverlap    = rk.getDWORD(VALUE_SAMPLE_OVERLAP, DEFAULT_SAMPLE_OVERLAP);
    pmec->bCopyright        = rk.getDWORD(VALUE_COPYRIGHT, DEFAULT_COPYRIGHT);
    pmec->bOriginal         = rk.getDWORD(VALUE_ORIGINAL, DEFAULT_ORIGINAL);
    pmec->dwSampleRate      = rk.getDWORD(VALUE_SAMPLE_RATE, DEFAULT_SAMPLE_RATE);
    pmec->dwPES             = rk.getDWORD(VALUE_PES, DEFAULT_PES);

    pmec->ChMode            = (MPEG_mode)rk.getDWORD(VALUE_STEREO_MODE, DEFAULT_STEREO_MODE);
    pmec->dwForceMS         = rk.getDWORD(VALUE_FORCE_MS, DEFAULT_FORCE_MS);

    pmec->dwEnforceVBRmin   = rk.getDWORD(VALUE_ENFORCE_MIN, DEFAULT_ENFORCE_MIN);
    pmec->dwVoiceMode       = rk.getDWORD(VALUE_VOICE, DEFAULT_VOICE);
    pmec->dwKeepAllFreq     = rk.getDWORD(VALUE_KEEP_ALL_FREQ, DEFAULT_KEEP_ALL_FREQ);
    pmec->dwStrictISO       = rk.getDWORD(VALUE_STRICT_ISO, DEFAULT_STRICT_ISO);
    pmec->dwNoShortBlock    = rk.getDWORD(VALUE_DISABLE_SHORT_BLOCK, DEFAULT_DISABLE_SHORT_BLOCK);
    pmec->dwXingTag         = rk.getDWORD(VALUE_XING_TAG, DEFAULT_XING_TAG);
    pmec->dwModeFixed       = rk.getDWORD(VALUE_MODE_FIXED, DEFAULT_MODE_FIXED);

    rk.Close();
}

////////////////////////////////////////////////////////////////
//  Property page handling 
////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::GetPages(CAUUID *pcauuid) 
{
    GUID *pguid;

    pcauuid->cElems = 3;
    pcauuid->pElems = pguid = (GUID *) CoTaskMemAlloc(sizeof(GUID) * pcauuid->cElems);

    if (pcauuid->pElems == NULL)
        return E_OUTOFMEMORY;

    pguid[0] = CLSID_LAMEDShow_PropertyPage;
    pguid[1] = CLSID_LAMEDShow_PropertyPageAdv;
    pguid[2] = CLSID_LAMEDShow_About;

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::NonDelegatingQueryInterface(REFIID riid, void ** ppv) 
{

    if (riid == IID_ISpecifyPropertyPages)
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
	else if(riid == IID_IPersistStream)
        return GetInterface((IPersistStream *)this, ppv);
//    else if (riid == IID_IVAudioEncSettings)
//        return GetInterface((IVAudioEncSettings*) this, ppv);
    else if (riid == IID_IAudioEncoderProperties)
        return GetInterface((IAudioEncoderProperties*) this, ppv);

    return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

////////////////////////////////////////////////////////////////
//IVAudioEncSettings interface methods
////////////////////////////////////////////////////////////////

//
// IAudioEncoderProperties
//
STDMETHODIMP CMpegAudEnc::get_PESOutputEnabled(DWORD *dwEnabled)
{
    *dwEnabled = (DWORD)m_Encoder.IsPES();
    DbgLog((LOG_TRACE, 1, TEXT("get_PESOutputEnabled -> %d"), *dwEnabled));

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_PESOutputEnabled(DWORD dwEnabled)
{
    m_Encoder.SetPES((BOOL)!!dwEnabled);
    DbgLog((LOG_TRACE, 1, TEXT("set_PESOutputEnabled(%d)"), !!dwEnabled));

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_MPEGLayer(DWORD *dwLayer)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwLayer = (DWORD)mec.lLayer;

    DbgLog((LOG_TRACE, 1, TEXT("get_MPEGLayer -> %d"), *dwLayer));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_MPEGLayer(DWORD dwLayer)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    if (dwLayer == 2)
        mec.lLayer = 2;
    else if (dwLayer == 1)
        mec.lLayer = 1;
    m_Encoder.SetOutputType(mec);

    DbgLog((LOG_TRACE, 1, TEXT("set_MPEGLayer(%d)"), dwLayer));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_Bitrate(DWORD *dwBitrate)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwBitrate = (DWORD)mec.dwBitrate;
    DbgLog((LOG_TRACE, 1, TEXT("get_Bitrate -> %d"), *dwBitrate));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_Bitrate(DWORD dwBitrate)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwBitrate = dwBitrate;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_Bitrate(%d)"), dwBitrate));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_Variable(DWORD *dwVariable)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwVariable = (DWORD)(mec.vmVariable == vbr_off ? 0 : 1);
    DbgLog((LOG_TRACE, 1, TEXT("get_Variable -> %d"), *dwVariable));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_Variable(DWORD dwVariable)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);

    mec.vmVariable = dwVariable ? vbr_rh : vbr_off;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_Variable(%d)"), dwVariable));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_VariableMin(DWORD *dwMin)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwMin = (DWORD)mec.dwVariableMin;
    DbgLog((LOG_TRACE, 1, TEXT("get_Variablemin -> %d"), *dwMin));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_VariableMin(DWORD dwMin)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwVariableMin = dwMin;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_Variablemin(%d)"), dwMin));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_VariableMax(DWORD *dwMax)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwMax = (DWORD)mec.dwVariableMax;
    DbgLog((LOG_TRACE, 1, TEXT("get_Variablemax -> %d"), *dwMax));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_VariableMax(DWORD dwMax)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwVariableMax = dwMax;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_Variablemax(%d)"), dwMax));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_Quality(DWORD *dwQuality)             
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwQuality=(DWORD)mec.dwQuality;
    DbgLog((LOG_TRACE, 1, TEXT("get_Quality -> %d"), *dwQuality));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_Quality(DWORD dwQuality)             
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwQuality = dwQuality;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_Quality(%d)"), dwQuality));
    return S_OK;
}
STDMETHODIMP CMpegAudEnc::get_VariableQ(DWORD *dwVBRq)             
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwVBRq=(DWORD)mec.dwVBRq;
    DbgLog((LOG_TRACE, 1, TEXT("get_VariableQ -> %d"), *dwVBRq));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_VariableQ(DWORD dwVBRq)             
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwVBRq = dwVBRq;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_VariableQ(%d)"), dwVBRq));
    return S_OK;
}


STDMETHODIMP CMpegAudEnc::get_SourceSampleRate(DWORD *dwSampleRate)
{
    *dwSampleRate = 0;

    WAVEFORMATEX wf;
    if(FAILED(m_Encoder.GetInputType(&wf)))
        return E_FAIL;

    *dwSampleRate = wf.nSamplesPerSec;
    DbgLog((LOG_TRACE, 1, TEXT("get_SourceSampleRate -> %d"), *dwSampleRate));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_SourceChannels(DWORD *dwChannels)
{
    WAVEFORMATEX wf;
    if(FAILED(m_Encoder.GetInputType(&wf)))
        return E_FAIL;

    *dwChannels = wf.nChannels;
    DbgLog((LOG_TRACE, 1, TEXT("get_SourceChannels -> %d"), *dwChannels));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_SampleRate(DWORD *dwSampleRate)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwSampleRate = mec.dwSampleRate;
    DbgLog((LOG_TRACE, 1, TEXT("get_SampleRate -> %d"), *dwSampleRate));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_SampleRate(DWORD dwSampleRate)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    DWORD dwOldSampleRate = mec.dwSampleRate;
    mec.dwSampleRate = dwSampleRate;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_SampleRate(%d)"), dwSampleRate));

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_ChannelMode(DWORD *dwChannelMode)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwChannelMode = mec.ChMode;
    DbgLog((LOG_TRACE, 1, TEXT("get_ChannelMode -> %d"), *dwChannelMode));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_ChannelMode(DWORD dwChannelMode)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.ChMode = (MPEG_mode)dwChannelMode;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_ChannelMode(%d)"), dwChannelMode));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_ForceMS(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.dwForceMS;
    DbgLog((LOG_TRACE, 1, TEXT("get_ForceMS -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_ForceMS(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwForceMS = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_ForceMS(%d)"), dwFlag));
    return S_OK;
}


STDMETHODIMP CMpegAudEnc::get_CRCFlag(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bCRCProtect;
    DbgLog((LOG_TRACE, 1, TEXT("get_CRCFlag -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_ForceMono(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bForceMono;
    DbgLog((LOG_TRACE, 1, TEXT("get_ForceMono -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_SetDuration(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bSetDuration;
    DbgLog((LOG_TRACE, 1, TEXT("get_SetDuration -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_SampleOverlap(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bSampleOverlap;
    DbgLog((LOG_TRACE, 1, TEXT("get_SampleOverlap -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_CRCFlag(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bCRCProtect = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_CRCFlag(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_ForceMono(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bForceMono = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_ForceMono(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_SetDuration(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bSetDuration = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_SetDuration(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_SampleOverlap(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bSampleOverlap = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_SampleOverlap(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_EnforceVBRmin(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.dwEnforceVBRmin;
    DbgLog((LOG_TRACE, 1, TEXT("get_EnforceVBRmin -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_EnforceVBRmin(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwEnforceVBRmin = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_EnforceVBRmin(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_VoiceMode(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.dwVoiceMode;
    DbgLog((LOG_TRACE, 1, TEXT("get_VoiceMode -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_VoiceMode(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwVoiceMode = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_VoiceMode(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_KeepAllFreq(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.dwKeepAllFreq;
    DbgLog((LOG_TRACE, 1, TEXT("get_KeepAllFreq -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_KeepAllFreq(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwKeepAllFreq = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_KeepAllFreq(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_StrictISO(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.dwStrictISO;
    DbgLog((LOG_TRACE, 1, TEXT("get_StrictISO -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_StrictISO(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwStrictISO = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_StrictISO(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_NoShortBlock(DWORD *dwNoShortBlock)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwNoShortBlock = mec.dwNoShortBlock;
    DbgLog((LOG_TRACE, 1, TEXT("get_NoShortBlock -> %d"), *dwNoShortBlock));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_NoShortBlock(DWORD dwNoShortBlock)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwNoShortBlock = dwNoShortBlock;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_NoShortBlock(%d)"), dwNoShortBlock));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_XingTag(DWORD *dwXingTag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwXingTag = mec.dwXingTag;
    DbgLog((LOG_TRACE, 1, TEXT("get_XingTag -> %d"), *dwXingTag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_XingTag(DWORD dwXingTag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwXingTag = dwXingTag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_XingTag(%d)"), dwXingTag));
    return S_OK;
}



STDMETHODIMP CMpegAudEnc::get_OriginalFlag(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bOriginal;
    DbgLog((LOG_TRACE, 1, TEXT("get_OriginalFlag -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_OriginalFlag(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bOriginal = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_OriginalFlag(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_CopyrightFlag(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bCopyright;
    DbgLog((LOG_TRACE, 1, TEXT("get_CopyrightFlag -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_CopyrightFlag(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bCopyright = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_CopyrightFlag(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_ModeFixed(DWORD *dwModeFixed)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwModeFixed = mec.dwModeFixed;
    DbgLog((LOG_TRACE, 1, TEXT("get_ModeFixed -> %d"), *dwModeFixed));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_ModeFixed(DWORD dwModeFixed)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwModeFixed = dwModeFixed;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_ModeFixed(%d)"), dwModeFixed));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_ParameterBlockSize(BYTE *pcBlock, DWORD *pdwSize)
{
    DbgLog((LOG_TRACE, 1, TEXT("get_ParameterBlockSize -> %d%d"), *pcBlock, *pdwSize));

    if (pcBlock != NULL) {
        if (*pdwSize >= sizeof(MPEG_ENCODER_CONFIG)) {
            m_Encoder.GetOutputType((MPEG_ENCODER_CONFIG*)pcBlock);
            return S_OK;
        }
        else {
            *pdwSize = sizeof(MPEG_ENCODER_CONFIG);
            return E_FAIL;
        }
    }
    else if (pdwSize != NULL) {
        *pdwSize = sizeof(MPEG_ENCODER_CONFIG);
        return S_OK;
    }

    return E_FAIL;
}

STDMETHODIMP CMpegAudEnc::set_ParameterBlockSize(BYTE *pcBlock, DWORD dwSize)
{
    DbgLog((LOG_TRACE, 1, TEXT("get_ParameterBlockSize(%d, %d)"), *pcBlock, dwSize));
    if (sizeof(MPEG_ENCODER_CONFIG) == dwSize){
        m_Encoder.SetOutputType(*(MPEG_ENCODER_CONFIG*)pcBlock);
        return S_OK;
    }
    else return E_FAIL; 
}


STDMETHODIMP CMpegAudEnc::DefaultAudioEncoderProperties()
{
    DbgLog((LOG_TRACE, 1, TEXT("DefaultAudioEncoderProperties()")));

    HRESULT hr = InputTypeDefined();
    if (FAILED(hr))
        return hr;

    DWORD dwSourceSampleRate;
    get_SourceSampleRate(&dwSourceSampleRate);

    set_PESOutputEnabled(DEFAULT_PES);
    set_MPEGLayer(DEFAULT_LAYER);

    set_Bitrate(DEFAULT_BITRATE);
    set_Variable(FALSE);
    set_VariableMin(DEFAULT_VARIABLEMIN);
    set_VariableMax(DEFAULT_VARIABLEMAX);
    set_Quality(DEFAULT_ENCODING_QUALITY);
    set_VariableQ(DEFAULT_VBR_QUALITY);

    set_SampleRate(dwSourceSampleRate);
    set_CRCFlag(DEFAULT_CRC);
    set_ForceMono(DEFAULT_FORCE_MONO);
    set_SetDuration(DEFAULT_SET_DURATION);
    set_SampleOverlap(DEFAULT_SAMPLE_OVERLAP);
    set_OriginalFlag(DEFAULT_ORIGINAL);
    set_CopyrightFlag(DEFAULT_COPYRIGHT);

    set_EnforceVBRmin(DEFAULT_ENFORCE_MIN);
    set_VoiceMode(DEFAULT_VOICE);
    set_KeepAllFreq(DEFAULT_KEEP_ALL_FREQ);
    set_StrictISO(DEFAULT_STRICT_ISO);
    set_NoShortBlock(DEFAULT_DISABLE_SHORT_BLOCK);
    set_XingTag(DEFAULT_XING_TAG);
    set_ForceMS(DEFAULT_FORCE_MS);
    set_ChannelMode(DEFAULT_STEREO_MODE);
    set_ModeFixed(DEFAULT_MODE_FIXED);

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::LoadAudioEncoderPropertiesFromRegistry()
{
    DbgLog((LOG_TRACE, 1, TEXT("LoadAudioEncoderPropertiesFromRegistry()")));

    MPEG_ENCODER_CONFIG mec;
    ReadPresetSettings(&mec);
    if(m_Encoder.SetOutputType(mec) == S_FALSE)
        return S_FALSE;
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::SaveAudioEncoderPropertiesToRegistry()
{
    DbgLog((LOG_TRACE, 1, TEXT("SaveAudioEncoderPropertiesToRegistry()")));
    Lame::CRegKey rk;

    MPEG_ENCODER_CONFIG mec;
    if(m_Encoder.GetOutputType(&mec) == S_FALSE)
        return E_FAIL;

    if(rk.Create(HKEY_CURRENT_USER, KEY_LAME_ENCODER))
    {
        rk.setDWORD(VALUE_BITRATE, mec.dwBitrate);
        rk.setDWORD(VALUE_VARIABLE, mec.vmVariable);
        rk.setDWORD(VALUE_VARIABLEMIN, mec.dwVariableMin);
        rk.setDWORD(VALUE_VARIABLEMAX, mec.dwVariableMax);
        rk.setDWORD(VALUE_QUALITY, mec.dwQuality);
        rk.setDWORD(VALUE_VBR_QUALITY, mec.dwVBRq);

        rk.setDWORD(VALUE_CRC, mec.bCRCProtect);
        rk.setDWORD(VALUE_FORCE_MONO, mec.bForceMono);
        rk.setDWORD(VALUE_SET_DURATION, mec.bSetDuration);
        rk.setDWORD(VALUE_SAMPLE_OVERLAP, mec.bSampleOverlap);
        rk.setDWORD(VALUE_PES, mec.dwPES);
        rk.setDWORD(VALUE_COPYRIGHT, mec.bCopyright);
        rk.setDWORD(VALUE_ORIGINAL, mec.bOriginal);
        rk.setDWORD(VALUE_SAMPLE_RATE, mec.dwSampleRate);

        rk.setDWORD(VALUE_STEREO_MODE, mec.ChMode);
        rk.setDWORD(VALUE_FORCE_MS, mec.dwForceMS);
        rk.setDWORD(VALUE_XING_TAG, mec.dwXingTag);
        rk.setDWORD(VALUE_DISABLE_SHORT_BLOCK, mec.dwNoShortBlock);
        rk.setDWORD(VALUE_STRICT_ISO, mec.dwStrictISO);
        rk.setDWORD(VALUE_KEEP_ALL_FREQ, mec.dwKeepAllFreq);
        rk.setDWORD(VALUE_VOICE, mec.dwVoiceMode);
        rk.setDWORD(VALUE_ENFORCE_MIN, mec.dwEnforceVBRmin);
        rk.setDWORD(VALUE_MODE_FIXED, mec.dwModeFixed);

        rk.Close();
    }

    // Reconnect filter graph
    Reconnect();

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::InputTypeDefined()
{
    WAVEFORMATEX wf;
    if(FAILED(m_Encoder.GetInputType(&wf)))
    {
        DbgLog((LOG_TRACE, 1, TEXT("!InputTypeDefined()")));
        return E_FAIL;
    }

    DbgLog((LOG_TRACE, 1, TEXT("InputTypeDefined()")));
    return S_OK;
}


STDMETHODIMP CMpegAudEnc::ApplyChanges()
{
    return Reconnect();
}

//
// CPersistStream stuff
//

// what is our class ID?
STDMETHODIMP CMpegAudEnc::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_LAMEDShowFilter;
    return S_OK;
}

HRESULT CMpegAudEnc::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("WriteToStream()")));

	MPEG_ENCODER_CONFIG mec;

	if(m_Encoder.GetOutputType(&mec) == S_FALSE)
		return E_FAIL;

    return pStream->Write(&mec, sizeof(mec), 0);
}


// what device should we use?  Used to re-create a .GRF file that we
// are in
HRESULT CMpegAudEnc::ReadFromStream(IStream *pStream)
{
	MPEG_ENCODER_CONFIG mec;

    HRESULT hr = pStream->Read(&mec, sizeof(mec), 0);
    if(FAILED(hr))
        return hr;

	if(m_Encoder.SetOutputType(mec) == S_FALSE)
		return S_FALSE;

    DbgLog((LOG_TRACE,1,TEXT("ReadFromStream() succeeded")));

    hr = S_OK;
    return hr;
}


// How long is our data?
int CMpegAudEnc::SizeMax()
{
    return sizeof(MPEG_ENCODER_CONFIG);
}





//////////////////////////////////////////////////////////////////////////
// CMpegAudEncOutPin is the one and only output pin of CMpegAudEnc  
// 
//////////////////////////////////////////////////////////////////////////
CMpegAudEncOutPin::CMpegAudEncOutPin( CMpegAudEnc * pFilter, HRESULT * pHr ) :
        CTransformOutputPin( NAME("LameEncoderOutputPin"), pFilter, pHr, L"Output\0" ),
        m_pFilter(pFilter)
{
    m_SetFormat = FALSE;
}

CMpegAudEncOutPin::~CMpegAudEncOutPin()
{
} 

STDMETHODIMP CMpegAudEncOutPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if(riid == IID_IAMStreamConfig) {
        CheckPointer(ppv, E_POINTER);
        return GetInterface((IAMStreamConfig*)(this), ppv);
    }
    return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}


//////////////////////////////////////////////////////////////////////////
// This is called after the output format has been negotiated and
// will update the LAME encoder settings so that it matches the
// settings specified in the MediaType structure.
//////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEncOutPin::SetMediaType(const CMediaType *pmt)
{
    // Retrieve the current LAME encoder configuration
    MPEG_ENCODER_CONFIG mec;
    m_pFilter->m_Encoder.GetOutputType(&mec);

    // Annotate if we are using the MEDIATYPE_Stream output type
    m_pFilter->m_bStreamOutput = (pmt->majortype == MEDIATYPE_Stream);

    if (pmt->majortype == MEDIATYPE_Stream) {
        // Update the encoder configuration using the settings that were
        // cached in the CMpegAudEncOutPin::GetMediaType() call
        mec.dwSampleRate = m_CurrentOutputFormat.nSampleRate;
        mec.dwBitrate = m_CurrentOutputFormat.nBitRate;
        mec.ChMode = m_CurrentOutputFormat.ChMode;
    }
    else {
        // Update the encoder configuration directly using the values
        // passed via the CMediaType structure.  
        MPEGLAYER3WAVEFORMAT *pfmt = (MPEGLAYER3WAVEFORMAT*) pmt->Format();
        mec.dwSampleRate = pfmt->wfx.nSamplesPerSec;
        mec.dwBitrate = pfmt->wfx.nAvgBytesPerSec * 8 / 1000;

        if (pfmt->wfx.nChannels == 1) { mec.ChMode = MONO; }
        else if (pfmt->wfx.nChannels == 2 && mec.ChMode == MONO && !mec.bForceMono) { mec.ChMode = STEREO; }
    }
    m_pFilter->m_Encoder.SetOutputType(mec);

    // Now configure this MediaType on the output pin
    HRESULT hr = CTransformOutputPin::SetMediaType(pmt);
    return hr;
}


//////////////////////////////////////////////////////////////////////////
// Retrieve the various MediaTypes that match the advertised formats
// supported on the output pin and configure an AM_MEDIA_TYPE output 
// structure that is based on the selected format.
//////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEncOutPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    if (iPosition < 0) return E_INVALIDARG;

    // If iPosition equals zero then we always return the currently configured MediaType 
    if (iPosition == 0) {
        *pmt = m_mt;
        return S_OK;
    }

    switch (iPosition)
    {
        case 1:
        {
            pmt->SetType(&MEDIATYPE_Audio);
            pmt->SetSubtype(&MEDIASUBTYPE_MP3);
            break;
        }
        case 2:
        {
            pmt->SetType(&MEDIATYPE_Stream);
            pmt->SetSubtype(&MEDIASUBTYPE_MPEG1Audio);
            pmt->SetFormatType(&GUID_NULL);
            break;
        }
        case 3:
        {   // The last case that we evaluate is the MPEG2_PES format, but if the 
            // encoder isn't configured for it then just return VFW_S_NO_MORE_ITEMS
            if ( !m_pFilter->m_Encoder.IsPES() ) { return VFW_S_NO_MORE_ITEMS; }

            pmt->SetType(&MEDIATYPE_MPEG2_PES);
            pmt->SetSubtype(&MEDIASUBTYPE_MPEG2_AUDIO);
            break;
        }
        default:
            return VFW_S_NO_MORE_ITEMS;
    }


    // Output capabilities are dependent on the input so insure it is connected
    if ( !m_pFilter->m_pInput->IsConnected() ) {
        pmt->SetFormatType(&FORMAT_None);
        return NOERROR;
    }


    // Annotate the current MediaType index for recall in CMpegAudEnc::Reconnect()
    m_pFilter->m_currentMediaTypeIndex = iPosition;

    // Configure the remaining AM_MEDIA_TYPE parameters using the cached encoder settings.
    // Since MEDIATYPE_Stream doesn't have a format block the current settings 
    // for CHANNEL MODE, BITRATE and SAMPLERATE are cached in m_CurrentOutputFormat for use
    // when we setup the LAME encoder in the call to CMpegAudEncOutPin::SetMediaType()
    MPEG_ENCODER_CONFIG mec;
    m_pFilter->m_Encoder.GetOutputType(&mec);           // Retrieve the current encoder config

    WAVEFORMATEX wf;                                    // Retrieve the input configuration
    m_pFilter->m_Encoder.GetInputType(&wf);

    // Use the current encoder sample rate unless it isn't a modulus of the input rate
    if ((wf.nSamplesPerSec % mec.dwSampleRate) == 0) { 
        m_CurrentOutputFormat.nSampleRate = mec.dwSampleRate;
    }
    else {
        m_CurrentOutputFormat.nSampleRate = wf.nSamplesPerSec;
    }

    // Select the output channel config based on the encoder config and input channel count
    m_CurrentOutputFormat.ChMode = mec.ChMode;
    switch (wf.nChannels)                    // Determine if we need to alter ChMode based upon the channel count and ForceMono flag 
    {
        case 1:
        {
            m_CurrentOutputFormat.ChMode = MONO;
            break;
        }
        case 2:
        {
            if (mec.ChMode == MONO && !mec.bForceMono) { m_CurrentOutputFormat.ChMode = STEREO; }
            else if ( mec.bForceMono ) { m_CurrentOutputFormat.ChMode = MONO; }
            break;
        }
    }

    // Select the encoder bit rate. In VBR mode we set the data rate parameter
    // of the WAVE_FORMAT_MPEGLAYER3 structure to the minimum VBR value
    m_CurrentOutputFormat.nBitRate = (mec.vmVariable == vbr_off) ? mec.dwBitrate : mec.dwVariableMin;

    if (pmt->majortype == MEDIATYPE_Stream) return NOERROR;     // No further config required for MEDIATYPE_Stream


    // Now configure the remainder of the WAVE_FORMAT_MPEGLAYER3 format block
    // and its parent AM_MEDIA_TYPE structure
    DECLARE_PTR(MPEGLAYER3WAVEFORMAT, p_mp3wvfmt, pmt->AllocFormatBuffer(sizeof(MPEGLAYER3WAVEFORMAT)));
    ZeroMemory(p_mp3wvfmt, sizeof(MPEGLAYER3WAVEFORMAT));

    p_mp3wvfmt->wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
    p_mp3wvfmt->wfx.nChannels = (m_CurrentOutputFormat.ChMode == MONO) ? 1 : 2;
    p_mp3wvfmt->wfx.nSamplesPerSec = m_CurrentOutputFormat.nSampleRate;
    p_mp3wvfmt->wfx.nAvgBytesPerSec = GET_DATARATE(m_CurrentOutputFormat.nBitRate);
    p_mp3wvfmt->wfx.nBlockAlign = 1;
    p_mp3wvfmt->wfx.wBitsPerSample = 0;
    p_mp3wvfmt->wfx.cbSize = sizeof(MPEGLAYER3WAVEFORMAT) - sizeof(WAVEFORMATEX);

    p_mp3wvfmt->wID = MPEGLAYER3_ID_MPEG;
    p_mp3wvfmt->fdwFlags = MPEGLAYER3_FLAG_PADDING_ISO;
    p_mp3wvfmt->nBlockSize = GET_FRAMELENGTH(m_CurrentOutputFormat.nBitRate, p_mp3wvfmt->wfx.nSamplesPerSec);
    p_mp3wvfmt->nFramesPerBlock = 1;
    p_mp3wvfmt->nCodecDelay = 0;

    pmt->SetTemporalCompression(FALSE);
    pmt->SetSampleSize(OUT_BUFFER_SIZE);
    pmt->SetFormat((LPBYTE)p_mp3wvfmt, sizeof(MPEGLAYER3WAVEFORMAT));
    pmt->SetFormatType(&FORMAT_WaveFormatEx);

    return NOERROR;
}


//////////////////////////////////////////////////////////////////////////
// This method is called to see if a given output format is supported
//////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEncOutPin::CheckMediaType(const CMediaType *pmtOut)
{
    // Fail if the input pin is not connected.
    if (!m_pFilter->m_pInput->IsConnected()) {
        return VFW_E_NOT_CONNECTED;
    }

    // Reject any media types that we know in advance our 
    // filter cannot use.
    if (pmtOut->majortype != MEDIATYPE_Audio && pmtOut->majortype != MEDIATYPE_Stream) { return S_FALSE; }

    // If SetFormat was previously called, check whether pmtOut exactly 
    // matches the format that was specified in SetFormat.
    // Return S_OK if they match, or VFW_E_INVALIDMEDIATYPE otherwise.)
    if ( m_SetFormat ) {
        if (*pmtOut != m_mt) { return VFW_E_INVALIDMEDIATYPE; }
        else { return S_OK; }
    }

    // Now do the normal check for this media type.
    HRESULT hr;
    hr = m_pFilter->CheckTransform (&m_pFilter->m_pInput->CurrentMediaType(),  // The input type.
                                    pmtOut);                                   // The proposed output type.

    if (hr == S_OK) {
        return S_OK;           // This format is compatible with the current input type.
    }
 
    // This format is not compatible with the current input type. 
    // Maybe we can reconnect the input pin with a new input type.
    
    // Enumerate the upstream filter's preferred output types, and 
    // see if one of them will work.
    CMediaType *pmtEnum;
    BOOL fFound = FALSE;
    IEnumMediaTypes *pEnum;
    hr = m_pFilter->m_pInput->GetConnected()->EnumMediaTypes(&pEnum);
    if (hr != S_OK) {
        return E_FAIL;
    }

    while (hr = pEnum->Next(1, (AM_MEDIA_TYPE **)&pmtEnum, NULL), hr == S_OK)
    {
        // Check this input type against the proposed output type.
        hr = m_pFilter->CheckTransform(pmtEnum, pmtOut);
        if (hr != S_OK) {
            DeleteMediaType(pmtEnum);
            continue; // Try the next one.
        }

        // This input type is a possible candidate. But, we have to make
        // sure that the upstream filter can switch to this type. 
        hr = m_pFilter->m_pInput->GetConnected()->QueryAccept(pmtEnum);
        if (hr != S_OK) {
            // The upstream filter will not switch to this type.
            DeleteMediaType(pmtEnum);
            continue; // Try the next one.
        }
        fFound = TRUE;
        DeleteMediaType(pmtEnum);
        break;
    }
    pEnum->Release();

    if (fFound) {
        // This output type is OK, but if we are asked to use it, we will
        // need to reconnect our input pin. (See SetFormat, below.)
        return S_OK;
    }
    else {
        return VFW_E_INVALIDMEDIATYPE;
    }
}



//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CMpegAudEncOutPin::SetFormat(AM_MEDIA_TYPE *pmt)
{
    CheckPointer(pmt, E_POINTER);
    HRESULT hr;

    // Hold the filter state lock, to make sure that streaming isn't 
    // in the middle of starting or stopping:
    CAutoLock cObjectLock(&m_pFilter->m_csFilter);

    // Cannot set the format unless the filter is stopped.
    if (m_pFilter->m_State != State_Stopped) {
        return VFW_E_NOT_STOPPED;
    }

    // The set of possible output formats depends on the input format,
    // so if the input pin is not connected, return a failure code.
    if (!m_pFilter->m_pInput->IsConnected()) {
        return VFW_E_NOT_CONNECTED;
    }

    // If the pin is already using this format, there's nothing to do.
    if (IsConnected() && CurrentMediaType() == *pmt) {
        if ( m_SetFormat ) return S_OK;
    }

    // See if this media type is acceptable.
    if ((hr = CheckMediaType((CMediaType *)pmt)) != S_OK) {
        return hr;
    }

    // If we're connected to a downstream filter, we have to make
    // sure that the downstream filter accepts this media type.
    if (IsConnected()) {
        hr = GetConnected()->QueryAccept(pmt);
        if (hr != S_OK) {
            return VFW_E_INVALIDMEDIATYPE;
        }
    }

    // Now make a note that from now on, this is the only format allowed,
    // and refuse anything but this in the CheckMediaType() code above.
    m_SetFormat = TRUE;
    m_mt = *pmt;

    // Changing the format means reconnecting if necessary.
    if (IsConnected()) {
        m_pFilter->m_pGraph->Reconnect(this);
    }

    return NOERROR;
}

HRESULT STDMETHODCALLTYPE CMpegAudEncOutPin::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    *ppmt = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMpegAudEncOutPin::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    // The set of possible output formats depends on the input format,
    // so if the input pin is not connected, return a failure code.
    if (!m_pFilter->m_pInput->IsConnected()) {
        return VFW_E_NOT_CONNECTED;
    }

    // Retrieve the current encoder configuration
    MPEG_ENCODER_CONFIG mec;
    m_pFilter->m_Encoder.GetOutputType(&mec);

    // If the encoder is in VBR mode GetStreamCaps() isn't implemented
    if (mec.vmVariable != vbr_off) { *piCount = 0; }
    else { *piCount = m_pFilter->m_CapsNum; }

    *piSize = sizeof(AUDIO_STREAM_CONFIG_CAPS);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMpegAudEncOutPin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
    // The set of possible output formats depends on the input format,
    // so if the input pin is not connected, return a failure code.
    if (!m_pFilter->m_pInput->IsConnected()) {
        return VFW_E_NOT_CONNECTED;
    }

    // If we don't have a capabilities array GetStreamCaps() isn't implemented
    if (m_pFilter->m_CapsNum == 0) return E_NOTIMPL;

    // If the encoder is in VBR mode GetStreamCaps() isn't implemented
    MPEG_ENCODER_CONFIG mec;
    m_pFilter->m_Encoder.GetOutputType(&mec);
    if (mec.vmVariable != vbr_off) return E_NOTIMPL;

    if (iIndex < 0) return E_INVALIDARG;
    if (iIndex > m_pFilter->m_CapsNum) return S_FALSE;

    // Load the MPEG Layer3 WaveFormatEx structure with the appropriate entries 
    // for this IAMStreamConfig index element.
    *pmt = CreateMediaType(&m_mt);
    if (*pmt == NULL) return E_OUTOFMEMORY;

    DECLARE_PTR(MPEGLAYER3WAVEFORMAT, p_mp3wvfmt, (*pmt)->pbFormat);

    (*pmt)->majortype = MEDIATYPE_Audio;
    (*pmt)->subtype = MEDIASUBTYPE_MP3;
    (*pmt)->bFixedSizeSamples = TRUE;
    (*pmt)->bTemporalCompression = FALSE;
    (*pmt)->lSampleSize = OUT_BUFFER_SIZE;
    (*pmt)->formattype = FORMAT_WaveFormatEx;
    (*pmt)->cbFormat = sizeof(MPEGLAYER3WAVEFORMAT);

    p_mp3wvfmt->wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
    p_mp3wvfmt->wfx.nChannels = 2;
    p_mp3wvfmt->wfx.nSamplesPerSec = m_pFilter->OutputCaps[iIndex].nSampleRate;
    p_mp3wvfmt->wfx.nAvgBytesPerSec = GET_DATARATE(m_pFilter->OutputCaps[iIndex].nBitRate);
    p_mp3wvfmt->wfx.nBlockAlign = 1;
    p_mp3wvfmt->wfx.wBitsPerSample = 0;
    p_mp3wvfmt->wfx.cbSize = sizeof(MPEGLAYER3WAVEFORMAT) - sizeof(WAVEFORMATEX);

    p_mp3wvfmt->wID = MPEGLAYER3_ID_MPEG;
    p_mp3wvfmt->fdwFlags = MPEGLAYER3_FLAG_PADDING_ISO;
    p_mp3wvfmt->nBlockSize = GET_FRAMELENGTH(m_pFilter->OutputCaps[iIndex].nBitRate, m_pFilter->OutputCaps[iIndex].nSampleRate);
    p_mp3wvfmt->nFramesPerBlock = 1;
    p_mp3wvfmt->nCodecDelay = 0;

    // Set up the companion AUDIO_STREAM_CONFIG_CAPS structure
    // We are only using the CHANNELS element of the structure
    DECLARE_PTR(AUDIO_STREAM_CONFIG_CAPS, pascc, pSCC);

    ZeroMemory(pascc, sizeof(AUDIO_STREAM_CONFIG_CAPS));
    pascc->guid = MEDIATYPE_Audio;

    pascc->MinimumChannels = 1;
    pascc->MaximumChannels = 2;
    pascc->ChannelsGranularity = 1;

    pascc->MinimumSampleFrequency = p_mp3wvfmt->wfx.nSamplesPerSec;
    pascc->MaximumSampleFrequency = p_mp3wvfmt->wfx.nSamplesPerSec;
    pascc->SampleFrequencyGranularity = 0;

    pascc->MinimumBitsPerSample = p_mp3wvfmt->wfx.wBitsPerSample;
    pascc->MaximumBitsPerSample = p_mp3wvfmt->wfx.wBitsPerSample;
    pascc->BitsPerSampleGranularity = 0;

    return S_OK;
}

