/*
*      Copyright (C) 2010 Team XBMC
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

#include "LAVFDemuxer.h"
#include "LAVFUtils.h"
#include "LAVFStreamInfo.h"

CLAVFDemuxer::CLAVFDemuxer(CCritSec *pLock)
    : CBaseDemuxer("lavf demuxer", pLock), m_avFormat(NULL), m_rtCurrent(0)
{
  m_dllAvFormat.Load(); 
  m_dllAvCodec.Load();
  m_dllAvUtil.Load();
  m_dllAvFormat.av_register_all();
}

CLAVFDemuxer::~CLAVFDemuxer()
{
  if (m_avFormat)
  {
    m_dllAvFormat.av_close_input_file(m_avFormat);
    m_avFormat = NULL;
  }
}

STDMETHODIMP CLAVFDemuxer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  CheckPointer(ppv, E_POINTER);

  *ppv = NULL;

  return
    QI(IKeyFrameInfo)
    QI2(IAMExtendedSeeking)
    __super::NonDelegatingQueryInterface(riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// Demuxer Functions
STDMETHODIMP CLAVFDemuxer::Open(LPCOLESTR pszFileName)
{
  CAutoLock lock(m_pLock);
  HRESULT hr = S_OK;

  int ret; // return code from avformat functions

  // Convert the filename from wchar to char for avformat
  char fileName[1024];
  wcstombs_s(NULL, fileName, 1024, pszFileName, _TRUNCATE);

  ret = m_dllAvFormat.av_open_input_file(&m_avFormat, fileName, NULL, FFMPEG_FILE_BUFFER_SIZE, NULL);
  if (ret < 0)
  {
    DbgLog((LOG_ERROR, 0, TEXT("av_open_input_file failed")));
    goto done;
  }

  ret = m_dllAvFormat.av_find_stream_info(m_avFormat);
  if (ret < 0)
  {
    DbgLog((LOG_ERROR, 0, TEXT("av_find_stream_info failed")));
    goto done;
  }

  m_bMatroska = (_strnicmp(m_avFormat->iformat->name, "matroska", 8) == 0);
  m_bAVI = (_strnicmp(m_avFormat->iformat->name, "avi", 3) == 0);
  m_bMPEGTS = (_strnicmp(m_avFormat->iformat->name, "mpegts", 6) == 0);

  CHECK_HR(hr = CreateStreams());

  return S_OK;
done:
  // Cleanup
  if (m_avFormat)
  {
    m_dllAvFormat.av_close_input_file(m_avFormat);
    m_avFormat = NULL;
  }
  return E_FAIL;
}

REFERENCE_TIME CLAVFDemuxer::GetDuration() const
{
  int64_t iLength = 0;
  if (m_avFormat->duration == (int64_t)AV_NOPTS_VALUE || m_avFormat->duration < 0LL)
  {
    // no duration is available for us
    // try to calculate it
    // TODO
    /*if (m_rtCurrent != Packet::INVALID_TIME && m_avFormat->file_size > 0 && m_avFormat->pb && m_avFormat->pb->pos > 0) {
    iLength = (((m_rtCurrent * m_avFormat->file_size) / m_avFormat->pb->pos) / 1000) & 0xFFFFFFFF;
    }*/
    DbgLog((LOG_ERROR, 1, TEXT("duration is not available")));
  }
  else
  {
    iLength = m_avFormat->duration;
  }
  return ConvertTimestampToRT(iLength, 1, AV_TIME_BASE, 0);
}

