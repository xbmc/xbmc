
#include "stdafx.h"
#include "GUISettings.h"
#include "application.h"
#include "util.h"
#include "utils/log.h"
#include "localizestrings.h"
#include "stdstring.h"
#include "GraphicContext.h"
#include "GUIWindowMusicBase.h"
#include "utils/FanController.h"
#include "XBAudioConfig.h"
#include "XBVideoConfig.h"

using namespace std;

//	String id's of the masks
#define MASK_MINS			14044
#define MASK_SECS			14045
#define MASK_MS				14046
#define MASK_PERCENT	14047
#define MASK_KBPS			14048
#define MASK_KB				14049
#define MASK_DB				14050

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

CSettingFloat::CSettingFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
:CSetting(iOrder, strSetting, iLabel, iControlType)
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
:CSetting(iOrder, strSetting, iLabel, iControlType)
{
	m_iData = iData;
	m_iMin = iMin;
	m_iMax = iMax;
	m_iStep = iStep;
	m_iFormat = -1;
	if (strFormat)
		m_strFormat = strFormat;
	else
		m_strFormat = "%i";
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat)
:CSetting(iOrder, strSetting, iLabel, iControlType)
{
	m_iData = iData;
	m_iMin = iMin;
	m_iMax = iMax;
	m_iStep = iStep;
	if (iFormat>-1)
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
	if (sscanf(strValue,"%x", &iHexValue))
		SetData(iHexValue);
}

CStdString CSettingHex::ToString()
{
	CStdString strValue;
	strValue.Format("%x", m_iData);
	return strValue;
}

