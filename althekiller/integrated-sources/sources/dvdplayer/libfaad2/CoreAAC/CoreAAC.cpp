/* 
 * CoreAAC - AAC DirectShow Decoder Filter
 *
 * Modification to decode AAC without ADTS and multichannel support
 * (c) 2003 christophe.paris@free.fr
 *
 * Under section 8 of the GNU General Public License, the copyright
 * holders of CoreAAC explicitly forbid distribution in the following
 * countries:
 * - Japan
 * - United States of America 
 *
 *
 * AAC DirectShow Decoder Filter
 * Copyright (C) 2003 Robert Cioch
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */
 

#include <windows.h>
#include <streams.h>
#include <initguid.h>
#include <olectl.h>
#include <transfrm.h>

#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

#include <stdio.h>

#include "AACProfilesName.h"
#include "ICoreAAC.h"
#include "CoreAACGUID.h"
#include "CoreAACInfoProp.h"
#include "CoreAACAboutProp.h"
#include "CoreAAC.h"

// ============================================================================
//  Registration setup stuff

AMOVIESETUP_MEDIATYPE sudInputType[] =
{
	{ &MEDIATYPE_Audio, &MEDIASUBTYPE_AAC }
};

AMOVIESETUP_MEDIATYPE sudOutputType[] =
{
	{ &MEDIATYPE_Audio, &MEDIASUBTYPE_PCM }
};

AMOVIESETUP_PIN sudPins[] =
{
	{ L"Input",
		FALSE,							// bRendered
		FALSE,							// bOutput
		FALSE,							// bZero
		FALSE,							// bMany
		&CLSID_NULL,					// clsConnectsToFilter
		NULL,							// ConnectsToPin
		NUMELMS(sudInputType),			// Number of media types
		sudInputType
	},
	{ L"Output",
		FALSE,							// bRendered
		TRUE,							// bOutput
		FALSE,							// bZero
		FALSE,							// bMany
		&CLSID_NULL,					// clsConnectsToFilter
		NULL,							// ConnectsToPin
		NUMELMS(sudOutputType),			// Number of media types
		sudOutputType
	}
};

AMOVIESETUP_FILTER sudDecoder =
{
	&CLSID_DECODER,
	L"CoreAAC Audio Decoder",
	MERIT_PREFERRED,
	NUMELMS(sudPins),
	sudPins
};

// ============================================================================
// COM Global table of objects in this dll

CFactoryTemplate g_Templates[] = 
{
  { L"CoreAAC Audio Decoder", &CLSID_DECODER, CCoreAACDecoder::CreateInstance, NULL, &sudDecoder },
  { L"CoreAAC Audio Decoder Info", &CLSID_CoreAACInfoProp, CCoreAACInfoProp::CreateInstance, NULL, NULL},
  { L"CoreAAC Audio Decoder About", &CLSID_CoreAACAboutProp, CCoreAACAboutProp::CreateInstance, NULL, NULL},
};

// Count of objects listed in g_cTemplates
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// ============================================================================

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

// ----------------------------------------------------------------------------

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

// ----------------------------------------------------------------------------

// The streams.h DLL entrypoint.
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

// The entrypoint required by the MSVC runtimes. This is used instead
// of DllEntryPoint directly to ensure global C++ classes get initialised.
BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
	
    return DllEntryPoint(reinterpret_cast<HINSTANCE>(hDllHandle), dwReason, lpreserved);
}

// ----------------------------------------------------------------------------

CUnknown *WINAPI CCoreAACDecoder::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	CCoreAACDecoder *pNewObject = new CCoreAACDecoder(punk, phr);
	if (!pNewObject)
		*phr = E_OUTOFMEMORY;
	return pNewObject;
}

// ----------------------------------------------------------------------------

void SaveInt(char* keyname, int value)
{
	HKEY hKey;
	DWORD dwDisp;
	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER,
		"Software\\CoreAAC", 0, "REG_SZ",
		REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp))
	{
		DWORD dwSize = sizeof(DWORD);
		RegSetValueEx(hKey, keyname, 0, REG_DWORD, (CONST BYTE*)&value, dwSize);
		RegCloseKey(hKey);
	}
}