STDMETHODIMP CLAVFDemuxer::GetNextPacket(Packet **ppPacket)
{
  CheckPointer(ppPacket, E_POINTER);

  // If true, S_FALSE is returned, indicating a soft-failure
  bool bReturnEmpty = false;

  // Read packet
  AVPacket pkt;
  Packet *pPacket = NULL;

  // assume we are not eof
  if (m_avFormat->pb)
  {
    m_avFormat->pb->eof_reached = 0;
  }

  int result = 0;
  try
  {
    result = m_dllAvFormat.av_read_frame(m_avFormat, &pkt);
  }
  catch (...)
  {
    // ignore..
  }

  if (result == AVERROR(EINTR) || result == AVERROR(EAGAIN))
  {
    // timeout, probably no real error, return empty packet
    bReturnEmpty = true;
  }
  else if (result < 0)
  {
    // meh, fail
  }
  else if (pkt.size < 0 || pkt.stream_index >= MAX_STREAMS)
  {
    // XXX, in some cases ffmpeg returns a negative packet size
    if (m_avFormat->pb && !m_avFormat->pb->eof_reached)
    {
      bReturnEmpty = true;
    }
    m_dllAvCodec.av_free_packet(&pkt);
  }
  else
  {
    // Check right here if the stream is active, we can drop the package otherwise.
    BOOL streamActive = FALSE;
    for (int i = 0; i < unknown; i++)
    {
      if (m_dActiveStreams[i] == pkt.stream_index)
      {
        streamActive = TRUE;
        break;
      }
    }
    if (!streamActive)
    {
      m_dllAvCodec.av_free_packet(&pkt);
      return S_FALSE;
    }

    AVStream *stream = m_avFormat->streams[pkt.stream_index];
    pPacket = new Packet();

    // libavformat sometimes bugs and sends dts/pts 0 instead of invalid.. correct this
    if (pkt.dts == 0)
    {
      pkt.dts = AV_NOPTS_VALUE;
    }
    if (pkt.pts == 0)
    {
      pkt.pts = AV_NOPTS_VALUE;
    }

    // we need to get duration slightly different for matroska embedded text subtitels
    if (m_bMatroska && stream->codec->codec_id == CODEC_ID_TEXT && pkt.convergence_duration != 0)
    {
      pkt.duration = (int)pkt.convergence_duration;
    }

    if (m_bAVI && stream->codec && stream->codec->codec_type == CODEC_TYPE_VIDEO)
    {
      // AVI's always have borked pts, specially if m_pFormatContext->flags includes
      // AVFMT_FLAG_GENPTS so always use dts
      pkt.pts = AV_NOPTS_VALUE;
    }

    if (pkt.data)
    {
      pPacket->SetData(pkt.data, pkt.size);
    }

    pPacket->StreamId = (DWORD)pkt.stream_index;

    REFERENCE_TIME pts = (REFERENCE_TIME)ConvertTimestampToRT(pkt.pts, stream->time_base.num, stream->time_base.den);
    REFERENCE_TIME dts = (REFERENCE_TIME)ConvertTimestampToRT(pkt.dts, stream->time_base.num, stream->time_base.den);
    REFERENCE_TIME duration = (REFERENCE_TIME)ConvertTimestampToRT(pkt.duration, stream->time_base.num, stream->time_base.den, 0);

    REFERENCE_TIME rt = m_rtCurrent;
    // Try the different times set, pts first, dts when pts is not valid
    if (pts != Packet::INVALID_TIME)
    {
      rt = pts;
    }
    else if (dts != Packet::INVALID_TIME)
    {
      rt = dts;
    }

    // stupid VC1
    if (stream->codec->codec_id == CODEC_ID_VC1 && dts != Packet::INVALID_TIME)
    {
      rt = dts;
    }

    pPacket->rtStart = rt;
    pPacket->rtStop = rt + ((duration > 0) ? duration : 1);

    if (stream->codec->codec_type == CODEC_TYPE_SUBTITLE)
    {
      pPacket->bDiscontinuity = TRUE;
    }
    else
    {
      pPacket->bSyncPoint = (duration > 0) ? 1 : 0;
      pPacket->bAppendable = !pPacket->bSyncPoint;
    }

    // Update current time
    m_rtCurrent = pPacket->rtStart;

    m_dllAvCodec.av_free_packet(&pkt);
  }

  if (bReturnEmpty && !pPacket)
  {
    return S_FALSE;
  }
  if (!pPacket)
  {
    return E_FAIL;
  }

  *ppPacket = pPacket;
  return S_OK;
}

STDMETHODIMP CLAVFDemuxer::Seek(REFERENCE_TIME rTime)
{
  int videoStreamId = m_dActiveStreams[video];
  int64_t seek_pts = 0;
  // If we have a video stream, seek on that one. If we don't, well, then don't!
  if (videoStreamId != -1)
  {
    AVStream *stream = m_avFormat->streams[videoStreamId];
    seek_pts = ConvertRTToTimestamp(rTime, stream->time_base.num, stream->time_base.den);
  }
  else
  {
    seek_pts = ConvertRTToTimestamp(rTime, 1, AV_TIME_BASE);
  }

  int ret = m_dllAvFormat.avformat_seek_file(m_avFormat, videoStreamId, _I64_MIN, seek_pts, _I64_MAX, 0);
  //int ret = av_seek_frame(m_avFormat, -1, seek_pts, 0);
  if (ret < 0)
  {
    CLog::Log(LOGERROR,"Seek failed");
  }

  return S_OK;
}

const char *CLAVFDemuxer::GetContainerFormat() const
{
  return m_avFormat->iformat->name;
}

