/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDecodeSession.h"

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

std::optional<DVDDecodeSession> OpenDVDDecodeSession(const CFileItem& fileItem,
                                                     int codecOptions,
                                                     const std::string& redactPath)
{
  CFileItem item(fileItem);
  item.SetMimeTypeForInternetFile();

  DVDDecodeSession session;
  session.inputStream = CDVDFactoryInputStream::CreateInputStream(nullptr, item);
  if (!session.inputStream || !session.inputStream->Open())
  {
    CLog::Log(LOGERROR, "DVDDecodeSession: error opening input stream for {}", redactPath);
    return std::nullopt;
  }

  session.demuxer.reset(CDVDFactoryDemuxer::CreateDemuxer(session.inputStream, true));
  if (!session.demuxer)
  {
    CLog::Log(LOGERROR, "DVDDecodeSession: error creating demuxer for {}", redactPath);
    return std::nullopt;
  }

  for (CDemuxStream* stream : session.demuxer->GetStreams())
  {
    if (!stream)
      continue;

    // Ignore picture attachments; assume the first video stream, which is the base layer
    // in Dolby Vision dual-track files.
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
    CLog::Log(LOGERROR, "DVDDecodeSession: error creating video codec for {}", redactPath);
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

  // num streams * 160 frames, should get a valid frame, if not abort.
  for (int attemptsLeft = demuxer.GetNrOfStreams() * 160; attemptsLeft >= 0; attemptsLeft--)
  {
    DemuxPacket* pPacket = demuxer.Read();
    packetsTried++;

    if (!pPacket)
      break;

    if (pPacket->iStreamId != videoStream)
    {
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      continue;
    }

    codec.AddData(*pPacket);
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);

    iDecoderState = CDVDVideoCodec::VC_NONE;
    while (iDecoderState == CDVDVideoCodec::VC_NONE)
    {
      iDecoderState = codec.GetPicture(&picture);
    }

    if (iDecoderState == CDVDVideoCodec::VC_PICTURE)
    {
      if (!(picture.iFlags & DVP_FLAG_DROPPED))
        break;
    }
  }

  return iDecoderState == CDVDVideoCodec::VC_PICTURE && !(picture.iFlags & DVP_FLAG_DROPPED);
}