int LoadInt(char* keyname, int default_value)
{
	HKEY hKey;
	int result = default_value;
	
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
		"Software\\CoreAAC", 0, KEY_READ, &hKey))
	{
		DWORD dwTmp = 0;
		DWORD dwcbData = sizeof(DWORD);
		if(RegQueryValueEx(hKey, keyname, NULL, NULL, (LPBYTE) &dwTmp, &dwcbData) == ERROR_SUCCESS)
		{
			result = dwTmp;
		}
		RegCloseKey(hKey);
	}
	return result;
}

// ----------------------------------------------------------------------------

CCoreAACDecoder::CCoreAACDecoder(LPUNKNOWN lpunk, HRESULT *phr) :
	CTransformFilter(NAME("CoreAAC Audio Decoder"), lpunk, CLSID_DECODER),
	m_decHandle(NULL),
	m_decoderSpecificLen(0),
	m_decoderSpecific(NULL),
	m_Channels(0),
	m_SamplesPerSec(0),
	m_BitsPerSample(0),
	m_Bitrate(0),
	m_brCalcFrames(0),
	m_brBytesConsumed(0),
	m_DecodedFrames(0),
	m_OutputBuffLen(0),
	m_DownMatrix(false)
{
	NOTE("CCoreAACDecoder::CCoreAACDecoder");

	m_ProfileName[0] = '\0';
	m_DownMatrix = LoadInt("DownMatrix", TRUE) ? true : false;	
}

// ----------------------------------------------------------------------------

CCoreAACDecoder::~CCoreAACDecoder()
{
	NOTE("CCoreAACDecoder::~CCoreAACDecoder");

	SaveInt("DownMatrix",m_DownMatrix);

	if(m_decHandle)
	{
		faacDecClose(m_decHandle);
		m_decHandle = NULL;
	}
	if(m_decoderSpecific)
	{
		delete m_decoderSpecific;
		m_decoderSpecific = NULL;
	}
}

// ----------------------------------------------------------------------------

STDMETHODIMP CCoreAACDecoder::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if(riid == IID_ICoreAACDec)
		return GetInterface((ICoreAACDec *)this, ppv);	
	else if (riid == IID_ISpecifyPropertyPages)
		return GetInterface((ISpecifyPropertyPages *)this, ppv);
	else
		return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

// ----------------------------------------------------------------------------
// property pages

STDMETHODIMP CCoreAACDecoder::GetPages(CAUUID *pPages)
{
	pPages->cElems = 2;
	pPages->pElems = (GUID *)CoTaskMemAlloc(pPages->cElems * sizeof(GUID));
	if (!pPages->pElems)
		return E_OUTOFMEMORY;

	pPages->pElems[0] = CLSID_CoreAACInfoProp;
	pPages->pElems[1] = CLSID_CoreAACAboutProp;

	return S_OK;
}
 
// ============================================================================
// accept only aac audio wrapped in waveformat

HRESULT CCoreAACDecoder::CheckInputType(const CMediaType *mtIn)
{
	if (*mtIn->Type() != MEDIATYPE_Audio || *mtIn->Subtype() != MEDIASUBTYPE_AAC)
		return VFW_E_TYPE_NOT_ACCEPTED;

	if (*mtIn->FormatType() != FORMAT_WaveFormatEx)
		return VFW_E_TYPE_NOT_ACCEPTED;

	WAVEFORMATEX *wfex = (WAVEFORMATEX *)mtIn->Format();
	if (wfex->wFormatTag != WAVE_FORMAT_AAC)
		return VFW_E_TYPE_NOT_ACCEPTED;

	if(wfex->cbSize < 2)
		return VFW_E_TYPE_NOT_ACCEPTED;

	m_decoderSpecificLen = wfex->cbSize;
	if(m_decoderSpecific)
	{
		delete m_decoderSpecific;
		m_decoderSpecific = NULL;
	}
	m_decoderSpecific = new unsigned char[m_decoderSpecificLen];
	
	// Keep decoderSpecific initialization data (appended to the WAVEFORMATEX struct)
	memcpy(m_decoderSpecific,(char*)wfex+sizeof(WAVEFORMATEX), m_decoderSpecificLen);
	
	return S_OK;
}

