/*
 * asap_dsf.cpp - ASAP DirectShow source filter
 *
 * Copyright (C) 2008-2009  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <streams.h>

#include "asap.h"

static const char extensions[][5] =
	{ ".sap", ".cmc", ".cm3", ".cmr", ".cms", ".dmc", ".dlt", ".mpt", ".mpd", ".rmt", ".tmc", ".tm8", ".tm2" };
#define N_EXTS (sizeof(extensions) / sizeof(extensions[0]))

#define BITS_PER_SAMPLE      16
#define MIN_BUFFERED_BLOCKS  4096

#define SZ_ASAP_SOURCE       L"ASAP source filter"

static const char CLSID_ASAPSource_str[] = "{8E6205A0-19E2-4037-AF32-B29A9B9D0C93}";
static const GUID CLSID_ASAPSource = 
	{ 0x8e6205a0, 0x19e2, 0x4037, { 0xaf, 0x32, 0xb2, 0x9a, 0x9b, 0x9d, 0xc, 0x93 } };

class CASAPSourceStream : public CSourceStream, IMediaSeeking
{
	CCritSec cs;
	ASAP_State asap;
	BOOL loaded;
	int duration;
	LONGLONG blocks;

public:

	CASAPSourceStream(HRESULT *phr, CSource *pFilter)
		: CSourceStream(NAME("ASAPSourceStream"), phr, pFilter, L"Out"), loaded(FALSE), duration(0)
	{
	}

	DECLARE_IUNKNOWN

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
	{
		if (riid == IID_IMediaSeeking)
			return GetInterface((IMediaSeeking *) this, ppv);
		return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
	}

	BOOL Load(const char *filename)
	{
		HANDLE fh = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (fh == INVALID_HANDLE_VALUE)
			return FALSE;
		byte module[ASAP_MODULE_MAX];
		int module_len;
		BOOL ok = ReadFile(fh, module, ASAP_MODULE_MAX, (LPDWORD) &module_len, NULL);
		CloseHandle(fh);
		if (!ok)
			return FALSE;
		CAutoLock lck(&cs);
		loaded = ASAP_Load(&asap, filename, module, module_len);
		if (!loaded)
			return FALSE;
		int song = asap.module_info.default_song;
		duration = asap.module_info.durations[song];
		ASAP_PlaySong(&asap, song, duration);
		blocks = 0;
		return TRUE;
	}

	HRESULT GetMediaType(CMediaType *pMediaType)
	{
		CheckPointer(pMediaType, E_POINTER);
		CAutoLock lck(&cs);
		if (!loaded)
			return E_FAIL;
		WAVEFORMATEX *wfx = (WAVEFORMATEX *) pMediaType->AllocFormatBuffer(sizeof(WAVEFORMATEX));
		CheckPointer(wfx, E_OUTOFMEMORY);
		int channels = asap.module_info.channels;
		pMediaType->SetType(&MEDIATYPE_Audio);
		pMediaType->SetSubtype(&MEDIASUBTYPE_PCM);
		pMediaType->SetTemporalCompression(FALSE);
		pMediaType->SetSampleSize(channels * (BITS_PER_SAMPLE / 8));
		pMediaType->SetFormatType(&FORMAT_WaveFormatEx);
		wfx->wFormatTag = WAVE_FORMAT_PCM;
		wfx->nChannels = channels;
		wfx->nSamplesPerSec = ASAP_SAMPLE_RATE;
		wfx->nBlockAlign = channels * (BITS_PER_SAMPLE / 8);
		wfx->nAvgBytesPerSec = ASAP_SAMPLE_RATE * wfx->nBlockAlign;
		wfx->wBitsPerSample = BITS_PER_SAMPLE;
		wfx->cbSize = 0;
		return S_OK;
	}

	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest)
	{
		CheckPointer(pAlloc, E_POINTER);
		CheckPointer(pRequest, E_POINTER);
		CAutoLock lck(&cs);
		if (!loaded)
			return E_FAIL;
		if (pRequest->cBuffers == 0)
			pRequest->cBuffers = 2;
		int bytes = MIN_BUFFERED_BLOCKS * asap.module_info.channels * (BITS_PER_SAMPLE / 8);
		if (pRequest->cbBuffer < bytes)
			pRequest->cbBuffer = bytes;
		ALLOCATOR_PROPERTIES actual;
		HRESULT hr = pAlloc->SetProperties(pRequest, &actual);
		if (FAILED(hr))
			return hr;
		if (actual.cbBuffer < bytes)
			return E_FAIL;
		return S_OK;
	}

	HRESULT FillBuffer(IMediaSample *pSample)
	{
		CheckPointer(pSample, E_POINTER);
		CAutoLock lck(&cs);
		if (!loaded)
			return E_FAIL;
		BYTE *pData;
		HRESULT hr = pSample->GetPointer(&pData);
		if (FAILED(hr))
			return hr;
		int cbData = pSample->GetSize();
		cbData = ASAP_Generate(&asap, pData, cbData, (ASAP_SampleFormat) BITS_PER_SAMPLE);
		if (cbData == 0)
			return S_FALSE;
		pSample->SetActualDataLength(cbData);
		LONGLONG startTime = blocks * UNITS / ASAP_SAMPLE_RATE;
		blocks += cbData / (asap.module_info.channels * (BITS_PER_SAMPLE / 8));
		LONGLONG endTime = blocks * UNITS / ASAP_SAMPLE_RATE;
		pSample->SetTime(&startTime, &endTime);
		pSample->SetSyncPoint(TRUE);
		return S_OK;
	}

	STDMETHODIMP GetCapabilities(DWORD *pCapabilities)
	{
		CheckPointer(pCapabilities, E_POINTER);
		*pCapabilities = AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanSeekForwards | AM_SEEKING_CanSeekBackwards | AM_SEEKING_CanGetDuration;
		return S_OK;
	}

	STDMETHODIMP CheckCapabilities(DWORD *pCapabilities)
	{
		CheckPointer(pCapabilities, E_POINTER);
		DWORD result = *pCapabilities & (AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanSeekForwards | AM_SEEKING_CanSeekBackwards | AM_SEEKING_CanGetDuration);
		if (result == *pCapabilities)
			return S_OK;
		*pCapabilities = result;
		if (result != 0)
			return S_FALSE;
		return E_FAIL;
	}

	STDMETHODIMP IsFormatSupported(const GUID *pFormat)
	{
		CheckPointer(pFormat, E_POINTER);
		if (IsEqualGUID(*pFormat, TIME_FORMAT_MEDIA_TIME))
			return S_OK;
		return S_FALSE;
	}

	STDMETHODIMP QueryPreferredFormat(GUID *pFormat)
	{
		CheckPointer(pFormat, E_POINTER);
		*pFormat = TIME_FORMAT_MEDIA_TIME;
		return S_OK;
	}

	STDMETHODIMP GetTimeFormat(GUID *pFormat)
	{
		CheckPointer(pFormat, E_POINTER);
		*pFormat = TIME_FORMAT_MEDIA_TIME;
		return S_OK;
	}

	STDMETHODIMP IsUsingTimeFormat(const GUID *pFormat)
	{
		CheckPointer(pFormat, E_POINTER);
		if (IsEqualGUID(*pFormat, TIME_FORMAT_MEDIA_TIME))
			return S_OK;
		return S_FALSE;
	}

	STDMETHODIMP SetTimeFormat(const GUID *pFormat)
	{
		CheckPointer(pFormat, E_POINTER);
		if (IsEqualGUID(*pFormat, TIME_FORMAT_MEDIA_TIME))
			return S_OK;
		return E_INVALIDARG;
	}

	STDMETHODIMP GetDuration(LONGLONG *pDuration)
	{
		CheckPointer(pDuration, E_POINTER);
		*pDuration = duration * (UNITS / MILLISECONDS);
		return S_OK;
	}

	STDMETHODIMP GetStopPosition(LONGLONG *pStop)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags)
	{
		if ((dwCurrentFlags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_AbsolutePositioning)
		{
			CheckPointer(pCurrent, E_POINTER);
			int position = (int) (*pCurrent / (UNITS / MILLISECONDS));
			CAutoLock lck(&cs);
			ASAP_Seek(&asap, position);
			blocks = 0;
			if ((dwCurrentFlags & AM_SEEKING_ReturnTime) != 0)
				*pCurrent = position * (UNITS / MILLISECONDS);
			return S_OK;
		}
		return E_INVALIDARG;
	}

	STDMETHODIMP GetPositions(LONGLONG *pCurrent, LONGLONG *pStop)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP SetRate(double dRate)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP GetRate(double *dRate)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP GetPreroll(LONGLONG *pllPreroll)
	{
		return E_NOTIMPL;
	}
};

class CASAPSource : public CSource, IFileSourceFilter
{
	CASAPSourceStream *m_pin;
	WCHAR *m_filename;
	CMediaType m_mt;

	CASAPSource(IUnknown *pUnk, HRESULT *phr)
		: CSource(NAME("ASAPSource"), pUnk, CLSID_ASAPSource), m_pin(NULL), m_filename(NULL)
	{
		m_pin = new CASAPSourceStream(phr, this);
		if (m_pin == NULL && phr != NULL)
			*phr = E_OUTOFMEMORY;
	}

	~CASAPSource()
	{
		if (m_pin != NULL)
			delete m_pin;
		if (m_filename != NULL)
			delete[] m_filename;
	}

public:

	DECLARE_IUNKNOWN

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
	{
		if (riid == IID_IFileSourceFilter)
			return GetInterface((IFileSourceFilter *) this, ppv);
		return CSource::NonDelegatingQueryInterface(riid, ppv);
	}

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt)
	{
		CheckPointer(pszFileName, E_POINTER);
		int cch = lstrlenW(pszFileName) + 1;
		char *filename = new char[cch * 2];
		CheckPointer(filename, E_OUTOFMEMORY);
		if (WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, filename, cch, NULL, NULL) <= 0)
			return HRESULT_FROM_WIN32(GetLastError());
		BOOL ok = m_pin->Load(filename);
		delete[] filename;
		if (!ok)
			return E_FAIL;
		if (m_filename != NULL)
			delete[] m_filename;
		m_filename = new WCHAR[cch];
		CheckPointer(m_filename, E_OUTOFMEMORY);
		CopyMemory(m_filename, pszFileName, cch * sizeof(WCHAR));
		if (pmt == NULL) {
			m_mt.SetType(&MEDIATYPE_Stream);
			m_mt.SetSubtype(&MEDIASUBTYPE_NULL);
		}
		else
			m_mt = *pmt;
		return S_OK;
	}

	STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt)
	{
		CheckPointer(ppszFileName, E_POINTER);
		if (m_filename == NULL)
			return E_FAIL;
		DWORD n = (lstrlenW(m_filename) + 1) * sizeof(WCHAR);
		*ppszFileName = (LPOLESTR) CoTaskMemAlloc(n);
		CheckPointer(*ppszFileName, E_OUTOFMEMORY);
		CopyMemory(*ppszFileName, m_filename, n);
		if (pmt != NULL)
			CopyMediaType(pmt, &m_mt);
		return S_OK;
	}

	static CUnknown * WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr)
	{
		CASAPSource *pNewFilter = new CASAPSource(pUnk, phr);
		if (phr != NULL && pNewFilter == NULL)
			*phr = E_OUTOFMEMORY;
		return pNewFilter;
	}
};

static const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_PCM
};

static const AMOVIESETUP_PIN sudASAPSourceStream =
{
	L"Output",
	FALSE,
	TRUE,
	FALSE,
	FALSE,
	&CLSID_NULL,
	NULL,
	1,
	&sudPinTypes
};

static const AMOVIESETUP_FILTER sudASAPSource =
{
	&CLSID_ASAPSource,
	SZ_ASAP_SOURCE,
	MERIT_NORMAL,
	1,
	&sudASAPSourceStream
};

CFactoryTemplate g_Templates[1] =
{
	{
		SZ_ASAP_SOURCE,
		&CLSID_ASAPSource,
		CASAPSource::CreateInstance,
		NULL,
		&sudASAPSource
	}
};

int g_cTemplates = 1;

STDAPI DllRegisterServer()
{
	HRESULT hr = AMovieDllRegisterServer2(TRUE);
	if (FAILED(hr))
		return hr;

	HKEY hMTKey;
	HKEY hWMPKey;
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, "Media Type\\Extensions", 0, NULL, 0, KEY_WRITE, NULL, &hMTKey, NULL) != ERROR_SUCCESS)
		return E_FAIL;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Multimedia\\WMPlayer\\Extensions", 0, NULL, 0, KEY_WRITE, NULL, &hWMPKey, NULL) != ERROR_SUCCESS)
		return E_FAIL;
	for (int i = 0; i < N_EXTS; i++) {
		HKEY hKey;
		if (RegCreateKeyEx(hMTKey, extensions[i], 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS
		 || RegSetValueEx(hKey, "Source Filter", 0, REG_SZ, (const BYTE *) &CLSID_ASAPSource_str, sizeof(CLSID_ASAPSource_str)) != ERROR_SUCCESS
		 || RegCloseKey(hKey) != ERROR_SUCCESS)
			 return E_FAIL;
		static const DWORD perms = 15;
		static const DWORD runtime = 7;
		if (RegCreateKeyEx(hWMPKey, extensions[i], 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS
		 || RegSetValueEx(hKey, "Permissions", 0, REG_DWORD, (const BYTE *) &perms, sizeof(perms)) != ERROR_SUCCESS
		 || RegSetValueEx(hKey, "Runtime", 0, REG_DWORD, (const BYTE *) &runtime, sizeof(runtime)) != ERROR_SUCCESS
		 || RegCloseKey(hKey) != ERROR_SUCCESS)
			 return E_FAIL;
	}
	if (RegCloseKey(hWMPKey) != ERROR_SUCCESS || RegCloseKey(hMTKey) != ERROR_SUCCESS)
		return E_FAIL;
	return S_OK;
}

STDAPI DllUnregisterServer()
{
	HKEY hWMPKey;
	HKEY hMTKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Multimedia\\WMPlayer\\Extensions", 0, DELETE, &hWMPKey) != ERROR_SUCCESS)
		return E_FAIL;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, "Media Type\\Extensions", 0, DELETE, &hMTKey) != ERROR_SUCCESS)
		return E_FAIL;
	for (int i = 0; i < N_EXTS; i++) {
		RegDeleteKey(hWMPKey, extensions[i]);
		RegDeleteKey(hMTKey, extensions[i]);
	}
	RegCloseKey(hMTKey);
	RegCloseKey(hWMPKey);

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	return DllEntryPoint(hInstance, dwReason, lpReserved);
}
