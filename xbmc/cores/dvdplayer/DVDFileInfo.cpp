/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "threads/SystemClock.h"
#include "DVDFileInfo.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "pictures/Picture.h"
#include "video/VideoInfoTag.h"
#include "filesystem/StackDirectory.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"

#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamBluray.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDDemuxers/DVDDemuxFFmpeg.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"

#include "DllAvCodec.h"
#include "DllSwScale.h"
#include "filesystem/File.h"


bool CDVDFileInfo::GetFileDuration(const CStdString &path, int& duration)
{
  std::auto_ptr<CDVDInputStream> input;
  std::auto_ptr<CDVDDemux> demux;

  input.reset(CDVDFactoryInputStream::CreateInputStream(NULL, path, ""));
  if (!input.get())
    return false;

  if (!input->Open(path, ""))
    return false;

  demux.reset(CDVDFactoryDemuxer::CreateDemuxer(input.get()));
  if (!demux.get())
    return false;

  duration = demux->GetStreamLength();
  if (duration > 0)
    return true;
  else
    return false;
}

bool CDVDFileInfo::ExtractThumb(const CStdString &strPath, const CStdString &strTarget, CStreamDetails *pStreamDetails)
{
  unsigned int nTime = XbmcThreads::SystemClockMillis();
  CDVDInputStream *pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, strPath, "");
  if (!pInputStream)
  {
    CLog::Log(LOGERROR, "InputStream: Error creating stream for %s", strPath.c_str());
    return false;
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CLog::Log(LOGERROR, "InputStream: dvd streams not supported for thumb extraction, file: %s", strPath.c_str());
    delete pInputStream;
    return false;
  }

  if (!pInputStream->Open(strPath.c_str(), ""))
  {
    CLog::Log(LOGERROR, "InputStream: Error opening, %s", strPath.c_str());
    if (pInputStream)
      delete pInputStream;
    return false;
  }

  CDVDDemux *pDemuxer = NULL;

  try
  {
    pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(pInputStream);
    if(!pDemuxer)
    {
      delete pInputStream;
      CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
      return false;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opening demuxer", __FUNCTION__);
    if (pDemuxer)
      delete pDemuxer;
    delete pInputStream;
    return false;
  }

  if (pStreamDetails)
    DemuxerToStreamDetails(pInputStream, pDemuxer, *pStreamDetails, strPath);

  CDemuxStream* pStream = NULL;
  int nVideoStream = -1;
  for (int i = 0; i < pDemuxer->GetNrOfStreams(); i++)
  {
    pStream = pDemuxer->GetStream(i);
    if (pStream)
    {
      if(pStream->type == STREAM_VIDEO)
        nVideoStream = i;
      else
        pStream->SetDiscard(AVDISCARD_ALL);
    }
  }

  bool bOk = false;
  if (nVideoStream != -1)
  {
    CDVDVideoCodec *pVideoCodec;

    CDVDStreamInfo hint(*pDemuxer->GetStream(nVideoStream), true);
    hint.software = true;

    if (hint.codec == CODEC_ID_MPEG2VIDEO || hint.codec == CODEC_ID_MPEG1VIDEO)
    {
      // libmpeg2 is not thread safe so use ffmepg for mpeg2/mpeg1 thumb extraction
      CDVDCodecOptions dvdOptions;
      pVideoCodec = CDVDFactoryCodec::OpenCodec(new CDVDVideoCodecFFmpeg(), hint, dvdOptions);
    }
    else
    {
      pVideoCodec = CDVDFactoryCodec::CreateVideoCodec( hint );
    }

    if (pVideoCodec)
    {
      int nTotalLen = pDemuxer->GetStreamLength();
      int nSeekTo = nTotalLen / 3;

      CLog::Log(LOGDEBUG,"%s - seeking to pos %dms (total: %dms) in %s", __FUNCTION__, nSeekTo, nTotalLen, strPath.c_str());
      if (pDemuxer->SeekTime(nSeekTo, true))
      {
        DemuxPacket* pPacket = NULL;
        int iDecoderState = VC_ERROR;
        DVDVideoPicture picture;

        // num streams * 40 frames, should get a valid frame, if not abort.
        int abort_index = pDemuxer->GetNrOfStreams() * 40;
        do
        {
          pPacket = pDemuxer->Read();
          if (!pPacket)
            break;

          if (pPacket->iStreamId != nVideoStream)
          {
            CDVDDemuxUtils::FreeDemuxPacket(pPacket);
            continue;
          }

          iDecoderState = pVideoCodec->Decode(pPacket->pData, pPacket->iSize, pPacket->dts, pPacket->pts);
          CDVDDemuxUtils::FreeDemuxPacket(pPacket);

          if (iDecoderState & VC_ERROR)
            break;

          if (iDecoderState & VC_PICTURE)
          {
            memset(&picture, 0, sizeof(DVDVideoPicture));
            if (pVideoCodec->GetPicture(&picture))
            {
              if(!(picture.iFlags & DVP_FLAG_DROPPED))
                break;
            }
          }

        } while (abort_index--);

        if (iDecoderState & VC_PICTURE && !(picture.iFlags & DVP_FLAG_DROPPED))
        {
          {
            int nWidth = g_advancedSettings.m_thumbSize;
            double aspect = (double)picture.iDisplayWidth / (double)picture.iDisplayHeight;
            if(hint.forced_aspect)
              aspect = hint.aspect;
            int nHeight = (int)((double)g_advancedSettings.m_thumbSize / aspect);

            DllSwScale dllSwScale;
            dllSwScale.Load();

            BYTE *pOutBuf = new BYTE[nWidth * nHeight * 4];
            struct SwsContext *context = dllSwScale.sws_getContext(picture.iWidth, picture.iHeight,
                  PIX_FMT_YUV420P, nWidth, nHeight, PIX_FMT_BGRA, SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);
            uint8_t *src[] = { picture.data[0], picture.data[1], picture.data[2], 0 };
            int     srcStride[] = { picture.iLineSize[0], picture.iLineSize[1], picture.iLineSize[2], 0 };
            uint8_t *dst[] = { pOutBuf, 0, 0, 0 };
            int     dstStride[] = { nWidth*4, 0, 0, 0 };

            if (context)
            {
              dllSwScale.sws_scale(context, src, srcStride, 0, picture.iHeight, dst, dstStride);
              dllSwScale.sws_freeContext(context);

              CPicture::CreateThumbnailFromSurface(pOutBuf, nWidth, nHeight, nWidth * 4, strTarget);
              bOk = true;
            }

            dllSwScale.Unload();
            delete [] pOutBuf;
          }
        }
        else
        {
          CLog::Log(LOGDEBUG,"%s - decode failed in %s", __FUNCTION__, strPath.c_str());
        }
      }
      delete pVideoCodec;
    }
  }

  if (pDemuxer)
    delete pDemuxer;

  delete pInputStream;

  if(!bOk)
  {
    XFILE::CFile file;
    if(file.OpenForWrite(strTarget))
      file.Close();
  }

  unsigned int nTotalTime = XbmcThreads::SystemClockMillis() - nTime;
  CLog::Log(LOGDEBUG,"%s - measured %u ms to extract thumb from file <%s> ", __FUNCTION__, nTotalTime, strPath.c_str());
  return bOk;
}

