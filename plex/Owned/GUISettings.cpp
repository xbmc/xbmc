/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
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
#include "AdvancedSettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"
#include "windowing/WindowingFactory.h"
#include "powermanagement/PowerManager.h"
#include "cores/dvdplayer/DVDCodecs/Video/CrystalHD.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/AEAudioFormat.h"
#include "guilib/GUIFont.h" // for FONT_STYLE_* definitions
#if defined(TARGET_DARWIN_OSX)
  #include "cores/AudioEngine/Engines/CoreAudio/CoreAudioHardware.h"
#endif
#include "guilib/GUIFontManager.h"
#include "utils/Weather.h"
#include "LangInfo.h"
#include "pvr/PVRManager.h"
#include "utils/XMLUtils.h"
#if defined(TARGET_DARWIN)
  #include "osx/DarwinUtils.h"
#endif
#include "Util.h"

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
    g_guiSettings.GetSettingsGroup(m_vecCategories[i], settings);
    if (settings.size())
      vecCategories.push_back(m_vecCategories[i]);
  }
}

#define SETTINGS_PICTURES     WINDOW_SETTINGS_MYPICTURES - WINDOW_SETTINGS_START
#define SETTINGS_PROGRAMS     WINDOW_SETTINGS_MYPROGRAMS - WINDOW_SETTINGS_START
#define SETTINGS_WEATHER      WINDOW_SETTINGS_MYWEATHER - WINDOW_SETTINGS_START
#define SETTINGS_MUSIC        WINDOW_SETTINGS_MYMUSIC - WINDOW_SETTINGS_START
#define SETTINGS_SYSTEM       WINDOW_SETTINGS_SYSTEM - WINDOW_SETTINGS_START
#define SETTINGS_VIDEOS       WINDOW_SETTINGS_MYVIDEOS - WINDOW_SETTINGS_START
#define SETTINGS_SERVICE      WINDOW_SETTINGS_SERVICE - WINDOW_SETTINGS_START
#define SETTINGS_APPEARANCE   WINDOW_SETTINGS_APPEARANCE - WINDOW_SETTINGS_START
#define SETTINGS_PVR          WINDOW_SETTINGS_MYPVR - WINDOW_SETTINGS_START

// Settings are case sensitive
CGUISettings::CGUISettings(void)
{
}

