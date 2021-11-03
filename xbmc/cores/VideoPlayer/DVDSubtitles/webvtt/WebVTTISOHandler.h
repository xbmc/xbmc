/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WebVTTHandler.h"

class CCharArrayParser;


class CWebVTTISOHandler : public CWebVTTHandler
{
public:
  CWebVTTISOHandler(){};
  ~CWebVTTISOHandler(){};

  /*!
  * \brief Decode a stream package of the WebVTT in MP4 encapsulated subtitles
  *        (ISO/IEC 14496-30:2014)
  * \param buffer The data buffer
  * \param bufferSize The buffer size
  * \param subList The list to be filled with decoded subtitles
  * \param[out] prevSubStopTime Provide the stop time value (depends on box type)
  */
  void DecodeStream(const char* buffer,
                    int bufferSize,
                    double pts,
                    std::vector<subtitleData>* subList,
                    double& prevSubStopTime);

private:
  bool ParseVTTCueBox(CCharArrayParser& sampleData,
                      int remainingCueBoxChars,
                      std::vector<subtitleData>* subList);
};