/**
 * \brief Open the item pointed to by pItem and extact streamdetails
 * \return true if the stream details have changed
 */
bool CDVDFileInfo::GetFileStreamDetails(CFileItem *pItem)
{
  if (!pItem)
    return false;

  CStdString strFileNameAndPath;
  if (pItem->HasVideoInfoTag())
    strFileNameAndPath = pItem->GetVideoInfoTag()->m_strFileNameAndPath;
  else
    return false;

  CStdString playablePath = strFileNameAndPath;
  if (URIUtils::IsStack(playablePath))
    playablePath = XFILE::CStackDirectory::GetFirstStackedFile(playablePath);

  CDVDInputStream *pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, playablePath, "");
  if (!pInputStream)
    return false;

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || !pInputStream->Open(playablePath.c_str(), ""))
  {
    delete pInputStream;
    return false;
  }

  CDVDDemux *pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(pInputStream);
  if (pDemuxer)
  {
    bool retVal = DemuxerToStreamDetails(pInputStream, pDemuxer, pItem->GetVideoInfoTag()->m_streamDetails, strFileNameAndPath);
    delete pDemuxer;
    delete pInputStream;
    return retVal;
  }
  else
  {
    delete pInputStream;
    return false;
  }
}

/* returns true if details have been added */
bool CDVDFileInfo::DemuxerToStreamDetails(CDVDInputStream *pInputStream, CDVDDemux *pDemux, CStreamDetails &details, const CStdString &path)
{
  bool retVal = false;
  details.Reset();

  for (int iStream=0; iStream<pDemux->GetNrOfStreams(); iStream++)
  {
    CDemuxStream *stream = pDemux->GetStream(iStream);
    if (stream->type == STREAM_VIDEO)
    {
      CStreamDetailVideo *p = new CStreamDetailVideo();
      p->m_iWidth = ((CDemuxStreamVideo *)stream)->iWidth;
      p->m_iHeight = ((CDemuxStreamVideo *)stream)->iHeight;
      p->m_fAspect = ((CDemuxStreamVideo *)stream)->fAspect;
      if (p->m_fAspect == 0.0f)
        p->m_fAspect = (float)p->m_iWidth / p->m_iHeight;
      pDemux->GetStreamCodecName(iStream, p->m_strCodec);
      p->m_iDuration = pDemux->GetStreamLength();

      // stack handling
      if (URIUtils::IsStack(path))
      {
        CFileItemList files;
        XFILE::CStackDirectory stack;
        stack.GetDirectory(path, files);

        // skip first path as we already know the duration
        for (int i = 1; i < files.Size(); i++)
        {
           int duration = 0;
           if (CDVDFileInfo::GetFileDuration(files[i]->GetPath(), duration))
             p->m_iDuration = p->m_iDuration + duration;
        }
      }

      // finally, calculate seconds
      if (p->m_iDuration > 0)
        p->m_iDuration = p->m_iDuration / 1000;

      details.AddStream(p);
      retVal = true;
    }

    else if (stream->type == STREAM_AUDIO)
    {
      CStreamDetailAudio *p = new CStreamDetailAudio();
      p->m_iChannels = ((CDemuxStreamAudio *)stream)->iChannels;
      if (stream->language)
        p->m_strLanguage = stream->language;
      pDemux->GetStreamCodecName(iStream, p->m_strCodec);
      details.AddStream(p);
      retVal = true;
    }

    else if (stream->type == STREAM_SUBTITLE)
    {
      if (stream->language)
      {
        CStreamDetailSubtitle *p = new CStreamDetailSubtitle();
        p->m_strLanguage = stream->language;
        details.AddStream(p);
        retVal = true;
      }
    }
  }  /* for iStream */

  details.DetermineBestStreams();

  // correct bluray runtime. we need the duration from the input stream, not the demuxer.
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_BLURAY))
  {
    if(((CDVDInputStreamBluray*)pInputStream)->GetTotalTime() > 0)
    {
      ((CStreamDetailVideo*)details.GetNthStream(CStreamDetail::VIDEO,0))->m_iDuration = ((CDVDInputStreamBluray*)pInputStream)->GetTotalTime() / 1000;
    }
  }

  return retVal;
}