HRESULT CLAVFDemuxer::StreamInfo(DWORD streamId, LCID *plcid, WCHAR **ppszName) const
{
  if (streamId >= (DWORD)m_avFormat->nb_streams)
  {
    return E_FAIL;
  }

  if (plcid)
  {
    const char *lang = get_stream_language(m_avFormat->streams[streamId]);
    if (lang)
    {
      *plcid = ProbeLangForLCID(lang);
    }
    else
    {
      *plcid = 0;
    }
  }

  if (ppszName)
  {
    lavf_describe_stream(m_avFormat->streams[streamId], ppszName);
  }

  return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IAMExtendedSeeking
STDMETHODIMP CLAVFDemuxer::get_ExSeekCapabilities(long* pExCapabilities)
{
  CheckPointer(pExCapabilities, E_POINTER);
  *pExCapabilities = AM_EXSEEK_CANSEEK;
  if (m_avFormat->nb_chapters > 0) *pExCapabilities |= AM_EXSEEK_MARKERSEEK;
  return S_OK;
}

STDMETHODIMP CLAVFDemuxer::get_MarkerCount(long* pMarkerCount)
{
  CheckPointer(pMarkerCount, E_POINTER);
  *pMarkerCount = (long)m_avFormat->nb_chapters;
  return S_OK;
}

STDMETHODIMP CLAVFDemuxer::get_CurrentMarker(long* pCurrentMarker)
{
  CheckPointer(pCurrentMarker, E_POINTER);
  // Can the time_base change in between chapters?
  // Anyhow, we do the calculation in the loop, just to be safe
  for (unsigned int i = 0; i < m_avFormat->nb_chapters; i++)
  {
    int64_t pts = ConvertRTToTimestamp(m_rtCurrent, m_avFormat->chapters[i]->time_base.num, m_avFormat->chapters[i]->time_base.den);
    // Check if the pts is in between the bounds of the chapter
    if (pts >= m_avFormat->chapters[i]->start && pts <= m_avFormat->chapters[i]->end)
    {
      *pCurrentMarker = (i + 1);
      return S_OK;
    }
  }
  return E_FAIL;
}

STDMETHODIMP CLAVFDemuxer::GetMarkerTime(long MarkerNum, double* pMarkerTime)
{
  CheckPointer(pMarkerTime, E_POINTER);
  // Chapters go by a 1-based index, doh
  unsigned int index = MarkerNum - 1;
  if (index >= m_avFormat->nb_chapters)
  {
    return E_FAIL;
  }

  REFERENCE_TIME rt = ConvertTimestampToRT(m_avFormat->chapters[index]->start, m_avFormat->chapters[index]->time_base.num, m_avFormat->chapters[index]->time_base.den);
  *pMarkerTime = (double)rt / DSHOW_TIME_BASE;

  return S_OK;
}

STDMETHODIMP CLAVFDemuxer::GetMarkerName(long MarkerNum, BSTR* pbstrMarkerName)
{
  CheckPointer(pbstrMarkerName, E_POINTER);
  // Chapters go by a 1-based index, doh
  unsigned int index = MarkerNum - 1;
  if (index >= m_avFormat->nb_chapters)
  {
    return E_FAIL;
  }
  // Get the title, or generate one
  OLECHAR wTitle[128];
  if (m_dllAvFormat.av_metadata_get(m_avFormat->chapters[index]->metadata, "title", NULL, 0))
  {
    char *title = m_dllAvFormat.av_metadata_get(m_avFormat->chapters[index]->metadata, "title", NULL, 0)->value;
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wTitle, 128);
  }
  else
  {
    swprintf_s(wTitle, L"Chapter %d", MarkerNum);
  }
  *pbstrMarkerName = SysAllocString(wTitle);
  return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IKeyFrameInfo
STDMETHODIMP CLAVFDemuxer::GetKeyFrameCount(UINT& nKFs)
{
  if (m_dActiveStreams[video] < 0)
  {
    return E_NOTIMPL;
  }
  AVStream *stream = m_avFormat->streams[m_dActiveStreams[video]];
  nKFs = stream->nb_index_entries;
  return (stream->nb_index_entries == stream->nb_frames) ? S_FALSE : S_OK;
}

STDMETHODIMP CLAVFDemuxer::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
  CheckPointer(pFormat, E_POINTER);
  CheckPointer(pKFs, E_POINTER);

  if (m_dActiveStreams[video] < 0)
  {
    return E_NOTIMPL;
  }

  if (*pFormat != TIME_FORMAT_MEDIA_TIME) return E_INVALIDARG;

  AVStream *stream = m_avFormat->streams[m_dActiveStreams[video]];
  nKFs = stream->nb_index_entries;
  for (unsigned int i = 0; i < nKFs; i++)
  {
    pKFs[i] = ConvertTimestampToRT(stream->index_entries[i].timestamp, stream->time_base.num, stream->time_base.den);
  }
  return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Internal Functions
STDMETHODIMP CLAVFDemuxer::AddStream(int streamId)
{
  HRESULT hr = S_OK;
  AVStream *pStream = m_avFormat->streams[streamId];
  stream s;
  s.pid = streamId;
  s.streamInfo = new CLAVFStreamInfo(pStream, m_avFormat->iformat->name, hr);

  if (FAILED(hr))
  {
    delete s.streamInfo;
    return hr;
  }

  // HACK: Change codec_id to TEXT for SSA to prevent some evil doings
  if (pStream->codec->codec_type == AVMEDIA_TYPE_SUBTITLE)
  {
    pStream->codec->codec_id = CODEC_ID_TEXT;
  }

  switch (pStream->codec->codec_type)
  {
  case AVMEDIA_TYPE_VIDEO:
    m_streams[video].push_back(s);
    break;
  case AVMEDIA_TYPE_AUDIO:
    m_streams[audio].push_back(s);
    break;
  case AVMEDIA_TYPE_SUBTITLE:
    m_streams[subpic].push_back(s);
    break;
  default:
    // unsupported stream
    // Normally this should be caught while creating the stream info already.
    delete s.streamInfo;
    return E_FAIL;
    break;
  }
  return S_OK;
}

// Pin creation
STDMETHODIMP CLAVFDemuxer::CreateStreams()
{
  CAutoLock lock(m_pLock);

  // try to use non-blocking methods
  m_avFormat->flags |= AVFMT_FLAG_NONBLOCK;

  for (int i = 0; i < countof(m_streams); i++)
  {
    m_streams[i].Clear();
  }

  if (m_avFormat->nb_programs)
  {
    m_program = UINT_MAX;
    // look for first non empty stream and discard nonselected programs
    for (unsigned int i = 0; i < m_avFormat->nb_programs; i++)
    {
      if (m_program == UINT_MAX && m_avFormat->programs[i]->nb_stream_indexes > 0)
      {
        m_program = i;
      }

      if (i != m_program)
      {
        m_avFormat->programs[i]->discard = AVDISCARD_ALL;
      }
    }
    if (m_program == UINT_MAX)
    {
      m_program = 0;
    }
    // add streams from selected program
    for (unsigned int i = 0; i < m_avFormat->programs[m_program]->nb_stream_indexes; i++)
    {
      AddStream(m_avFormat->programs[m_program]->stream_index[i]);
    }
  }
  else
  {
    for (unsigned int streamId = 0; streamId < m_avFormat->nb_streams; streamId++)
    {
      AddStream(streamId);
    }
  }

  // Create fake subtitle pin
  if (!m_streams[subpic].empty())
  {
    CreateNoSubtitleStream();
  }
  return S_OK;
}

// Converts the lavf pts timestamp to a DShow REFERENCE_TIME
// Based on DVDDemuxFFMPEG
REFERENCE_TIME CLAVFDemuxer::ConvertTimestampToRT(int64_t pts, int num, int den, int64_t starttime) const
{
  DllAvUtil dllAvUtil;
  dllAvUtil.Load();
  if (pts == (int64_t)AV_NOPTS_VALUE)
  {
    return Packet::INVALID_TIME;
  }

  if (starttime == (int64_t)AV_NOPTS_VALUE)
  {
    starttime = dllAvUtil.av_rescale_rnd(m_avFormat->start_time, den, (int64_t)AV_TIME_BASE * num, AV_ROUND_NEAR_INF);
  }

  if (starttime != 0)
  {
    pts -= starttime;
  }

  // Let av_rescale do the work, its smart enough to not overflow
  REFERENCE_TIME timestamp = dllAvUtil.av_rescale_rnd(pts, (int64_t)num * DSHOW_TIME_BASE, den, AV_ROUND_NEAR_INF);

  return timestamp;
}

// Converts the lavf pts timestamp to a DShow REFERENCE_TIME
// Based on DVDDemuxFFMPEG
int64_t CLAVFDemuxer::ConvertRTToTimestamp(REFERENCE_TIME timestamp, int num, int den, int64_t starttime) const
{
  DllAvUtil dllAvUtil;
  dllAvUtil.Load();
  if (timestamp == Packet::INVALID_TIME)
  {
    return (int64_t)AV_NOPTS_VALUE;
  }

  if (starttime == (int64_t)AV_NOPTS_VALUE)
  {
    starttime = dllAvUtil.av_rescale_rnd(m_avFormat->start_time, den, (int64_t)AV_TIME_BASE * num, AV_ROUND_NEAR_INF);
  }

  int64_t pts = dllAvUtil.av_rescale_rnd(timestamp, den, (int64_t)num * DSHOW_TIME_BASE, AV_ROUND_NEAR_INF);
  if (starttime != 0)
  {
    pts += starttime;
  }

  return pts;
}
