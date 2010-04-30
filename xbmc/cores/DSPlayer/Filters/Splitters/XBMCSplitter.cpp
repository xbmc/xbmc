/*
 *      Copyright (C) 2005-2010 Team XBMC
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


#include "XBMCSplitter.h"

#include <initguid.h>
#include <moreuuids.h>
#include "XBMCStreamVideo.h"
#include "XBMCStreamAudio.h"
#include "DShowUtil/DShowUtil.h"

//
// CXBMCSplitterFilter
//

CXBMCSplitterFilter::CXBMCSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
  :CSource(NAME("CXBMCSplitterFilter"), pUnk, __uuidof(this), phr)
  ,m_rtStart((long)0)
  ,m_pAudioStream(GetOwner(), this, phr)
  ,m_pVideoStream(GetOwner(), this, phr)
{
  
  m_rtStop = _I64_MAX / 2;
    m_rtDuration = m_rtStop;
    m_dRateSeeking = 1.0;
	
    m_dwSeekingCaps = AM_SEEKING_CanGetDuration		
		| AM_SEEKING_CanGetStopPos
		| AM_SEEKING_CanSeekForwards
        | AM_SEEKING_CanSeekBackwards
        | AM_SEEKING_CanSeekAbsolute;
}

CXBMCSplitterFilter::~CXBMCSplitterFilter()
{

}

// ============================================================================
// CXBMCSplitterFilter IFileSourceFilter interface
// ============================================================================

STDMETHODIMP CXBMCSplitterFilter::Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
  CLog::Log(LOGDEBUG,"%s",__FUNCTION__);
	CheckPointer(lpwszFileName, E_POINTER);	
	
	// lstrlenW is one of the few Unicode functions that works on win95
	int cch = lstrlenW(lpwszFileName) + 1;
	
	m_pFileName = new WCHAR[cch];
	
	if (m_pFileName!=NULL)
	{
		CopyMemory(m_pFileName, lpwszFileName, cch*sizeof(WCHAR));	
	} else {
		NOTE("CCAudioSource::Load filename is empty !");
		return VFW_E_CANNOT_RENDER;
	}
  TCHAR *lpszFileName=0;
	lpszFileName = new char[cch * 2];
	if (!lpszFileName) {
		NOTE("MkxFilter::Load new lpszFileName failed E_OUTOFMEMORY");
		return E_OUTOFMEMORY;
	}
	WideCharToMultiByte(GetACP(), 0, lpwszFileName, -1, lpszFileName, cch, NULL, NULL);
  CAutoLock cAutoLock(&m_cStateLock);
  OpenDemux(CStdString(lpszFileName));

}

bool CXBMCSplitterFilter::OpenDemux(CStdString pFile)
{
  m_iCurrentPts = DVD_NOPTS_VALUE;
  AVProbeData   pd;
  AVInputFormat* iformat = NULL;
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllAvFormat.Load())  
  {
    CLog::Log(LOGERROR,"CDVDDemuxFFmpeg::Open - failed to load ffmpeg libraries");
    return false;
  }
  // register codecs
  m_dllAvFormat.av_register_all();
  if( m_dllAvFormat.av_open_input_file(&m_pFormatContext, pFile.c_str(), iformat, FFMPEG_FILE_BUFFER_SIZE, NULL) < 0 )
  {
    return false;
  }
  // we need to know if this is matroska or avi later
  m_bMatroska = strcmp(m_pFormatContext->iformat->name, "matroska") == 0;
  m_bAVI = strcmp(m_pFormatContext->iformat->name, "avi") == 0;


  CLog::Log(LOGDEBUG, "%s - av_find_stream_info starting", __FUNCTION__);
  int iErr = m_dllAvFormat.av_find_stream_info(m_pFormatContext);
  if (iErr < 0)
  {
    CLog::Log(LOGWARNING,"could not find codec parameters for %s", pFile.c_str());
    return false;
  }
  CLog::Log(LOGDEBUG, "%s - av_find_stream_info finished", __FUNCTION__);

  m_pFormatContext->flags |= AVFMT_FLAG_NONBLOCK;

  m_dllAvFormat.dump_format(m_pFormatContext, 0, pFile.c_str(), 0);
  
  UpdateCurrentPTS();
  
  if (m_pFormatContext->nb_programs)
  {
    //TODO
    CLog::Log(LOGERROR, "%s - m_pFormatContext->nb_programs not coded yet", __FUNCTION__);
  }
  else
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
    {
      AddStream(i);
    }
  }
return true;
}

void CXBMCSplitterFilter::AddStream(int iId)
{
  
  CStdStringW name, label;
  AVStream *pStream = m_pFormatContext->streams[iId];
  if (!pStream)
    return;
  AVCodecContext *pCodecContext = pStream->codec;
  
  if (pCodecContext->codec_type == CODEC_TYPE_VIDEO)
  {
    m_pVideoStream.SetAVStream(pStream, m_pFormatContext);
  }
  else if(pCodecContext->codec_type == CODEC_TYPE_AUDIO)
  {
    m_pAudioStream.SetAVStream(pStream, m_pFormatContext);
  }
  
}

void CXBMCSplitterFilter::UpdateCurrentPTS()
{
  m_iCurrentPts = DVD_NOPTS_VALUE;
  for(unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    AVStream *stream = m_pFormatContext->streams[i];
    if(stream && stream->cur_dts != (int64_t)AV_NOPTS_VALUE)
    {
      double ts = ConvertTimestamp(stream->cur_dts, stream->time_base.den, stream->time_base.num);
      if(m_iCurrentPts == DVD_NOPTS_VALUE || m_iCurrentPts > ts )
        m_iCurrentPts = ts;
    }
  }

}

double CXBMCSplitterFilter::ConvertTimestamp(int64_t pts, int den, int num)
{
  if (pts == (int64_t)AV_NOPTS_VALUE)
    return DVD_NOPTS_VALUE;

  // do calculations in floats as they can easily overflow otherwise
  // we don't care for having a completly exact timestamp anyway
  double timestamp = (double)pts * num  / den;
  double starttime = 0.0f;

  if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE)
    starttime = (double)m_pFormatContext->start_time / AV_TIME_BASE;

  if(timestamp > starttime)
    timestamp -= starttime;
  else if( timestamp + 0.1f > starttime )
    timestamp = 0;

  return timestamp*DVD_TIME_BASE;
}