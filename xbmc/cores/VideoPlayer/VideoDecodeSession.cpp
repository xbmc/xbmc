/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDecodeSession.h"

#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "FileItem.h"
#include "Process/ProcessInfo.h"
#include "utils/log.h"

#include <vector>

std::optional<VideoDecodeSession> OpenVideoDecodeSession(const CFileItem& fileItem,
                                                         int codecOptions,
                                                         const std::string& redactPath)
{
  CFileItem item(fileItem);
  item.SetMimeTypeForInternetFile();

  VideoDecodeSession session;
  session.inputStream = CDVDFactoryInputStream::CreateInputStream(nullptr, item);
  if (!session.inputStream || !session.inputStream->Open())
  {
    CLog::Log(LOGERROR, "VideoDecodeSession: error opening input stream for {}", redactPath);
    return std::nullopt;
  }

  session.demuxer.reset(CDVDFactoryDemuxer::CreateDemuxer(session.inputStream, true));
  if (!session.demuxer)
  {
    CLog::Log(LOGERROR, "VideoDecodeSession: error creating demuxer for {}", redactPath);
    return std::nullopt;
  }

  for (CDemuxStream* stream : session.demuxer->GetStreams())
  {
    if (!stream)
      continue;

    // The first video stream, which is the base layer in Dolby Vision dual-track files.
    if (stream->type == StreamType::VIDEO && !(stream->flags & AV_DISPOSITION_ATTACHED_PIC) &&
        session.videoStream == -1)
    {
      session.videoStream = stream->uniqueId;
      session.demuxerId = stream->demuxerId;
    }
    else
      session.demuxer->EnableStream(stream->demuxerId, stream->uniqueId, false);
  }

  if (session.videoStream == -1)
    return std::nullopt;

  session.processInfo.reset(CProcessInfo::CreateInstance());
  std::vector<AVPixelFormat> pixFmts{AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P10,
                                     AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV422P10,
                                     AV_PIX_FMT_YUV444P, AV_PIX_FMT_YUV444P10};
  session.processInfo->SetPixFormats(pixFmts);

  session.hint.Assign(*session.demuxer->GetStream(session.demuxerId, session.videoStream), true);
  session.hint.codecOptions = codecOptions;

  session.codec = CDVDFactoryCodec::CreateVideoCodec(session.hint, *session.processInfo);
  if (!session.codec)
  {
    CLog::Log(LOGERROR, "VideoDecodeSession: error creating video codec for {}", redactPath);
    return std::nullopt;
  }

  return session;
}

bool SeekAndDecodePictureAt(CDVDDemux& demuxer,
                            CDVDVideoCodec& codec,
                            int videoStream,
                            int64_t seekToMs,
                            const std::string& redactPath,
                            VideoPicture& picture,
                            int& packetsTried)
{
  CLog::LogF(LOGDEBUG, "seeking to pos {}ms (total: {}ms) in {}", seekToMs,
             demuxer.GetStreamLength(), redactPath);

  if (!demuxer.SeekTime(static_cast<double>(seekToMs), true))
    return false;

  CDVDVideoCodec::VCReturn iDecoderState = CDVDVideoCodec::VC_NONE;

  // The packets of the other streams are dropped, so only the video packets count against the
  // budget: lossless audio alone brings dozens per video frame. The amount read is limited too.
  constexpr int MAX_VIDEO_PACKETS = 640;
  constexpr int64_t MAX_READ_BYTES = 64 * 1024 * 1024;

  int videoPacketsLeft = MAX_VIDEO_PACKETS;
  int64_t bytesLeft = MAX_READ_BYTES;
  bool keyFrameSeen = !demuxer.ReportsKeyFrames();
  while (videoPacketsLeft > 0 && bytesLeft > 0)
  {
    DemuxPacket* pPacket = demuxer.Read();
    packetsTried++;

    if (!pPacket)
      break;

    bytesLeft -= pPacket->iSize;

    if (pPacket->iStreamId != videoStream)
    {
      // A read that returned nothing counts like a video packet, so that a stalled source ends too
      if (pPacket->iSize == 0)
        --videoPacketsLeft;

      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      continue;
    }

    --videoPacketsLeft;

    // A seek can land between keyframes, nothing before the first one can be decoded
    keyFrameSeen = keyFrameSeen || pPacket->m_keyFrame;
    if (!keyFrameSeen)
    {
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      continue;
    }

    codec.AddData(*pPacket);
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);

    // A filter holding a frame back answers VC_NONE, so the codec is not asked for ever.
    iDecoderState = CDVDVideoCodec::VC_NONE;
    for (int asks = 50; asks > 0 && iDecoderState == CDVDVideoCodec::VC_NONE; --asks)
      iDecoderState = codec.GetPicture(&picture);

    if (iDecoderState == CDVDVideoCodec::VC_PICTURE)
    {
      if (!(picture.iFlags & DVP_FLAG_DROPPED))
        break;
    }
  }

  return iDecoderState == CDVDVideoCodec::VC_PICTURE && !(picture.iFlags & DVP_FLAG_DROPPED);
}