// ============================================================================
// propose proper waveformat

HRESULT CCoreAACDecoder::GetMediaType(int iPosition, CMediaType *mtOut)
{
	if (!m_pInput->IsConnected())
	{
		return E_UNEXPECTED;
	}
	
	if (iPosition < 0)
	{
		return E_INVALIDARG;
	}
	
	if (iPosition > 0)
	{
		return VFW_S_NO_MORE_ITEMS;
	}
	
	// Some drivers don't like WAVEFORMATEXTENSIBLE when channels are <= 2 so
	// we fall back to a classic WAVEFORMATEX struct in this case 
	
	WAVEFORMATEXTENSIBLE wfex;
	ZeroMemory(&wfex, sizeof(WAVEFORMATEXTENSIBLE));
	
	wfex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
	wfex.Format.wFormatTag = (m_Channels <= 2) ? WAVE_FORMAT_PCM : WAVE_FORMAT_EXTENSIBLE;
	wfex.Format.cbSize = (m_Channels <= 2) ? 0 : sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	wfex.Format.nChannels = (unsigned short)m_Channels;
	wfex.Format.nSamplesPerSec = (unsigned short)m_SamplesPerSec;
	wfex.Format.wBitsPerSample = m_BitsPerSample;
	wfex.Format.nBlockAlign = (unsigned short)((wfex.Format.nChannels * wfex.Format.wBitsPerSample) / 8);
	wfex.Format.nAvgBytesPerSec = wfex.Format.nSamplesPerSec * wfex.Format.nBlockAlign;
	switch(m_Channels)
	{
	case 1:
		wfex.dwChannelMask = KSAUDIO_SPEAKER_MONO;		
		break;
	case 2:
		wfex.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
		break;
	case 3:
		wfex.dwChannelMask = KSAUDIO_SPEAKER_STEREO | SPEAKER_FRONT_CENTER;
		break;
	case 4:
		//wfex.dwChannelMask = KSAUDIO_SPEAKER_QUAD;
		wfex.dwChannelMask = (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER);
		break;
	case 5:
		wfex.dwChannelMask = KSAUDIO_SPEAKER_QUAD | SPEAKER_FRONT_CENTER;
		break;
	case 6:
		wfex.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
		break;
	default:
		wfex.dwChannelMask = KSAUDIO_SPEAKER_DIRECTOUT; // XXX : or SPEAKER_ALL ??
		break;
	}
	wfex.Samples.wValidBitsPerSample = wfex.Format.wBitsPerSample;	
	
	mtOut->SetType(&MEDIATYPE_Audio);
	mtOut->SetSubtype(&MEDIASUBTYPE_PCM);
	mtOut->SetFormatType(&FORMAT_WaveFormatEx);
	mtOut->SetFormat( (BYTE*) &wfex, sizeof(WAVEFORMATEX)+wfex.Format.cbSize);
	mtOut->SetTemporalCompression(FALSE);
	
	return S_OK;
}

// ----------------------------------------------------------------------------

HRESULT CCoreAACDecoder::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	if (*mtOut->Type() != MEDIATYPE_Audio	||
		*mtOut->Subtype() != MEDIASUBTYPE_PCM ||
		*mtOut->FormatType() != FORMAT_WaveFormatEx)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	
	return S_OK; 
}

// ----------------------------------------------------------------------------

// 960 for LD or else 1024 (expanded to 2048 for HE-AAC)
#define MAXFRAMELEN 2048

