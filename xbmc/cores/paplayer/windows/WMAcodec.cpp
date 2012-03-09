/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "WMAcodec.h"
#include "utils/log.h"
#include "system.h"

#pragma comment(lib, "wmvcore.lib")

#define BUFFER_SIZE 882000 //4*44100*5

WMAcodec::WMAcodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_TotalTime=0;
  m_Bitrate = 0;
  m_CodecName = "WMA";
  m_bnomoresamples = false;
  m_uimaxwritebuffer = 0;
  m_pStream   = NULL;
  
  ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	m_ISyncReader = NULL;
}

WMAcodec::~WMAcodec()
{
  DeInit();
  ::CoUninitialize();
}

bool WMAcodec::Init(const CStdString &strFile, unsigned int filecache)
{
  HRESULT hr;
  IWMHeaderInfo* wmHeaderInfo;
  IWMProfile* wmProfile;
  IWMStreamConfig* wmStreamConfig;
  IWMMediaProps* wmMediaProperties;
  WMT_ATTR_DATATYPE wmAttrDataType;
  QWORD durationInNano;
  WORD lengthDataType = sizeof(QWORD);
  WORD wAudioStreamNum = -1;
  DWORD sizeMediaType;
  DWORD dwStreams = 0;
  WMT_STREAM_SELECTION	wmtSS = WMT_ON;
  GUID pguidStreamType;
  WORD wmStreamNum = 0;

  hr  = WMCreateSyncReader(NULL, WMT_RIGHT_PLAYBACK, &m_ISyncReader);
	if(hr!=S_OK)
	{
    CLog::Log(LOGERROR,"WMAcodec: error creating WMCreateSyncReader");
    return false;
  }

  SAFE_DELETE(m_pStream);

  m_pStream = new XBMCistream();
  if(m_pStream == NULL)
    return false;

  hr = m_pStream->Open(strFile);
  if(hr!=S_OK)
  {
    CLog::Log(LOGERROR,"WMAcodec: error opening file %s!",strFile.c_str());
    return false;
  }

  hr = m_ISyncReader->OpenStream(m_pStream);
  if(hr!=S_OK)
	{
    CLog::Log(LOGERROR,"WMAcodec: error opening file %s!",strFile.c_str());
    return false;
  }

  m_ISyncReader->QueryInterface(&wmHeaderInfo);
  wmHeaderInfo->GetAttributeByName(&wmStreamNum, L"Duration", &wmAttrDataType, (BYTE*)&durationInNano, &lengthDataType ) ;
  m_TotalTime = (long long)(durationInNano/10000);

  m_ISyncReader->QueryInterface(&wmProfile);

  hr = wmProfile->GetStreamCount(&dwStreams);
  if(hr!=S_OK)
	{
    CLog::Log(LOGERROR,"WMAcodec: GetStreamCount failed (hr=0x%08x).", hr );
    SAFE_RELEASE(wmProfile);
    return false;
  }

  for ( DWORD i = 0; i < dwStreams; i++ )
  {
    hr = wmProfile->GetStream(i, &wmStreamConfig);
    if(hr!=S_OK)
	  {
      CLog::Log(LOGERROR,"WMAcodec: GetStream failed (hr=0x%08x).", hr );
      SAFE_RELEASE(wmProfile);
      return false;
    }

    hr = wmStreamConfig->GetStreamNumber( &wmStreamNum );
    if(hr!=S_OK)
	  {
      CLog::Log(LOGERROR,"WMAcodec: GetStreamNumber failed (hr=0x%08x).", hr );
      SAFE_RELEASE(wmProfile);
      SAFE_RELEASE(wmStreamConfig);
      return false;
    }
    hr = wmStreamConfig->GetStreamType( &pguidStreamType );
    if(hr!=S_OK)
	  {
      CLog::Log(LOGERROR,"WMAcodec: GetStreamNumber failed (hr=0x%08x).", hr );
      SAFE_RELEASE(wmProfile);
      SAFE_RELEASE(wmStreamConfig);
      return false;
    }
    if( WMMEDIATYPE_Audio == pguidStreamType )
    {
      wAudioStreamNum = wmStreamNum;
      break;
    }
    SAFE_RELEASE(wmStreamConfig);
  }

  if(wAudioStreamNum == -1)
  {
    SAFE_RELEASE(wmProfile);
    return false;
  }

  m_ISyncReader->SetStreamsSelected(1, &wAudioStreamNum, &wmtSS);
  m_ISyncReader->SetReadStreamSamples(wAudioStreamNum, false);

  
  wmStreamConfig->QueryInterface(&wmMediaProperties);
  wmMediaProperties->GetMediaType(NULL, &sizeMediaType);

  WM_MEDIA_TYPE* mediaType = (WM_MEDIA_TYPE*)LocalAlloc(LPTR,sizeMediaType);
  wmMediaProperties->GetMediaType(mediaType, &sizeMediaType);

  WAVEFORMATEX* inputFormat = (WAVEFORMATEX*)mediaType->pbFormat;

  switch(inputFormat->wFormatTag)
  {
  case WAVE_FORMAT_WMAUDIO2:
    m_CodecName = "WMA2";
    break;
  case WAVE_FORMAT_WMAUDIO3:
    m_CodecName = "WMA3";
    break;
  case WAVE_FORMAT_WMAUDIO_LOSSLESS:
    m_CodecName = "WMA Lossless";
    break;
  }

  // somehow I only get garbage if I use the formats provided
  // from the header. Other sample rate or bits per sample
  // don't work. dunno where my mistake is.
  m_Channels = 2; //inputFormat->nChannels;
  m_SampleRate = 44100; //inputFormat->nSamplesPerSec;
  m_BitsPerSample = 16; //inputFormat->wBitsPerSample;
  m_Bitrate = inputFormat->nAvgBytesPerSec/8;

  if(!m_pcmBuffer.Create(BUFFER_SIZE))
    return false;

  SAFE_RELEASE(wmMediaProperties);
  SAFE_RELEASE(wmStreamConfig);
  SAFE_RELEASE(wmProfile);
  SAFE_RELEASE(wmHeaderInfo);

  LocalFree(mediaType);

  return true;
}

