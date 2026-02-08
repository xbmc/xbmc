/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDVideoCodec.h"
#include "DVDVideoPP.h"

extern "C"
{
#include <libpostproc/postprocess.h>
}

#include <string>

class CProcessInfo;

class CDVDVideoPPFFmpeg : public IDVDVideoPP
{
public:

  explicit CDVDVideoPPFFmpeg(CProcessInfo &processInfo);
  ~CDVDVideoPPFFmpeg() override;

  void SetType(const std::string& mType, bool deinterlace) override;
  void Process(VideoPicture* picture) override;

protected:
  std::string m_sType;
  CProcessInfo &m_processInfo;

  void *m_pContext;
  void *m_pMode;
  bool m_deinterlace;

  void Dispose();

  int m_iInitWidth, m_iInitHeight;
  bool CheckInit(int iWidth, int iHeight);
  bool CheckFrameBuffer(const VideoPicture* pSource);
};


