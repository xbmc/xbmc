/*
*      Copyright (C) 2017 Team Kodi
*      https://kodi.tv
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#ifndef ODBSETTING_H
#define ODBSETTING_H

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

#include <string>

#include "ODBFile.h"

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
table("settings"))
class CODBSetting
{
public:
  CODBSetting()
  {
    m_deinterlace = false;
    m_viewMode = 0;
    m_zoomAmount = 0.0;
    m_pixelRatio = 0.0;
    m_verticalShift = 0.0;
    m_audioStream = 0;
    m_subtitleStream = 0;
    m_subtitleDelay = 0.0;
    m_subtitlesOn = false;
    m_brightness = 0.0;
    m_contrast  = 0.0;
    m_gamma = 0.0;
    m_volumeAmplification = 0.0;
    m_audioDelay = 0.0;
    m_outputToAllSpeakers = false;
    m_resumeTime = 0;
    m_sharpness = 0.0;
    m_noiseReduction = 0.0;
    m_nonLinStretch = false;
    m_postProcess = false;
    m_scalingMethod = 0;
    m_deinterlaceMode = 0;
    m_stereoMode = 0;
    m_stereoInvert = false;
    m_videoStream = 0;
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idSetting;
  odb::lazy_shared_ptr<CODBFile> m_file;
  bool m_deinterlace;
  int m_viewMode;
  float m_zoomAmount;
  float m_pixelRatio;
  float m_verticalShift;
  int m_audioStream;
  int m_subtitleStream;
  float m_subtitleDelay;
  bool m_subtitlesOn;
  float m_brightness;
  float m_contrast;
  float m_gamma;
  float m_volumeAmplification;
  float m_audioDelay;
  bool m_outputToAllSpeakers;
  int m_resumeTime;
  float m_sharpness;
  float m_noiseReduction;
  bool m_nonLinStretch;
  bool m_postProcess;
  int m_scalingMethod;
  int m_deinterlaceMode;
  int m_stereoMode;
  bool m_stereoInvert;
  int m_videoStream;
  
private:
  friend class odb::access;
  
PRAGMA_DB (index member(m_file))
  
};

PRAGMA_DB (view object(CODBSetting) \
                object(CODBFile: CODBSetting::m_file) \
                object(CODBPath: CODBFile::m_path) \
                query(distinct))
struct ODBView_Setting_Paths
{
  std::shared_ptr<CODBSetting> setting;
};

#endif /* ODBSETTING_H */
