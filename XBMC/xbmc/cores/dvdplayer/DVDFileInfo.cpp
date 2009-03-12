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

#include "stdafx.h"
#include "FileItem.h"
#include "Settings.h"
#include "Picture.h"


#include "DVDFileInfo.h"
#include "DVDStreamInfo.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDDemuxers/DVDDemuxFFmpeg.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"

#include "../ffmpeg/DllAvFormat.h"
#include "../ffmpeg/DllAvCodec.h"
#include "../ffmpeg/DllSwScale.h"


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

bool CDVDFileInfo::ExtractThumb(const CStdString &strPath, const CStdString &strTarget)
{
  int nTime = timeGetTime();
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
    CLog::Log(LOGERROR, "%s - Exception thrown when opeing demuxer", __FUNCTION__);
    if (pDemuxer)
      delete pDemuxer;
    delete pInputStream;
    return false;
  }

  CDemuxStream* pStream = NULL;
  int nVideoStream = -1;
  for (int i = 0; i < pDemuxer->GetNrOfStreams(); i++)
  {
    pStream = pDemuxer->GetStream(i);
    if (pStream && pStream->type == STREAM_VIDEO)
    {
      nVideoStream = i;
      break;
    }
  }

  bool bOk = false;
  if (nVideoStream != -1)
  {
    CDVDStreamInfo hint(*pStream, true);
    hint.RequestThumbnail = true;
    CDVDVideoCodec *pVideoCodec = CDVDFactoryCodec::CreateVideoCodec( hint );
    if (pVideoCodec)
    {
      int nTotalLen = pDemuxer->GetStreamLength();
      int nSeekTo = nTotalLen / 3;

      CLog::Log(LOGDEBUG,"%s - seeking to pos %dms (total: %dms) in %s", __FUNCTION__, nSeekTo, nTotalLen, strPath.c_str());
      if (pDemuxer->SeekTime(nSeekTo, true))
      {
        DemuxPacket* pPacket = NULL;
  
        bool bHasFrame = false;
        while (!bHasFrame)
        {
          bool bFound = false;
          do
          {
            pPacket = pDemuxer->Read();
            if (pPacket)
            {
              if (pPacket->iStreamId == nVideoStream)
                bFound = true;
              else
                CDVDDemuxUtils::FreeDemuxPacket(pPacket);
            }
            else
              break;
          }   while (!bFound);

          if (pPacket)
          {
            int iDecoderState = pVideoCodec->Decode(pPacket->pData, pPacket->iSize, pPacket->pts);
            CDVDDemuxUtils::FreeDemuxPacket(pPacket);

            if (iDecoderState & VC_PICTURE)
            {
              bHasFrame = true;
              DVDVideoPicture picture;
              memset(&picture, 0, sizeof(DVDVideoPicture));
              if (pVideoCodec->GetPicture(&picture))
              {
                int nWidth = g_advancedSettings.m_thumbSize;
                double aspect = (double)picture.iWidth / (double)picture.iHeight;
                int nHeight = (int)((double)g_advancedSettings.m_thumbSize / aspect);

                DllSwScale dllSwScale;
                dllSwScale.Load();

                BYTE *pOutBuf = (BYTE*)new int[nWidth * nHeight * 4];
                struct SwsContext *context = dllSwScale.sws_getContext(picture.iWidth, picture.iHeight, 
                      PIX_FMT_YUV420P, nWidth, nHeight, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);
                uint8_t *src[] = { picture.data[0], picture.data[1], picture.data[2] };
                int     srcStride[] = { picture.iLineSize[0], picture.iLineSize[1], picture.iLineSize[2] };
                uint8_t *dst[] = { pOutBuf, 0, 0 };
                int     dstStride[] = { nWidth*4, 0, 0 };

                if (context)
                {
                  dllSwScale.sws_scale(context, src, srcStride, 0, picture.iHeight, dst, dstStride);  
                  dllSwScale.sws_freeContext(context);

                  CPicture out;
                  out.CreateThumbnailFromSurface(pOutBuf, nWidth, nHeight, nWidth * 4, strTarget);
                  bOk = true; 
                }

                dllSwScale.Unload();                
                delete [] pOutBuf;
              }
              else 
              {
                CLog::Log(LOGDEBUG,"%s - coudln't get picture from decoder in  %s", __FUNCTION__, strPath.c_str());
              }
            }
          }
          else 
          {
            CLog::Log(LOGDEBUG,"%s - decode failed in %s", __FUNCTION__, strPath.c_str());
            break;
          }
 
        }
      }
      delete pVideoCodec;
    }
  }

  if (pDemuxer)
    delete pDemuxer;

  delete pInputStream;

  int nTotalTime = timeGetTime() - nTime;
  CLog::Log(LOGDEBUG,"%s - measured %d ms to extract thumb from file <%s> ", __FUNCTION__, nTotalTime, strPath.c_str());
  return bOk;
}


void CDVDFileInfo::GetFileMetaData(const CStdString &strPath, CFileItem *pItem)
{
  if (!pItem)
    return;

  CDVDInputStream *pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, strPath, "");
  if (!pInputStream)
  {
    CLog::Log(LOGERROR, "%s - Error creating stream for %s", __FUNCTION__, strPath.c_str());
    return ;
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || !pInputStream->Open(strPath.c_str(), ""))
  {
    CLog::Log(LOGERROR, "%s - invalid stream in %s", __FUNCTION__, strPath.c_str());
    delete pInputStream;
    return ;
  }

  CDVDDemuxFFmpeg *pDemuxer = new CDVDDemuxFFmpeg;

  try
  {
    if (!pDemuxer->Open(pInputStream))
    {
      CLog::Log(LOGERROR, "%s - Error opening demuxer", __FUNCTION__);
      delete pDemuxer;
      delete pInputStream;
      return ;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opeing demuxer", __FUNCTION__);
    if (pDemuxer)
      delete pDemuxer;
    delete pInputStream;
    return ;
  }

  AVFormatContext *pContext = pDemuxer->m_pFormatContext; 
  if (pContext)
  {
    int nLenMsec = pDemuxer->GetStreamLength();
    CStdString strDuration;
    int nHours = nLenMsec / 1000 / 60 / 60;
    int nMinutes = ((nLenMsec / 1000) - nHours * 3600) / 60;
    int nSec = (nLenMsec / 1000)  - nHours * 3600 - nMinutes * 60;
    strDuration.Format("%d", nLenMsec);
    pItem->SetProperty("duration-msec", strDuration);
    strDuration.Format("%02d:%02d:%02d", nHours, nMinutes, nSec);
    pItem->SetProperty("duration-str", strDuration);
    pItem->SetProperty("title", pContext->title);
    pItem->SetProperty("author", pContext->author);
    pItem->SetProperty("copyright", pContext->copyright);
    pItem->SetProperty("comment", pContext->comment);
    pItem->SetProperty("album", pContext->album);
    strDuration.Format("%d", pContext->year);
    pItem->SetProperty("year", strDuration);
    strDuration.Format("%d", pContext->track);
    pItem->SetProperty("track", strDuration);
    pItem->SetProperty("genre", pContext->genre);
  }

  delete pDemuxer;
  pInputStream->Close();
  delete pInputStream;
  
}
