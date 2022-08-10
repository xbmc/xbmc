/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SubtitlesStyle.h"
#include "threads/CriticalSection.h"
#include "utils/ColorUtils.h"

#include <memory>

#include <ass/ass.h>
#include <ass/ass_types.h>

/** Wrapper for Libass **/

static constexpr int ASS_NO_ID = -1;

enum ASSSubType
{
  NATIVE = 0,
  ADAPTED
};

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

  ASS_Image* RenderImage(double pts,
                         KODI::SUBTITLES::STYLE::renderOpts opts,
                         bool updateStyle,
                         const std::shared_ptr<struct KODI::SUBTITLES::STYLE::style>& subStyle,
                         int* changes = NULL);

  ASS_Event* GetEvents();

  /*!
  * \brief Get the number of events (subtitle entries) in the ASS track
  * \return The number of events in the ASS track
  */
  int GetNrOfEvents() const;

  /*!
  * \brief Decode Header of ASS/SSA, needed to properly decode
  * demux packets with DecodeDemuxPkt
  * \return True if success, false if error
  */
  bool DecodeHeader(char* data, int size);

  /*!
  * \brief Decode ASS/SSA demux packet (depends from DecodeHeader)
  * \return True if success, false if error
  */
  bool DecodeDemuxPkt(const char* data, int size, double start, double duration);

  /*!
  * \brief Create a new ASS track based on an SSA buffer
  * \return True if success, false if error
  */
  bool CreateTrack(char* buf, size_t size);

  /*!
  * \brief Flush buffered events
  */
  void FlushEvents();

  /*!
  * \brief Get PlayResY value
  * \return The PlayResY value of current track
  */
  int GetPlayResY();

protected:
  /*!
  * \brief Create a new empty ASS track
  * \return True if success, false if error
  */
  bool CreateTrack();

  /*!
  * \brief Create a new empty ASS style
  * \return True if success, false if error
  */
  bool CreateStyle();

  /*!
  * \brief Specify whether the subtitles are
  * native (loaded from ASS/SSA file or stream)
  * or adapted (converted from other types e.g. SubRip)
  */
  void SetSubtitleType(ASSSubType type) { m_subtitleType = type; }

  /*!
  * \brief Add an ASS event to show a subtitle on a specified time
  * \param text The subtitle text
  * \param startTime The PTS start time of the Event
  * \param stopTime The PTS stop time of the Event
  * \return Return the Event ID, otherwise ASS_NO_ID if fails
  */
  int AddEvent(const char* text, double startTime, double stopTime);

  /*!
  * \brief Add an ASS event to show a subtitle on a specified time
  * \param text The subtitle text
  * \param startTime The PTS start time of the Event
  * \param stopTime The PTS stop time of the Event
  * \param opts Subtitle options
  * \return Return the Event ID, otherwise ASS_NO_ID if fails
  */
  int AddEvent(const char* text,
               double startTime,
               double stopTime,
               KODI::SUBTITLES::STYLE::subtitleOpts* opts);

  /*!
  * \brief Append text to the specified event
  */
  void AppendTextToEvent(int eventId, const char* text);

  /*!
  * \brief Delete old events only if the total number of events reaches the threshold
  * \param nEvents The number of events to delete
  * \param threshold Start deleting only when the number of events is reached
  * \return The updated ID of the last Event, otherwise ASS_NO_ID if error or no events
  */
  int DeleteEvents(int nEvents, int threshold);

  /*!
  * \brief Change the stop time of an Event with the specified time
  * \param eventId The ASS Event ID
  * \param stopTime The PTS stop time
  */
  void ChangeEventStopTime(int eventId, double stopTime);

  friend class CSubtitlesAdapter;


private:
  void ConfigureAssOverride(const std::shared_ptr<struct KODI::SUBTITLES::STYLE::style>& subStyle,
                            ASS_Style* style);
  void ApplyStyle(const std::shared_ptr<struct KODI::SUBTITLES::STYLE::style>& subStyle,
                  KODI::SUBTITLES::STYLE::renderOpts opts);

  ASS_Library* m_library = nullptr;
  ASS_Track* m_track = nullptr;
  ASS_Renderer* m_renderer = nullptr;
  mutable CCriticalSection m_section;
  ASSSubType m_subtitleType{NATIVE};

  // current default style ID of the ASS track
  int m_currentDefaultStyleId{ASS_NO_ID};

  // default allocated style ID for the kodi user configured subtitle style
  int m_defaultKodiStyleId{ASS_NO_ID};
  std::string m_defaultFontFamilyName;
};
