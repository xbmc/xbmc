#include "stdafx.h"
#include "XBAudioConfig.h"
#include "xbox/undocumented.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

XBAudioConfig g_audioConfig;

XBAudioConfig::XBAudioConfig()
{
  m_dwAudioFlags = XGetAudioFlags();
}

XBAudioConfig::~XBAudioConfig()
{}

bool XBAudioConfig::HasDigitalOutput()
{
  DWORD dwAVPack = XGetAVPack();
  if (dwAVPack == XC_AV_PACK_SCART ||
      dwAVPack == XC_AV_PACK_HDTV ||
      dwAVPack == XC_AV_PACK_VGA ||
      dwAVPack == XC_AV_PACK_SVIDEO)
    return true;
  return false;
}

void XBAudioConfig::SetAC3Enabled(bool bEnable)
{
  if (bEnable)
    m_dwAudioFlags |= XC_AUDIO_FLAGS_ENABLE_AC3;
  else
    m_dwAudioFlags &= ~XC_AUDIO_FLAGS_ENABLE_AC3;
}

bool XBAudioConfig::GetAC3Enabled()
{
  if (!HasDigitalOutput()) return false;
  return (XC_AUDIO_FLAGS_ENCODED(XGetAudioFlags()) & XC_AUDIO_FLAGS_ENABLE_AC3) != 0;
}

void XBAudioConfig::SetDTSEnabled(bool bEnable)
{
  if (bEnable)
    m_dwAudioFlags |= XC_AUDIO_FLAGS_ENABLE_DTS;
  else
    m_dwAudioFlags &= ~XC_AUDIO_FLAGS_ENABLE_DTS;
}

bool XBAudioConfig::GetDTSEnabled()
{
  if (!HasDigitalOutput()) return false;
  return (XC_AUDIO_FLAGS_ENCODED(XGetAudioFlags()) & XC_AUDIO_FLAGS_ENABLE_DTS) != 0;
}

bool XBAudioConfig::NeedsSave()
{
  if (!HasDigitalOutput()) return false;
  return m_dwAudioFlags != XGetAudioFlags();
}

// USE VERY CAREFULLY!!
void XBAudioConfig::Save()
{
  if (!NeedsSave()) return ;
  // update the EEPROM settings
  DWORD type = REG_BINARY;
  ExSaveNonVolatileSetting(XC_AUDIO_FLAGS, &type, (PULONG)&m_dwAudioFlags, 4);
  // check that we updated correctly
  if (m_dwAudioFlags != XGetAudioFlags())
  {
    int test = 1;
  }
}
