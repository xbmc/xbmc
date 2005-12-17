#include "stdafx.h"
#include "GUISettings.h"
#include "application.h"
#include "util.h"
#include "GUIWindowMusicBase.h"
#include "utils/FanController.h"
#include "XBAudioConfig.h"
#include "XBVideoConfig.h"
#include <xfont.h>

// String id's of the masks
#define MASK_MINS   14044
#define MASK_SECS   14045
#define MASK_MS    14046
#define MASK_PERCENT 14047
#define MASK_KBPS   14048
#define MASK_KB    14049
#define MASK_DB    14050

#define TEXT_OFF 351

class CGUISettings g_guiSettings;

struct sortsettings
{
  bool operator()(const CSetting* pSetting1, const CSetting* pSetting2)
  {
    return pSetting1->GetOrder() < pSetting2->GetOrder();
  }
};

void CSettingBool::FromString(const CStdString &strValue)
{
  m_bData = (strValue == "true");
}

CStdString CSettingBool::ToString()
{
  return m_bData ? "true" : "false";
}

CSettingSeparator::CSettingSeparator(int iOrder, const char *strSetting)
    : CSetting(iOrder, strSetting, 0, SEPARATOR_CONTROL)
{
}

CSettingFloat::CSettingFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_fData = fData;
  m_fMin = fMin;
  m_fStep = fStep;
  m_fMax = fMax;
}

void CSettingFloat::FromString(const CStdString &strValue)
{
  SetData((float)atof(strValue.c_str()));
}

CStdString CSettingFloat::ToString()
{
  CStdString strValue;
  strValue.Format("%f", m_fData);
  return strValue;
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_iData = iData;
  m_iMin = iMin;
  m_iMax = iMax;
  m_iStep = iStep;
  m_iFormat = -1;
  m_iLabelMin = -1;
  if (strFormat)
    m_strFormat = strFormat;
  else
    m_strFormat = "%i";
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_iData = iData;
  m_iMin = iMin;
  m_iMax = iMax;
  m_iStep = iStep;
  m_iLabelMin = iLabelMin;
  if (iFormat > -1)
    m_iFormat = iFormat;
  else
    m_strFormat = "%i";
}

void CSettingInt::FromString(const CStdString &strValue)
{
  SetData(atoi(strValue.c_str()));
}

CStdString CSettingInt::ToString()
{
  CStdString strValue;
  strValue.Format("%i", m_iData);
  return strValue;
}

void CSettingHex::FromString(const CStdString &strValue)
{
  int iHexValue;
  if (sscanf(strValue, "%x", &iHexValue))
    SetData(iHexValue);
}

CStdString CSettingHex::ToString()
{
  CStdString strValue;
  strValue.Format("%x", m_iData);
  return strValue;
}

CSettingString::CSettingString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_strData = strData;
  m_bAllowEmpty = bAllowEmpty;
  m_iHeadingString = iHeadingString;
}

void CSettingString::FromString(const CStdString &strValue)
{
  m_strData = strValue;
}

CStdString CSettingString::ToString()
{
  return m_strData;
}

