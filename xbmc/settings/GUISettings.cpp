/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUISettings.h"
#include <limits.h>
#include <float.h>
#include "Settings.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "storage/MediaManager.h"
#ifdef _LINUX
#include "LinuxTimezone.h"
#endif
#include "Application.h"
#include "filesystem/SpecialProtocol.h"
#include "AdvancedSettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"
#include "tinyXML/tinyxml.h"
#include "windowing/WindowingFactory.h"
#include "powermanagement/PowerManager.h"
#include "cores/dvdplayer/DVDCodecs/Video/CrystalHD.h"
#include "utils/PCMRemap.h"
#include "guilib/GUIFont.h" // for FONT_STYLE_* definitions
#include "guilib/GUIFontManager.h"
#include "utils/Weather.h"
#include "LangInfo.h"
#include "pvr/PVRManager.h"
#if defined(__APPLE__)
  #include "osx/DarwinUtils.h"
#endif

using namespace std;
using namespace ADDON;
using namespace PVR;

// String id's of the masks
#define MASK_DAYS   17999
#define MASK_HOURS  17998
#define MASK_MINS   14044
#define MASK_SECS   14045
#define MASK_MS    14046
#define MASK_PERCENT 14047
#define MASK_KBPS   14048
#define MASK_MB    17997
#define MASK_KB    14049
#define MASK_DB    14050

#define MAX_RESOLUTIONS 128

#define TEXT_OFF  351
#define TEXT_NONE 231

#ifdef _LINUX
#define DEFAULT_VISUALISATION "visualization.glspectrum"
#elif defined(_WIN32)
#ifdef HAS_DX
#define DEFAULT_VISUALISATION "visualization.milkdrop"
#else
#define DEFAULT_VISUALISATION "visualization.glspectrum"
#endif
#endif

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

CStdString CSettingBool::ToString() const
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

CStdString CSettingFloat::ToString() const
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
  m_iFormat = iFormat;
  if (m_iFormat < 0)
    m_strFormat = "%i";
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel,
                         int iData, const map<int,int>& entries, int iControlType)
  : CSetting(iOrder, strSetting, iLabel, iControlType),
    m_entries(entries)
{
  m_iData = iData;
  m_iMin = -1;
  m_iMax = -1;
  m_iStep = 1;
  m_iLabelMin = -1;
}

void CSettingInt::FromString(const CStdString &strValue)
{
  int id = atoi(strValue.c_str());
  SetData(id);
}

CStdString CSettingInt::ToString() const
{
  CStdString strValue;
  strValue.Format("%i", m_iData);
  return strValue;
}

void CSettingHex::FromString(const CStdString &strValue)
{
  int iHexValue;
  if (sscanf(strValue, "%x", (unsigned int *)&iHexValue))
    SetData(iHexValue);
}

CStdString CSettingHex::ToString() const
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

CStdString CSettingString::ToString() const
{
  return m_strData;
}