CSettingString::CSettingString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType)
:CSetting(iOrder, strSetting, iLabel, iControlType)
{
	m_strData = strData;
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
	// Pictures settings
	AddGroup(0, 1);
	AddCategory(0, "Slideshow", 108);
	AddInt(1,"Slideshow.StayTime", 224, 9, 1, 1, 100, SPIN_CONTROL_INT_PLUS, MASK_SECS);
	AddInt(2,"Slideshow.TransistionTime", 225, 1500, 100, 100, 10000, SPIN_CONTROL_INT_PLUS, MASK_MS);
	AddInt(3,"Slideshow.MoveAmount", 13311, 20, 0, 1, 40, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
	AddInt(4,"Slideshow.ZoomAmount", 13310, 7, 0, 1, 40, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
	AddInt(5,"Slideshow.BlackBarCompensation", 13312, 20, 0, 1, 40, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
	AddBool(6,"Slideshow.Shuffle", 13319, false);
	AddCategory(0, "Pictures", 14018);
	AddBool(1,"Pictures.HideParentDirItems", 13306, false);
	AddBool(2,"Pictures.UseAutoSwitching", 14011, false);
	AddBool(3,"Pictures.AutoSwitchUseLargeThumbs", 14012, false);
	AddInt(4,"Pictures.AutoSwitchMethod", 14013, 0, 0, 1, 2, SPIN_CONTROL_TEXT);
	AddInt(5,"Pictures.AutoSwitchPercentage", 14014, 50, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
	// Programs settings
	AddGroup(1, 0);
	AddCategory(1, "MyPrograms", 0);
	AddBool(1,"MyPrograms.Flatten", 348, true);
	AddBool(2,"MyPrograms.DefaultXBEOnly", 349, true);
	AddBool(3,"MyPrograms.UseDirectoryName", 506, false);
	AddBool(4,"MyPrograms.NoShortcuts", 508, true);

	AddCategory(1, "ProgramsLists", 14018);
	AddBool(1,"ProgramsLists.HideParentDirItems", 13306, true);
	AddBool(2,"ProgramsLists.UseAutoSwitching", 14011, false);
	AddBool(3,"ProgramsLists.AutoSwitchUseLargeThumbs", 14012, false);
	AddInt(4,"ProgramsLists.AutoSwitchMethod", 14013, 0, 0, 1, 2, SPIN_CONTROL_TEXT);
	AddInt(5,"ProgramsLists.AutoSwitchPercentage", 14014, 50, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
	
	AddCategory(1, "XLinkKai", 714);
	AddBool(1,"XLinkKai.EnableNotifications", 14008, true);
	AddString(2,"XLinkKai.GamesDir", 14009, "f:\\games");
	AddString(3,"XLinkKai.UserName", 709, "");
	AddString(4,"XLinkKai.Password", 710, "", BUTTON_CONTROL_HIDDEN_INPUT);
	AddString(5,"XLinkKai.Server", 14042, "", BUTTON_CONTROL_IP_INPUT);
	// My Weather settings
	AddGroup(2, 8);
	AddCategory(2, "Weather", 8);
	AddInt(1,"Weather.RefreshTime", 397, 30, 15, 15, 120, SPIN_CONTROL_INT_PLUS, MASK_MINS);
	AddInt(2,"Weather.TemperatureUnits", 398, 0, 0, 1, 1, SPIN_CONTROL_TEXT);
	AddInt(3,"Weather.SpeedUnits", 399, 0, 0, 1, 2, SPIN_CONTROL_TEXT);
	AddString(4,"Weather.AreaCode1", 14019, "UKXX0085", BUTTON_CONTROL_STANDARD);
	AddString(5,"Weather.AreaCode2", 14020, "NLXX0002", BUTTON_CONTROL_STANDARD);
	AddString(6,"Weather.AreaCode3", 14021, "CAXX0343", BUTTON_CONTROL_STANDARD);
	// My Music Settings
	AddGroup(3, 2);
	AddCategory(3, "MyMusic", 249);
	AddString(1,"MyMusic.Visualisation", 250, "goom.vis", SPIN_CONTROL_TEXT);
	AddBool(2,"MyMusic.AutoPlayNextItem", 489, true);
	AddBool(3,"MyMusic.Repeat", 488, false);
	AddBool(4,"MyMusic.UseCDDB", 227, true);
	AddBool(5,"MyMusic.UseTags", 258, true);
	AddInt(6,"MyMusic.OSDTimeout", 13314, 5, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_SECS);
	AddCategory(3, "MusicLists", 14018);
	AddBool(1,"MusicLists.HideTrackNumber", 13307, false);
	AddBool(2,"MusicLists.HideParentDirItems", 13306, true);
	AddBool(3,"MusicLists.UseAutoSwitching", 14011, false);
	AddBool(4,"MusicLists.AutoSwitchUseLargeThumbs", 14012, false);
	AddInt(5,"MusicLists.AutoSwitchMethod", 14013, 0, 0, 1, 2, SPIN_CONTROL_TEXT);
	AddInt(6,"MusicLists.AutoSwitchPercentage", 14014, 50, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
	AddCategory(3, "MusicLibrary", 14022);
	AddBool(1,"MusicLibrary.ShufflePlaylistsOnLoad", 228, false);
	AddString(2,"MusicLibrary.Cleanup", 334, "", BUTTON_CONTROL_STANDARD);
	AddString(3,"MusicLibrary.DeleteAlbumInfo", 422, "", BUTTON_CONTROL_STANDARD);
	AddString(4,"MusicLibrary.DeleteCDDBInfo", 423, "", BUTTON_CONTROL_STANDARD);
	
	// music osd 13314
	AddCategory(3, "CDDARipper", 620);
	AddBool(1,"CDDARipper.UseTrackNumber", 624, true);
	AddInt(2,"CDDARipper.Encoder", 621, CDDARIP_ENCODER_LAME, CDDARIP_ENCODER_LAME, 1, CDDARIP_ENCODER_WAV, SPIN_CONTROL_TEXT);
	AddInt(3,"CDDARipper.Quality", 622, CDDARIP_QUALITY_CBR, CDDARIP_QUALITY_CBR, 1, CDDARIP_QUALITY_EXTREME, SPIN_CONTROL_TEXT);
	AddInt(4,"CDDARipper.Bitrate", 623, 192, 128, 32, 320, SPIN_CONTROL_INT_PLUS, MASK_KBPS);

	AddCategory(3, "Karaoke", 13327);
	AddBool(1,"Karaoke.Enabled", 13323, false);
	AddInt(2,"Karaoke.BackgroundAlpha", 13324, 0, 0, 5, 255, SPIN_CONTROL_INT);
	AddInt(3,"Karaoke.ForegroundAlpha", 13325, 255, 0, 5, 255, SPIN_CONTROL_INT);
	AddFloat(4,"Karaoke.SyncDelay", 13326, 0.8f, -3.0f, 0.1f, 3.0f, SPIN_CONTROL_FLOAT);

	AddCategory(3, "AudioOutput", 481);
	AddInt(2,"AudioOutput.Mode", 337, AUDIO_ANALOG, AUDIO_ANALOG, 1, AUDIO_DIGITAL, SPIN_CONTROL_TEXT);
	AddBool(3,"AudioOutput.PCMPassThrough", 13337, true);
	AddBool(4,"AudioOutput.OutputToAllSpeakers", 252, false);
	AddBool(5,"AudioOutput.AC3PassThrough", 364, true);
	AddBool(6,"AudioOutput.DTSPassThrough", 254, true);
	AddBool(7,"AudioOutput.HighQualityResampling", 473, true);
	AddInt(8,"AudioOutput.Headroom", 494, 6, 0, 6, 12, SPIN_CONTROL_INT_PLUS, MASK_DB);
	// don't show the one underneath here
	AddInt(1,"AudioVideo.VolumeAmplification", 290, 0, -200, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_DB);
	// System settings
	AddGroup(4, 13000);
	AddCategory(4, "System", 13000);
	AddInt(1, "System.HDSpinDownTime", 229, 0, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);	// Minutes
	AddInt(2, "System.RemotePlayHDSpinDown", 13001, 0, 0, 1, 3, SPIN_CONTROL_TEXT);	// off, music, video, both
	AddInt(3, "System.RemotePlayHDSpinDownMinDuration", 13004, 20, 0, 1, 20, SPIN_CONTROL_INT_PLUS, MASK_MINS); // Minutes
	AddInt(4, "System.RemotePlayHDSpinDownDelay", 13003, 20, 5, 5, 300, SPIN_CONTROL_INT_PLUS, MASK_SECS);	// seconds
	AddInt(5,"System.ShutDownTime", 357, 0, 0, 5, 120, SPIN_CONTROL_INT_PLUS, MASK_MINS);
	AddBool(6,"System.ShutDownWhilePlaying", 14043, false);
	AddBool(7,"System.FanSpeedControl", 13302, false);
	AddInt(8,"System.FanSpeed", 13300, CFanController::Instance()->GetFanSpeed(), 5, 5, 50, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
	AddBool(9,"System.AutoTemperature", 13301, false);
	AddInt(10,"System.TargetTemperature", 13299, 55, 40, 1, 68, SPIN_CONTROL_TEXT);

	AddCategory(4, "Autorun", 447);
	AddBool(1,"Autorun.DVD", 240, true);
	AddBool(2,"Autorun.VCD", 241, true);
	AddBool(3,"Autorun.CDDA", 242, true);
	AddBool(4,"Autorun.Xbox", 243, true);
	AddBool(5,"Autorun.Video", 244, true);
	AddBool(6,"Autorun.Music", 245, true);
	AddBool(7,"Autorun.Pictures", 246, true);

	AddCategory(4, "Cache", 439);
	AddInt(1,"CacheVideo.HardDisk", 14025, 1024, 256, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
	AddInt(2,"CacheVideo.DVDRom", 14026, 8192, 256, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
	AddInt(3,"CacheVideo.LAN", 14027, 8192, 256, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
	AddInt(4,"CacheVideo.Internet", 14028, 2048, 256, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
	AddInt(5,"CacheAudio.HardDisk", 14029, 256, 256, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
	AddInt(6,"CacheAudio.DVDRom", 14030, 256, 256, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
	AddInt(7,"CacheAudio.LAN", 14031, 256, 256, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
	AddInt(8,"CacheAudio.Internet", 14032, 256, 256, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
	AddInt(9,"CacheDVD.HardDisk", 14033, 16384, 256, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
	AddInt(10,"CacheDVD.DVDRom", 14034, 16384, 256, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
	AddInt(11,"CacheDVD.LAN", 14035, 16384, 256, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);

	AddCategory(4, "LCD", 448);
	AddInt(1,"LCD.Mode", 456, LCD_MODE_NONE, LCD_MODE_NONE, 1, LCD_MODE_NOTV, SPIN_CONTROL_TEXT);
	AddInt(2,"LCD.Type", 4501, LCD_MODE_TYPE_LCD, LCD_MODE_TYPE_LCD, 1, LCD_MODE_TYPE_VFD, SPIN_CONTROL_TEXT);
	AddInt(3,"LCD.ModChip", 471, MODCHIP_SMARTXX, MODCHIP_SMARTXX, 1, MODCHIP_XECUTER3, SPIN_CONTROL_TEXT);
	AddInt(4,"LCD.Columns", 450, 20, 1, 1, 20, SPIN_CONTROL_INT);
	AddInt(5,"LCD.Rows", 455, 4, 1, 1, 4, SPIN_CONTROL_INT);
	AddInt(6,"LCD.BackLight", 463, 80, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
	AddInt(7,"LCD.Contrast", 465, 100, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
	AddHex(8,"LCD.Row1Address", 451, 0, 0, 0x4, 0x100, SPIN_CONTROL_INT_PLUS, "%x");
	AddHex(9,"LCD.Row2Address", 452, 0x40, 0, 0x4, 0x100, SPIN_CONTROL_INT_PLUS, "%x");
	AddHex(10,"LCD.Row3Address", 453, 0x14, 0, 0x4, 0x100, SPIN_CONTROL_INT_PLUS, "%x");
	AddHex(11,"LCD.Row4Address", 454, 0x54, 0, 0x4, 0x100, SPIN_CONTROL_INT_PLUS, "%x");

	// video settings
	AddGroup(5, 3);
	AddCategory(5, "MyVideos", 3);
	AddString(1,"MyVideos.Calibrate", 214, "", BUTTON_CONTROL_STANDARD);
	AddBool(2, "MyVideos.WidescreenSwitching", 223, false);
	AddBool(3, "MyVideos.PAL60Switching", 226, true);
	AddBool(4, "MyVideos.FrameRateConversions", 336, false);
	AddBool(5, "MyVideos.UseGUIResolution", 495, true);
	AddInt(6,"MyVideos.OSDTimeout", 472, 5, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_SECS);

	AddCategory(5, "VideoLists", 14018);
	AddBool(1,"VideoLists.HideParentDirItems", 13306, true);
	AddBool(2,"VideoLists.UseAutoSwitching", 14011, false);
	AddBool(3,"VideoLists.AutoSwitchUseLargeThumbs", 14012, false);
	AddInt(4,"VideoLists.AutoSwitchMethod", 14013, 0, 0, 1, 2, SPIN_CONTROL_TEXT);
	AddInt(5,"VideoLists.AutoSwitchPercentage", 14014, 50, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
	
	AddCategory(5, "PostProcessing", 14041);
	AddBool(1, "PostProcessing.DeInterlace", 285, false);
	AddBool(2, "PostProcessing.Enable", 286, false);
	AddBool(3, "PostProcessing.Auto", 307, true);	// only has effect if PostProcessing.Enable is on.
	AddBool(4, "PostProcessing.VerticalDeBlocking", 308, false);
	AddInt(5, "PostProcessing.VerticalDeBlockLevel", 308, 0, 0, 1, 100, SPIN_CONTROL_INT);
	AddBool(6, "PostProcessing.HorizontalDeBlocking", 309, false);
	AddInt(7, "PostProcessing.HorizontalDeBlockLevel", 309, 0, 0, 1, 100, SPIN_CONTROL_INT);
	AddBool(8, "PostProcessing.AutoBrightnessContrastLevels", 310, false);
	AddBool(9, "PostProcessing.DeRing", 311, false);

	AddCategory(5, "Filters", 230);
	AddInt(1, "Filters.Flicker", 13100, 1, 0, 1, 5, SPIN_CONTROL_INT);
	AddBool(2, "Filters.Soften", 215, false);

	AddCategory(5, "Subtitles", 287);
	AddString(1, "Subtitles.Font", 288, "arial-iso-8859-1", SPIN_CONTROL_TEXT);
	AddInt(2, "Subtitles.Height", 289, 28, 16, 2, 74, SPIN_CONTROL_TEXT);	// use text as there is a disk based lookup needed
	AddInt(3, "Subtitles.Style", 736, XFONT_BOLD, XFONT_NORMAL, 1, XFONT_BOLDITALICS, SPIN_CONTROL_TEXT);
	AddInt(4, "Subtitles.Color", 737, SUBTITLE_COLOR_YELLOW, SUBTITLE_COLOR_YELLOW, 1, SUBTITLE_COLOR_WHITE, SPIN_CONTROL_TEXT);
	AddString(5, "Subtitles.CharSet", 735, "ISO-8859-1", SPIN_CONTROL_TEXT);
	AddBool(6, "Subtitles.FlipBiDiCharSet", 13304, false);
	AddInt(7, "Subtitles.EnlargePercentage", 492, 0, 0, 10, 200, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);

	AddCategory(5, "Audio", 481);

	// network settings
	AddGroup(6, 705);
	AddCategory(6, "Network", 705);
	AddInt(1, "Network.Assignment", 715, NETWORK_DASH, NETWORK_DASH, 1, NETWORK_STATIC, SPIN_CONTROL_TEXT);
	AddString(2, "Network.IPAddress", 719, "192.168.0.3", BUTTON_CONTROL_IP_INPUT);
	AddString(3, "Network.Subnet", 720, "255.255.255.0", BUTTON_CONTROL_IP_INPUT);
	AddString(4, "Network.Gateway", 721, "192.168.0.1", BUTTON_CONTROL_IP_INPUT);
	AddString(5, "Network.DNS", 722, "0.0.0.0", BUTTON_CONTROL_IP_INPUT);
	AddBool(6, "Network.UseHTTPProxy", 708, false);
	AddString(7, "Network.HTTPProxyServer", 706, "");
	AddString(8, "Network.HTTPProxyPort", 707, "8080");
		
	AddCategory(6, "Servers", 14036);
	AddBool(1, "Servers.FTPServer", 167, true);
	AddBool(2, "Servers.TimeServer", 168, false);
	AddString(3, "Servers.TimeAddress", 731, "207.46.130.100");
	AddBool(3, "Servers.WebServer", 263, false);
	AddString(4, "Servers.WebServerPort", 730, "80");
	AddString(5, "Servers.WebServerPassword", 733, "", BUTTON_CONTROL_HIDDEN_INPUT);

	// appearance settings
	AddGroup(7, 480);
	AddCategory(7, "LookAndFeel", 14037);
	AddString(1, "LookAndFeel.Skin", 166, "Project Mayhem", SPIN_CONTROL_TEXT);
	AddInt(2, "LookAndFeel.Resolution", 169, (int)g_graphicsContext.GetVideoResolution(),(int)HDTV_1080i, 1, (int)PAL60_16x9, SPIN_CONTROL_TEXT);
	AddString(3, "LookAndFeel.Language", 248, "english", SPIN_CONTROL_TEXT);
	AddString(4, "LookAndFeel.Font", 13303, "Default", SPIN_CONTROL_TEXT);
	AddString(5, "LookAndFeel.CharSet", 735, "ISO-8859-1", SPIN_CONTROL_TEXT);
	AddBool(6, "LookAndFeel.Clock12Hour", 14051, false);
	AddBool(7, "LookAndFeel.SwapMonthAndDay", 14052, false);
	AddBool(8, "LookAndFeel.EnableRSSFeeds", 13305, true);
	AddString(9, "LookAndFeel.GUICentering", 213, "", BUTTON_CONTROL_STANDARD);

	AddCategory(7, "FileLists", 14018);
	AddBool(1,"FileLists.HideExtensions", 497, false);
	AddBool(2,"FileLists.HideParentDirItems", 13306, false);

	AddCategory(7, "ScreenSaver", 360);
	AddString(1, "ScreenSaver.Mode", 356, "Dim", SPIN_CONTROL_TEXT);
	AddInt(2, "ScreenSaver.Time", 355, 3, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
	AddInt(3, "ScreenSaver.DimLevel", 362, 20, 10, 10, 80, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);

	AddCategory(7, "UIFilters", 14053);
	AddInt(1, "UIFilters.Flicker", 13100, 1, 0, 1, 5, SPIN_CONTROL_INT);
	AddBool(2, "UIFilters.Soften", 215, false);

	AddInt(-1, "UIOffset.X", 0, 0, -128, 1, 128, SPIN_CONTROL_INT);
	AddInt(-1, "UIOffset.Y", 0, 0, -128, 1, 128, SPIN_CONTROL_INT);
}

CGUISettings::~CGUISettings(void)
{
	for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
		delete (*it).second;
	settingsMap.clear();
	for (unsigned int i=0; i<settingsGroups.size(); i++)
		delete settingsGroups[i];
	settingsGroups.clear();
}

void CGUISettings::AddGroup(DWORD dwGroupID, DWORD dwLabelID)
{
	CSettingsGroup *pGroup = new CSettingsGroup(dwGroupID, dwLabelID);
	if (pGroup)
		settingsGroups.push_back(pGroup);
}

void CGUISettings::AddCategory(DWORD dwGroupID, const char *strSetting, DWORD dwLabelID)
{
	for (unsigned int i=0; i<settingsGroups.size(); i++)
	{
		if (settingsGroups[i]->GetGroupID() == dwGroupID)
			settingsGroups[i]->AddCategory(strSetting, dwLabelID);
	}
}

CSettingsGroup *CGUISettings::GetGroup(DWORD dwGroupID)
{
	for (unsigned int i=0; i<settingsGroups.size(); i++)
	{
		if (settingsGroups[i]->GetGroupID() == dwGroupID)
			return settingsGroups[i];
	}
	CLog::DebugLog("Error: Requested setting group (%i) was not found.  It must be case-sensitive", dwGroupID);
	return NULL;
}

void CGUISettings::AddBool(int iOrder, const char *strSetting, int iLabel, bool bData, int iControlType)
{
	CSettingBool* pSetting = new CSettingBool(iOrder, strSetting, iLabel, bData, iControlType);
	if (!pSetting) return;
	settingsMap.insert(pair<CStdString,CSetting*>(strSetting, pSetting));
}

// hmmmm - would be nice to return a bool - what should the default be?
bool CGUISettings::GetBool(const char *strSetting)
{
	mapIter it = settingsMap.find(strSetting);
	if (it != settingsMap.end())
	{	// old category
		return ((CSettingBool*)(*it).second)->GetData();
	}
	// Assert here and write debug output
	CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
	return false;
}

void CGUISettings::SetBool(const char *strSetting, bool bSetting)
{
	mapIter it = settingsMap.find(strSetting);
	if (it != settingsMap.end())
	{	// old category
		((CSettingBool*)(*it).second)->SetData(bSetting);
		return;
	}
	// Assert here and write debug output
	CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::ToggleBool(const char *strSetting)
{
	mapIter it = settingsMap.find(strSetting);
	if (it != settingsMap.end())
	{	// old category
		((CSettingBool*)(*it).second)->SetData(!((CSettingBool *)(*it).second)->GetData());
		return;
	}
	// Assert here and write debug output
	CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::AddFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
{
	CSettingFloat* pSetting = new CSettingFloat(iOrder, strSetting, iLabel, fData, fMin, fStep, fMax, iControlType);
	if (!pSetting) return;
	settingsMap.insert(pair<CStdString,CSetting*>(strSetting,pSetting));
}

float CGUISettings::GetFloat(const char *strSetting)
{
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
	mapIter it = settingsMap.find(strSetting);
	if (it != settingsMap.end())
	{
		((CSettingFloat *)(*it).second)->SetData(fSetting);
		return;
	}
	// Assert here and write debug output
	CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
	CSettingInt* pSetting = new CSettingInt(iOrder, strSetting, iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
	if (!pSetting) return;
	settingsMap.insert(pair<CStdString,CSetting*>(strSetting,pSetting));
}

void CGUISettings::AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat)
{
	CSettingInt* pSetting = new CSettingInt(iOrder, strSetting, iLabel, iData, iMin, iStep, iMax, iControlType, iFormat);
	if (!pSetting) return;
	settingsMap.insert(pair<CStdString,CSetting*>(strSetting,pSetting));
}

void CGUISettings::AddHex(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
	CSettingHex* pSetting = new CSettingHex(iOrder, strSetting, iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
	if (!pSetting) return;
	settingsMap.insert(pair<CStdString,CSetting*>(strSetting,pSetting));
}

int CGUISettings::GetInt(const char *strSetting)
{
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
	mapIter it = settingsMap.find(strSetting);
	if (it != settingsMap.end())
	{
		((CSettingInt *)(*it).second)->SetData(iSetting);
		if (strcmp(strSetting,"LookAndFeel.Resolution") == 0)
			g_guiSettings.m_LookAndFeelResolution = (RESOLUTION)iSetting;
		return;
	}
	// Assert here and write debug output
	CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::AddString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType)
{
	CSettingString* pSetting = new CSettingString(iOrder, strSetting, iLabel, strData, iControlType);
	if (!pSetting) return;
	settingsMap.insert(pair<CStdString,CSetting*>(strSetting,pSetting));
}

CStdString CGUISettings::GetString(const char *strSetting)
{
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
	mapIter it = settingsMap.find(strSetting);
	if (it != settingsMap.end())
	{
		((CSettingString *)(*it).second)->SetData(strData);
		return;
	}
	// Assert here and write debug output
	CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

CSetting *CGUISettings::GetSetting(const char *strSetting)
{
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
		return;
	}
	const TiXmlNode *pChild = pRootElement->FirstChild(strSetting);
	if (pChild)
	{
		if (pChild->FirstChild())
		{
			CStdString strValue=pChild->FirstChild()->Value();
			if (strValue.size() )
			{
				if (strValue != "-")
				{	// update our item
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
	sort(settings.begin(),settings.end(),sortsettings());
}

void CGUISettings::LoadXML(TiXmlElement *pRootElement)
{	// load our stuff...
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
	{
		CStdStringArray strSplit;
		StringUtils::SplitString((*it).first, ".", strSplit);
		if (strSplit.size()>1)
		{
			const TiXmlNode *pChild = pRootElement->FirstChild(strSplit[0].c_str());
			if (pChild)
			{
				const TiXmlNode *pGrandChild = pChild->FirstChild(strSplit[1].c_str());
				if (pGrandChild && pGrandChild->FirstChild())
				{
					CStdString strValue=pGrandChild->FirstChild()->Value();
					if (strValue.size() )
					{
						if (strValue != "-")
						{	// update our item
							(*it).second->FromString(strValue);
						}
					}
				}
			}
		}
	}
	// Get hardware based stuff...
	if (GetInt("AudioOutput.Mode") == AUDIO_DIGITAL && !g_audioConfig.HasDigitalOutput())
		SetInt("AudioOutput.Mode", AUDIO_ANALOG);
	SetBool("AudioOutput.AC3PassThrough", g_audioConfig.GetAC3Enabled());
	SetBool("AudioOutput.DTSPassThrough", g_audioConfig.GetDTSEnabled());
	g_guiSettings.m_LookAndFeelResolution = (RESOLUTION)GetInt("LookAndFeel.Resolution");
	if (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
	{
		SetInt("LookAndFeel.Resolution", g_videoConfig.GetSafeMode());
	}
}

void CGUISettings::SaveXML(TiXmlNode *pRootNode)
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
	{
		CStdStringArray strSplit;
		StringUtils::SplitString((*it).first, ".", strSplit);
		if (strSplit.size()>1)
		{
			TiXmlNode *pChild = pRootNode->FirstChild(strSplit[0].c_str());
			if (!pChild)
			{	// add our group tag
				TiXmlElement newElement(strSplit[0].c_str());
				pChild = pRootNode->InsertEndChild(newElement);
			}

			if (pChild)
			{	// successfully added (or found) our group
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

