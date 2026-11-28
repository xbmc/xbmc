/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"

#include <memory>
#include <set>
#include <string>

class CVideoBufferPoolDMA;
class CVideoBufferPoolDRMPRIMEFFmpeg;

class CDVDVideoCodecDRMPRIME : public CDVDVideoCodec
{
public:
  explicit CDVDVideoCodecDRMPRIME(CProcessInfo& processInfo);
  ~CDVDVideoCodecDRMPRIME() override;

  static std::unique_ptr<CDVDVideoCodec> Create(CProcessInfo& processInfo);
  static void Register();

  bool Open(CDVDStreamInfo& hints, CDVDCodecOptions& options) override;
  bool AddData(const DemuxPacket& packet) override;
  void Reset() override;
  CDVDVideoCodec::VCReturn GetPicture(VideoPicture* pVideoPicture) override;
  void Reopen() override;
  const char* GetName() override { return m_name.c_str(); }
  unsigned GetConvergeCount() override { return m_pictureDecoded ? 0 : m_packetsSinceOpen; }
  unsigned GetAllowedReferences() override { return 5; }
  void SetCodecControl(int flags) override;

protected:
  void Drain();
  void SetPictureParams(VideoPicture* pVideoPicture);
  void UpdateProcessInfo(struct AVCodecContext* avctx, const enum AVPixelFormat fmt);
  static enum AVPixelFormat GetFormat(struct AVCodecContext* avctx, const enum AVPixelFormat* fmt);
  static int GetBuffer(struct AVCodecContext* avctx, AVFrame* frame, int flags);

  std::string m_name;
  int m_codecControlFlags = 0;
  CDVDStreamInfo m_hints;
  CDVDCodecOptions m_options;
  std::set<std::string> m_skipDecoders;
  bool m_hwDecoder = false;
  bool m_hwFormatOffered = false;
  bool m_hwFormatFailed = false;
  bool m_noSupportedFormat = false;
  unsigned m_packetsSinceOpen = 0;
  bool m_pictureDecoded = false;
  double m_DAR = 1.0;
  AVCodecContext* m_pCodecContext = nullptr;
  AVFrame* m_pFrame = nullptr;
  std::shared_ptr<CVideoBufferPoolDRMPRIMEFFmpeg> m_hwVideoBufferPool;
  std::shared_ptr<CVideoBufferPoolDMA> m_swVideoBufferPool;
};