HRESULT CCoreAACDecoder::DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pProperties)
{
	pProperties->cBuffers = 8;
	m_OutputBuffLen = m_Channels * MAXFRAMELEN * sizeof(short);
	pProperties->cbBuffer = m_OutputBuffLen;
	
	NOTE1("CCoreAACDecoder::DecideBufferSize %d", pProperties->cbBuffer);

	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAllocator->SetProperties(pProperties, &Actual);
	if(FAILED(hr))
		return hr;

	if (Actual.cbBuffer < pProperties->cbBuffer || Actual.cBuffers < pProperties->cBuffers)
		return E_INVALIDARG;

	return S_OK;
}

// ----------------------------------------------------------------------------

HRESULT CCoreAACDecoder::CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin)
{
	HRESULT hr = CTransformFilter::CompleteConnect(direction, pReceivePin);
	
	if(direction == PINDIR_INPUT)
	{
		if(m_decHandle)
		{
			faacDecClose(m_decHandle);
			m_decHandle = NULL;
		}
		m_decHandle = faacDecOpen();

        faacDecConfigurationPtr config;		
        config = faacDecGetCurrentConfiguration(m_decHandle);
		config->downMatrix = m_DownMatrix;
        faacDecSetConfiguration(m_decHandle, config);

		// Initialize the decoder
		unsigned long SamplesPerSec = 0;
		unsigned char Channels = 0;
		if(faacDecInit2(m_decHandle, m_decoderSpecific, m_decoderSpecificLen,
			&SamplesPerSec, &Channels) < 0)
		{
			return E_FAIL;
		}
		
		if(m_DownMatrix)
		{
			Channels = 2; // TODO : check with mono
		}

		mp4AudioSpecificConfig info;
		AudioSpecificConfig(m_decoderSpecific,m_decoderSpecificLen,&info);
		
		wsprintf(m_ProfileName,"%s%s",
			ObjectTypesNameTable[info.objectTypeIndex],
#if 0
			info.sbr_present_flag ?
#else
			false ?
#endif
			"+SBR" :
			""
			);

		m_Channels = Channels;
		m_SamplesPerSec = SamplesPerSec;
		m_BitsPerSample = 16; // we always decode to the default 16 bits (we could add 24,32,float)
		
		m_brCalcFrames = 0;
		m_brBytesConsumed = 0;
		m_DecodedFrames = 0;
	}

	return hr;
}

// ----------------------------------------------------------------------------

HRESULT CCoreAACDecoder::StartStreaming(void)
{
	m_brCalcFrames = 0;
	m_brBytesConsumed = 0;
	m_DecodedFrames = 0;
	return CTransformFilter::StartStreaming();
}

// ----------------------------------------------------------------------------

HRESULT CCoreAACDecoder::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	if (m_State == State_Stopped)
	{	
		pOut->SetActualDataLength(0);
		return S_OK;
	}

	if(pIn->IsPreroll() == S_OK)
	{
		return S_FALSE;
	}

	// Decode the sample data
	DWORD ActualDstLength;
	BYTE *pSrc, *pDst;
	DWORD SrcLength = pIn->GetActualDataLength();
	DWORD DstLength = pOut->GetSize();
	HRESULT hr;
	hr = pIn->GetPointer(&pSrc);
	if(hr != S_OK)
		return hr;
	hr = pOut->GetPointer(&pDst);
	if(hr != S_OK)
		return hr;
	
	if(!pSrc || !pDst || (DstLength < m_OutputBuffLen))
		return S_FALSE;  

	// Decode data
	// (use our buffer calculated len, as the Waveout renderer seems to report wrongly a bigger size)	
	if(!Decode(pSrc, SrcLength, pDst, m_OutputBuffLen, &ActualDstLength))
		return S_FALSE;

	NOTE3("Transform: %u->%u (%u)\n", SrcLength, ActualDstLength, m_OutputBuffLen);

	// Copy the actual data length
	pOut->SetActualDataLength(ActualDstLength);
	return S_OK;
}

// ----------------------------------------------------------------------------

// AAC order : C, L, R, L", R", LFE
// DShow order : L, R, C, LFE, L", R"

