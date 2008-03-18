#pragma once

#define XC_AUDIO_FLAGS 9

class XBAudioConfig
{
public:
  XBAudioConfig();
  ~XBAudioConfig();

  bool HasDigitalOutput();

  bool GetAC3Enabled();
  void SetAC3Enabled(bool bEnable);
  bool GetDTSEnabled();
  void SetDTSEnabled(bool bEnable);
  bool NeedsSave();
  void Save();

private:
#ifdef HAS_XBOX_AUDIO
  DWORD m_dwAudioFlags;
#endif
};

extern XBAudioConfig g_audioConfig;
