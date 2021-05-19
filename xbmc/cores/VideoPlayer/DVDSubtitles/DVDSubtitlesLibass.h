/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <ass/ass.h>

/** Wrapper for Libass **/

class CDVDSubtitlesLibass
{
public:
  CDVDSubtitlesLibass();
  ~CDVDSubtitlesLibass();

  /*!
  * \brief Configure libass. This method groups any configurations
  * that might change throughout the lifecycle of libass (e.g. fonts)
  */
  void Configure();

  ASS_Image* RenderImage(int frameWidth, int frameHeight, int videoWidth, int videoHeight, int sourceWidth, int sourceHeight,
                         double pts, int useMargin = 0, double position = 0.0, int* changes = NULL);
  ASS_Event* GetEvents();

  /*!
  * \brief Get the number of events (subtitle entries) in the ASS track
  * \return The number of events in the ASS track
  */
  int GetNrOfEvents() const;

  bool DecodeHeader(char* data, int size);
  bool DecodeDemuxPkt(const char* data, int size, double start, double duration);
  bool CreateTrack(char* buf, size_t size);

private:
  ASS_Library* m_library = nullptr;
  ASS_Track* m_track = nullptr;
  ASS_Renderer* m_renderer = nullptr;
  mutable CCriticalSection m_section;
};

