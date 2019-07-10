/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDResource.h"
#include "threads/CriticalSection.h"

#include <ass/ass.h>

/** Wrapper for Libass **/

class CDVDSubtitlesLibass : public IDVDResourceCounted<CDVDSubtitlesLibass>
{
public:
  CDVDSubtitlesLibass();
  ~CDVDSubtitlesLibass() override;

  ASS_Image* RenderImage(int frameWidth, int frameHeight, int videoWidth, int videoHeight, int sourceWidth, int sourceHeight,
                         double pts, int useMargin = 0, double position = 0.0, int* changes = NULL);
  ASS_Event* GetEvents();

  int GetNrOfEvents();

  bool DecodeHeader(char* data, int size);
  bool DecodeDemuxPkt(const char* data, int size, double start, double duration);
  bool CreateTrack(char* buf, size_t size);

private:
  ASS_Library* m_library = nullptr;
  ASS_Track* m_track = nullptr;
  ASS_Renderer* m_renderer = nullptr;
  CCriticalSection m_section;
};

