/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SubtitlesStyle.h"

#include <memory>

class CDVDOverlay;
class CDVDSubtitlesLibass;

static constexpr int NO_SUBTITLE_ID = -1;

class CSubtitlesAdapter
{
public:
  CSubtitlesAdapter();
  ~CSubtitlesAdapter();

  /*!
  * \brief Initialize the subtitles adapter
  * \return True if success, false if error
  */
  bool Initialize();

  /*!
  * \brief Add a subtitle
  * \param startTime The PTS start time of the subtitle
  * \param stopTime The PTS stop time of the subtitle
  * \return Return the subtitle ID, otherwise NO_SUBTITLE_ID if fails
  */
  int AddSubtitle(const char* text, double startTime, double stopTime);

  /*!
  * \brief Append text to the specified subtitle ID
  * \param subtitleId The subtitle ID
  * \param text The text to append
  */
  void AppendToSubtitle(int subtitleId, const char* text);

  /*!
  * \brief Delete old subtitles only if the total number of subtitles added reaches the threshold
  * \param nSubtitles The number of subtitles to delete
  * \param threshold Start deleting only when the number of subtitles is reached
  * \return The updated ID of the last subtitle, otherwise NO_SUBTITLE_ID if error or no subtitles
  */
  int DeleteSubtitles(int nSubtitles, int threshold);

  /*!
  * \brief Change the stop time of a subtitle ID with the specified time
  * \param subtitleId The subtitle ID
  * \param stopTime The PTS stop time
  */
  void ChangeSubtitleStopTime(int subtitleId, double stopTime);

  void FlushSubtitles();

  CDVDOverlay* CreateOverlay();

private:
  std::shared_ptr<CDVDSubtitlesLibass> m_libass;
};
