#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "settings/dialogs/GUIDialogSettings.h"
typedef std::vector<int> Features;

class CGUIDialogAudioSubtitleSettings :
      public CGUIDialogSettings
{
public:
  CGUIDialogAudioSubtitleSettings(void);
  virtual ~CGUIDialogAudioSubtitleSettings(void);
  virtual void FrameMove();

  static CStdString PercentAsDecibel(float value, float minimum);
  static CStdString FormatDelay(float value, float minimum);
  static CStdString FormatDecibel(float value, float minimum);

protected:
  virtual void CreateSettings();
  virtual void OnSettingChanged(SettingInfo &setting);

  void AddAudioStreams(unsigned int id);
  void AddSubtitleStreams(unsigned int id);
  bool SupportsAudioFeature(int feature);
  bool SupportsSubtitleFeature(int feature);

  float m_volume;
  int m_audioStream;
  int m_subtitleStream;
  bool m_outputmode;
  bool m_subtitleVisible;
  Features m_audioCaps;
  Features m_subCaps;
};
