/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDVideoCodecFFmpeg.h"

struct AVBufferRef;
struct AVFrame;

class CDVDVideoCodecNVDEC : public CDVDVideoCodecFFmpeg
{
public:
  explicit CDVDVideoCodecNVDEC(CProcessInfo& processInfo);
  ~CDVDVideoCodecNVDEC() override;

  static std::unique_ptr<CDVDVideoCodec> Create(CProcessInfo& processInfo);
  static void Register();

  bool Open(CDVDStreamInfo& hints, CDVDCodecOptions& options) override;
  void Reopen() override;
  CDVDVideoCodec::VCReturn GetPicture(VideoPicture* pVideoPicture) override;

private:
  static enum AVPixelFormat GetFormatNVDEC(AVCodecContext* avctx, const enum AVPixelFormat* fmts);

  AVBufferRef* m_hwDeviceRef = nullptr;
  AVFrame* m_swFrame = nullptr;
};