CSettingPath::CSettingPath(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
    : CSettingString(iOrder, strSetting, iLabel, strData, iControlType, bAllowEmpty, iHeadingString)
{
}

CSettingAddon::CSettingAddon(int iOrder, const char *strSetting, int iLabel, const char *strData, const TYPE type)
  : CSettingString(iOrder, strSetting, iLabel, strData, BUTTON_CONTROL_STANDARD, false, -1)
  , m_type(type)
{
}

void CSettingsGroup::GetCategories(vecSettingsCategory &vecCategories)
{
  vecCategories.clear();
  for (unsigned int i = 0; i < m_vecCategories.size(); i++)
  {
    vecSettings settings;
    // check whether we actually have these settings available.
    g_guiSettings.GetSettingsGroup(m_vecCategories[i]->m_strCategory, settings);
    if (settings.size())
      vecCategories.push_back(m_vecCategories[i]);
  }
}

// Settings are case sensitive
CGUISettings::CGUISettings(void)
{
}

void CGUISettings::Initialize()
{
  ZeroMemory(&m_replayGain, sizeof(ReplayGainSettings));

  // Pictures settings
  AddGroup(0, 1);
  CSettingsCategory* pic = AddCategory(0, "pictures", 14081);
  AddBool(pic, "pictures.usetags", 14082, true);
  AddBool(pic,"pictures.generatethumbs",13360,true);
  AddBool(pic, "pictures.useexifrotation", 20184, true);
  AddBool(pic, "pictures.showvideos", 22022, true);
  // FIXME: hide this setting until it is properly respected. In the meanwhile, default to AUTO.
  AddInt(NULL, "pictures.displayresolution", 169, (int)RES_AUTORES, (int)RES_AUTORES, 1, (int)RES_AUTORES, SPIN_CONTROL_TEXT);

  CSettingsCategory* cat = AddCategory(0, "slideshow", 108);
  AddInt(cat, "slideshow.staytime", 12378, 5, 1, 1, 100, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddBool(cat, "slideshow.displayeffects", 12379, true);
  AddBool(NULL, "slideshow.shuffle", 13319, false);

  // Programs settings
//  AddGroup(1, 0);

  // My Weather settings
  AddGroup(2, 8);
  CSettingsCategory* wea = AddCategory(2, "weather", 16000);
  AddInt(NULL, "weather.currentlocation", 0, 1, 1, 1, 3, SPIN_CONTROL_INT_PLUS);
  AddDefaultAddon(wea, "weather.addon", 24027, "weather.wunderground", ADDON_SCRIPT_WEATHER);
  AddString(wea, "weather.addonsettings", 21417, "", BUTTON_CONTROL_STANDARD, true);

  // My Music Settings
  AddGroup(3, 2);
  CSettingsCategory* ml = AddCategory(3,"musiclibrary",14022);
  AddBool(NULL, "musiclibrary.enabled", 418, true);
  AddBool(ml, "musiclibrary.showcompilationartists", 13414, true);
  AddSeparator(ml,"musiclibrary.sep1");
  AddBool(ml,"musiclibrary.downloadinfo", 20192, false);
  AddDefaultAddon(ml, "musiclibrary.albumsscraper", 20193, "metadata.albums.allmusic.com", ADDON_SCRAPER_ALBUMS);
  AddDefaultAddon(ml, "musiclibrary.artistsscraper", 20194, "metadata.artists.allmusic.com", ADDON_SCRAPER_ARTISTS);
  AddBool(ml, "musiclibrary.updateonstartup", 22000, false);
  AddBool(ml, "musiclibrary.backgroundupdate", 22001, false);
  AddSeparator(ml,"musiclibrary.sep2");
  AddString(ml, "musiclibrary.cleanup", 334, "", BUTTON_CONTROL_STANDARD);
  AddString(ml, "musiclibrary.export", 20196, "", BUTTON_CONTROL_STANDARD);
  AddString(ml, "musiclibrary.import", 20197, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* mp = AddCategory(3, "musicplayer", 14086);
  AddBool(mp, "musicplayer.autoplaynextitem", 489, true);
  AddBool(mp, "musicplayer.queuebydefault", 14084, false);
  AddSeparator(mp, "musicplayer.sep1");
  map<int,int> gain;
  gain.insert(make_pair(351,REPLAY_GAIN_NONE));
  gain.insert(make_pair(639,REPLAY_GAIN_TRACK));
  gain.insert(make_pair(640,REPLAY_GAIN_ALBUM));

  AddInt(mp, "musicplayer.replaygaintype", 638, REPLAY_GAIN_ALBUM, gain, SPIN_CONTROL_TEXT);
  AddInt(NULL, "musicplayer.replaygainpreamp", 641, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddInt(NULL, "musicplayer.replaygainnogainpreamp", 642, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddBool(NULL, "musicplayer.replaygainavoidclipping", 643, false);
  AddInt(mp, "musicplayer.crossfade", 13314, 0, 0, 1, 15, SPIN_CONTROL_INT_PLUS, MASK_SECS, TEXT_OFF);
  AddBool(mp, "musicplayer.crossfadealbumtracks", 13400, true);
  AddSeparator(mp, "musicplayer.sep3");
  AddDefaultAddon(mp, "musicplayer.visualisation", 250, DEFAULT_VISUALISATION, ADDON_VIZ);

  CSettingsCategory* mf = AddCategory(3, "musicfiles", 14081);
  AddBool(mf, "musicfiles.usetags", 258, true);
  AddString(mf, "musicfiles.trackformat", 13307, "[%N. ]%A - %T", EDIT_CONTROL_INPUT, false, 16016);
  AddString(mf, "musicfiles.trackformatright", 13387, "%D", EDIT_CONTROL_INPUT, false, 16016);
  // advanced per-view trackformats.
  AddString(NULL, "musicfiles.nowplayingtrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(NULL, "musicfiles.nowplayingtrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(NULL, "musicfiles.librarytrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(NULL, "musicfiles.librarytrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddBool(mf, "musicfiles.findremotethumbs", 14059, true);

  CSettingsCategory* scr = AddCategory(3, "scrobbler", 15221);
  AddBool(scr, "scrobbler.lastfmsubmit", 15201, false);
  AddBool(scr, "scrobbler.lastfmsubmitradio", 15250, false);
  AddString(scr,"scrobbler.lastfmusername", 15202, "", EDIT_CONTROL_INPUT, false, 15202);
  AddString(scr,"scrobbler.lastfmpass", 15203, "", EDIT_CONTROL_MD5_INPUT, false, 15203);
  AddSeparator(scr, "scrobbler.sep1");
  AddBool(scr, "scrobbler.librefmsubmit", 15217, false);
  AddString(scr, "scrobbler.librefmusername", 15218, "", EDIT_CONTROL_INPUT, false, 15218);
  AddString(scr, "scrobbler.librefmpass", 15219, "", EDIT_CONTROL_MD5_INPUT, false, 15219);

  CSettingsCategory* acd = AddCategory(3, "audiocds", 620);
  AddBool(acd, "audiocds.autorun", 14085, false);
  AddBool(acd, "audiocds.usecddb", 227, true);
  AddSeparator(acd, "audiocds.sep1");
  AddPath(acd,"audiocds.recordingpath",20000,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false,657);
  AddString(acd, "audiocds.trackpathformat", 13307, "%A - %B/[%N. ][%A - ]%T", EDIT_CONTROL_INPUT, false, 16016);
  map<int,int> encoders;
  encoders.insert(make_pair(34000,CDDARIP_ENCODER_LAME));
  encoders.insert(make_pair(34001,CDDARIP_ENCODER_VORBIS));
  encoders.insert(make_pair(34002,CDDARIP_ENCODER_WAV));
  encoders.insert(make_pair(34005,CDDARIP_ENCODER_FLAC));
  AddInt(acd, "audiocds.encoder", 621, CDDARIP_ENCODER_FLAC, encoders, SPIN_CONTROL_TEXT);

  map<int,int> qualities;
  qualities.insert(make_pair(604,CDDARIP_QUALITY_CBR));
  qualities.insert(make_pair(601,CDDARIP_QUALITY_MEDIUM));
  qualities.insert(make_pair(602,CDDARIP_QUALITY_STANDARD));
  qualities.insert(make_pair(603,CDDARIP_QUALITY_EXTREME));
  AddInt(acd, "audiocds.quality", 622, CDDARIP_QUALITY_CBR, qualities, SPIN_CONTROL_TEXT);
  AddInt(acd, "audiocds.bitrate", 623, 192, 128, 32, 320, SPIN_CONTROL_INT_PLUS, MASK_KBPS);
  AddInt(acd, "audiocds.compressionlevel", 665, 5, 0, 1, 8, SPIN_CONTROL_INT_PLUS);

#ifdef HAS_KARAOKE
  CSettingsCategory* kar = AddCategory(3, "karaoke", 13327);
  AddBool(kar, "karaoke.enabled", 13323, false);
  // auto-popup the song selector dialog when the karaoke song was just finished and playlist is empty.
  AddBool(kar, "karaoke.autopopupselector", 22037, false);
  AddSeparator(kar, "karaoke.sep1");
  AddString(kar, "karaoke.font", 22030, "arial.ttf", SPIN_CONTROL_TEXT);
  AddInt(kar, "karaoke.fontheight", 22031, 36, 16, 2, 74, SPIN_CONTROL_TEXT); // use text as there is a disk based lookup needed
  map<int,int> colors;
  for (int i = KARAOKE_COLOR_START; i <= KARAOKE_COLOR_END; i++)
    colors.insert(make_pair(22040 + i, i));
  AddInt(kar, "karaoke.fontcolors", 22032, KARAOKE_COLOR_START, colors, SPIN_CONTROL_TEXT);
  AddString(kar, "karaoke.charset", 22033, "DEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(kar,"karaoke.sep2");
  AddString(kar, "karaoke.export", 22038, "", BUTTON_CONTROL_STANDARD);
  AddString(kar, "karaoke.importcsv", 22036, "", BUTTON_CONTROL_STANDARD);
#endif

  // System settings
  AddGroup(4, 13000);
  CSettingsCategory* vs = AddCategory(4, "videoscreen", 21373);

#if (defined(__APPLE__) && defined(__arm__))
  // define but hide display, resolution and blankdisplays settings on atv2/ios, they are not user controlled
  AddInt(NULL, "videoscreen.screen", 240, 0, -1, 1, g_Windowing.GetNumScreens(), SPIN_CONTROL_TEXT);
  AddInt(NULL, "videoscreen.resolution", 131, -1, 0, 1, INT_MAX, SPIN_CONTROL_TEXT);
  AddBool(NULL, "videoscreen.blankdisplays", 13130, false);
#else
  // this setting would ideally not be saved, as its value is systematically derived from videoscreen.screenmode.
  // contains a DISPLAYMODE
  AddInt(vs, "videoscreen.screen", 240, 0, -1, 1, g_Windowing.GetNumScreens(), SPIN_CONTROL_TEXT);
  // this setting would ideally not be saved, as its value is systematically derived from videoscreen.screenmode.
  // contains an index to the g_settings.m_ResInfo array. the only meaningful fields are iScreen, iWidth, iHeight.
#if defined (__APPLE__)
  AddInt(vs, "videoscreen.resolution", 131, -1, 0, 1, INT_MAX, SPIN_CONTROL_TEXT);
#else
  AddInt(vs, "videoscreen.resolution", 169, -1, 0, 1, INT_MAX, SPIN_CONTROL_TEXT);
#endif
  AddString(g_application.IsStandAlone() ? vs : NULL, "videoscreen.screenmode", 243, "DESKTOP", SPIN_CONTROL_TEXT);

#if defined(_WIN32) || defined (__APPLE__)
  // We prefer a fake fullscreen mode (window covering the screen rather than dedicated fullscreen)
  // as it works nicer with switching to other applications. However on some systems vsync is broken
  // when we do this (eg non-Aero on ATI in particular) and on others (AppleTV) we can't get XBMC to
  // the front
  bool fakeFullScreen = true;
  bool showSetting = true;
  if (g_sysinfo.IsAeroDisabled())
    fakeFullScreen = false;

#if defined(_WIN32) && defined(HAS_GL)
  fakeFullScreen = true;
  showSetting = false;
#endif

#if defined (__APPLE__)
  if (g_sysinfo.IsAppleTV())
  {
    fakeFullScreen = false;
  }
  showSetting = false;
#endif
  AddBool(showSetting ? vs : NULL, "videoscreen.fakefullscreen", 14083, fakeFullScreen);
  AddBool(vs, "videoscreen.blankdisplays", 13130, false);
  AddSeparator(vs, "videoscreen.sep1");
#endif
#endif

  map<int,int> vsync;
#if defined(_LINUX) && !defined(__APPLE__)
  vsync.insert(make_pair(13101,VSYNC_DRIVER));
#endif
  vsync.insert(make_pair(13106,VSYNC_DISABLED));
  vsync.insert(make_pair(13107,VSYNC_VIDEO));
  vsync.insert(make_pair(13108,VSYNC_ALWAYS));
  AddInt(vs, "videoscreen.vsync", 13105, DEFAULT_VSYNC, vsync, SPIN_CONTROL_TEXT);

  AddString(vs, "videoscreen.guicalibration",214,"", BUTTON_CONTROL_STANDARD);
#ifndef HAS_DX
  // Todo: Implement test pattern for DX
  AddString(vs, "videoscreen.testpattern",226,"", BUTTON_CONTROL_STANDARD);
#endif
#if defined(HAS_LCD)
  AddBool(vs, "videoscreen.haslcd", 4501, false);
#endif

  CSettingsCategory* ao = AddCategory(4, "audiooutput", 772);

  map<int,int> audiomode;
  audiomode.insert(make_pair(338,AUDIO_ANALOG));
  audiomode.insert(make_pair(339,AUDIO_IEC958));
  audiomode.insert(make_pair(420,AUDIO_HDMI  ));
  AddInt(ao, "audiooutput.mode", 337, AUDIO_ANALOG, audiomode, SPIN_CONTROL_TEXT);

  map<int,int> channelLayout;
  for(int layout = 0; layout < PCM_MAX_LAYOUT; ++layout)
    channelLayout.insert(make_pair(34101+layout, layout));
  AddInt(ao, "audiooutput.channellayout", 34100, PCM_LAYOUT_2_0, channelLayout, SPIN_CONTROL_TEXT);
  AddBool(ao, "audiooutput.dontnormalizelevels", 346, true);

#if (defined(__APPLE__) && defined(__arm__))
  AddBool(g_sysinfo.IsAppleTV2() ? ao : NULL, "audiooutput.ac3passthrough", 364, false);
  AddBool(g_sysinfo.IsAppleTV2() ? ao : NULL, "audiooutput.dtspassthrough", 254, false);
#else
  AddBool(ao, "audiooutput.ac3passthrough", 364, true);
  AddBool(ao, "audiooutput.dtspassthrough", 254, true);
#endif
  AddBool(NULL, "audiooutput.passthroughaac", 299, false);
  AddBool(NULL, "audiooutput.passthroughmp1", 300, false);
  AddBool(NULL, "audiooutput.passthroughmp2", 301, false);
  AddBool(NULL, "audiooutput.passthroughmp3", 302, false);

#ifdef __APPLE__
  AddString(ao, "audiooutput.audiodevice", 545, "Default", SPIN_CONTROL_TEXT);
#elif defined(_LINUX)
  AddSeparator(ao, "audiooutput.sep1");
  AddString(ao, "audiooutput.audiodevice", 545, "default", SPIN_CONTROL_TEXT);
  AddString(ao, "audiooutput.customdevice", 1300, "", EDIT_CONTROL_INPUT);
  AddSeparator(ao, "audiooutput.sep2");
  AddString(ao, "audiooutput.passthroughdevice", 546, "iec958", SPIN_CONTROL_TEXT);
  AddString(ao, "audiooutput.custompassthrough", 1301, "", EDIT_CONTROL_INPUT);
  AddSeparator(ao, "audiooutput.sep3");
#elif defined(_WIN32)
  AddString(ao, "audiooutput.audiodevice", 545, "Default", SPIN_CONTROL_TEXT);
#endif

  CSettingsCategory* in = AddCategory(4, "input", 14094);
  AddString(in, "input.peripherals", 35000, "", BUTTON_CONTROL_STANDARD);
#if defined(__APPLE__)
  map<int,int> remotemode;
  remotemode.insert(make_pair(13610,APPLE_REMOTE_DISABLED));
  remotemode.insert(make_pair(13611,APPLE_REMOTE_STANDARD));
  remotemode.insert(make_pair(13612,APPLE_REMOTE_UNIVERSAL));
  remotemode.insert(make_pair(13613,APPLE_REMOTE_MULTIREMOTE));
  AddInt(in, "input.appleremotemode", 13600, APPLE_REMOTE_STANDARD, remotemode, SPIN_CONTROL_TEXT);
#if !defined(__arm__)
  AddBool(in, "input.appleremotealwayson", 13602, false);
#else
  AddBool(NULL, "input.appleremotealwayson", 13602, false);
#endif
  AddInt(NULL, "input.appleremotesequencetime", 13603, 500, 50, 50, 1000, SPIN_CONTROL_INT_PLUS, MASK_MS, TEXT_OFF);
  AddSeparator(in, "input.sep1");
#endif
  AddBool(in, "input.remoteaskeyboard", 21449, false);
#if defined(TARGET_DARWIN_IOS)
  AddBool(NULL, "input.enablemouse", 21369, true);
#else
  AddBool(in, "input.enablemouse", 21369, true);
#endif

  CSettingsCategory* pwm = AddCategory(4, "powermanagement", 14095);
  // Note: Application.cpp might hide powersaving settings if not supported.
  AddInt(pwm, "powermanagement.displaysoff", 1450, 0, 0, 5, 120, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  AddInt(pwm, "powermanagement.shutdowntime", 357, 0, 0, 5, 120, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);

  map<int,int> shutdown;
  if (g_powerManager.CanPowerdown())
    shutdown.insert(make_pair(13005,POWERSTATE_SHUTDOWN));

  if (g_powerManager.CanHibernate())
    shutdown.insert(make_pair(13010,POWERSTATE_HIBERNATE));

  if (g_powerManager.CanSuspend())
    shutdown.insert(make_pair(13011,POWERSTATE_SUSPEND));

  // In standalone mode we default to another.
  if (g_application.IsStandAlone())
    AddInt(pwm, "powermanagement.shutdownstate", 13008, POWERSTATE_SHUTDOWN, shutdown, SPIN_CONTROL_TEXT);
  else
  {
    shutdown.insert(make_pair(13009,POWERSTATE_QUIT));
    shutdown.insert(make_pair(13014,POWERSTATE_MINIMIZE));
    AddInt(pwm, "powermanagement.shutdownstate", 13008, POWERSTATE_QUIT, shutdown, SPIN_CONTROL_TEXT);
  }

  CSettingsCategory* dbg = AddCategory(4, "debug", 14092);
  AddBool(dbg, "debug.showloginfo", 20191, false);
  AddPath(dbg, "debug.screenshotpath",20004,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false,657);

  CSettingsCategory* mst = AddCategory(4, "masterlock", 12360);
  AddString(mst, "masterlock.lockcode"       , 20100, "-", BUTTON_CONTROL_STANDARD);
  AddBool(mst, "masterlock.startuplock"      , 20076,false);
  // hidden masterlock settings
  AddInt(NULL,"masterlock.maxretries", 12364, 3, 3, 1, 100, SPIN_CONTROL_TEXT);

  AddCategory(4, "cache", 439);
  AddInt(NULL, "cache.harddisk", 14025, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(NULL, "cache.sep1");
  AddInt(NULL, "cachevideo.dvdrom", 14026, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(NULL, "cachevideo.lan", 14027, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(NULL, "cachevideo.internet", 14028, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(NULL, "cache.sep2");
  AddInt(NULL, "cacheaudio.dvdrom", 14030, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(NULL, "cacheaudio.lan", 14031, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(NULL, "cacheaudio.internet", 14032, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(NULL, "cache.sep3");
  AddInt(NULL, "cachedvd.dvdrom", 14034, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(NULL, "cachedvd.lan", 14035, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(NULL, "cache.sep4");
  AddInt(NULL, "cacheunknown.internet", 14060, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);

  // video settings
  AddGroup(5, 3);
  CSettingsCategory* vdl = AddCategory(5, "videolibrary", 14022);
  AddBool(NULL, "videolibrary.enabled", 418, true);
  AddBool(vdl, "videolibrary.showunwatchedplots", 20369, true);
  AddBool(NULL, "videolibrary.seasonthumbs", 20382, true);
  AddBool(vdl, "videolibrary.actorthumbs", 20402, true);

  map<int, int> flattenTVShowOptions;
  flattenTVShowOptions.insert(make_pair(20420, 0));
  flattenTVShowOptions.insert(make_pair(20421, 1));
  flattenTVShowOptions.insert(make_pair(20422, 2));
  AddInt(vdl, "videolibrary.flattentvshows", 20412, 1, flattenTVShowOptions, SPIN_CONTROL_TEXT);

  AddBool(vdl, "videolibrary.groupmoviesets", 20458, false);
  AddBool(vdl, "videolibrary.updateonstartup", 22000, false);
  AddBool(vdl, "videolibrary.backgroundupdate", 22001, false);
  AddSeparator(vdl, "videolibrary.sep3");
  AddString(vdl, "videolibrary.cleanup", 334, "", BUTTON_CONTROL_STANDARD);
  AddString(vdl, "videolibrary.export", 647, "", BUTTON_CONTROL_STANDARD);
  AddString(vdl, "videolibrary.import", 648, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* vp = AddCategory(5, "videoplayer", 14086);

  map<int, int> renderers;
  renderers.insert(make_pair(13416, RENDER_METHOD_AUTO));

#ifdef HAS_DX
  if (g_sysinfo.IsVistaOrHigher())
    renderers.insert(make_pair(16319, RENDER_METHOD_DXVA));
  renderers.insert(make_pair(13431, RENDER_METHOD_D3D_PS));
  renderers.insert(make_pair(13419, RENDER_METHOD_SOFTWARE));
#endif

#ifdef HAS_GL
  renderers.insert(make_pair(13417, RENDER_METHOD_ARB));
  renderers.insert(make_pair(13418, RENDER_METHOD_GLSL));
  renderers.insert(make_pair(13419, RENDER_METHOD_SOFTWARE));
#endif
  AddInt(vp, "videoplayer.rendermethod", 13415, RENDER_METHOD_AUTO, renderers, SPIN_CONTROL_TEXT);

#ifdef HAVE_LIBVDPAU
  AddBool(vp, "videoplayer.usevdpau", 13425, true);
#endif
#ifdef HAVE_LIBVA
  AddBool(vp, "videoplayer.usevaapi", 13426, true);
#endif
#ifdef HAS_DX
  AddBool(g_sysinfo.IsVistaOrHigher() ? vp: NULL, "videoplayer.usedxva2", 13427, g_sysinfo.IsVistaOrHigher() ? true : false);
#endif
#ifdef HAVE_LIBCRYSTALHD
  AddBool(CCrystalHD::GetInstance()->DevicePresent() ? vp: NULL, "videoplayer.usechd", 13428, true);
#endif
#ifdef HAVE_LIBVDADECODER
  AddBool(g_sysinfo.HasVDADecoder() ? vp: NULL, "videoplayer.usevda", 13429, true);
#endif
#ifdef HAVE_LIBOPENMAX
  AddBool(vp, "videoplayer.useomx", 13430, true);
#endif
#ifdef HAVE_VIDEOTOOLBOXDECODER
  AddBool(g_sysinfo.HasVideoToolBoxDecoder() ? vp: NULL, "videoplayer.usevideotoolbox", 13432, true);
#endif

#ifdef HAS_GL
  AddBool(NULL, "videoplayer.usepbo", 13424, true);
#endif

  // FIXME: hide this setting until it is properly respected. In the meanwhile, default to AUTO.
  //AddInt(5, "videoplayer.displayresolution", 169, (int)RES_AUTORES, (int)RES_AUTORES, 1, (int)CUSTOM+MAX_RESOLUTIONS, SPIN_CONTROL_TEXT);
  AddInt(NULL, "videoplayer.displayresolution", 169, (int)RES_AUTORES, (int)RES_AUTORES, 1, (int)RES_AUTORES, SPIN_CONTROL_TEXT);
#if !(defined(__APPLE__) && defined(__arm__))
  AddBool(vp, "videoplayer.adjustrefreshrate", 170, false);
  AddInt(vp, "videoplayer.pauseafterrefreshchange", 13550, 0, 0, 1, MAXREFRESHCHANGEDELAY, SPIN_CONTROL_TEXT);
#else
  AddBool(NULL, "videoplayer.adjustrefreshrate", 170, false);
  AddInt(NULL, "videoplayer.pauseafterrefreshchange", 13550, 0, 0, 1, MAXREFRESHCHANGEDELAY, SPIN_CONTROL_TEXT);
#endif
  //sync settings not available on windows gl build
#if defined(_WIN32) && defined(HAS_GL)
  #define SYNCSETTINGS 0
#else
  #define SYNCSETTINGS 1
#endif
  AddBool(SYNCSETTINGS ? vp : NULL, "videoplayer.usedisplayasclock", 13510, false);

  map<int, int> syncTypes;
  syncTypes.insert(make_pair(13501, SYNC_DISCON));
  syncTypes.insert(make_pair(13502, SYNC_SKIPDUP));
  syncTypes.insert(make_pair(13503, SYNC_RESAMPLE));
  AddInt(SYNCSETTINGS ? vp : NULL, "videoplayer.synctype", 13500, SYNC_RESAMPLE, syncTypes, SPIN_CONTROL_TEXT);
  AddFloat(NULL, "videoplayer.maxspeedadjust", 13504, 5.0f, 0.0f, 0.1f, 10.0f);

  map<int, int> resampleQualities;
  resampleQualities.insert(make_pair(13506, RESAMPLE_LOW));
  resampleQualities.insert(make_pair(13507, RESAMPLE_MID));
  resampleQualities.insert(make_pair(13508, RESAMPLE_HIGH));
  resampleQualities.insert(make_pair(13509, RESAMPLE_REALLYHIGH));
  AddInt(NULL, "videoplayer.resamplequality", 13505, RESAMPLE_MID, resampleQualities, SPIN_CONTROL_TEXT);
  AddInt(vp, "videoplayer.errorinaspect", 22021, 0, 0, 1, 20, SPIN_CONTROL_INT_PLUS, MASK_PERCENT, TEXT_NONE);

  map<int,int> stretch;
  stretch.insert(make_pair(630,VIEW_MODE_NORMAL));
  stretch.insert(make_pair(633,VIEW_MODE_WIDE_ZOOM));
  stretch.insert(make_pair(634,VIEW_MODE_STRETCH_16x9));
  stretch.insert(make_pair(631,VIEW_MODE_ZOOM));
  AddInt(vp, "videoplayer.stretch43", 173, VIEW_MODE_NORMAL, stretch, SPIN_CONTROL_TEXT);
#ifdef HAVE_LIBVDPAU
  AddBool(NULL, "videoplayer.vdpau_allow_xrandr", 13122, false);
#endif
#if defined(HAS_GL) || HAS_GLES == 2  // May need changing for GLES
  AddSeparator(vp, "videoplayer.sep1.5");
#ifdef HAVE_LIBVDPAU
  AddBool(NULL, "videoplayer.vdpauUpscalingLevel", 13121, false);
  AddBool(vp, "videoplayer.vdpaustudiolevel", 13122, false);
#endif
#endif
  AddSeparator(vp, "videoplayer.sep5");
  AddBool(vp, "videoplayer.teletextenabled", 23050, true);

  CSettingsCategory* vid = AddCategory(5, "myvideos", 14081);

  map<int, int> myVideosSelectActions;
  myVideosSelectActions.insert(make_pair(22080, SELECT_ACTION_CHOOSE));
  myVideosSelectActions.insert(make_pair(208,   SELECT_ACTION_PLAY_OR_RESUME));
  myVideosSelectActions.insert(make_pair(13404, SELECT_ACTION_RESUME));
  myVideosSelectActions.insert(make_pair(22081, SELECT_ACTION_INFO));
  
  AddInt(vid, "myvideos.selectaction", 22079, SELECT_ACTION_PLAY_OR_RESUME, myVideosSelectActions, SPIN_CONTROL_TEXT);
  AddBool(NULL, "myvideos.treatstackasfile", 20051, true);
  AddBool(vid, "myvideos.extractflags",20433, true);
  AddBool(vid, "myvideos.filemetadata", 20419, true);
  AddBool(NULL, "myvideos.extractthumb",20433, true);

  CSettingsCategory* sub = AddCategory(5, "subtitles", 287);
  AddString(sub, "subtitles.font", 14089, "arial.ttf", SPIN_CONTROL_TEXT);
  AddInt(sub, "subtitles.height", 289, 28, 16, 2, 74, SPIN_CONTROL_TEXT); // use text as there is a disk based lookup needed

  map<int, int> fontStyles;
  fontStyles.insert(make_pair(738, FONT_STYLE_NORMAL));
  fontStyles.insert(make_pair(739, FONT_STYLE_BOLD));
  fontStyles.insert(make_pair(740, FONT_STYLE_ITALICS));
  fontStyles.insert(make_pair(741, FONT_STYLE_BOLD_ITALICS));

  AddInt(sub, "subtitles.style", 736, FONT_STYLE_BOLD, fontStyles, SPIN_CONTROL_TEXT);
  AddInt(sub, "subtitles.color", 737, SUBTITLE_COLOR_START + 1, SUBTITLE_COLOR_START, 1, SUBTITLE_COLOR_END, SPIN_CONTROL_TEXT);
  AddString(sub, "subtitles.charset", 735, "DEFAULT", SPIN_CONTROL_TEXT);
  AddBool(sub,"subtitles.overrideassfonts", 21368, false);
  AddSeparator(sub, "subtitles.sep1");
  AddPath(sub, "subtitles.custompath", 21366, "", BUTTON_CONTROL_PATH_INPUT, false, 657);

  map<int, int> subtitleAlignments;
  subtitleAlignments.insert(make_pair(21461, SUBTITLE_ALIGN_MANUAL));
  subtitleAlignments.insert(make_pair(21462, SUBTITLE_ALIGN_BOTTOM_INSIDE));
  subtitleAlignments.insert(make_pair(21463, SUBTITLE_ALIGN_BOTTOM_OUTSIDE));
  subtitleAlignments.insert(make_pair(21464, SUBTITLE_ALIGN_TOP_INSIDE));
  subtitleAlignments.insert(make_pair(21465, SUBTITLE_ALIGN_TOP_OUTSIDE));
  AddInt(sub, "subtitles.align", 21460, SUBTITLE_ALIGN_MANUAL, subtitleAlignments, SPIN_CONTROL_TEXT);

  CSettingsCategory* dvd = AddCategory(5, "dvds", 14087);
  AddBool(dvd, "dvds.autorun", 14088, false);
  AddInt(dvd, "dvds.playerregion", 21372, 0, 0, 1, 8, SPIN_CONTROL_INT_PLUS, -1, TEXT_OFF);
  AddBool(dvd, "dvds.automenu", 21882, false);

  AddDefaultAddon(NULL, "scrapers.moviesdefault", 21413, "metadata.themoviedb.org", ADDON_SCRAPER_MOVIES);
  AddDefaultAddon(NULL, "scrapers.tvshowsdefault", 21414, "metadata.tvdb.com", ADDON_SCRAPER_TVSHOWS);
  AddDefaultAddon(NULL, "scrapers.musicvideosdefault", 21415, "metadata.yahoomusic.com", ADDON_SCRAPER_MUSICVIDEOS);
  AddBool(NULL, "scrapers.langfallback", 21416, false);

  // network settings
  AddGroup(6, 705);

  CSettingsCategory* srv = AddCategory(6, "services", 14036);
  AddString(srv,"services.devicename", 1271, "XBMC", EDIT_CONTROL_INPUT);
  AddSeparator(srv,"services.sep4");
  AddBool(srv, "services.upnpserver", 21360, false);
  AddBool(srv, "services.upnprenderer", 21881, false);
  AddSeparator(srv,"services.sep3");
#ifdef HAS_WEB_SERVER
  AddBool(srv,  "services.webserver",        263, false);
#ifdef _LINUX
  AddString(srv,"services.webserverport",    730, (geteuid()==0)?"80":"8080", EDIT_CONTROL_NUMBER_INPUT, false, 730);
#else
  AddString(srv,"services.webserverport",    730, "80", EDIT_CONTROL_NUMBER_INPUT, false, 730);
#endif
  AddString(srv,"services.webserverusername",1048, "xbmc", EDIT_CONTROL_INPUT);
  AddString(srv,"services.webserverpassword",733, "", EDIT_CONTROL_HIDDEN_INPUT, true, 733);
  AddDefaultAddon(srv, "services.webskin",199, DEFAULT_WEB_INTERFACE, ADDON_WEB_INTERFACE);
#endif
#ifdef HAS_EVENT_SERVER
  AddSeparator(srv,"services.sep1");
  AddBool(srv,  "services.esenabled",         791, true);
  AddString(NULL,"services.esport",            792, "9777", EDIT_CONTROL_NUMBER_INPUT, false, 792);
  AddInt(NULL,   "services.esportrange",       793, 10, 1, 1, 100, SPIN_CONTROL_INT);
  AddInt(NULL,   "services.esmaxclients",      797, 20, 1, 1, 100, SPIN_CONTROL_INT);
  AddBool(srv,  "services.esallinterfaces",   794, false);
  AddInt(NULL,   "services.esinitialdelay",    795, 750, 5, 5, 10000, SPIN_CONTROL_INT);
  AddInt(NULL,   "services.escontinuousdelay", 796, 25, 5, 5, 10000, SPIN_CONTROL_INT);
#endif
#ifdef HAS_ZEROCONF
  AddSeparator(srv, "services.sep2");
#ifdef TARGET_WINDOWS
  AddBool(srv, "services.zeroconf", 1260, false);
#else
  AddBool(srv, "services.zeroconf", 1260, true);
#endif
#endif

#ifdef HAS_AIRPLAY
  AddSeparator(srv, "services.sep5");
  AddBool(srv, "services.airplay", 1270, false);
  AddBool(srv, "services.useairplaypassword", 1272, false);
  AddString(srv, "services.airplaypassword", 733, "", EDIT_CONTROL_HIDDEN_INPUT, false, 733);
  AddSeparator(srv, "services.sep6");  
#endif

#ifndef _WIN32
  CSettingsCategory* smb = AddCategory(6, "smb", 1200);
  AddString(smb, "smb.winsserver",  1207,   "",  EDIT_CONTROL_IP_INPUT);
  AddString(smb, "smb.workgroup",   1202,   "WORKGROUP", EDIT_CONTROL_INPUT, false, 1202);
#endif

  CSettingsCategory* net = AddCategory(6, "network", 798);
  if (g_application.IsStandAlone())
  {
#ifndef __APPLE__
    AddString(NULL, "network.interface",775,"", SPIN_CONTROL_TEXT);

    map<int, int> networkAssignments;
    networkAssignments.insert(make_pair(716, NETWORK_DHCP));
    networkAssignments.insert(make_pair(717, NETWORK_STATIC));
    networkAssignments.insert(make_pair(787, NETWORK_DISABLED));
    AddInt(NULL, "network.assignment", 715, NETWORK_DHCP, networkAssignments, SPIN_CONTROL_TEXT);
    AddString(NULL, "network.ipaddress", 719, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
    AddString(NULL, "network.subnet", 720, "255.255.255.0", EDIT_CONTROL_IP_INPUT);
    AddString(NULL, "network.gateway", 721, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
    AddString(NULL, "network.dns", 722, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
    AddString(NULL, "network.dnssuffix", 22002, "", EDIT_CONTROL_INPUT, true);
    AddString(NULL, "network.essid", 776, "0.0.0.0", BUTTON_CONTROL_STANDARD);

    map<int, int> networkEncapsulations;
    networkEncapsulations.insert(make_pair(780, ENC_NONE));
    networkEncapsulations.insert(make_pair(781, ENC_WEP));
    networkEncapsulations.insert(make_pair(782, ENC_WPA));
    networkEncapsulations.insert(make_pair(783, ENC_WPA2));
    AddInt(NULL, "network.enc", 778, ENC_NONE, networkEncapsulations, SPIN_CONTROL_TEXT);
    AddString(NULL, "network.key", 777, "0.0.0.0", EDIT_CONTROL_INPUT);
#ifndef _WIN32
    AddString(NULL, "network.save", 779, "", BUTTON_CONTROL_STANDARD);
#endif
    AddSeparator(NULL, "network.sep1");
#endif
  }
  AddBool(net, "network.usehttpproxy", 708, false);
  AddString(net, "network.httpproxyserver", 706, "", EDIT_CONTROL_INPUT);
  AddString(net, "network.httpproxyport", 730, "8080", EDIT_CONTROL_NUMBER_INPUT, false, 707);
  AddString(net, "network.httpproxyusername", 1048, "", EDIT_CONTROL_INPUT);
  AddString(net, "network.httpproxypassword", 733, "", EDIT_CONTROL_HIDDEN_INPUT,true,733);
  AddInt(net, "network.bandwidth", 14041, 0, 0, 512, 100*1024, SPIN_CONTROL_INT_PLUS, MASK_KBPS, TEXT_OFF);

  // appearance settings
  AddGroup(7, 480);
  CSettingsCategory* laf = AddCategory(7,"lookandfeel", 166);
  AddDefaultAddon(laf, "lookandfeel.skin",166,DEFAULT_SKIN, ADDON_SKIN);
  AddString(laf, "lookandfeel.skintheme",15111,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(laf, "lookandfeel.skincolors",14078, "SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(laf, "lookandfeel.font",13303,"Default", SPIN_CONTROL_TEXT);
  AddInt(laf, "lookandfeel.skinzoom",20109, 0, -20, 2, 20, SPIN_CONTROL_INT, MASK_PERCENT);
  AddInt(laf, "lookandfeel.startupwindow",512,1, WINDOW_HOME, 1, WINDOW_PYTHON_END, SPIN_CONTROL_TEXT);
  AddString(laf, "lookandfeel.soundskin",15108,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(laf, "lookandfeel.sep2");
  AddBool(laf, "lookandfeel.enablerssfeeds",13305,  true);
  AddString(laf, "lookandfeel.rssedit", 21450, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* loc = AddCategory(7, "locale", 14090);
  AddString(loc, "locale.language",248,"english", SPIN_CONTROL_TEXT);
  AddString(loc, "locale.country", 20026, "USA", SPIN_CONTROL_TEXT);
  AddString(loc, "locale.charset", 14091, "DEFAULT", SPIN_CONTROL_TEXT); // charset is set by the language file
  
  bool use_timezone = false;
  
#if defined(_LINUX)
#if defined(__APPLE__)
  if (g_sysinfo.IsAppleTV2() && GetIOSVersion() < 4.3)
#endif
    use_timezone = true;  
  
  if (use_timezone)
  {  
    AddSeparator(loc, "locale.sep1");
    AddString(loc, "locale.timezonecountry", 14079, g_timezone.GetCountryByTimezone(g_timezone.GetOSConfiguredTimezone()), SPIN_CONTROL_TEXT);
    AddString(loc, "locale.timezone", 14080, g_timezone.GetOSConfiguredTimezone(), SPIN_CONTROL_TEXT);
  }	
#endif
#ifdef HAS_TIME_SERVER
  AddSeparator(loc, "locale.sep2");
  AddBool(loc, "locale.timeserver", 168, false);
  AddString(loc, "locale.timeserveraddress", 731, "pool.ntp.org", EDIT_CONTROL_INPUT);
#endif

  CSettingsCategory* fl = AddCategory(7, "filelists", 14081);
  AddBool(fl, "filelists.showparentdiritems", 13306, true);
  AddBool(fl, "filelists.showextensions", 497, true);
  AddBool(fl, "filelists.ignorethewhensorting", 13399, true);
  AddBool(fl, "filelists.allowfiledeletion", 14071, false);
  AddBool(fl, "filelists.showaddsourcebuttons", 21382,  true);
  AddBool(fl, "filelists.showhidden", 21330, false);

  CSettingsCategory* ss = AddCategory(7, "screensaver", 360);
  AddInt(ss, "screensaver.time", 355, 3, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddDefaultAddon(ss, "screensaver.mode", 356, "screensaver.xbmc.builtin.dim", ADDON_SCREENSAVER);
  AddString(ss, "screensaver.settings", 21417, "", BUTTON_CONTROL_STANDARD);
  AddString(ss, "screensaver.preview", 1000, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(ss, "screensaver.sep1");
  AddBool(ss, "screensaver.usemusicvisinstead", 13392, true);
  AddBool(ss, "screensaver.usedimonpause", 22014, true);

  AddCategory(7, "window", 0);
  AddInt(NULL, "window.width",  0, 720, 10, 1, INT_MAX, SPIN_CONTROL_INT);
  AddInt(NULL, "window.height", 0, 480, 10, 1, INT_MAX, SPIN_CONTROL_INT);

  AddPath(NULL,"system.playlistspath",20006,"set default",BUTTON_CONTROL_PATH_INPUT,false);

  // tv settings (access over TV menu from home window)
  AddGroup(8, 19180);
  CSettingsCategory* pvr = AddCategory(8, "pvrmanager", 128);
  AddBool(pvr, "pvrmanager.enabled", 449, false);
  AddSeparator(pvr, "pvrmanager.sep1");
  AddBool(pvr, "pvrmanager.syncchannelgroups", 19221, true);
  AddBool(pvr, "pvrmanager.backendchannelorder", 19231, false);
  AddBool(pvr, "pvrmanager.usebackendchannelnumbers", 19234, false);
  AddSeparator(pvr, "pvrmanager.sep2");
  AddString(pvr, "pvrmanager.channelmanager", 19199, "", BUTTON_CONTROL_STANDARD);
  AddString(pvr, "pvrmanager.channelscan", 19117, "", BUTTON_CONTROL_STANDARD);
  AddString(pvr, "pvrmanager.resetdb", 19185, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* pvrm = AddCategory(8, "pvrmenu", 19181);
  AddBool(pvrm, "pvrmenu.infoswitch", 19178, true);
  AddBool(pvrm, "pvrmenu.infotimeout", 19179, true);
  AddBool(pvrm, "pvrmenu.closechannelosdonswitch", 19229, false);
  AddInt(pvrm, "pvrmenu.infotime", 19184, 5, 1, 1, 10, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddBool(pvrm, "pvrmenu.hidevideolength", 19169, true);
  AddSeparator(pvrm, "pvrmenu.sep1");
  AddString(pvrm, "pvrmenu.iconpath", 19018, "", BUTTON_CONTROL_PATH_INPUT, false, 657);
  AddString(pvrm, "pvrmenu.searchicons", 19167, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* pvre = AddCategory(8, "epg", 19069);
  AddInt(pvre, "epg.defaultguideview", 19065, GUIDE_VIEW_NOW, GUIDE_VIEW_CHANNEL, 1, GUIDE_VIEW_TIMELINE, SPIN_CONTROL_TEXT);
  AddInt(pvre, "epg.daystodisplay", 19182, 2, 1, 1, 14, SPIN_CONTROL_INT_PLUS, MASK_DAYS);
  AddSeparator(pvre, "epg.sep1");
  AddInt(pvre, "epg.epgupdate", 19071, 120, 15, 15, 480, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddBool(pvre, "epg.preventupdateswhileplayingtv", 19230, false);
  AddBool(pvre, "epg.ignoredbforclient", 19072, false);
  AddString(pvre, "epg.resetepg", 19187, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* pvrp = AddCategory(8, "pvrplayback", 19177);
  AddBool(pvrp, "pvrplayback.playminimized", 19171, true);
  AddInt(pvrp, "pvrplayback.startlast", 19189, START_LAST_CHANNEL_OFF, START_LAST_CHANNEL_OFF, 1, START_LAST_CHANNEL_ON, SPIN_CONTROL_TEXT);
  AddBool(pvrp, "pvrplayback.switchautoclose", 19168, true);
  AddBool(pvrp, "pvrplayback.signalquality", 19037, true);
  AddSeparator(pvrp, "pvrplayback.sep1");
  AddInt(pvrp, "pvrplayback.scantime", 19170, 15, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddInt(pvrp, "pvrplayback.channelentrytimeout", 19073, 0, 0, 250, 2000, SPIN_CONTROL_INT_PLUS, MASK_MS);

  CSettingsCategory* pvrr = AddCategory(8, "pvrrecord", 19043);
  AddInt(pvrr, "pvrrecord.instantrecordtime", 19172, 180, 1, 1, 720, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddInt(pvrr, "pvrrecord.defaultpriority", 19173, 50, 1, 1, 100, SPIN_CONTROL_INT_PLUS);
  AddInt(pvrr, "pvrrecord.defaultlifetime", 19174, 99, 1, 1, 365, SPIN_CONTROL_INT_PLUS, MASK_DAYS);
  AddInt(pvrr, "pvrrecord.marginstart", 19175, 2, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddInt(pvrr, "pvrrecord.marginend", 19176, 10, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddSeparator(pvrr, "pvrrecord.sep1");
  AddBool(pvr, "pvrrecord.timernotifications", 19233, true);

  CSettingsCategory* pvrpwr = AddCategory(8, "pvrpowermanagement", 14095);
  AddBool(pvrpwr, "pvrpowermanagement.enabled", 305, false);
  AddSeparator(pvrpwr, "pvrpowermanagement.sep1");
  AddInt(pvrpwr, "pvrpowermanagement.backendidletime", 19244, 15, 0, 5, 360, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  AddString(pvrpwr, "pvrpowermanagement.setwakeupcmd", 19245, "/usr/bin/setwakeup.sh", EDIT_CONTROL_INPUT, true);
  AddInt(pvrpwr, "pvrpowermanagement.prewakeup", 19246, 15, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  AddSeparator(pvrpwr, "pvrpowermanagement.sep2");
  AddBool(pvrpwr, "pvrpowermanagement.dailywakeup", 19247, false);
  AddString(pvrpwr, "pvrpowermanagement.dailywakeuptime", 19248, "00:00:00", EDIT_CONTROL_INPUT);
}

CGUISettings::~CGUISettings(void)
{
  Clear();
}

void CGUISettings::AddGroup(int groupID, int labelID)
{
  CSettingsGroup *pGroup = new CSettingsGroup(groupID, labelID);
  if (pGroup)
    settingsGroups.push_back(pGroup);
}

CSettingsCategory* CGUISettings::AddCategory(int groupID, const char *strSetting, int labelID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == groupID)
      return settingsGroups[i]->AddCategory(CStdString(strSetting).ToLower(), labelID);
  }
  return NULL;
}

CSettingsGroup *CGUISettings::GetGroup(int groupID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == groupID)
      return settingsGroups[i];
  }
  CLog::Log(LOGDEBUG, "Error: Requested setting group (%i) was not found.  "
                      "It must be case-sensitive",
            groupID);
  return NULL;
}

void CGUISettings::AddSeparator(CSettingsCategory* cat, const char *strSetting)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingSeparator *pSetting = new CSettingSeparator(iOrder, CStdString(strSetting).ToLower());
  if (!pSetting) return;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddBool(CSettingsCategory* cat, const char *strSetting, int iLabel, bool bData, int iControlType)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingBool* pSetting = new CSettingBool(iOrder, CStdString(strSetting).ToLower(), iLabel, bData, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}
bool CGUISettings::GetBool(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  CStdString lower(strSetting);
  lower.ToLower();
  constMapIter it = settingsMap.find(lower);
  if (it != settingsMap.end())
  { // old category
    return ((CSettingBool*)(*it).second)->GetData();
  }
  // Backward compatibility (skins use this setting)
  if (lower == "lookandfeel.enablemouse")
    return GetBool("input.enablemouse");
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return false;
}

void CGUISettings::SetBool(const char *strSetting, bool bSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(bSetting);

    SetChanged();

    return ;
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::ToggleBool(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(!((CSettingBool *)(*it).second)->GetData());

    SetChanged();

    return ;
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::AddFloat(CSettingsCategory* cat, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingFloat* pSetting = new CSettingFloat(iOrder, CStdString(strSetting).ToLower(), iLabel, fData, fMin, fStep, fMax, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

float CGUISettings::GetFloat(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    return ((CSettingFloat *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  //ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return 0.0f;
}

void CGUISettings::SetFloat(const char *strSetting, float fSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingFloat *)(*it).second)->SetData(fSetting);

    SetChanged();

    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::LoadMasterLock(TiXmlElement *pRootElement)
{
  std::map<CStdString,CSetting*>::iterator it = settingsMap.find("masterlock.maxretries");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.startuplock");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
}


void CGUISettings::AddInt(CSettingsCategory* cat, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddInt(CSettingsCategory* cat, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin/*=-1*/)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, iFormat, iLabelMin);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddInt(CSettingsCategory* cat, const char *strSetting,
                          int iLabel, int iData, const map<int,int>& entries,
                          int iControlType)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, entries, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddHex(CSettingsCategory* cat, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingHex* pSetting = new CSettingHex(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

int CGUISettings::GetInt(const char *strSetting) const
{
  ASSERT(settingsMap.size());

  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    return ((CSettingInt *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  CLog::Log(LOGERROR,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  //ASSERT(false);
  return 0;
}

void CGUISettings::SetInt(const char *strSetting, int iSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingInt *)(*it).second)->SetData(iSetting);

    SetChanged();

    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
}

void CGUISettings::AddString(CSettingsCategory* cat, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingString* pSetting = new CSettingString(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddPath(CSettingsCategory* cat, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingPath* pSetting = new CSettingPath(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddDefaultAddon(CSettingsCategory* cat, const char *strSetting, int iLabel, const char *strData, const TYPE type)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingAddon* pSetting = new CSettingAddon(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, type);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

const CStdString &CGUISettings::GetString(const char *strSetting, bool bPrompt /* = true */) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    CSettingString* result = ((CSettingString *)(*it).second);
    if (result->GetData() == "select folder" || result->GetData() == "select writable folder")
    {
      CStdString strData = "";
      if (bPrompt)
      {
        VECSOURCES shares;
        g_mediaManager.GetLocalDrives(shares);
        if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares,g_localizeStrings.Get(result->GetLabel()),strData,result->GetData() == "select writable folder"))
        {
          result->SetData(strData);
          g_settings.Save();
        }
        else
          return StringUtils::EmptyString;
      }
      else
        return StringUtils::EmptyString;
    }
    return result->GetData();
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  //ASSERT(false);
  // hardcoded return value so that compiler is happy
  return StringUtils::EmptyString;
}

void CGUISettings::SetString(const char *strSetting, const char *strData)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingString *)(*it).second)->SetData(strData);

    SetChanged();

    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

CSetting *CGUISettings::GetSetting(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
    return (*it).second;
  else
    return NULL;
}

// get all the settings beginning with the term "strGroup"
void CGUISettings::GetSettingsGroup(const char *strGroup, vecSettings &settings)
{
  vecSettings unorderedSettings;
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    if ((*it).first.Left(strlen(strGroup)).Equals(strGroup) && (*it).second->GetOrder() > 0 && !(*it).second->IsAdvanced())
      unorderedSettings.push_back((*it).second);
  }
  // now order them...
  sort(unorderedSettings.begin(), unorderedSettings.end(), sortsettings());

  // remove any instances of 2 separators in a row
  bool lastWasSeparator(false);
  for (vecSettings::iterator it = unorderedSettings.begin(); it != unorderedSettings.end(); it++)
  {
    CSetting *setting = *it;
    // only add separators if we don't have 2 in a row
    if (setting->GetType() == SETTINGS_TYPE_SEPARATOR)
    {
      if (!lastWasSeparator)
        settings.push_back(setting);
      lastWasSeparator = true;
    }
    else
    {
      lastWasSeparator = false;
      settings.push_back(setting);
    }
  }
}

void CGUISettings::LoadXML(TiXmlElement *pRootElement, bool hideSettings /* = false */)
{ // load our stuff...
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    LoadFromXML(pRootElement, it, hideSettings);
  }
  // Get hardware based stuff...
  CLog::Log(LOGNOTICE, "Getting hardware information now...");
  // FIXME: Check if the hardware supports it (if possible ;)
  //SetBool("audiooutput.ac3passthrough", g_audioConfig.GetAC3Enabled());
  //SetBool("audiooutput.dtspassthrough", g_audioConfig.GetDTSEnabled());
  CLog::Log(LOGINFO, "Using %s output", GetInt("audiooutput.mode") == AUDIO_ANALOG ? "analog" : "digital");
  CLog::Log(LOGINFO, "AC3 pass through is %s", GetBool("audiooutput.ac3passthrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "DTS pass through is %s", GetBool("audiooutput.dtspassthrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "AAC pass through is %s", GetBool("audiooutput.passthroughaac") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "MP1 pass through is %s", GetBool("audiooutput.passthroughmp1") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "MP2 pass through is %s", GetBool("audiooutput.passthroughmp2") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "MP3 pass through is %s", GetBool("audiooutput.passthroughmp3") ? "enabled" : "disabled");

  g_guiSettings.m_LookAndFeelResolution = GetResolution();
#ifdef __APPLE__
  // trap any previous vsync by driver setting, does not exist on OSX
  if (GetInt("videoscreen.vsync") == VSYNC_DRIVER)
  {
    SetInt("videoscreen.vsync", VSYNC_ALWAYS);
  }
#endif
 // DXMERGE: This might have been useful?
 // g_videoConfig.SetVSyncMode((VSYNC)GetInt("videoscreen.vsync"));
  CLog::Log(LOGNOTICE, "Checking resolution %i", g_guiSettings.m_LookAndFeelResolution);
  if (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  {
    CLog::Log(LOGNOTICE, "Setting safe mode %i", RES_DESKTOP);
    SetResolution(RES_DESKTOP);
  }

  // Move replaygain settings into our struct
  m_replayGain.iPreAmp = GetInt("musicplayer.replaygainpreamp");
  m_replayGain.iNoGainPreAmp = GetInt("musicplayer.replaygainnogainpreamp");
  m_replayGain.iType = GetInt("musicplayer.replaygaintype");
  m_replayGain.bAvoidClipping = GetBool("musicplayer.replaygainavoidclipping");
  
  bool use_timezone = false;
  
#if defined(_LINUX)
#if defined(__APPLE__) 
  if (g_sysinfo.IsAppleTV2() && GetIOSVersion() < 4.3)
#endif
    use_timezone = true;
  
  if (use_timezone)
  {  
    CStdString timezone = GetString("locale.timezone");
    if(timezone == "0" || timezone.IsEmpty())
    {
      timezone = g_timezone.GetOSConfiguredTimezone();
      SetString("locale.timezone", timezone);
    }
    g_timezone.SetTimezone(timezone);	
  }
#endif
}

void CGUISettings::LoadFromXML(TiXmlElement *pRootElement, mapIter &it, bool advanced /* = false */)
{
  CStdStringArray strSplit;
  StringUtils::SplitString((*it).first, ".", strSplit);
  if (strSplit.size() > 1)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild(strSplit[0].c_str());
    if (pChild)
    {
      const TiXmlElement *pGrandChild = pChild->FirstChildElement(strSplit[1].c_str());
      if (pGrandChild)
      {
        CStdString strValue = pGrandChild->FirstChild() ? pGrandChild->FirstChild()->Value() : "";
        if (strValue != "-")
        { // update our item
          if ((*it).second->GetType() == SETTINGS_TYPE_PATH)
          { // check our path
            int pathVersion = 0;
            pGrandChild->Attribute("pathversion", &pathVersion);
            strValue = CSpecialProtocol::ReplaceOldPath(strValue, pathVersion);
          }
          (*it).second->FromString(strValue);
          if (advanced)
            (*it).second->SetAdvanced();
        }
      }
    }
  }

  SetChanged();
}

void CGUISettings::SaveXML(TiXmlNode *pRootNode)
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    // don't save advanced settings
    CStdString first = (*it).first;
    if ((*it).second->IsAdvanced() || (*it).second->GetType() == SETTINGS_TYPE_SEPARATOR)
      continue;

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
        if ((*it).second->GetType() == SETTINGS_TYPE_PATH)
          newElement.SetAttribute("pathversion", CSpecialProtocol::path_version);
        TiXmlNode *pNewNode = pChild->InsertEndChild(newElement);
        if (pNewNode)
        {
          TiXmlText value((*it).second->ToString());
          pNewNode->InsertEndChild(value);
        }
      }
    }
  }

  SetChanged();
}

void CGUISettings::Clear()
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
    delete (*it).second;
  settingsMap.clear();
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
    delete settingsGroups[i];
  settingsGroups.clear();

  SetChanged();
}

float square_error(float x, float y)
{
  float yonx = (x > 0) ? y / x : 0;
  float xony = (y > 0) ? x / y : 0;
  return std::max(yonx, xony);
}

RESOLUTION CGUISettings::GetResolution() const
{
  return GetResFromString(GetString("videoscreen.screenmode"));
}

RESOLUTION CGUISettings::GetResFromString(const CStdString &res)
{
  if (res == "DESKTOP")
    return RES_DESKTOP;
  else if (res == "WINDOW")
    return RES_WINDOW;
  else if (res.GetLength()==20)
  {
    // format: SWWWWWHHHHHRRR.RRRRR, where S = screen, W = width, H = height, R = refresh
    int screen = atol(res.Mid(0,1).c_str());
    int width = atol(res.Mid(1,5).c_str());
    int height = atol(res.Mid(6,5).c_str());
    float refresh = (float)atof(res.Mid(11).c_str());
    // find the closest match to these in our res vector.  If we have the screen, we score the res
    RESOLUTION bestRes = RES_DESKTOP;
    float bestScore = FLT_MAX;
    for (unsigned int i = RES_DESKTOP; i < g_settings.m_ResInfo.size(); ++i)
    {
      const RESOLUTION_INFO &info = g_settings.m_ResInfo[i];
      if (info.iScreen != screen)
        continue;
      float score = 10*(square_error((float)info.iWidth, (float)width) + square_error((float)info.iHeight, (float)height)) + square_error(info.fRefreshRate, refresh);
      if (score < bestScore)
      {
        bestScore = score;
        bestRes = (RESOLUTION)i;
      }
    }
    return bestRes;
  }
  return RES_DESKTOP;
}

void CGUISettings::SetResolution(RESOLUTION res)
{
  CStdString mode;
  if (res == RES_DESKTOP)
    mode = "DESKTOP";
  else if (res == RES_WINDOW)
    mode = "WINDOW";
  else if (res >= RES_CUSTOM && res < (RESOLUTION)g_settings.m_ResInfo.size())
  {
    const RESOLUTION_INFO &info = g_settings.m_ResInfo[res];
    mode.Format("%1i%05i%05i%09.5f", info.iScreen, info.iWidth, info.iHeight, info.fRefreshRate);
  }
  else
  {
    CLog::Log(LOGWARNING, "%s, setting invalid resolution %i", __FUNCTION__, res);
    mode = "DESKTOP";
  }
  SetString("videoscreen.screenmode", mode);
  m_LookAndFeelResolution = res;

  SetChanged();
}

bool CGUISettings::SetLanguage(const CStdString &strLanguage)
{
  CStdString strPreviousLanguage = GetString("locale.language");
  CStdString strNewLanguage = strLanguage;
  if (strNewLanguage != strPreviousLanguage)
  {
    CStdString strLangInfoPath;
    strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", strNewLanguage.c_str());
    if (!g_langInfo.Load(strLangInfoPath))
      return false;

    if (g_langInfo.ForceUnicodeFont() && !g_fontManager.IsFontSetUnicode())
    {
      CLog::Log(LOGINFO, "Language needs a ttf font, loading first ttf font available");
      CStdString strFontSet;
      if (g_fontManager.GetFirstFontSetUnicode(strFontSet))
        strNewLanguage = strFontSet;
      else
        CLog::Log(LOGERROR, "No ttf font found but needed: %s", strFontSet.c_str());
    }
    SetString("locale.language", strNewLanguage);

    g_charsetConverter.reset();

    CStdString strLanguagePath;
    strLanguagePath.Format("special://xbmc/language/%s/strings.xml", strNewLanguage.c_str());
    if (!g_localizeStrings.Load(strLanguagePath))
      return false;

    // also tell our weather and skin to reload as these are localized
    g_weatherManager.Refresh();
    g_PVRManager.LocalizationChanged();
    g_application.ReloadSkin();
  }

  return true;
}