// Settings are case sensitive
CGUISettings::CGUISettings(void)
{
  ZeroMemory(&m_replayGain, sizeof(ReplayGainSettings));

  // Pictures settings
  AddGroup(0, 1);
  AddCategory(0, "Pictures", 16000);
  AddBool(1, "Pictures.HideParentDirItems", 13306, false);
  AddBool(2, "Pictures.UseAutoSwitching", 14011, false);
  AddBool(3, "Pictures.AutoSwitchUseLargeThumbs", 14012, false);
  AddBool(4, "Pictures.HideFilenamesInThumbPanel",15215,false);
  AddBool(5, "Pictures.AllowFileDeletion", 14071, false);
  AddBool(6, "Pictures.HandleArchives", 516, true);

  AddCategory(0, "Slideshow", 108);
  AddInt(1, "Slideshow.StayTime", 224, 9, 1, 1, 100, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddInt(2, "Slideshow.TransistionTime", 225, 2500, 100, 100, 10000, SPIN_CONTROL_INT_PLUS, MASK_MS);
  AddInt(3, "Slideshow.MoveAmount", 13311, 10, 0, 1, 40, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddInt(4, "Slideshow.ZoomAmount", 13310, 30, 0, 1, 40, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddInt(5, "Slideshow.BlackBarCompensation", 13312, 20, 0, 1, 40, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddSeparator(6, "Slideshow.Sep1");
  AddBool(7, "Slideshow.Shuffle", 13319, false);

  // Programs settings
  AddGroup(1, 0);
  AddCategory(1, "MyPrograms", 16000);
  AddBool(1, "MyPrograms.Flatten", 348, true);
  AddBool(2, "MyPrograms.DefaultXBEOnly", 349, true);
  AddBool(3, "MyPrograms.UseDirectoryName", 506, false);
  AddBool(4, "MyPrograms.NoShortcuts", 508, true);
  AddBool(5, "MyPrograms.CacheProgramThumbs", 509, true);
  AddSeparator(6,"MyPrograms.Sep1");
  AddBool(7, "MyPrograms.GameAutoRegion",511,true);
  AddInt(8, "MyPrograms.NTSCMode", 16110, 0, 0, 1, 3, SPIN_CONTROL_TEXT);
  
  AddCategory(1,"ProgramFiles",744);
  AddBool(1, "ProgramFiles.HideParentDirItems", 13306, true);
  AddBool(2, "ProgramFiles.UseAutoSwitching", 14011, false);
  AddBool(3, "ProgramFiles.AutoSwitchUseLargeThumbs", 14012, false);
  AddInt(4, "ProgramFiles.AutoSwitchMethod", 14013, 0, 0, 1, 2, SPIN_CONTROL_TEXT);
  AddInt(5, "ProgramFiles.AutoSwitchPercentage", 14014, 50, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);

  AddCategory(1, "XLinkKai", 714);
  AddBool(1, "XLinkKai.Enabled", 14072, false);
  AddBool(2, "XLinkKai.EnableNotifications", 14008, true);
  AddString(3, "XLinkKai.GamesDir", 14009, "f:\\games", BUTTON_CONTROL_MISC_INPUT, false, 14009);
  AddString(4, "XLinkKai.UserName", 709, "", BUTTON_CONTROL_INPUT, false, 709);
  AddString(5, "XLinkKai.Password", 710, "", BUTTON_CONTROL_HIDDEN_INPUT, false, 710);
  AddString(6, "XLinkKai.Server", 14042, "", BUTTON_CONTROL_IP_INPUT);

  // My Weather settings
  AddGroup(2, 8);
  AddCategory(2, "Weather", 16000);
  AddInt(1, "Weather.RefreshTime", 397, 30, 15, 15, 120, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddSeparator(2, "Weather.Sep1");
  AddInt(3, "Weather.TemperatureUnits", 398, 0, 0, 1, 1, SPIN_CONTROL_TEXT);
  AddInt(4, "Weather.SpeedUnits", 399, 0, 0, 1, 2, SPIN_CONTROL_TEXT);
  AddSeparator(5, "Weather.Sep2");
  AddString(6, "Weather.AreaCode1", 14019, "GMXX0154", BUTTON_CONTROL_STANDARD);
  AddString(7, "Weather.AreaCode2", 14020, "UKXX0085", BUTTON_CONTROL_STANDARD);
  AddString(8, "Weather.AreaCode3", 14021, "CAXX0343", BUTTON_CONTROL_STANDARD);

  // My Music Settings
  AddGroup(3, 2);
  AddCategory(3, "MyMusic", 16000);
  AddString(1, "MyMusic.Visualisation", 250, "milkdrop.vis", SPIN_CONTROL_TEXT);
  AddSeparator(2, "MyMusic.Sep1");
  AddBool(3, "MyMusic.HideParentDirItems", 13306, true);
  AddBool(4, "MyMusic.IgnoreTheWhenSorting", 13399, true);
  AddString(5, "MyMusic.TrackFormat", 13307, "%N. %A - %T", BUTTON_CONTROL_INPUT, false, 16016);
  AddString(6, "MyMusic.TrackFormatRight", 13387, "%D", BUTTON_CONTROL_INPUT, false, 16016);
  AddSeparator(7, "MyMusic.Sep2");
  AddBool(8, "MyMusic.UseAudioScrobbler", 15201, false);
  AddBool(9, "MyMusic.LastFMRecordToProfile", 15250, false);
  AddString(10,"MyMusic.AudioScrobblerUserName", 15202, "", BUTTON_CONTROL_INPUT, false, 15202);
  AddString(11,"MyMusic.AudioScrobblerPassword", 15203, "", BUTTON_CONTROL_HIDDEN_INPUT, false, 15203); 
  AddSeparator(12, "MyMusic.Sep3");
  AddString(13, "MyMusic.CleanupMusicLibrary", 334, "", BUTTON_CONTROL_STANDARD);

  AddCategory(3, "MusicFiles", 744);
  AddBool(1, "MusicFiles.AutoPlayNextItem", 489, true);
  AddBool(2, "MusicFiles.Repeat", 488, false);
  AddSeparator(3, "MusicFiles.Sep1");
  AddBool(4, "MusicFiles.UseTags", 258, true);
  AddBool(5, "MusicFiles.UseCDDB", 227, true);
  AddBool(6, "MusicFiles.FindRemoteThumbs", 14059, true);
  AddSeparator(7, "MusicFiles.Sep2");
  AddBool(8, "MusicFiles.UseAutoSwitching", 14011, false);
  AddBool(9, "MusicFiles.AutoSwitchUseLargeThumbs", 14012, false);
  AddInt(10, "MusicFiles.AutoSwitchMethod", 14013, 0, 0, 1, 2, SPIN_CONTROL_TEXT);
  AddInt(11, "MusicFiles.AutoSwitchPercentage", 14014, 50, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddBool(12, "MusicFiles.AllowFileDeletion", 14071, false);
  AddBool(13, "MusicFiles.HandleArchives", 516, true);

  AddCategory(3, "MusicPlaylist", 136);
  AddBool(1, "MusicPlaylist.ClearPlaylistsOnEnd",239,false);
  AddBool(2, "MusicPlaylist.ShufflePlaylistsOnLoad", 228, false);

  AddCategory(3, "CDDARipper", 620);
  AddBool(1, "CDDARipper.UseTrackNumber", 624, true);
  AddInt(2, "CDDARipper.Encoder", 621, CDDARIP_ENCODER_LAME, CDDARIP_ENCODER_LAME, 1, CDDARIP_ENCODER_WAV, SPIN_CONTROL_TEXT);
  AddInt(3, "CDDARipper.Quality", 622, CDDARIP_QUALITY_CBR, CDDARIP_QUALITY_CBR, 1, CDDARIP_QUALITY_EXTREME, SPIN_CONTROL_TEXT);
  AddInt(4, "CDDARipper.Bitrate", 623, 192, 128, 32, 320, SPIN_CONTROL_INT_PLUS, MASK_KBPS);

  AddCategory(3, "MusicPlayer", 16003);
  AddString(1, "MusicPlayer.JumpToAudioHardware", 16001, "", BUTTON_CONTROL_STANDARD);
  AddBool(2, "MusicPlayer.OutputToAllSpeakers", 252, false);
  AddSeparator(3, "MusicPlayer.Sep1");
  AddInt(4, "MusicPlayer.ReplayGainType", 638, REPLAY_GAIN_ALBUM, REPLAY_GAIN_NONE, 1, REPLAY_GAIN_TRACK, SPIN_CONTROL_TEXT);
  AddInt(5, "MusicPlayer.ReplayGainPreAmp", 641, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddInt(6, "MusicPlayer.ReplayGainNoGainPreAmp", 642, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddBool(7, "MusicPlayer.ReplayGainAvoidClipping", 643, false);
  AddSeparator(8, "MusicPlayer.Sep2");
  AddInt(9, "MusicPlayer.CrossFade", 13314, 0, 0, 1, 10, SPIN_CONTROL_INT_PLUS, MASK_SECS, TEXT_OFF);
  AddBool(10, "MusicPlayer.CrossFadeAlbumTracks", 13400, true);
  

  AddCategory(3, "Karaoke", 13327);
  AddBool(1, "Karaoke.Enabled", 13323, false);
  AddInt(2, "Karaoke.BackgroundAlpha", 13324, 0, 0, 5, 255, SPIN_CONTROL_INT);
  AddInt(3, "Karaoke.ForegroundAlpha", 13325, 255, 0, 5, 255, SPIN_CONTROL_INT);
  AddFloat(4, "Karaoke.SyncDelay", 13326, -0.8f, -3.0f, 0.1f, 3.0f, SPIN_CONTROL_FLOAT);
  AddSeparator(5, "Karaoke.Sep1");

  // Karaoke patch (114097) ...
  AddBool(6, "Karaoke.VoiceEnabled", 13361, false);
  AddString(7, "Karaoke.Port0VoiceMask", 13382, "None", SPIN_CONTROL_TEXT);
  AddString(8, "Karaoke.Port1VoiceMask", 13383, "None", SPIN_CONTROL_TEXT);
  AddString(9, "Karaoke.Port2VoiceMask", 13384, "None", SPIN_CONTROL_TEXT);
  AddString(10, "Karaoke.Port3VoiceMask", 13385, "None", SPIN_CONTROL_TEXT);
  AddInt(11, "Karaoke.Volume", 13376, 100, 0, 1, 100, SPIN_CONTROL_INT, MASK_PERCENT);

//  AddCategory(3, "VoiceOnPort0", 13371);
//  AddBool(2, "VoiceOnPort0.EnableDevice", 13375, false);
  AddInt(3, "VoiceOnPort0.Volume", 13376, 100, 0, 1 , 100, SPIN_CONTROL_INT);
  AddString(4, "VoiceOnPort0.VoiceMask", 13362, "None", SPIN_CONTROL_TEXT);
  AddBool(5, "VoiceOnPort0.EnablefSpecEnergyWeight", 13363, false);
  AddFloat(6, "VoiceOnPort0.fSpecEnergyWeight", 13364, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(7, "VoiceOnPort0.EnablefPitchScale", 13365, false);
  AddFloat(8, "VoiceOnPort0.fPitchScale", 13366, 0.8f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(9, "VoiceOnPort0.EnablefWhisperValue", 13367, false);
  AddFloat(10, "VoiceOnPort0.fWhisperValue", 13368, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(11, "VoiceOnPort0.EnablefRoboticValue", 13369, false);
  AddFloat(12, "VoiceOnPort0.fRoboticValue", 13370, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);

//  AddCategory(3, "VoiceOnPort1", 13372);
//  AddBool(2, "VoiceOnPort1.EnableDevice", 13375, false);
  AddInt(3, "VoiceOnPort1.Volume", 13376, 100, 0, 1 , 100, SPIN_CONTROL_INT);
  AddString(4, "VoiceOnPort1.VoiceMask", 13362, "None", SPIN_CONTROL_TEXT);
  AddBool(5, "VoiceOnPort1.EnablefSpecEnergyWeight", 13363, false);
  AddFloat(6, "VoiceOnPort1.fSpecEnergyWeight", 13364, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(7, "VoiceOnPort1.EnablefPitchScale", 13365, false);
  AddFloat(8, "VoiceOnPort1.fPitchScale", 13366, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(9, "VoiceOnPort1.EnablefWhisperValue", 13367, false);
  AddFloat(10, "VoiceOnPort1.fWhisperValue", 13368, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(11, "VoiceOnPort1.EnablefRoboticValue", 13369, false);
  AddFloat(12, "VoiceOnPort1.fRoboticValue", 13370, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);

//  AddCategory(3, "VoiceOnPort2", 13373);
//  AddBool(2, "VoiceOnPort2.EnableDevice", 13375, false);
  AddInt(3, "VoiceOnPort2.Volume", 13376, 100, 0, 1 , 100, SPIN_CONTROL_INT);
  AddString(4, "VoiceOnPort2.VoiceMask", 13362, "None", SPIN_CONTROL_TEXT);
  AddBool(5, "VoiceOnPort2.EnablefSpecEnergyWeight", 13363, false);
  AddFloat(6, "VoiceOnPort2.fSpecEnergyWeight", 13364, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(7, "VoiceOnPort2.EnablefPitchScale", 13365, false);
  AddFloat(8, "VoiceOnPort2.fPitchScale", 13366, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(9, "VoiceOnPort2.EnablefWhisperValue", 13367, false);
  AddFloat(10, "VoiceOnPort2.fWhisperValue", 13368, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(11, "VoiceOnPort2.EnablefRoboticValue", 13369, false);
  AddFloat(12, "VoiceOnPort2.fRoboticValue", 13370, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);

//  AddCategory(3, "VoiceOnPort3", 13374);
//  AddBool(2, "VoiceOnPort3.EnableDevice", 13375, false);
  AddInt(3, "VoiceOnPort3.Volume", 13376, 100, 0, 1 , 100, SPIN_CONTROL_INT);
  AddString(4, "VoiceOnPort3.VoiceMask", 13362, "None", SPIN_CONTROL_TEXT);
  AddBool(5, "VoiceOnPort3.EnablefSpecEnergyWeight", 13363, false);
  AddFloat(6, "VoiceOnPort3.fSpecEnergyWeight", 13364, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(7, "VoiceOnPort3.EnablefPitchScale", 13365, false);
  AddFloat(8, "VoiceOnPort3.fPitchScale", 13366, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(9, "VoiceOnPort3.EnablefWhisperValue", 13367, false);
  AddFloat(10, "VoiceOnPort3.fWhisperValue", 13368, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  AddBool(11, "VoiceOnPort3.EnablefRoboticValue", 13369, false);
  AddFloat(12, "VoiceOnPort3.fRoboticValue", 13370, 0.0f, 0.0f, 0.01f, 1.0f, SPIN_CONTROL_FLOAT);
  // ... Karaoke patch (114097)

  // System settings
  AddGroup(4, 13000);
  AddCategory(4, "System", 13000);
  AddInt(1, "System.HDSpinDownTime", 229, 0, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF); // Minutes
  AddInt(2, "System.RemotePlayHDSpinDown", 13001, 0, 0, 1, 3, SPIN_CONTROL_TEXT); // off, music, video, both
  AddInt(3, "System.RemotePlayHDSpinDownMinDuration", 13004, 20, 0, 1, 20, SPIN_CONTROL_INT_PLUS, MASK_MINS); // Minutes
  AddInt(4, "System.RemotePlayHDSpinDownDelay", 13003, 20, 5, 5, 300, SPIN_CONTROL_INT_PLUS, MASK_SECS); // seconds
  AddSeparator(5, "System.Sep1");
  AddInt(6, "System.ShutDownTime", 357, 0, 0, 5, 120, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  AddBool(7, "System.ShutDownWhilePlaying", 14043, false);
  AddSeparator(8, "System.Sep2");
  AddBool(9, "System.FanSpeedControl", 13302, false);
  AddInt(10, "System.FanSpeed", 13300, CFanController::Instance()->GetFanSpeed(), 5, 1, 50, SPIN_CONTROL_TEXT);
  AddSeparator(11, "System.Sep3");
  AddBool(12, "System.AutoTemperature", 13301, false);
  AddInt(13, "System.TargetTemperature", 13299, 55, 40, 1, 68, SPIN_CONTROL_TEXT);

  AddCategory(4, "Autorun", 447);
  AddBool(1, "Autorun.DVD", 240, true);
  AddBool(2, "Autorun.VCD", 241, true);
  AddBool(3, "Autorun.CDDA", 242, true);
  AddBool(4, "Autorun.Xbox", 243, true);
  AddBool(5, "Autorun.Video", 244, true);
  AddBool(6, "Autorun.Music", 245, true);
  AddBool(7, "Autorun.Pictures", 246, true);

  AddCategory(4, "Cache", 439);
  AddInt(1, "Cache.HardDisk", 14025, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddSeparator(2, "Cache.Sep1");
  AddInt(3, "CacheVideo.DVDRom", 14026, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddInt(4, "CacheVideo.LAN", 14027, 8192, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddInt(5, "CacheVideo.Internet", 14028, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddSeparator(6, "Cache.Sep2");
  AddInt(7, "CacheAudio.DVDRom", 14030, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddInt(8, "CacheAudio.LAN", 14031, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddInt(9, "CacheAudio.Internet", 14032, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddSeparator(10, "Cache.Sep3");
  AddInt(11, "CacheDVD.DVDRom", 14034, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddInt(12, "CacheDVD.LAN", 14035, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddSeparator(13, "Cache.Sep4");
  AddInt(14, "CacheUnknown.Internet", 14060, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);

  AddCategory(4, "LED", 13338);
  AddInt(1, "LED.Colour", 13339, LED_COLOUR_NO_CHANGE, LED_COLOUR_NO_CHANGE, 1, LED_COLOUR_OFF, SPIN_CONTROL_TEXT);
  AddInt(2, "LED.DisableOnPlayback", 13345, LED_PLAYBACK_OFF, LED_PLAYBACK_OFF, 1, LED_PLAYBACK_VIDEO_MUSIC, SPIN_CONTROL_TEXT);

  AddCategory(4, "LCD", 448);
  AddInt(1, "LCD.Mode", 456, LCD_MODE_NONE, LCD_MODE_NONE, 1, LCD_MODE_NOTV, SPIN_CONTROL_TEXT);
  AddInt(2, "LCD.Type", 4501, LCD_TYPE_LCD_HD44780, LCD_TYPE_LCD_HD44780, 1, LCD_TYPE_VFD, SPIN_CONTROL_TEXT);
  AddInt(3, "LCD.ModChip", 471, MODCHIP_SMARTXX, MODCHIP_SMARTXX, 1, MODCHIP_XECUTER3, SPIN_CONTROL_TEXT);
  AddInt(4, "LCD.Columns", 450, 20, 1, 1, 20, SPIN_CONTROL_INT);
  AddInt(5, "LCD.Rows", 455, 4, 1, 1, 4, SPIN_CONTROL_INT);
  AddInt(6, "LCD.BackLight", 463, 80, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddInt(7, "LCD.Contrast", 465, 100, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddHex(8, "LCD.Row1Address", 451, 0, 0, 0x4, 0x100, SPIN_CONTROL_INT_PLUS, "%x");
  AddHex(9, "LCD.Row2Address", 452, 0x40, 0, 0x4, 0x100, SPIN_CONTROL_INT_PLUS, "%x");
  AddHex(10, "LCD.Row3Address", 453, 0x14, 0, 0x4, 0x100, SPIN_CONTROL_INT_PLUS, "%x");
  AddHex(11, "LCD.Row4Address", 454, 0x54, 0, 0x4, 0x100, SPIN_CONTROL_INT_PLUS, "%x");

  AddCategory(4, "AudioOutput", 772);
  AddInt(1, "AudioOutput.Headroom", 494, 6, 0, 6, 12, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddSeparator(2, "AudioOutput.Sep1");
  AddInt(3, "AudioOutput.Mode", 337, AUDIO_ANALOG, AUDIO_ANALOG, 1, AUDIO_DIGITAL, SPIN_CONTROL_TEXT);
  AddBool(4, "AudioOutput.AC3PassThrough", 364, true);
  AddBool(5, "AudioOutput.DTSPassThrough", 254, true);

  // video settings
  AddGroup(5, 3);
  AddCategory(5, "MyVideos", 16000);
  AddString(1, "MyVideos.Calibrate", 214, "", BUTTON_CONTROL_STANDARD);
  AddBool(2, "MyVideos.WidescreenSwitching", 223, false);
  AddBool(3, "MyVideos.PAL60Switching", 226, true);
  AddBool(4, "MyVideos.FrameRateConversions", 336, false);
  AddSeparator(5, "MyVideos.Sep1");
  AddInt(6, "MyVideos.OSDTimeout", 472, 5, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_SECS, TEXT_OFF);
  AddSeparator(7, "MyVideos.Sep2");
  AddInt(8, "MyVideos.ViewMode", 13377, VIEW_MODE_NORMAL, VIEW_MODE_NORMAL, 1, VIEW_MODE_CUSTOM, SPIN_CONTROL_TEXT);
  AddInt(9, "MyVideos.Brightness", 13378, 50, 0, 1, 100, SPIN_CONTROL_INT);
  AddInt(10, "MyVideos.Contrast", 13379, 50, 0, 1, 100, SPIN_CONTROL_INT);
  AddInt(11, "MyVideos.Gamma", 13380, 20, 0, 1, 100, SPIN_CONTROL_INT);
  AddSeparator(12, "MyVideos.Sep3");
  AddBool(13, "MyVideos.HideParentDirItems", 13306, true);
  AddBool(14, "MyVideos.IgnoreTheWhenSorting", 13399, true);
  // hidden setting for blackbars
  AddInt(0, "Videos.BlackBarColour", 0, 1, 1, 1, 255, SPIN_CONTROL_INT);

  AddCategory(5, "VideoFiles", 744);
  AddBool(1, "VideoFiles.FindRemoteThumbs", 14059, true);
  AddSeparator(2, "VideoFiles.Sep1");
  AddBool(3, "VideoFiles.UseAutoSwitching", 14011, false);
  AddBool(4, "VideoFiles.AutoSwitchUseLargeThumbs", 14012, false);
  AddInt(5, "VideoFiles.AutoSwitchMethod", 14013, 0, 0, 1, 2, SPIN_CONTROL_TEXT);
  AddInt(6, "VideoFiles.AutoSwitchPercentage", 14014, 50, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddBool(7, "VideoFiles.AllowFileDeletion", 14071, false);
  AddBool(8,"VideoFiles.HandleArchives",516, true);

  AddCategory(5, "VideoPlayer", 16003);
  AddString(1, "VideoPlayer.JumpToAudioHardware", 16001, "", BUTTON_CONTROL_STANDARD);
  AddBool(2, "VideoPlayer.OutputToAllSpeakers", 252, false);
  AddSeparator(3, "VideoPlayer.Sep1");
  AddInt(4, "VideoPlayer.VolumeAmplification", 290, 0, -200, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddSeparator(5, "VideoPlayer.Sep2");
  AddInt(6, "VideoPlayer.RenderMethod", 13354, RENDER_HQ_RGB_SHADER, RENDER_LQ_RGB_SHADER, 1, RENDER_HQ_RGB_SHADER, SPIN_CONTROL_TEXT);
  AddBool(7, "VideoPlayer.UseGUIResolution", 495, true);
  AddSeparator(10, "VideoPlayer.Sep3");
  AddBool(11, "VideoPlayer.UseTimeBasedSeeking", 13386, false);
  AddInt(12, "VideoPlayer.BypassCDSelection", 13169, 0, 0, 1, 37, SPIN_CONTROL_TEXT);

  AddCategory(5, "Subtitles", 287);
  AddString(1, "Subtitles.Font", 288, "arial-iso-8859-1", SPIN_CONTROL_TEXT);
  AddInt(2, "Subtitles.Height", 289, 28, 16, 2, 74, SPIN_CONTROL_TEXT); // use text as there is a disk based lookup needed
  AddInt(3, "Subtitles.Style", 736, XFONT_BOLD, XFONT_NORMAL, 1, XFONT_BOLDITALICS, SPIN_CONTROL_TEXT);
  AddInt(4, "Subtitles.Color", 737, SUBTITLE_COLOR_START, SUBTITLE_COLOR_START, 1, SUBTITLE_COLOR_END, SPIN_CONTROL_TEXT);
  AddString(5, "Subtitles.CharSet", 735, "ISO-8859-1", SPIN_CONTROL_TEXT);
  AddBool(6, "Subtitles.FlipBiDiCharSet", 13304, false);
  AddSeparator(7, "Subtitles.Sep1");
  AddInt(8, "Subtitles.EnlargePercentage", 492, 100, 30, 10, 200, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);

  AddCategory(5, "PostProcessing", 14041);
  AddBool(2, "PostProcessing.Enable", 286, false);
  AddBool(3, "PostProcessing.Auto", 307, true); // only has effect if PostProcessing.Enable is on.
  AddBool(4, "PostProcessing.VerticalDeBlocking", 308, false);
  AddInt(5, "PostProcessing.VerticalDeBlockLevel", 308, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(6, "PostProcessing.HorizontalDeBlocking", 309, false);
  AddInt(7, "PostProcessing.HorizontalDeBlockLevel", 309, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(8, "PostProcessing.AutoBrightnessContrastLevels", 310, false);
  AddBool(9, "PostProcessing.DeRing", 311, false);

  AddCategory(5, "Filters", 230);
  AddInt(1, "Filters.Flicker", 13100, 1, 0, 1, 5, SPIN_CONTROL_INT);
  AddBool(2, "Filters.Soften", 215, false);
  AddSeparator(3, "Filters.Sep1");
  AddBool(4, "Filters.Noise", 14058, false);
  AddInt(5, "Filters.NoiseLevel", 14058, 10, 1, 1, 100, SPIN_CONTROL_INT);
  AddSeparator(6, "Filters.Sep2");
  AddBool(7, "Filters.UseAutosync", 15214, false);

  // network settings
  AddGroup(6, 705);
  AddCategory(6, "Network", 705);
  AddInt(1, "Network.Assignment", 715, NETWORK_DASH, NETWORK_DASH, 1, NETWORK_STATIC, SPIN_CONTROL_TEXT);
  AddString(2, "Network.IPAddress", 719, "192.168.0.3", BUTTON_CONTROL_IP_INPUT);
  AddString(3, "Network.Subnet", 720, "255.255.255.0", BUTTON_CONTROL_IP_INPUT);
  AddString(4, "Network.Gateway", 721, "192.168.0.1", BUTTON_CONTROL_IP_INPUT);
  AddString(5, "Network.DNS", 722, "0.0.0.0", BUTTON_CONTROL_IP_INPUT);
  AddSeparator(6, "Network.Sep1");
  AddBool(7, "Network.UseHTTPProxy", 708, false);
  AddString(8, "Network.HTTPProxyServer", 706, "", BUTTON_CONTROL_IP_INPUT);
  AddString(9, "Network.HTTPProxyPort", 707, "8080", BUTTON_CONTROL_INPUT, false, 707);
  AddSeparator(10, "Network.Sep2");
  AddBool(11, "Network.EnableInternet", 14054, true);
  AddSeparator(12, "Network.Sep3");
  AddBool(13,   "Network.EnableRSSFeeds",13305,  true);

  //GeminiServer
  AddCategory(6, "Servers", 14036);
  AddBool(1,  "Servers.FTPServer",        167, true);
  AddString(2,"Servers.FTPServerUser",    1245, "xbox", SPIN_CONTROL_TEXT);
  AddString(3,"Servers.FTPServerPassword",1246, "xbox", BUTTON_CONTROL_HIDDEN_INPUT, true, 1246);
  AddBool(4,  "Servers.FTPAutoFatX",      771, true);
  AddSeparator(5, "Servers.Sep1");
  AddBool(6,  "Servers.WebServer",        263, false);
  AddString(7,"Servers.WebServerPort",    730, "80", BUTTON_CONTROL_INPUT, false, 730);
  AddString(8,"Servers.WebServerPassword",733, "", BUTTON_CONTROL_HIDDEN_INPUT, true, 733);

  //GeminiServer
  AddCategory(6,"Autodetect",           1250  );
  AddBool(1,    "Autodetect.OnOff",     1251, true);
  AddString(2,  "Autodetect.NickName",  1252, "XBMC-NickName",BUTTON_CONTROL_INPUT, false, 1252);
  AddInt(3,     "Autodetect.PingTime",  1256, 30, 5, 5, 240, SPIN_CONTROL_INT, MASK_SECS);
  AddBool(4,    "Autodetect.PopUpInfo", 1254, true);
  AddSeparator(5, "Autodetect.Sep1");
  AddBool(6,    "Autodetect.SendUserPw",1255, true);
  AddBool(7,    "Autodetect.CreateLink",1253, false);
  
  //GeminiServer
  AddCategory(6, "Smb", 1200);
  AddInt(1,    "Smb.SimpAdvance", 1208,   1, 0, 1, 2, SPIN_CONTROL_TEXT);
  AddString(2, "Smb.Ip",          1201,   "192.168.0.106",  BUTTON_CONTROL_IP_INPUT);
  AddString(3, "Smb.Workgroup",   1202,   "WORKGROUP", BUTTON_CONTROL_INPUT, false, 1202);
  AddString(4, "Smb.Username",    1203,   "Username", BUTTON_CONTROL_INPUT, false, 1203);
  AddString(5, "Smb.Password",    1204,   "Password", BUTTON_CONTROL_HIDDEN_INPUT, true, 1204);
  AddSeparator(6, "Smb.Sep1");
  AddString(7, "Smb.Winsserver",  1207,   "-",  BUTTON_CONTROL_IP_INPUT);
  AddString(8, "Smb.ShareName",   1205,   "WORKGROUP (SMB) Network", BUTTON_CONTROL_INPUT, false, 1205);
  AddInt(9,    "Smb.ShareGroup",  1206,   SMB_SHARE_MU_VI_PIC_FIL, SMB_SHARE_MUSIC, 1, SMB_SHARE_MU_VI_PIC_FIL, SPIN_CONTROL_TEXT);
  AddSeparator(10, "Smb.Sep2");
  AddString(11, "Smb.SetSmb",      14070,  "", BUTTON_CONTROL_STANDARD);

  // appearance settings
  AddGroup(7, 480);
  AddCategory(7,"LookAndFeel", 14037);
  AddString(1, "LookAndFeel.Skin",166,"Project Mayhem III", SPIN_CONTROL_TEXT);
  AddString(2, "LookAndFeel.SkinTheme",15111,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(3, "LookAndFeel.SoundSkin",15108,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddInt(4, "LookAndFeel.Rumble",15110,0, 0, 1, 10, SPIN_CONTROL_TEXT);
  AddString(5, "LookAndFeel.Font",13303,"Default", SPIN_CONTROL_TEXT);
  AddInt(6, "LookAndFeel.StartUpWindow",512,1, 0, 1, 6, SPIN_CONTROL_TEXT);
  AddSeparator(7, "LookAndFeel.Sep1");
  AddString(8, "LookAndFeel.Language",248,"english", SPIN_CONTROL_TEXT);
  AddString(9, "LookAndFeel.CharSet",735,"ISO-8859-1", SPIN_CONTROL_TEXT);
  AddSeparator(10, "LookAndFeel.Sep2");
  AddInt(11, "LookAndFeel.Resolution",169,(int)AUTORES, (int)HDTV_1080i, 1, (int)AUTORES, SPIN_CONTROL_TEXT);
  AddString(12, "LookAndFeel.GUICentering",213,"", BUTTON_CONTROL_STANDARD);
  AddSeparator(13, "LookAndFeel.Sep3");
  AddBool(14, "LookAndFeel.FullDirectoryHistory", 15106, true);
  
  AddCategory(7, "FileLists", 14018);
  AddBool(1, "FileLists.HideExtensions", 497, false);
  AddBool(2, "FileLists.HideParentDirItems", 13306, false);

  AddCategory(7, "ScreenSaver", 360);
  AddString(1, "ScreenSaver.Mode", 356, "Dim", SPIN_CONTROL_TEXT);
  AddString(2, "ScreenSaver.Preview", 1000, "", BUTTON_CONTROL_STANDARD);
  AddBool(3, "ScreenSaver.UseMusicVisInstead", 13392, true);
  AddInt(4, "ScreenSaver.Time", 355, 3, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddSeparator(5, "ScreenSaver.Sep1");
  AddInt(6, "ScreenSaver.DimLevel", 362, 20, 10, 10, 80, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddString(7, "ScreenSaver.SlideShowPath", 774, "F:\\Pictures\\", BUTTON_CONTROL_MISC_INPUT, false, 774); // GeminiServer: PictureSlideShow

  AddCategory(7, "UIFilters", 14053);
  AddInt(1, "UIFilters.Flicker", 13100, 5, 0, 1, 5, SPIN_CONTROL_INT);
  AddBool(2, "UIFilters.Soften", 215, false);

  // GeminiServer
  AddCategory(7, "XBDateTime", 14063);
  AddBool(1,   "XBDateTime.Clock12Hour"      , 14051, false);
  AddBool(2,   "XBDateTime.SwapMonthAndDay"  , 14052, false);
  AddSeparator(3, "XBDateTime.Sep1");
  AddBool(4,   "XBDateTime.TimeServer"       , 168  , false);
  AddString(5, "XBDateTime.TimeAddress"      , 731  , "207.46.130.100", BUTTON_CONTROL_IP_INPUT);
  AddSeparator(6, "XBDateTime.Sep2");
  AddString(7, "XBDateTime.Time", 14065, "", BUTTON_CONTROL_MISC_INPUT);
  AddString(8, "XBDateTime.Date", 14064, "", BUTTON_CONTROL_MISC_INPUT);

  //GeminiServer
  AddCategory(7, "Masterlock", 12360);
  AddInt(1,   "Masterlock.Mastermode"       , 12364, LOCK_MODE_EVERYONE, LOCK_MODE_EVERYONE, 1, LOCK_MODE_QWERTY, SPIN_CONTROL_TEXT); // 0:always Unlocked, 1:Numeric, 2:Gamepad, 3:Text
  AddString(2,"Masterlock.Mastercode"       , 12365, "-", BUTTON_CONTROL_STANDARD); // This is the CODE, Changing in addition with Mastermode!
  AddBool(3,  "Masterlock.MasterUser"       , 12375,false); //true: master user can open all shares without cheking the fixed share lock in xml
  AddSeparator(4, "Masterlock.Sep1");
  AddInt(5,   "Masterlock.Maxretry"         , 12361, 0, 0, 1, 9, SPIN_CONTROL_TEXT); //Max Retry is 3, 0 is off
  AddBool(6,  "Masterlock.Enableshutdown"   , 12362,false); //talse:0 is off, true:1 will shutdows if Maxrety is reached
  AddSeparator(7, "Masterlock.Sep2");     // The seperator is cool but this section needs space!
  AddBool(8,  "Masterlock.Protectshares"    , 12363,false); //false:0 is Normal user Mode, true:1 is Mastermode
  AddBool(9,  "Masterlock.StartupLock"      , 12369,false); //false:0 is no ask StarupCode, true:1 ask for MasterCode if is false switxh off xbmc
  AddSeparator(10, "Masterlock.Sep3");     // The seperator is cool but this section needs space!
  AddInt(11,   "Masterlock.LockSettingsFilemanager"    , 12372, 0, 0, 1, 3, SPIN_CONTROL_TEXT);
  AddInt(12,   "Masterlock.LockHomeMedia"    , 12374, LOCK_DISABLED, LOCK_DISABLED, 1, LOCK_MU_VI_PIC_PROG, SPIN_CONTROL_TEXT); // LockHomeMedia, for lock the Video/Music/Programs/Pictures

  AddSeparator(12, "Masterlock.Sep4");
  AddString(13,"Masterlock.SetMasterlock"   , 14070, "", BUTTON_CONTROL_STANDARD);
  
  AddInt( -1, "UIOffset.X", 0, 0, -128, 1, 128, SPIN_CONTROL_INT);
  AddInt( -1, "UIOffset.Y", 0, 0, -128, 1, 128, SPIN_CONTROL_INT);
}

CGUISettings::~CGUISettings(void)
{}

void CGUISettings::AddGroup(DWORD dwGroupID, DWORD dwLabelID)
{
  CSettingsGroup *pGroup = new CSettingsGroup(dwGroupID, dwLabelID);
  if (pGroup)
    settingsGroups.push_back(pGroup);
}

void CGUISettings::AddCategory(DWORD dwGroupID, const char *strSetting, DWORD dwLabelID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == dwGroupID)
      settingsGroups[i]->AddCategory(strSetting, dwLabelID);
  }
}

CSettingsGroup *CGUISettings::GetGroup(DWORD dwGroupID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == dwGroupID)
      return settingsGroups[i];
  }
  CLog::DebugLog("Error: Requested setting group (%i) was not found.  It must be case-sensitive", dwGroupID);
  return NULL;
}

void CGUISettings::AddSeparator(int iOrder, const char *strSetting)
{
  CSettingSeparator *pSetting = new CSettingSeparator(iOrder, strSetting);
  if (!pSetting) return;
  settingsMap.insert(pair<CStdString, CSetting*>(strSetting, pSetting));
}

void CGUISettings::AddBool(int iOrder, const char *strSetting, int iLabel, bool bData, int iControlType)
{
  CSettingBool* pSetting = new CSettingBool(iOrder, strSetting, iLabel, bData, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(strSetting, pSetting));
}
bool CGUISettings::GetBool(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
  { // old category
    return ((CSettingBool*)(*it).second)->GetData();
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return false;
}

void CGUISettings::SetBool(const char *strSetting, bool bSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(bSetting);
    return ;
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::ToggleBool(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(!((CSettingBool *)(*it).second)->GetData());
    return ;
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::AddFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
{
  CSettingFloat* pSetting = new CSettingFloat(iOrder, strSetting, iLabel, fData, fMin, fStep, fMax, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(strSetting, pSetting));
}

float CGUISettings::GetFloat(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
  {
    return ((CSettingFloat *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return 0.0f;
}

void CGUISettings::SetFloat(const char *strSetting, float fSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
  {
    ((CSettingFloat *)(*it).second)->SetData(fSetting);
    return ;
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  CSettingInt* pSetting = new CSettingInt(iOrder, strSetting, iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(strSetting, pSetting));
}

void CGUISettings::AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin/*=-1*/)
{
  CSettingInt* pSetting = new CSettingInt(iOrder, strSetting, iLabel, iData, iMin, iStep, iMax, iControlType, iFormat, iLabelMin);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(strSetting, pSetting));
}

void CGUISettings::AddHex(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  CSettingHex* pSetting = new CSettingHex(iOrder, strSetting, iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(strSetting, pSetting));
}

int CGUISettings::GetInt(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
  {
    return ((CSettingInt *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return 0;
}

void CGUISettings::SetInt(const char *strSetting, int iSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
  {
    ((CSettingInt *)(*it).second)->SetData(iSetting);
    if (strcmp(strSetting, "LookAndFeel.Resolution") == 0)
      g_guiSettings.m_LookAndFeelResolution = (RESOLUTION)iSetting;
    return ;
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::AddString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  CSettingString* pSetting = new CSettingString(iOrder, strSetting, iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(strSetting, pSetting));
}

CStdString CGUISettings::GetString(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
  {
    return ((CSettingString *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return "";
}

void CGUISettings::SetString(const char *strSetting, const char *strData)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
  {
    ((CSettingString *)(*it).second)->SetData(strData);
    return ;
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

CSetting *CGUISettings::GetSetting(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
    return (*it).second;
  else
    return NULL;
}

void CGUISettings::ReadXML(TiXmlElement* pRootElement, const char *strSetting)
{
  mapIter it = settingsMap.find(strSetting);
  if (it == settingsMap.end())
  {
    CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
    return ;
  }
  const TiXmlNode *pChild = pRootElement->FirstChild(strSetting);
  if (pChild)
  {
    if (pChild->FirstChild())
    {
      CStdString strValue = pChild->FirstChild()->Value();
      if (strValue.size() )
      {
        if (strValue != "-")
        { // update our item
          (*it).second->FromString(strValue);
        }
      }
    }
  }
  CLog::Log(LOGDEBUG, "  %s: %s", strSetting, (*it).second->ToString().c_str());
}

void CGUISettings::WriteXML(TiXmlNode* pRootNode, const char *strSetting)
{
  TiXmlElement newElement(strSetting);
  TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
  if (pNewNode)
  {
    mapIter it = settingsMap.find(strSetting);
    if (it != settingsMap.end())
    {
      TiXmlText value((*it).second->ToString());
      pNewNode->InsertEndChild(value);
    }
  }
}

// get all the settings beginning with the term "strGroup"
void CGUISettings::GetSettingsGroup(const char *strGroup, vecSettings &settings)
{
  settings.clear();
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    if ((*it).first.Left(strlen(strGroup)) == strGroup && (*it).second->GetOrder() >= 0)
      settings.push_back((*it).second);
  }
  // now order them...
  sort(settings.begin(), settings.end(), sortsettings());
}

void CGUISettings::LoadXML(TiXmlElement *pRootElement)
{ // load our stuff...
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    CStdStringArray strSplit;
    StringUtils::SplitString((*it).first, ".", strSplit);
    if (strSplit.size() > 1)
    {
      const TiXmlNode *pChild = pRootElement->FirstChild(strSplit[0].c_str());
      if (pChild)
      {
        const TiXmlNode *pGrandChild = pChild->FirstChild(strSplit[1].c_str());
        if (pGrandChild && pGrandChild->FirstChild())
        {
          CStdString strValue = pGrandChild->FirstChild()->Value();
          if (strValue.size() )
          {
            if (strValue != "-")
            { // update our item
              (*it).second->FromString(strValue);
              CLog::Log(LOGDEBUG, "  %s: %s", (*it).first.c_str(), (*it).second->ToString().c_str());
            }
          }
        }
      }
    }
  }
  // Get hardware based stuff...
  CLog::Log(LOGNOTICE, "Getting hardware information now...");
  if (GetInt("AudioOutput.Mode") == AUDIO_DIGITAL && !g_audioConfig.HasDigitalOutput())
    SetInt("AudioOutput.Mode", AUDIO_ANALOG);
  SetBool("AudioOutput.AC3PassThrough", g_audioConfig.GetAC3Enabled());
  SetBool("AudioOutput.DTSPassThrough", g_audioConfig.GetDTSEnabled());
  CLog::Log(LOGINFO, "Using %s output", GetInt("AudioOutput.Mode") == AUDIO_ANALOG ? "analog" : "digital");
  CLog::Log(LOGINFO, "AC3 pass through is %s", GetBool("AudioOutput.AC3PassThrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "DTS pass through is %s", GetBool("AudioOutput.DTSPassThrough") ? "enabled" : "disabled");
  g_guiSettings.m_LookAndFeelResolution = (RESOLUTION)GetInt("LookAndFeel.Resolution");
  CLog::Log(LOGNOTICE, "Checking resolution %i", g_guiSettings.m_LookAndFeelResolution);
  g_videoConfig.PrintInfo();
  if (
    (g_guiSettings.m_LookAndFeelResolution == AUTORES) ||
    (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  )
  {
    RESOLUTION newRes = g_videoConfig.GetBestMode();
    if (g_guiSettings.m_LookAndFeelResolution == AUTORES)
    {
      //"LookAndFeel.Resolution" will stay at AUTORES, m_LookAndFeelResolution will be the real mode
      CLog::Log(LOGNOTICE, "Setting autoresolution mode %i", newRes);
      g_guiSettings.m_LookAndFeelResolution = newRes;
    }
    else
    {
      CLog::Log(LOGNOTICE, "Setting safe mode %i", newRes);
      SetInt("LookAndFeel.Resolution", newRes);
    }
  }
  // Move replaygain settings into our struct
  m_replayGain.iPreAmp = GetInt("MusicPlayer.ReplayGainPreAmp");
  m_replayGain.iNoGainPreAmp = GetInt("MusicPlayer.ReplayGainNoGainPreAmp");
  m_replayGain.iType = GetInt("MusicPlayer.ReplayGainType");
  m_replayGain.bAvoidClipping = GetBool("MusicPlayer.ReplayGainAvoidClipping");
}

void CGUISettings::SaveXML(TiXmlNode *pRootNode)
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    CStdStringArray strSplit;
    StringUtils::SplitString((*it).first, ".", strSplit);
    if (strSplit.size() > 1)
    {
      TiXmlNode *pChild = pRootNode->FirstChild(strSplit[0].c_str());
      if (!pChild)
      { // add our group tag
        TiXmlElement newElement(strSplit[0].c_str());
        pChild = pRootNode->InsertEndChild(newElement);
      }

      if (pChild)
      { // successfully added (or found) our group
        TiXmlElement newElement(strSplit[1]);
        TiXmlNode *pNewNode = pChild->InsertEndChild(newElement);
        if (pNewNode)
        {
          TiXmlText value((*it).second->ToString());
          pNewNode->InsertEndChild(value);
        }
      }
    }
  }
}

void CGUISettings::Clear()
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
    delete (*it).second;
  settingsMap.clear();
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
    delete settingsGroups[i];
  settingsGroups.clear();
}