void CGUISettings::Initialize()
{
  ZeroMemory(&m_replayGain, sizeof(ReplayGainSettings));

  // Pictures settings
  AddGroup(SETTINGS_PICTURES, 1);
  CSettingsCategory* pic = AddCategory(SETTINGS_PICTURES, "pictures", 14081);
  AddBool(pic, "pictures.usetags", 14082, true);
  AddBool(pic,"pictures.generatethumbs",13360,true);
  AddBool(pic, "pictures.useexifrotation", 20184, true);
  AddBool(pic, "pictures.showvideos", 22022, true);
  // FIXME: hide this setting until it is properly respected. In the meanwhile, default to AUTO.
  AddInt(NULL, "pictures.displayresolution", 169, (int)RES_AUTORES, (int)RES_AUTORES, 1, (int)RES_AUTORES, SPIN_CONTROL_TEXT);

  CSettingsCategory* cat = AddCategory(SETTINGS_PICTURES, "slideshow", 108);
  AddInt(cat, "slideshow.staytime", 12378, 5, 1, 1, 100, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddBool(cat, "slideshow.displayeffects", 12379, true);
  AddBool(NULL, "slideshow.shuffle", 13319, false);

  // Programs settings
//  AddGroup(1, 0);

  // My Weather settings
  AddGroup(SETTINGS_WEATHER, 8);
  CSettingsCategory* wea = AddCategory(SETTINGS_WEATHER, "weather", 16000);
  AddInt(NULL, "weather.currentlocation", 0, 1, 1, 1, 3, SPIN_CONTROL_INT_PLUS);
  AddDefaultAddon(wea, "weather.addon", 24029, "weather.wunderground", ADDON_SCRIPT_WEATHER);
  AddString(wea, "weather.addonsettings", 21417, "", BUTTON_CONTROL_STANDARD, true);

  // My Music Settings
  AddGroup(SETTINGS_MUSIC, 2);
  CSettingsCategory* ml = AddCategory(SETTINGS_MUSIC,"musiclibrary",14022);
  AddBool(NULL, "musiclibrary.enabled", 418, true);
  AddBool(ml, "musiclibrary.showcompilationartists", 13414, true);
  AddSeparator(ml,"musiclibrary.sep1");
  AddBool(ml,"musiclibrary.downloadinfo", 20192, false);
  AddDefaultAddon(ml, "musiclibrary.albumsscraper", 20193, "metadata.album.universal", ADDON_SCRAPER_ALBUMS);
  AddDefaultAddon(ml, "musiclibrary.artistsscraper", 20194, "metadata.artists.universal", ADDON_SCRAPER_ARTISTS);
  AddBool(ml, "musiclibrary.updateonstartup", 22000, false);
  AddBool(ml, "musiclibrary.backgroundupdate", 22001, false);
  AddSeparator(ml,"musiclibrary.sep2");
  AddString(ml, "musiclibrary.cleanup", 334, "", BUTTON_CONTROL_STANDARD);
  AddString(ml, "musiclibrary.export", 20196, "", BUTTON_CONTROL_STANDARD);
  AddString(ml, "musiclibrary.import", 20197, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* mp = AddCategory(SETTINGS_MUSIC, "musicplayer", 14086);
  AddBool(mp, "musicplayer.autoplaynextitem", 489, true);
  AddBool(mp, "musicplayer.queuebydefault", 14084, false);
  AddSeparator(mp, "musicplayer.sep1");
  map<int,int> gain;
  gain.insert(make_pair(351,REPLAY_GAIN_NONE));
  gain.insert(make_pair(639,REPLAY_GAIN_TRACK));
  gain.insert(make_pair(640,REPLAY_GAIN_ALBUM));

  AddInt(mp, "musicplayer.replaygaintype", 638, REPLAY_GAIN_ALBUM, gain, SPIN_CONTROL_TEXT);
  AddInt(mp, "musicplayer.replaygainpreamp", 641, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddInt(mp, "musicplayer.replaygainnogainpreamp", 642, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddBool(mp, "musicplayer.replaygainavoidclipping", 643, false);
  AddSeparator(mp, "musicplayer.sep2");
  AddInt(mp, "musicplayer.crossfade", 13314, 0, 0, 1, 15, SPIN_CONTROL_INT_PLUS, MASK_SECS, TEXT_OFF);
  AddBool(mp, "musicplayer.crossfadealbumtracks", 13400, true);
  AddSeparator(mp, "musicplayer.sep3");
  AddDefaultAddon(mp, "musicplayer.visualisation", 250, DEFAULT_VISUALISATION, ADDON_VIZ);

  CSettingsCategory* mf = AddCategory(SETTINGS_MUSIC, "musicfiles", 14081);
  AddBool(mf, "musicfiles.usetags", 258, true);
  AddString(mf, "musicfiles.trackformat", 13307, "[%N. ]%A - %T", EDIT_CONTROL_INPUT, false, 16016);
  AddString(mf, "musicfiles.trackformatright", 13387, "%D", EDIT_CONTROL_INPUT, false, 16016);
  // advanced per-view trackformats.
  AddString(NULL, "musicfiles.nowplayingtrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(NULL, "musicfiles.nowplayingtrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(NULL, "musicfiles.librarytrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(NULL, "musicfiles.librarytrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddBool(mf, "musicfiles.findremotethumbs", 14059, true);

  CSettingsCategory* scr = AddCategory(SETTINGS_MUSIC, "scrobbler", 15221);
  AddBool(scr, "scrobbler.lastfmsubmit", 15201, false);
  AddBool(scr, "scrobbler.lastfmsubmitradio", 15250, false);
  AddString(scr,"scrobbler.lastfmusername", 15202, "", EDIT_CONTROL_INPUT, false, 15202);
  AddString(scr,"scrobbler.lastfmpass", 15203, "", EDIT_CONTROL_MD5_INPUT, false, 15203);
  AddSeparator(scr, "scrobbler.sep1");
  AddBool(scr, "scrobbler.librefmsubmit", 15217, false);
  AddString(scr, "scrobbler.librefmusername", 15218, "", EDIT_CONTROL_INPUT, false, 15218);
  AddString(scr, "scrobbler.librefmpass", 15219, "", EDIT_CONTROL_MD5_INPUT, false, 15219);

  CSettingsCategory* acd = AddCategory(SETTINGS_MUSIC, "audiocds", 620);
  map<int,int> autocd;
  autocd.insert(make_pair(16018, AUTOCD_NONE));
  autocd.insert(make_pair(14098, AUTOCD_PLAY));
#ifdef HAS_CDDA_RIPPER
  autocd.insert(make_pair(14096, AUTOCD_RIP));
#endif
  AddInt(acd,"audiocds.autoaction",14097,AUTOCD_NONE, autocd, SPIN_CONTROL_TEXT);
  AddBool(acd, "audiocds.usecddb", 227, true);
  AddSeparator(acd, "audiocds.sep1");
  AddPath(acd,"audiocds.recordingpath",20000,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false,657);
  AddString(acd, "audiocds.trackpathformat", 13307, "%A - %B/[%N. ][%A - ]%T", EDIT_CONTROL_INPUT, false, 16016);
  map<int,int> encoders;
#ifdef HAVE_LIBMP3LAME
  encoders.insert(make_pair(34000,CDDARIP_ENCODER_LAME));
#endif
#ifdef HAVE_LIBVORBISENC
  encoders.insert(make_pair(34001,CDDARIP_ENCODER_VORBIS));
#endif
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
  AddBool(acd, "audiocds.ejectonrip", 14099, true);

#ifdef HAS_KARAOKE
  CSettingsCategory* kar = AddCategory(SETTINGS_MUSIC, "karaoke", 13327);
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
  AddGroup(SETTINGS_SYSTEM, 13000);
  CSettingsCategory* vs = AddCategory(SETTINGS_SYSTEM, "videoscreen", 21373);

  // this setting would ideally not be saved, as its value is systematically derived from videoscreen.screenmode.
  // contains a DISPLAYMODE
#if !defined(TARGET_DARWIN_IOS_ATV2) && !defined(TARGET_RASPBERRY_PI)
  AddInt(vs, "videoscreen.screen", 240, 0, -1, 1, 32, SPIN_CONTROL_TEXT);
#endif
  // this setting would ideally not be saved, as its value is systematically derived from videoscreen.screenmode.
  // contains an index to the g_settings.m_ResInfo array. the only meaningful fields are iScreen, iWidth, iHeight.
#if defined(TARGET_DARWIN)
  #if !defined(TARGET_DARWIN_IOS_ATV2)
    AddInt(vs, "videoscreen.resolution", 131, -1, 0, 1, INT_MAX, SPIN_CONTROL_TEXT);
  #endif
#else
  AddInt(vs, "videoscreen.resolution", 169, -1, 0, 1, INT_MAX, SPIN_CONTROL_TEXT);
#endif
  AddString(g_application.IsStandAlone() ? vs : NULL, "videoscreen.screenmode", 243, "DESKTOP", SPIN_CONTROL_TEXT);

#if defined(_WIN32) || defined(TARGET_DARWIN)
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

#if defined(TARGET_DARWIN)
  showSetting = false;
#endif
  AddBool(showSetting ? vs : NULL, "videoscreen.fakefullscreen", 14083, fakeFullScreen);
#ifdef TARGET_DARWIN_IOS
  AddBool(NULL, "videoscreen.blankdisplays", 13130, false);
#else
  AddBool(vs, "videoscreen.blankdisplays", 13130, false);
#endif
  AddSeparator(vs, "videoscreen.sep1");
#endif

  map<int,int> vsync;
#if defined(_LINUX) && !defined(TARGET_DARWIN)
  vsync.insert(make_pair(13101,VSYNC_DRIVER));
#endif
  vsync.insert(make_pair(13106,VSYNC_DISABLED));
  vsync.insert(make_pair(13107,VSYNC_VIDEO));
  vsync.insert(make_pair(13108,VSYNC_ALWAYS));
  AddInt(vs, "videoscreen.vsync", 13105, DEFAULT_VSYNC, vsync, SPIN_CONTROL_TEXT);

  AddString(vs, "videoscreen.guicalibration",214,"", BUTTON_CONTROL_STANDARD);
#if defined(HAS_GL)
  // Todo: Implement test pattern for DX
  AddString(vs, "videoscreen.testpattern",226,"", BUTTON_CONTROL_STANDARD);
#endif
#if defined(HAS_LCD)
  AddBool(vs, "videoscreen.haslcd", 4501, false);
#endif

  CSettingsCategory* ao = AddCategory(SETTINGS_SYSTEM, "audiooutput", 772);

  map<int,int> audiomode;
  audiomode.insert(make_pair(338,AUDIO_ANALOG));
#if !defined(TARGET_RASPBERRY_PI)
  audiomode.insert(make_pair(339,AUDIO_IEC958));
#endif
  audiomode.insert(make_pair(420,AUDIO_HDMI  ));
#if defined(TARGET_RASPBERRY_PI)
  AddInt(ao, "audiooutput.mode", 337, AUDIO_HDMI, audiomode, SPIN_CONTROL_TEXT);
#else
  AddInt(ao, "audiooutput.mode", 337, AUDIO_ANALOG, audiomode, SPIN_CONTROL_TEXT);
#endif

  map<int,int> channelLayout;
  for(int layout = AE_CH_LAYOUT_2_0; layout < AE_CH_LAYOUT_MAX; ++layout)
    channelLayout.insert(make_pair(34100+layout, layout));
  AddInt(ao, "audiooutput.channellayout", 34100, AE_CH_LAYOUT_2_0, channelLayout, SPIN_CONTROL_TEXT);
  AddBool(ao, "audiooutput.normalizelevels", 346, false);
  AddBool(ao, "audiooutput.stereoupmix", 252, false);

#if defined(TARGET_DARWIN_IOS)
  CSettingsCategory* aocat = g_sysinfo.IsAppleTV2() ? ao : NULL;
#else
  CSettingsCategory* aocat = ao;
#endif

  AddBool(aocat, "audiooutput.ac3passthrough"   , 364, true);
  AddBool(aocat, "audiooutput.dtspassthrough"   , 254, true);


#if !defined(TARGET_DARWIN) && !defined(TARGET_RASPBERRY_PI)
  AddBool(aocat, "audiooutput.passthroughaac"   , 299, false);
#endif
#if !defined(TARGET_DARWIN_IOS) && !defined(TARGET_RASPBERRY_PI)
  AddBool(aocat, "audiooutput.multichannellpcm" , 348, true );
#endif
#if !defined(TARGET_DARWIN) && !defined(TARGET_RASPBERRY_PI)
  AddBool(aocat, "audiooutput.truehdpassthrough", 349, true );
  AddBool(aocat, "audiooutput.dtshdpassthrough" , 347, true );
#endif

#if !defined(TARGET_RASPBERRY_PI)
#if defined(TARGET_DARWIN)
  #if defined(TARGET_DARWIN_IOS)
    CStdString defaultDeviceName = "Default";
  #else
    CStdString defaultDeviceName;
    CCoreAudioHardware::GetOutputDeviceName(defaultDeviceName);
  #endif
  AddString(ao, "audiooutput.audiodevice", 545, defaultDeviceName.c_str(), SPIN_CONTROL_TEXT);
  AddString(NULL, "audiooutput.passthroughdevice", 546, defaultDeviceName.c_str(), SPIN_CONTROL_TEXT);
#else
  AddSeparator(ao, "audiooutput.sep1");
  AddString   (ao, "audiooutput.audiodevice"      , 545, CStdString(CAEFactory::GetDefaultDevice(false)), SPIN_CONTROL_TEXT);
  AddString   (ao, "audiooutput.passthroughdevice", 546, CStdString(CAEFactory::GetDefaultDevice(true )), SPIN_CONTROL_TEXT);
  AddSeparator(ao, "audiooutput.sep2");
#endif
#endif

#if !defined(TARGET_RASPBERRY_PI)
  map<int,int> guimode;
  guimode.insert(make_pair(34121, AE_SOUND_IDLE  ));
  guimode.insert(make_pair(34122, AE_SOUND_ALWAYS));
  guimode.insert(make_pair(34123, AE_SOUND_OFF   ));
  AddInt(ao, "audiooutput.guisoundmode", 34120, AE_SOUND_IDLE, guimode, SPIN_CONTROL_TEXT);
#endif

  CSettingsCategory* in = AddCategory(SETTINGS_SYSTEM, "input", 14094);
  AddString(in, "input.peripherals", 35000, "", BUTTON_CONTROL_STANDARD);
#if defined(TARGET_DARWIN)
  map<int,int> remotemode;
  remotemode.insert(make_pair(13610,APPLE_REMOTE_DISABLED));
  remotemode.insert(make_pair(13611,APPLE_REMOTE_STANDARD));
  remotemode.insert(make_pair(13612,APPLE_REMOTE_UNIVERSAL));
  remotemode.insert(make_pair(13613,APPLE_REMOTE_MULTIREMOTE));
  AddInt(in, "input.appleremotemode", 13600, APPLE_REMOTE_STANDARD, remotemode, SPIN_CONTROL_TEXT);
#if defined(TARGET_DARWIN_OSX)
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
#if defined(HAS_SDL_JOYSTICK)
  AddBool(in, "input.enablejoystick", 35100, true);
  AddBool(in, "input.disablejoystickwithimon", 35101, true);
  GetSetting("input.disablejoystickwithimon")->SetVisible(false);
#endif

  CSettingsCategory* net = AddCategory(SETTINGS_SYSTEM, "network", 798);
  if (g_application.IsStandAlone())
  {
#if !defined(TARGET_DARWIN)
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

  CSettingsCategory* pwm = AddCategory(SETTINGS_SYSTEM, "powermanagement", 14095);
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

  CSettingsCategory* dbg = AddCategory(SETTINGS_SYSTEM, "debug", 14092);
  AddBool(dbg, "debug.showloginfo", 20191, false);
  AddPath(dbg, "debug.screenshotpath",20004,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false,657);

  CSettingsCategory* mst = AddCategory(SETTINGS_SYSTEM, "masterlock", 12360);
  AddString(mst, "masterlock.lockcode"       , 20100, "-", BUTTON_CONTROL_STANDARD);
  AddBool(mst, "masterlock.startuplock"      , 20076,false);
  // hidden masterlock settings
  AddInt(NULL,"masterlock.maxretries", 12364, 3, 3, 1, 100, SPIN_CONTROL_TEXT);

  AddCategory(SETTINGS_SYSTEM, "cache", 439);
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
  AddGroup(SETTINGS_VIDEOS, 3);
  CSettingsCategory* vdl = AddCategory(SETTINGS_VIDEOS, "videolibrary", 14022);
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

  CSettingsCategory* vp = AddCategory(SETTINGS_VIDEOS, "videoplayer", 14086);

  AddBool(vp, "videoplayer.autoplaynextitem", 13433, false);
  AddSeparator(vp, "videoplayer.sep1");

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

  map<int, int> adjustTypes;
  adjustTypes.insert(make_pair(351, ADJUST_REFRESHRATE_OFF));
  adjustTypes.insert(make_pair(36035, ADJUST_REFRESHRATE_ALWAYS));
  adjustTypes.insert(make_pair(36036, ADJUST_REFRESHRATE_ON_STARTSTOP));

#if !defined(TARGET_DARWIN_IOS)
  AddInt(vp, "videoplayer.adjustrefreshrate", 170, ADJUST_REFRESHRATE_OFF, adjustTypes, SPIN_CONTROL_TEXT);
//  AddBool(vp, "videoplayer.adjustrefreshrate", 170, false);
  AddInt(vp, "videoplayer.pauseafterrefreshchange", 13550, 0, 0, 1, MAXREFRESHCHANGEDELAY, SPIN_CONTROL_TEXT);
#else
  AddInt(NULL, "videoplayer.adjustrefreshrate", 170, ADJUST_REFRESHRATE_OFF, adjustTypes, SPIN_CONTROL_TEXT);
  //AddBool(NULL, "videoplayer.adjustrefreshrate", 170, false);
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
  AddBool(vp, "Videoplayer.teletextscale", 23055, true);

  CSettingsCategory* vid = AddCategory(SETTINGS_VIDEOS, "myvideos", 14081);

  map<int, int> myVideosSelectActions;
  myVideosSelectActions.insert(make_pair(22080, SELECT_ACTION_CHOOSE));
  myVideosSelectActions.insert(make_pair(208,   SELECT_ACTION_PLAY_OR_RESUME));
  myVideosSelectActions.insert(make_pair(13404, SELECT_ACTION_RESUME));
  myVideosSelectActions.insert(make_pair(22081, SELECT_ACTION_INFO));

  AddInt(vid, "myvideos.selectaction", 22079, SELECT_ACTION_PLAY_OR_RESUME, myVideosSelectActions, SPIN_CONTROL_TEXT);
  AddBool(vid, "myvideos.extractflags",20433, true);
  AddBool(vid, "myvideos.replacelabels", 20419, true);
  AddBool(NULL, "myvideos.extractthumb",20433, true);

  CSettingsCategory* sub = AddCategory(SETTINGS_VIDEOS, "subtitles", 287);
  AddString(sub, "subtitles.font", 14089, "arial.ttf", SPIN_CONTROL_TEXT);
  AddInt(sub, "subtitles.height", 289, 28, 16, 2, 74, SPIN_CONTROL_TEXT); // use text as there is a disk based lookup needed

  map<int, int> fontStyles;
  fontStyles.insert(make_pair(738, FONT_STYLE_NORMAL));
  fontStyles.insert(make_pair(739, FONT_STYLE_BOLD));
  fontStyles.insert(make_pair(740, FONT_STYLE_ITALICS));
  fontStyles.insert(make_pair(741, FONT_STYLE_BOLD | FONT_STYLE_ITALICS));

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

  CSettingsCategory* dvd = AddCategory(SETTINGS_VIDEOS, "dvds", 14087);
  AddBool(dvd, "dvds.autorun", 14088, false);
  AddInt(dvd, "dvds.playerregion", 21372, 0, 0, 1, 8, SPIN_CONTROL_INT_PLUS, -1, TEXT_OFF);
  AddBool(dvd, "dvds.automenu", 21882, false);

  AddDefaultAddon(NULL, "scrapers.moviesdefault", 21413, "metadata.themoviedb.org", ADDON_SCRAPER_MOVIES);
  AddDefaultAddon(NULL, "scrapers.tvshowsdefault", 21414, "metadata.tvdb.com", ADDON_SCRAPER_TVSHOWS);
  AddDefaultAddon(NULL, "scrapers.musicvideosdefault", 21415, "metadata.musicvideos.last.fm", ADDON_SCRAPER_MUSICVIDEOS);

  // service settings
  AddGroup(SETTINGS_SERVICE, 14036);

  CSettingsCategory* srvGeneral = AddCategory(SETTINGS_SERVICE, "general", 16000);
  AddString(srvGeneral,"services.devicename", 1271, "XBMC", EDIT_CONTROL_INPUT);

  CSettingsCategory* srvUpnp = AddCategory(SETTINGS_SERVICE, "upnp", 20187);
  AddBool(srvUpnp, "services.upnpserver", 21360, false);
  AddBool(srvUpnp, "services.upnpannounce", 20188, true);
  AddBool(srvUpnp, "services.upnprenderer", 21881, false);

#ifdef HAS_WEB_SERVER
  CSettingsCategory* srvWeb = AddCategory(SETTINGS_SERVICE, "webserver", 33101);
  AddBool(srvWeb,  "services.webserver",        263, false);
  AddString(srvWeb,"services.webserverport",    730, CUtil::CanBindPrivileged()?"80":"8080", EDIT_CONTROL_NUMBER_INPUT, false, 730);
  AddString(srvWeb,"services.webserverusername",1048, "xbmc", EDIT_CONTROL_INPUT);
  AddString(srvWeb,"services.webserverpassword",733, "", EDIT_CONTROL_HIDDEN_INPUT, true, 733);
  AddDefaultAddon(srvWeb, "services.webskin",199, DEFAULT_WEB_INTERFACE, ADDON_WEB_INTERFACE);
#endif
#ifdef HAS_EVENT_SERVER
  CSettingsCategory* srvEvent = AddCategory(SETTINGS_SERVICE, "remotecontrol", 790);
  AddBool(srvEvent,  "services.esenabled",         791, true);
  AddString(NULL,"services.esport",            792, "9777", EDIT_CONTROL_NUMBER_INPUT, false, 792);
  AddInt(NULL,   "services.esportrange",       793, 10, 1, 1, 100, SPIN_CONTROL_INT);
  AddInt(NULL,   "services.esmaxclients",      797, 20, 1, 1, 100, SPIN_CONTROL_INT);
  AddBool(srvEvent,  "services.esallinterfaces",   794, false);
  AddInt(NULL,   "services.esinitialdelay",    795, 750, 5, 5, 10000, SPIN_CONTROL_INT);
  AddInt(NULL,   "services.escontinuousdelay", 796, 25, 5, 5, 10000, SPIN_CONTROL_INT);
#endif
#ifdef HAS_ZEROCONF
  CSettingsCategory* srvZeroconf = AddCategory(SETTINGS_SERVICE, "zeroconf", 1259);
#ifdef TARGET_WINDOWS
  AddBool(srvZeroconf, "services.zeroconf", 1260, false);
#else
  AddBool(srvZeroconf, "services.zeroconf", 1260, true);
#endif
#endif

#ifdef HAS_AIRPLAY
  CSettingsCategory* srvAirplay = AddCategory(SETTINGS_SERVICE, "airplay", 1273);
  AddBool(srvAirplay, "services.airplay", 1270, false);
  AddBool(srvAirplay, "services.useairplaypassword", 1272, false);
  AddString(srvAirplay, "services.airplaypassword", 733, "", EDIT_CONTROL_HIDDEN_INPUT, false, 733);
#endif

#ifndef _WIN32
  CSettingsCategory* srvSmb = AddCategory(SETTINGS_SERVICE, "smb", 1200);
  AddString(srvSmb, "smb.winsserver",  1207,   "",  EDIT_CONTROL_IP_INPUT);
  AddString(srvSmb, "smb.workgroup",   1202,   "WORKGROUP", EDIT_CONTROL_INPUT, false, 1202);
#endif

  // appearance settings
  AddGroup(SETTINGS_APPEARANCE, 480);
  CSettingsCategory* laf = AddCategory(SETTINGS_APPEARANCE,"lookandfeel", 166);
  AddDefaultAddon(laf, "lookandfeel.skin",166,DEFAULT_SKIN, ADDON_SKIN);
  AddString(laf, "lookandfeel.skinsettings", 21417, "", BUTTON_CONTROL_STANDARD);
  AddString(laf, "lookandfeel.skintheme",15111,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(laf, "lookandfeel.skincolors",14078, "SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(laf, "lookandfeel.font",13303,"Default", SPIN_CONTROL_TEXT);
  AddInt(laf, "lookandfeel.skinzoom",20109, 0, -20, 2, 20, SPIN_CONTROL_INT, MASK_PERCENT);
  AddInt(laf, "lookandfeel.startupwindow",512,1, WINDOW_HOME, 1, WINDOW_PYTHON_END, SPIN_CONTROL_TEXT);
  AddString(laf, "lookandfeel.soundskin",15108,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(laf, "lookandfeel.sep2");
  AddBool(laf, "lookandfeel.enablerssfeeds",13305,  true);
  AddString(laf, "lookandfeel.rssedit", 21450, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* loc = AddCategory(SETTINGS_APPEARANCE, "locale", 14090);
  AddString(loc, "locale.language",248,"english", SPIN_CONTROL_TEXT);
  AddString(loc, "locale.country", 20026, "USA", SPIN_CONTROL_TEXT);
  AddString(loc, "locale.charset", 14091, "DEFAULT", SPIN_CONTROL_TEXT); // charset is set by the language file

  bool use_timezone = false;

#if defined(_LINUX)
#if defined(TARGET_DARWIN)
  if (g_sysinfo.IsAppleTV2() && GetIOSVersion() < 4.3)
#endif
#if !defined(TARGET_ANDROID)
    use_timezone = true;
#endif

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
  AddSeparator(loc, "locale.sep3");
  AddString(loc, "locale.audiolanguage", 285, "original", SPIN_CONTROL_TEXT);
  AddString(loc, "locale.subtitlelanguage", 286, "original", SPIN_CONTROL_TEXT);

  CSettingsCategory* fl = AddCategory(SETTINGS_APPEARANCE, "filelists", 14081);
  AddBool(fl, "filelists.showparentdiritems", 13306, true);
  AddBool(fl, "filelists.showextensions", 497, true);
  AddBool(fl, "filelists.ignorethewhensorting", 13399, true);
  AddBool(fl, "filelists.allowfiledeletion", 14071, false);
  AddBool(fl, "filelists.showaddsourcebuttons", 21382,  true);
  AddBool(fl, "filelists.showhidden", 21330, false);

  CSettingsCategory* ss = AddCategory(SETTINGS_APPEARANCE, "screensaver", 360);
  AddInt(ss, "screensaver.time", 355, 3, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddDefaultAddon(ss, "screensaver.mode", 356, "screensaver.xbmc.builtin.dim", ADDON_SCREENSAVER);
  AddString(ss, "screensaver.settings", 21417, "", BUTTON_CONTROL_STANDARD);
  AddString(ss, "screensaver.preview", 1000, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(ss, "screensaver.sep1");
  AddBool(ss, "screensaver.usemusicvisinstead", 13392, true);
  AddBool(ss, "screensaver.usedimonpause", 22014, true);

  AddCategory(SETTINGS_APPEARANCE, "window", 0);
  AddInt(NULL, "window.width",  0, 720, 10, 1, INT_MAX, SPIN_CONTROL_INT);
  AddInt(NULL, "window.height", 0, 480, 10, 1, INT_MAX, SPIN_CONTROL_INT);

  AddPath(NULL,"system.playlistspath",20006,"set default",BUTTON_CONTROL_PATH_INPUT,false);

  // tv settings (access over TV menu from home window)
  AddGroup(SETTINGS_PVR, 19180);
  CSettingsCategory* pvr = AddCategory(SETTINGS_PVR, "pvrmanager", 128);
  AddBool(pvr, "pvrmanager.enabled", 449, false);
  AddSeparator(pvr, "pvrmanager.sep1");
  AddBool(pvr, "pvrmanager.syncchannelgroups", 19221, true);
  AddBool(pvr, "pvrmanager.backendchannelorder", 19231, true);
  AddBool(pvr, "pvrmanager.usebackendchannelnumbers", 19234, false);
  AddSeparator(pvr, "pvrmanager.sep2");
  AddString(pvr, "pvrmanager.channelmanager", 19199, "", BUTTON_CONTROL_STANDARD);
  AddString(pvr, "pvrmanager.channelscan", 19117, "", BUTTON_CONTROL_STANDARD);
  AddString(pvr, "pvrmanager.resetdb", 19185, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(pvr, "pvrmanager.sep3");
  AddBool(pvr, "pvrmanager.hideconnectionlostwarning", 19269, false);

  CSettingsCategory* pvrm = AddCategory(SETTINGS_PVR, "pvrmenu", 19181);
  AddBool(pvrm, "pvrmenu.infoswitch", 19178, true);
  AddBool(pvrm, "pvrmenu.infotimeout", 19179, true);
  AddBool(pvrm, "pvrmenu.closechannelosdonswitch", 19229, false);
  AddInt(pvrm, "pvrmenu.infotime", 19184, 5, 1, 1, 10, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddBool(pvrm, "pvrmenu.hidevideolength", 19169, true);
  AddSeparator(pvrm, "pvrmenu.sep1");
  AddString(pvrm, "pvrmenu.iconpath", 19018, "", BUTTON_CONTROL_PATH_INPUT, false, 657);
  AddString(pvrm, "pvrmenu.searchicons", 19167, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* pvre = AddCategory(SETTINGS_PVR, "epg", 19069);
  AddInt(pvre, "epg.defaultguideview", 19065, GUIDE_VIEW_TIMELINE, GUIDE_VIEW_CHANNEL, 1, GUIDE_VIEW_TIMELINE, SPIN_CONTROL_TEXT);
  AddInt(pvre, "epg.daystodisplay", 19182, 3, 1, 1, 14, SPIN_CONTROL_INT_PLUS, MASK_DAYS);
  AddSeparator(pvre, "epg.sep1");
  AddInt(pvre, "epg.epgupdate", 19071, 120, 15, 15, 2880, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddBool(pvre, "epg.preventupdateswhileplayingtv", 19230, false);
  AddBool(pvre, "epg.ignoredbforclient", 19072, false);
  AddBool(pvre, "epg.hidenoinfoavailable", 19268, true);
  AddString(pvre, "epg.resetepg", 19187, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* pvrp = AddCategory(SETTINGS_PVR, "pvrplayback", 19177);
  AddBool(pvrp, "pvrplayback.playminimized", 19171, true);
  AddInt(pvrp, "pvrplayback.startlast", 19189, START_LAST_CHANNEL_OFF, START_LAST_CHANNEL_OFF, 1, START_LAST_CHANNEL_ON, SPIN_CONTROL_TEXT);
  AddBool(pvrp, "pvrplayback.switchautoclose", 19168, true);
  AddBool(pvrp, "pvrplayback.signalquality", 19037, true);
  AddSeparator(pvrp, "pvrplayback.sep1");
  AddInt(pvrp, "pvrplayback.scantime", 19170, 10, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddInt(pvrp, "pvrplayback.channelentrytimeout", 19073, 0, 0, 250, 2000, SPIN_CONTROL_INT_PLUS, MASK_MS);

  CSettingsCategory* pvrr = AddCategory(SETTINGS_PVR, "pvrrecord", 19043);
  AddInt(pvrr, "pvrrecord.instantrecordtime", 19172, 120, 1, 1, 720, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddInt(pvrr, "pvrrecord.defaultpriority", 19173, 50, 1, 1, 100, SPIN_CONTROL_INT_PLUS);
  AddInt(pvrr, "pvrrecord.defaultlifetime", 19174, 99, 1, 1, 365, SPIN_CONTROL_INT_PLUS, MASK_DAYS);
  AddInt(pvrr, "pvrrecord.marginstart", 19175, 2, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddInt(pvrr, "pvrrecord.marginend", 19176, 10, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddSeparator(pvrr, "pvrrecord.sep1");
  AddBool(pvrr, "pvrrecord.timernotifications", 19233, true);

  CSettingsCategory* pvrpwr = AddCategory(SETTINGS_PVR, "pvrpowermanagement", 14095);
  AddBool(pvrpwr, "pvrpowermanagement.enabled", 305, false);
  AddSeparator(pvrpwr, "pvrpowermanagement.sep1");
  AddInt(pvrpwr, "pvrpowermanagement.backendidletime", 19244, 15, 0, 5, 360, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  AddString(pvrpwr, "pvrpowermanagement.setwakeupcmd", 19245, "", EDIT_CONTROL_INPUT, true);
  AddInt(pvrpwr, "pvrpowermanagement.prewakeup", 19246, 15, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  AddSeparator(pvrpwr, "pvrpowermanagement.sep2");
  AddBool(pvrpwr, "pvrpowermanagement.dailywakeup", 19247, false);
  AddString(pvrpwr, "pvrpowermanagement.dailywakeuptime", 19248, "00:00:00", EDIT_CONTROL_INPUT);

  CSettingsCategory* pvrpa = AddCategory(SETTINGS_PVR, "pvrparental", 19259);
  AddBool(pvrpa, "pvrparental.enabled", 449  , false);
  AddSeparator(pvrpa, "pvrparental.sep1");
  AddString(pvrpa, "pvrparental.pin", 19261, "", EDIT_CONTROL_HIDDEN_NUMBER_VERIFY_NEW, true);
  AddInt(pvrpa, "pvrparental.duration", 19260, 300, 5, 5, 1200, SPIN_CONTROL_INT_PLUS, MASK_SECS);
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

void CGUISettings::AddSetting(CSettingsCategory* cat, CSetting* setting)
{
  if (!setting)
    return;

  if (cat)
    cat->m_settings.push_back(setting);
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(setting->GetSetting()).ToLower(), setting));
}

void CGUISettings::AddSeparator(CSettingsCategory* cat, const char *strSetting)
{
  int iOrder = cat ? cat->m_settings.size() : 0;
  CSettingSeparator *pSetting = new CSettingSeparator(iOrder, CStdString(strSetting).ToLower());
  if (!pSetting) return;
  AddSetting(cat, pSetting);
}

void CGUISettings::AddBool(CSettingsCategory* cat, const char *strSetting, int iLabel, bool bData, int iControlType)
{
  int iOrder = cat ? cat->m_settings.size() : 0;
  CSettingBool* pSetting = new CSettingBool(iOrder, CStdString(strSetting).ToLower(), iLabel, bData, iControlType);
  if (!pSetting) return ;
  AddSetting(cat, pSetting);
}

bool CGUISettings::GetBool(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
  { // old category
    return ((CSettingBool*)(*it).second)->GetData();
  }
  // Backward compatibility (skins use this setting)
  if (strncmp(strSetting, "lookandfeel.enablemouse", 23) == 0)
    return GetBool("input.enablemouse");
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return false;
}

void CGUISettings::SetBool(const char *strSetting, bool bSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(strSetting);
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
  int iOrder = cat ? cat->m_settings.size() : 0;
  CSettingFloat* pSetting = new CSettingFloat(iOrder, CStdString(strSetting).ToLower(), iLabel, fData, fMin, fStep, fMax, iControlType);
  if (!pSetting) return ;
  AddSetting(cat, pSetting);
}

float CGUISettings::GetFloat(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(strSetting);
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
  mapIter it = settingsMap.find(strSetting);
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
  mapIter it = settingsMap.find("masterlock.maxretries");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.startuplock");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
}

void CGUISettings::AddInt(CSettingsCategory* cat, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  int iOrder = cat ? cat->m_settings.size() : 0;
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  AddSetting(cat, pSetting);
}

void CGUISettings::AddInt(CSettingsCategory* cat, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin/*=-1*/)
{
  int iOrder = cat ? cat->m_settings.size() : 0;
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, iFormat, iLabelMin);
  if (!pSetting) return ;
  AddSetting(cat, pSetting);
}

void CGUISettings::AddInt(CSettingsCategory* cat, const char *strSetting, int iLabel, int iData, const map<int,int>& entries, int iControlType)
{
  int iOrder = cat ? cat->m_settings.size() : 0;
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, entries, iControlType);
  if (!pSetting) return ;
  AddSetting(cat, pSetting);
}

void CGUISettings::AddHex(CSettingsCategory* cat, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  int iOrder = cat ? cat->m_settings.size() : 0;
  CSettingHex* pSetting = new CSettingHex(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  AddSetting(cat, pSetting);
}

int CGUISettings::GetInt(const char *strSetting) const
{
  ASSERT(settingsMap.size());

  constMapIter it = settingsMap.find(strSetting);
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
  mapIter it = settingsMap.find(strSetting);
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
  int iOrder = cat ? cat->m_settings.size() : 0;
  CSettingString* pSetting = new CSettingString(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  AddSetting(cat, pSetting);
}

void CGUISettings::AddPath(CSettingsCategory* cat, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  int iOrder = cat ? cat->m_settings.size() : 0;
  CSettingPath* pSetting = new CSettingPath(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  AddSetting(cat, pSetting);
}

void CGUISettings::AddDefaultAddon(CSettingsCategory* cat, const char *strSetting, int iLabel, const char *strData, const TYPE type)
{
  int iOrder = cat ? cat->m_settings.size() : 0;
  CSettingAddon* pSetting = new CSettingAddon(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, type);
  if (!pSetting) return ;
  AddSetting(cat, pSetting);
}

const CStdString &CGUISettings::GetString(const char *strSetting, bool bPrompt /* = true */) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(strSetting);
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
  mapIter it = settingsMap.find(strSetting);
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
  mapIter it = settingsMap.find(strSetting);
  if (it != settingsMap.end())
    return (*it).second;
  else
    return NULL;
}

// get all the settings beginning with the term "strGroup"
void CGUISettings::GetSettingsGroup(CSettingsCategory* cat, vecSettings &settings)
{
  if (!cat || cat->m_settings.size() <= 0)
    return;

  vecSettings unorderedSettings;
  for (unsigned int index = 0; index < cat->m_settings.size(); index++)
  {
    if (!cat->m_settings.at(index)->IsAdvanced())
      unorderedSettings.push_back(cat->m_settings.at(index));
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

#if defined(TARGET_DARWIN)
  // trap any previous vsync by driver setting, does not exist on OSX
  if (GetInt("videoscreen.vsync") == VSYNC_DRIVER)
  {
    SetInt("videoscreen.vsync", VSYNC_ALWAYS);
  }
#endif
 // DXMERGE: This might have been useful?
 // g_videoConfig.SetVSyncMode((VSYNC)GetInt("videoscreen.vsync"));

  // Move replaygain settings into our struct
  m_replayGain.iPreAmp = GetInt("musicplayer.replaygainpreamp");
  m_replayGain.iNoGainPreAmp = GetInt("musicplayer.replaygainnogainpreamp");
  m_replayGain.iType = GetInt("musicplayer.replaygaintype");
  m_replayGain.bAvoidClipping = GetBool("musicplayer.replaygainavoidclipping");

  bool use_timezone = false;

#if defined(_LINUX)
#if defined(TARGET_DARWIN)
  if (g_sysinfo.IsAppleTV2() && GetIOSVersion() < 4.3)
#endif
#if !defined(TARGET_ANDROID)
    use_timezone = true;
#endif

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

  CStdString streamLanguage = GetString("locale.audiolanguage");
  if (!streamLanguage.Equals("original") && !streamLanguage.Equals("default"))
    g_langInfo.SetAudioLanguage(streamLanguage);
  else
    g_langInfo.SetAudioLanguage("");

  streamLanguage = GetString("locale.subtitlelanguage");
  if (!streamLanguage.Equals("original") && !streamLanguage.Equals("default"))
    g_langInfo.SetSubtitleLanguage(streamLanguage);
  else
    g_langInfo.SetSubtitleLanguage("");
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
          newElement.SetAttribute("pathversion", XMLUtils::path_version);
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
  else if (res.GetLength() == 21)
  {
    // format: SWWWWWHHHHHRRR.RRRRRP, where S = screen, W = width, H = height, R = refresh, P = interlace
    int screen = atol(res.Mid(0,1).c_str());
    int width = atol(res.Mid(1,5).c_str());
    int height = atol(res.Mid(6,5).c_str());
    float refresh = (float)atof(res.Mid(11,9).c_str());
    // look for 'i' and treat everything else as progressive,
    // and use 100/200 to get a nice square_error.
    int interlaced = (res.Right(1) == "i") ? 100:200;
    // find the closest match to these in our res vector.  If we have the screen, we score the res
    RESOLUTION bestRes = RES_DESKTOP;
    float bestScore = FLT_MAX;
    for (unsigned int i = RES_DESKTOP; i < g_settings.m_ResInfo.size(); ++i)
    {
      const RESOLUTION_INFO &info = g_settings.m_ResInfo[i];
      if (info.iScreen != screen)
        continue;
      float score = 10 * (square_error((float)info.iScreenWidth, (float)width) +
        square_error((float)info.iScreenHeight, (float)height) +
        square_error(info.fRefreshRate, refresh) +
        square_error((float)((info.dwFlags & D3DPRESENTFLAG_INTERLACED) ? 100:200), (float)interlaced));
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
    mode.Format("%1i%05i%05i%09.5f%s", info.iScreen,
      info.iScreenWidth, info.iScreenHeight, info.fRefreshRate,
      (info.dwFlags & D3DPRESENTFLAG_INTERLACED) ? "i":"p");
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

    if (!g_localizeStrings.Load("special://xbmc/language/", strNewLanguage))
      return false;

    // also tell our weather and skin to reload as these are localized
    g_weatherManager.Refresh();
    g_PVRManager.LocalizationChanged();
    g_application.ReloadSkin();
  }

  return true;
}
