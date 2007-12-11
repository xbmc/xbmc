/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifdef _LINUX
#include "stdint.h"
#else
#define INT64_C __int64
#endif

#include "stdafx.h"
#include "ThumbLoader.h"
#include "Util.h"
#include "Picture.h"

#include "cores/ffmpeg/DllAvFormat.h"
#include "cores/ffmpeg/DllAvCodec.h"
#include "cores/ffmpeg/DllSwScale.h"

#define THUMB_WIDTH 256
#define THUMB_HEIGHT 144

using namespace XFILE;

CVideoThumbLoader::CVideoThumbLoader() 
{  
}

CVideoThumbLoader::~CVideoThumbLoader()
{
  StopThread();
}

void CVideoThumbLoader::OnLoaderStart() 
{
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllAvFormat.Load() || !m_dllSwScale.Load())  
  {
    CLog::Log(LOGERROR,"%s - failed to load ffmpeg lib", __FUNCTION__);
    return;
  }

  m_dllAvFormat.av_register_all();
  m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
}

void CVideoThumbLoader::OnLoaderFinish() 
{
  m_dllAvFormat.Unload();
  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
  m_dllSwScale.Unload();
}

bool CVideoThumbLoader::ExtractThumb(const CStdString &strPath, const CStdString &strTarget)
{
  CLog::Log(LOGDEBUG,"%s - trying to extract thumb from video file %s", __FUNCTION__, strPath.c_str());

  AVFormatContext *pFormatContext=NULL;

  if (m_dllAvFormat.av_open_input_file(&pFormatContext, strPath, 0, FFM_PACKET_SIZE, NULL) < 0)
  {
    CLog::Log(LOGERROR,"%s- failed to open input file %s", __FUNCTION__, strPath.c_str());
    return false;
  }

  if (m_dllAvFormat.av_find_stream_info(pFormatContext) < 0 )
  {
    CLog::Log(LOGERROR,"%s- failed to find stream info for %s", __FUNCTION__, strPath.c_str());
    m_dllAvFormat.av_close_input_file(pFormatContext);
    return false;
  }

  int nVideoStream = -1;
  for (unsigned int i = 0; i < pFormatContext->nb_streams; i++)
  {
    if (pFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
    {
      nVideoStream = i;
      break;
    }
  }

  if (nVideoStream < 0)
  {
    CLog::Log(LOGERROR,"%s- failed to find video stream for %s", __FUNCTION__, strPath.c_str());
    m_dllAvFormat.av_close_input_file(pFormatContext);
    return false;
  }

  AVStream *pStream = pFormatContext->streams[nVideoStream];
  
  int nOffset = 120; 
  int nDuration = pFormatContext->duration / AV_TIME_BASE;
  if (nOffset > nDuration / 2) // in case it is a short video - take a frame from the middle.
    nOffset = nDuration / 2;

  if (m_dllAvFormat.av_seek_frame(pFormatContext, -1, nOffset * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD) < 0)
  {
    CLog::Log(LOGERROR,"%s- failed to seek to %d in %s", __FUNCTION__, nOffset, strPath.c_str());
    m_dllAvFormat.av_close_input_file(pFormatContext);
    return false;
  }

  AVPacket pkt;
  AVFrame pic;
  int nHasPic;

  int result = m_dllAvFormat.av_read_frame(pFormatContext, &pkt);
  if (result < 0)
  {
    CLog::Log(LOGERROR,"%s- failed to read frame from %s", __FUNCTION__, strPath.c_str());
    m_dllAvFormat.av_close_input_file(pFormatContext);
    av_free_packet(&pkt);
    return false;
  }

  AVCodec *pCodec = m_dllAvCodec.avcodec_find_decoder(pStream->codec->codec_id);
  pStream->codec->workaround_bugs |= FF_BUG_AUTODETECT;
  if(pCodec==NULL || m_dllAvCodec.avcodec_open(pStream->codec, pCodec)<0) 
  {
    CLog::Log(LOGERROR,"%s- failed to open codec for %s", __FUNCTION__, strPath.c_str());
    m_dllAvFormat.av_close_input_file(pFormatContext);
    av_free_packet(&pkt);
    return false;
  }

  memset(&pic, 0, sizeof(AVFrame));
  pic.pts = AV_NOPTS_VALUE;
  pic.key_frame = 1;
  result = m_dllAvCodec.avcodec_decode_video(pStream->codec, &pic, &nHasPic, pkt.data, pkt.size);
  if (result < 0)
  {
    CLog::Log(LOGERROR,"%s- failed to open codec for %s", __FUNCTION__, strPath.c_str());
    m_dllAvFormat.av_close_input_file(pFormatContext);
    av_free_packet(&pkt);
    return false;
  }

  if (!nHasPic)
  {
    // in some cases data is gathered by the decoder. try and spew it out.
    result = m_dllAvCodec.avcodec_decode_video(pStream->codec, &pic, &nHasPic, NULL, 0);
  }

  if (result < 0 || !nHasPic)
  {
    m_dllAvFormat.av_close_input_file(pFormatContext);
    av_free_packet(&pkt);
    return false;
  }

  BYTE *pOutBuf = (BYTE*)new int[THUMB_WIDTH * THUMB_HEIGHT * 4];
  struct SwsContext *context = m_dllSwScale.sws_getContext(pStream->codec->width, pStream->codec->height, 
     pStream->codec->pix_fmt, THUMB_WIDTH, THUMB_HEIGHT, PIX_FMT_RGB32, SWS_BILINEAR, NULL, NULL, NULL);
  uint8_t *src[] = { pic.data[0], pic.data[1], pic.data[2] };
  int     srcStride[] = { pic.linesize[0], pic.linesize[1], pic.linesize[2] };
  uint8_t *dst[] = { pOutBuf, 0, 0 };
  int     dstStride[] = { THUMB_WIDTH*4, 0, 0 };

  if (context)
  {
    m_dllSwScale.sws_scale(context, src, srcStride, 0, pStream->codec->height, dst, dstStride);  
    m_dllSwScale.sws_freeContext(context);

    CPicture out;
    out.CreateThumbnailFromSurface(pOutBuf, THUMB_WIDTH, THUMB_HEIGHT, THUMB_WIDTH * 4, strTarget);
  }

  delete [] pOutBuf;
  av_free_packet(&pkt);
  m_dllAvCodec.avcodec_close(pStream->codec);
  m_dllAvFormat.av_close_input_file(pFormatContext);
  return context != NULL;
}

bool CVideoThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  CStdString cachedThumb(pItem->GetCachedVideoThumb());

  if (!pItem->HasThumbnail())
  {
    if (pItem->IsVideo() && !pItem->IsInternetStream() && !CFile::Exists(cachedThumb))
      CVideoThumbLoader::ExtractThumb(pItem->m_strPath, cachedThumb);
    pItem->SetUserVideoThumb();
  }
  else
  {
    // look for remote thumbs
    CStdString thumb(pItem->GetThumbnailImage());
    if (!CURL::IsFileOnly(thumb) && !CUtil::IsHD(thumb))
    {      
      if(CFile::Exists(cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
      else
      {
        CPicture pic;
        if(pic.DoCreateThumbnail(thumb, cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
        else
          pItem->SetThumbnailImage("");
      }
    }  
  }

  return true;
}

CProgramThumbLoader::CProgramThumbLoader()
{
}

CProgramThumbLoader::~CProgramThumbLoader()
{
}

bool CProgramThumbLoader::LoadItem(CFileItem *pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  if (!pItem->HasThumbnail())
    pItem->SetUserProgramThumb();
  return true;
}

CMusicThumbLoader::CMusicThumbLoader()
{
}

CMusicThumbLoader::~CMusicThumbLoader()
{
}

bool CMusicThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  if (!pItem->HasThumbnail())
    pItem->SetUserMusicThumb();
  else
  {
    // look for remote thumbs
    CStdString thumb(pItem->GetThumbnailImage());
    if (!CURL::IsFileOnly(thumb) && !CUtil::IsHD(thumb))
    {
      CStdString cachedThumb(pItem->GetCachedVideoThumb());
      if(CFile::Exists(cachedThumb))
        pItem->SetThumbnailImage(cachedThumb);
      else
      {
        CPicture pic;
        if(pic.DoCreateThumbnail(thumb, cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
        else
          pItem->SetThumbnailImage("");
      }
    }  
  }
  return true;
}

