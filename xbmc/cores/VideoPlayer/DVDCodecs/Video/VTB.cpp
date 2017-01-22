/*
 *      Copyright (C) 2005-2016 Team XBMC
 *      http://xbmc.org
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#include "platform/darwin/osx/CocoaInterface.h"
#include "platform/darwin/DarwinUtils.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "DVDVideoCodec.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "utils/log.h"
#include "VTB.h"
#include "utils/BitstreamConverter.h"
#include "utils/BitstreamReader.h"
#include "settings/Settings.h"
#include "ServiceBroker.h"

extern "C" {
#include "libavcodec/videotoolbox.h"
}

using namespace VTB;


CDecoder::CDecoder(CProcessInfo& processInfo) : m_processInfo(processInfo)
{
  m_avctx = nullptr;
}

CDecoder::~CDecoder()
{
  Close();
}

void CDecoder::Close()
{
  if (m_avctx)
  {
    av_videotoolbox_default_free(m_avctx);
    m_avctx = nullptr;
  }
}

bool CDecoder::Open(AVCodecContext *avctx, AVCodecContext* mainctx, enum AVPixelFormat fmt, unsigned int surfaces)
{
  if (!CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEVTB))
    return false;

  if (avctx->codec_id == AV_CODEC_ID_H264)
  {
    CBitstreamConverter bs;
    if (!bs.Open(avctx->codec_id, (uint8_t*)avctx->extradata, avctx->extradata_size, false))
    {
      return false;
    }
    CFDataRef avcCData = CFDataCreate(kCFAllocatorDefault,
                            (const uint8_t*)bs.GetExtraData(), bs.GetExtraSize());
    bool interlaced = true;
    int max_ref_frames;
    uint8_t *spc = (uint8_t*)CFDataGetBytePtr(avcCData) + 6;
    uint32_t sps_size = BS_RB16(spc);
    if (sps_size)
      bs.parseh264_sps(spc+3, sps_size-1, &interlaced, &max_ref_frames);
    CFRelease(avcCData);
    if (interlaced)
    {
      CLog::Log(LOGNOTICE, "%s - possible interlaced content.", __FUNCTION__);
      return false;
    }
  }

  if (av_videotoolbox_default_init(avctx) < 0)
    return false;

  m_avctx = avctx;

  mainctx->pix_fmt = fmt;
  mainctx->hwaccel_context = avctx->hwaccel_context;

  m_processInfo.SetVideoDeintMethod("none");

  std::list<EINTERLACEMETHOD> deintMethods;
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE);
  m_processInfo.UpdateDeinterlacingMethods(deintMethods);

  return true;
}

int CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  int status = Check(avctx);
  if(status)
    return status;

  if(frame)
  {
    m_renderPicture = (CVPixelBufferRef)frame->data[3];
    return VC_BUFFER | VC_PICTURE;
  }
  else
    return VC_BUFFER;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, DVDVideoPicture* picture)
{
  ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(picture);

  picture->format = RENDER_FMT_CVBREF;
  picture->cvBufferRef = m_renderPicture;
  return true;
}

int CDecoder::Check(AVCodecContext* avctx)
{
  return 0;
}

unsigned CDecoder::GetAllowedReferences()
{
  return 5;
}