const int MAXCHANNELS = 6;
const int chmap[MAXCHANNELS][MAXCHANNELS+1] = {
	// first column tell us if we need to remap
	{  0, },					// mono
	{  0, },					// l, r
	{  1, 1, 2, 0, },			// c ,l, r -> l, r, c
	{  1, 1, 2, 0, 3, },		// c, l, r, bc -> l, r, c, bc
	{  1, 1, 2, 0, 3, 4, },		// c, l, r, bl, br -> l, r, c, bl, br
	{  1, 1, 2, 0, 5, 3, 4 }	// c, l, r, bl, br, lfe -> l, r, c, lfe, bl, br
};

// ----------------------------------------------------------------------------

bool CCoreAACDecoder::Decode(BYTE *pSrc, DWORD SrcLength, BYTE *pDst, DWORD DstLength, DWORD *ActualDstLength)
{
	faacDecFrameInfo frameInfo;
	short *outsamples = (short *)faacDecDecode(m_decHandle, &frameInfo, pSrc, DstLength);

	if (frameInfo.error)
	{
		NOTE2("CCoreAACDecoder::Decode Error %d [%s]\n", 
			frameInfo.error, faacDecGetErrorMessage(frameInfo.error));
		return false;
	}

	m_brCalcFrames++;
	m_DecodedFrames++;
	m_brBytesConsumed += SrcLength;

	if(m_brCalcFrames == 43)
	{
		m_Bitrate = (int)((m_brBytesConsumed * 8) / (m_DecodedFrames / 43.07));
		m_brCalcFrames = 0;
	}

	if (!frameInfo.error && outsamples)
	{
		int channelidx = frameInfo.channels-1;
		if(chmap[channelidx][0])
		{
			// dshow remapping
			short *dstBuffer = (short*)pDst;
			for(unsigned int i = 0;
			    i < frameInfo.samples;
				i += frameInfo.channels, outsamples += frameInfo.channels)
			{
				for(unsigned int j=1; j <= frameInfo.channels; j++)
				{
					*dstBuffer++ = outsamples[chmap[channelidx][j]];
				}				
			}
		}
		else
		{
			memcpy(pDst, outsamples, frameInfo.samples * sizeof(short));
		}
	}
	else
		return false;

	*ActualDstLength = frameInfo.samples * sizeof(short);
	return true;
}

// ============================================================================
// ICoreAAC
// ============================================================================

STDMETHODIMP CCoreAACDecoder::get_ProfileName(char** name)
{
	CheckPointer(name,E_POINTER);
	*name = m_ProfileName;
	return S_OK;
}

STDMETHODIMP CCoreAACDecoder::get_SampleRate(int* sample_rate)
{
	CheckPointer(sample_rate,E_POINTER);
	*sample_rate = m_SamplesPerSec;
	return S_OK;
}

STDMETHODIMP CCoreAACDecoder::get_Channels(int *channels)
{
	CheckPointer(channels,E_POINTER);
	*channels = m_Channels;
	return S_OK;
}

STDMETHODIMP CCoreAACDecoder::get_BitsPerSample(int *bits_per_sample)
{
	CheckPointer(bits_per_sample,E_POINTER);
	*bits_per_sample = m_BitsPerSample;
	return S_OK;
}

STDMETHODIMP CCoreAACDecoder::get_Bitrate(int *bitrate)
{
	CheckPointer(bitrate,E_POINTER);
	*bitrate = m_Bitrate;
	return S_OK;
}

STDMETHODIMP CCoreAACDecoder::get_FramesDecoded(unsigned int *frames_decoded)
{
	CheckPointer(frames_decoded,E_POINTER);
	*frames_decoded = m_DecodedFrames;
	return S_OK;
}

STDMETHODIMP CCoreAACDecoder::get_DownMatrix(bool *down_matrix)
{
	CheckPointer(down_matrix,E_POINTER);
	*down_matrix = m_DownMatrix;
	return S_OK;
}

STDMETHODIMP CCoreAACDecoder::set_DownMatrix(bool down_matrix)
{
	m_DownMatrix = down_matrix;
	return S_OK;
}

// ============================================================================