void WMAcodec::DeInit()
{
  if(m_ISyncReader != NULL)
    m_ISyncReader->Close();

  SAFE_RELEASE(m_ISyncReader);
  SAFE_DELETE(m_pStream);
  m_pcmBuffer.Destroy();
  m_bnomoresamples = false;
}

__int64 WMAcodec::Seek(__int64 iSeekTime)
{
  m_pcmBuffer.Clear();
  m_bnomoresamples = false;
  if(m_ISyncReader->SetRange(iSeekTime*10000,0) == S_OK)
    return iSeekTime;
  return 0;
}

int WMAcodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  HRESULT hr;
  QWORD cnsSampleTime = 0;
	QWORD cnsSampleDuration = 0;
	DWORD dwFlags = 0;
  DWORD dwOutputNum;
  WORD wStreamNum;
  DWORD dwBufferLength;

  if(!m_bnomoresamples && m_pcmBuffer.getMaxWriteSize() > m_uimaxwritebuffer)
  {
    hr = m_ISyncReader->GetNextSample(0,
		  									&m_pINSSBuffer,
			  								&cnsSampleTime,
				  							&cnsSampleDuration,
					  						&dwFlags,
						  					&dwOutputNum,
							  				&wStreamNum);

    if(hr== NS_E_NO_MORE_SAMPLES)
		  m_bnomoresamples = true;

    if(m_uimaxwritebuffer == 0)
      m_pINSSBuffer->GetMaxLength(&m_uimaxwritebuffer);

    if(SUCCEEDED(hr))
	  {
	    unsigned char *buffer;
		  m_pINSSBuffer->GetBufferAndLength(&buffer,&dwBufferLength);

      m_pcmBuffer.WriteData((char *)buffer, dwBufferLength);

		  //cleaning up before reading next sample
		  SAFE_RELEASE(m_pINSSBuffer);
	  }
  }

  if ((unsigned int)size < m_pcmBuffer.getMaxReadSize())
  { 
    m_pcmBuffer.ReadData((char *)pBuffer, size);
    *actualsize=size;
  }
  else
  {
    m_pcmBuffer.ReadData((char *)pBuffer, m_pcmBuffer.getMaxReadSize());
    *actualsize=m_pcmBuffer.getMaxReadSize();
    if(m_bnomoresamples)
      return READ_EOF;
  }

  return READ_SUCCESS;
}

int WMAcodec::ReadSamples(float *pBuffer, int numsamples, int *actualsamples)
{
  int result = ReadPCM((BYTE *)pBuffer, numsamples * sizeof(float), actualsamples);
  *actualsamples /= sizeof(float);
  return result;
}

bool WMAcodec::CanInit()
{
  return (LoadLibrary("WMVcore.dll") != NULL);
}

