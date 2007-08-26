#pragma once

#include "GUIDialogSettings.h"

class CGUIDialogAudioSubtitleSettings :
      public CGUIDialogSettings
{
public:
  CGUIDialogAudioSubtitleSettings(void);
  virtual ~CGUIDialogAudioSubtitleSettings(void);
  virtual void Render();

protected:
  virtual void CreateSettings();
  virtual void OnSettingChanged(unsigned int setting);

  void AddAudioStreams(unsigned int id);
  void AddSubtitleStreams(unsigned int id);

  float m_volume;
  int m_audioStream;
  int m_subtitleStream;
  int m_outputmode;
  bool m_subtitleVisible;
};
