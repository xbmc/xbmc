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

#include "system.h"

#include "Settings.h"
#include "Application.h"
#include "Autorun.h"
#include "LangInfo.h"
#include "Util.h"
#include "events/EventLog.h"
#include "addons/AddonSystemSettings.h"
#include "addons/RepositoryUpdater.h"
#include "addons/Skin.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAESettings.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "filesystem/File.h"
#include "games/GameSettings.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIFontManager.h"
#include "guilib/StereoscopicsManager.h"
#include "input/KeyboardLayoutManager.h"
#if defined(TARGET_POSIX)
#include "linux/LinuxTimezone.h"
#endif // defined(TARGET_POSIX)
#include "network/NetworkServices.h"
#include "network/upnp/UPnPSettings.h"
#include "network/WakeOnAccess.h"
#if defined(TARGET_DARWIN_OSX)
#include "platform/darwin/osx/XBMCHelper.h"
#endif // defined(TARGET_DARWIN_OSX)
#if defined(TARGET_DARWIN)
#include "platform/darwin/DarwinUtils.h"
#endif
#if defined(TARGET_DARWIN_IOS)
#include "SettingAddon.h"
#endif
#if defined(TARGET_RASPBERRY_PI)
#include "linux/RBP.h"
#endif
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif // defined(HAS_LIBAMCODEC)
#include "peripherals/Peripherals.h"
#include "powermanagement/PowerManager.h"
#include "profiles/ProfilesManager.h"
#include "pvr/PVRSettings.h"
#include "pvr/windows/GUIWindowPVRGuide.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingConditions.h"
#include "settings/SettingUtils.h"
#include "settings/SkinSettings.h"
#include "settings/lib/SettingsManager.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/RssManager.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Weather.h"
#include "utils/XBMCTinyXML.h"
#include "utils/SeekHandler.h"
#include "utils/Variant.h"
#include "view/ViewStateSettings.h"
#include "input/InputManager.h"
#include "ServiceBroker.h"
#include "DiscSettings.h"

#define SETTINGS_XML_FOLDER "special://xbmc/system/settings/"

using namespace KODI;
using namespace XFILE;

const std::string CSettings::SETTING_LOOKANDFEEL_SKIN = "lookandfeel.skin";
const std::string CSettings::SETTING_LOOKANDFEEL_SKINSETTINGS = "lookandfeel.skinsettings";
const std::string CSettings::SETTING_LOOKANDFEEL_SKINTHEME = "lookandfeel.skintheme";
const std::string CSettings::SETTING_LOOKANDFEEL_SKINCOLORS = "lookandfeel.skincolors";
const std::string CSettings::SETTING_LOOKANDFEEL_FONT = "lookandfeel.font";
const std::string CSettings::SETTING_LOOKANDFEEL_SKINZOOM = "lookandfeel.skinzoom";
const std::string CSettings::SETTING_LOOKANDFEEL_STARTUPWINDOW = "lookandfeel.startupwindow";
const std::string CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN = "lookandfeel.soundskin";
const std::string CSettings::SETTING_LOOKANDFEEL_ENABLERSSFEEDS = "lookandfeel.enablerssfeeds";
const std::string CSettings::SETTING_LOOKANDFEEL_RSSEDIT = "lookandfeel.rssedit";
const std::string CSettings::SETTING_LOOKANDFEEL_STEREOSTRENGTH = "lookandfeel.stereostrength";
const std::string CSettings::SETTING_LOCALE_LANGUAGE = "locale.language";
const std::string CSettings::SETTING_LOCALE_COUNTRY = "locale.country";
const std::string CSettings::SETTING_LOCALE_CHARSET = "locale.charset";
const std::string CSettings::SETTING_LOCALE_KEYBOARDLAYOUTS = "locale.keyboardlayouts";
const std::string CSettings::SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT = "locale.activekeyboardlayout";
const std::string CSettings::SETTING_LOCALE_TIMEZONECOUNTRY = "locale.timezonecountry";
const std::string CSettings::SETTING_LOCALE_TIMEZONE = "locale.timezone";
const std::string CSettings::SETTING_LOCALE_SHORTDATEFORMAT = "locale.shortdateformat";
const std::string CSettings::SETTING_LOCALE_LONGDATEFORMAT = "locale.longdateformat";
const std::string CSettings::SETTING_LOCALE_TIMEFORMAT = "locale.timeformat";
const std::string CSettings::SETTING_LOCALE_USE24HOURCLOCK = "locale.use24hourclock";
const std::string CSettings::SETTING_LOCALE_TEMPERATUREUNIT = "locale.temperatureunit";
const std::string CSettings::SETTING_LOCALE_SPEEDUNIT = "locale.speedunit";
const std::string CSettings::SETTING_FILELISTS_SHOWPARENTDIRITEMS = "filelists.showparentdiritems";
const std::string CSettings::SETTING_FILELISTS_SHOWEXTENSIONS = "filelists.showextensions";
const std::string CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING = "filelists.ignorethewhensorting";
const std::string CSettings::SETTING_FILELISTS_ALLOWFILEDELETION = "filelists.allowfiledeletion";
const std::string CSettings::SETTING_FILELISTS_SHOWADDSOURCEBUTTONS = "filelists.showaddsourcebuttons";
const std::string CSettings::SETTING_FILELISTS_SHOWHIDDEN = "filelists.showhidden";
const std::string CSettings::SETTING_SCREENSAVER_MODE = "screensaver.mode";
const std::string CSettings::SETTING_SCREENSAVER_SETTINGS = "screensaver.settings";
const std::string CSettings::SETTING_SCREENSAVER_PREVIEW = "screensaver.preview";
const std::string CSettings::SETTING_SCREENSAVER_TIME = "screensaver.time";
const std::string CSettings::SETTING_SCREENSAVER_USEMUSICVISINSTEAD = "screensaver.usemusicvisinstead";
const std::string CSettings::SETTING_SCREENSAVER_USEDIMONPAUSE = "screensaver.usedimonpause";
const std::string CSettings::SETTING_WINDOW_WIDTH = "window.width";
const std::string CSettings::SETTING_WINDOW_HEIGHT = "window.height";
const std::string CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS = "videolibrary.showunwatchedplots";
const std::string CSettings::SETTING_VIDEOLIBRARY_ACTORTHUMBS = "videolibrary.actorthumbs";
const std::string CSettings::SETTING_MYVIDEOS_FLATTEN = "myvideos.flatten";
const std::string CSettings::SETTING_VIDEOLIBRARY_FLATTENTVSHOWS = "videolibrary.flattentvshows";
const std::string CSettings::SETTING_VIDEOLIBRARY_TVSHOWSSELECTFIRSTUNWATCHEDITEM = "videolibrary.tvshowsselectfirstunwatcheditem";
const std::string CSettings::SETTING_VIDEOLIBRARY_TVSHOWSINCLUDEALLSEASONSANDSPECIALS = "videolibrary.tvshowsincludeallseasonsandspecials";
const std::string CSettings::SETTING_VIDEOLIBRARY_SHOWALLITEMS = "videolibrary.showallitems";
const std::string CSettings::SETTING_VIDEOLIBRARY_GROUPMOVIESETS = "videolibrary.groupmoviesets";
const std::string CSettings::SETTING_VIDEOLIBRARY_GROUPSINGLEITEMSETS = "videolibrary.groupsingleitemsets";
const std::string CSettings::SETTING_VIDEOLIBRARY_UPDATEONSTARTUP = "videolibrary.updateonstartup";
const std::string CSettings::SETTING_VIDEOLIBRARY_BACKGROUNDUPDATE = "videolibrary.backgroundupdate";
const std::string CSettings::SETTING_VIDEOLIBRARY_CLEANUP = "videolibrary.cleanup";
const std::string CSettings::SETTING_VIDEOLIBRARY_EXPORT = "videolibrary.export";
const std::string CSettings::SETTING_VIDEOLIBRARY_IMPORT = "videolibrary.import";
const std::string CSettings::SETTING_VIDEOLIBRARY_SHOWEMPTYTVSHOWS = "videolibrary.showemptytvshows";
const std::string CSettings::SETTING_LOCALE_AUDIOLANGUAGE = "locale.audiolanguage";
const std::string CSettings::SETTING_VIDEOPLAYER_PREFERDEFAULTFLAG = "videoplayer.preferdefaultflag";
const std::string CSettings::SETTING_VIDEOPLAYER_AUTOPLAYNEXTITEM = "videoplayer.autoplaynextitem";
const std::string CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS = "videoplayer.seeksteps";
const std::string CSettings::SETTING_VIDEOPLAYER_SEEKDELAY = "videoplayer.seekdelay";
const std::string CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE = "videoplayer.adjustrefreshrate";
const std::string CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK = "videoplayer.usedisplayasclock";
const std::string CSettings::SETTING_VIDEOPLAYER_ERRORINASPECT = "videoplayer.errorinaspect";
const std::string CSettings::SETTING_VIDEOPLAYER_STRETCH43 = "videoplayer.stretch43";
const std::string CSettings::SETTING_VIDEOPLAYER_TELETEXTENABLED = "videoplayer.teletextenabled";
const std::string CSettings::SETTING_VIDEOPLAYER_TELETEXTSCALE = "videoplayer.teletextscale";
const std::string CSettings::SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE = "videoplayer.stereoscopicplaybackmode";
const std::string CSettings::SETTING_VIDEOPLAYER_QUITSTEREOMODEONSTOP = "videoplayer.quitstereomodeonstop";
const std::string CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD = "videoplayer.rendermethod";
const std::string CSettings::SETTING_VIDEOPLAYER_HQSCALERS = "videoplayer.hqscalers";
const std::string CSettings::SETTING_VIDEOPLAYER_USEAMCODEC = "videoplayer.useamcodec";
const std::string CSettings::SETTING_VIDEOPLAYER_USEAMCODECMPEG2 = "videoplayer.useamcodecmpeg2";
const std::string CSettings::SETTING_VIDEOPLAYER_USEAMCODECMPEG4 = "videoplayer.useamcodecmpeg4";
const std::string CSettings::SETTING_VIDEOPLAYER_USEAMCODECH264 = "videoplayer.useamcodech264";
const std::string CSettings::SETTING_VIDEOPLAYER_USEMEDIACODEC = "videoplayer.usemediacodec";
const std::string CSettings::SETTING_VIDEOPLAYER_USEMEDIACODECSURFACE = "videoplayer.usemediacodecsurface";
const std::string CSettings::SETTING_VIDEOPLAYER_USEVDPAU = "videoplayer.usevdpau";
const std::string CSettings::SETTING_VIDEOPLAYER_USEVDPAUMIXER = "videoplayer.usevdpaumixer";
const std::string CSettings::SETTING_VIDEOPLAYER_USEVDPAUMPEG2 = "videoplayer.usevdpaumpeg2";
const std::string CSettings::SETTING_VIDEOPLAYER_USEVDPAUMPEG4 = "videoplayer.usevdpaumpeg4";
const std::string CSettings::SETTING_VIDEOPLAYER_USEVDPAUVC1 = "videoplayer.usevdpauvc1";
const std::string CSettings::SETTING_VIDEOPLAYER_USEVAAPI = "videoplayer.usevaapi";
const std::string CSettings::SETTING_VIDEOPLAYER_USEVAAPIMPEG2 = "videoplayer.usevaapimpeg2";
const std::string CSettings::SETTING_VIDEOPLAYER_USEVAAPIMPEG4 = "videoplayer.usevaapimpeg4";
const std::string CSettings::SETTING_VIDEOPLAYER_USEVAAPIVC1 = "videoplayer.usevaapivc1";
const std::string CSettings::SETTING_VIDEOPLAYER_PREFERVAAPIRENDER = "videoplayer.prefervaapirender";
const std::string CSettings::SETTING_VIDEOPLAYER_USEDXVA2 = "videoplayer.usedxva2";
const std::string CSettings::SETTING_VIDEOPLAYER_USEOMXPLAYER = "videoplayer.useomxplayer";
const std::string CSettings::SETTING_VIDEOPLAYER_USEVTB = "videoplayer.usevtb";
const std::string CSettings::SETTING_VIDEOPLAYER_USEMMAL = "videoplayer.usemmal";
const std::string CSettings::SETTING_VIDEOPLAYER_USESTAGEFRIGHT = "videoplayer.usestagefright";
const std::string CSettings::SETTING_VIDEOPLAYER_LIMITGUIUPDATE = "videoplayer.limitguiupdate";
const std::string CSettings::SETTING_VIDEOPLAYER_SUPPORTMVC = "videoplayer.supportmvc";
const std::string CSettings::SETTING_MYVIDEOS_SELECTACTION = "myvideos.selectaction";
const std::string CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS = "myvideos.extractflags";
const std::string CSettings::SETTING_MYVIDEOS_EXTRACTCHAPTERTHUMBS = "myvideos.extractchapterthumbs";
const std::string CSettings::SETTING_MYVIDEOS_REPLACELABELS = "myvideos.replacelabels";
const std::string CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB = "myvideos.extractthumb";
const std::string CSettings::SETTING_MYVIDEOS_STACKVIDEOS = "myvideos.stackvideos";
const std::string CSettings::SETTING_LOCALE_SUBTITLELANGUAGE = "locale.subtitlelanguage";
const std::string CSettings::SETTING_SUBTITLES_PARSECAPTIONS = "subtitles.parsecaptions";
const std::string CSettings::SETTING_SUBTITLES_ALIGN = "subtitles.align";
const std::string CSettings::SETTING_SUBTITLES_STEREOSCOPICDEPTH = "subtitles.stereoscopicdepth";
const std::string CSettings::SETTING_SUBTITLES_FONT = "subtitles.font";
const std::string CSettings::SETTING_SUBTITLES_HEIGHT = "subtitles.height";
const std::string CSettings::SETTING_SUBTITLES_STYLE = "subtitles.style";
const std::string CSettings::SETTING_SUBTITLES_COLOR = "subtitles.color";
const std::string CSettings::SETTING_SUBTITLES_CHARSET = "subtitles.charset";
const std::string CSettings::SETTING_SUBTITLES_OVERRIDEASSFONTS = "subtitles.overrideassfonts";
const std::string CSettings::SETTING_SUBTITLES_LANGUAGES = "subtitles.languages";
const std::string CSettings::SETTING_SUBTITLES_STORAGEMODE = "subtitles.storagemode";
const std::string CSettings::SETTING_SUBTITLES_CUSTOMPATH = "subtitles.custompath";
const std::string CSettings::SETTING_SUBTITLES_PAUSEONSEARCH = "subtitles.pauseonsearch";
const std::string CSettings::SETTING_SUBTITLES_DOWNLOADFIRST = "subtitles.downloadfirst";
const std::string CSettings::SETTING_SUBTITLES_TV = "subtitles.tv";
const std::string CSettings::SETTING_SUBTITLES_MOVIE = "subtitles.movie";
const std::string CSettings::SETTING_DVDS_AUTORUN = "dvds.autorun";
const std::string CSettings::SETTING_DVDS_PLAYERREGION = "dvds.playerregion";
const std::string CSettings::SETTING_DVDS_AUTOMENU = "dvds.automenu";
const std::string CSettings::SETTING_DISC_PLAYBACK = "disc.playback";
const std::string CSettings::SETTING_BLURAY_PLAYERREGION = "bluray.playerregion";
const std::string CSettings::SETTING_ACCESSIBILITY_AUDIOVISUAL = "accessibility.audiovisual";
const std::string CSettings::SETTING_ACCESSIBILITY_AUDIOHEARING = "accessibility.audiohearing";
const std::string CSettings::SETTING_ACCESSIBILITY_SUBHEARING = "accessibility.subhearing";
const std::string CSettings::SETTING_SCRAPERS_MOVIESDEFAULT = "scrapers.moviesdefault";
const std::string CSettings::SETTING_SCRAPERS_TVSHOWSDEFAULT = "scrapers.tvshowsdefault";
const std::string CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT = "scrapers.musicvideosdefault";
const std::string CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS = "pvrmanager.syncchannelgroups";
const std::string CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER = "pvrmanager.backendchannelorder";
const std::string CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS = "pvrmanager.usebackendchannelnumbers";
const std::string CSettings::SETTING_PVRMANAGER_CHANNELMANAGER = "pvrmanager.channelmanager";
const std::string CSettings::SETTING_PVRMANAGER_GROUPMANAGER = "pvrmanager.groupmanager";
const std::string CSettings::SETTING_PVRMANAGER_CHANNELSCAN = "pvrmanager.channelscan";
const std::string CSettings::SETTING_PVRMANAGER_RESETDB = "pvrmanager.resetdb";
const std::string CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO = "pvrmenu.displaychannelinfo";
const std::string CSettings::SETTING_PVRMENU_CLOSECHANNELOSDONSWITCH = "pvrmenu.closechannelosdonswitch";
const std::string CSettings::SETTING_PVRMENU_ICONPATH = "pvrmenu.iconpath";
const std::string CSettings::SETTING_PVRMENU_SEARCHICONS = "pvrmenu.searchicons";
const std::string CSettings::SETTING_EPG_DAYSTODISPLAY = "epg.daystodisplay";
const std::string CSettings::SETTING_EPG_SELECTACTION = "epg.selectaction";
const std::string CSettings::SETTING_EPG_HIDENOINFOAVAILABLE = "epg.hidenoinfoavailable";
const std::string CSettings::SETTING_EPG_EPGUPDATE = "epg.epgupdate";
const std::string CSettings::SETTING_EPG_PREVENTUPDATESWHILEPLAYINGTV = "epg.preventupdateswhileplayingtv";
const std::string CSettings::SETTING_EPG_IGNOREDBFORCLIENT = "epg.ignoredbforclient";
const std::string CSettings::SETTING_EPG_RESETEPG = "epg.resetepg";
const std::string CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREEN = "pvrplayback.switchtofullscreen";
const std::string CSettings::SETTING_PVRPLAYBACK_STARTLAST = "pvrplayback.startlast";
const std::string CSettings::SETTING_PVRPLAYBACK_SIGNALQUALITY = "pvrplayback.signalquality";
const std::string CSettings::SETTING_PVRPLAYBACK_SCANTIME = "pvrplayback.scantime";
const std::string CSettings::SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH = "pvrplayback.confirmchannelswitch";
const std::string CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT = "pvrplayback.channelentrytimeout";
const std::string CSettings::SETTING_PVRPLAYBACK_FPS = "pvrplayback.fps";
const std::string CSettings::SETTING_PVRRECORD_INSTANTRECORDACTION = "pvrrecord.instantrecordaction";
const std::string CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME = "pvrrecord.instantrecordtime";
const std::string CSettings::SETTING_PVRRECORD_MARGINSTART = "pvrrecord.marginstart";
const std::string CSettings::SETTING_PVRRECORD_MARGINEND = "pvrrecord.marginend";
const std::string CSettings::SETTING_PVRRECORD_TIMERNOTIFICATIONS = "pvrrecord.timernotifications";
const std::string CSettings::SETTING_PVRRECORD_GROUPRECORDINGS = "pvrrecord.grouprecordings";
const std::string CSettings::SETTING_PVRPOWERMANAGEMENT_ENABLED = "pvrpowermanagement.enabled";
const std::string CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME = "pvrpowermanagement.backendidletime";
const std::string CSettings::SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD = "pvrpowermanagement.setwakeupcmd";
const std::string CSettings::SETTING_PVRPOWERMANAGEMENT_PREWAKEUP = "pvrpowermanagement.prewakeup";
const std::string CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUP = "pvrpowermanagement.dailywakeup";
const std::string CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME = "pvrpowermanagement.dailywakeuptime";
const std::string CSettings::SETTING_PVRPARENTAL_ENABLED = "pvrparental.enabled";
const std::string CSettings::SETTING_PVRPARENTAL_PIN = "pvrparental.pin";
const std::string CSettings::SETTING_PVRPARENTAL_DURATION = "pvrparental.duration";
const std::string CSettings::SETTING_PVRCLIENT_MENUHOOK = "pvrclient.menuhook";
const std::string CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS = "pvrtimers.hidedisabledtimers";
const std::string CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS = "musiclibrary.showcompilationartists";
const std::string CSettings::SETTING_MUSICLIBRARY_DOWNLOADINFO = "musiclibrary.downloadinfo";
const std::string CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER = "musiclibrary.albumsscraper";
const std::string CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER = "musiclibrary.artistsscraper";
const std::string CSettings::SETTING_MUSICLIBRARY_OVERRIDETAGS = "musiclibrary.overridetags";
const std::string CSettings::SETTING_MUSICLIBRARY_SHOWALLITEMS = "musiclibrary.showallitems";
const std::string CSettings::SETTING_MUSICLIBRARY_UPDATEONSTARTUP = "musiclibrary.updateonstartup";
const std::string CSettings::SETTING_MUSICLIBRARY_BACKGROUNDUPDATE = "musiclibrary.backgroundupdate";
const std::string CSettings::SETTING_MUSICLIBRARY_CLEANUP = "musiclibrary.cleanup";
const std::string CSettings::SETTING_MUSICLIBRARY_EXPORT = "musiclibrary.export";
const std::string CSettings::SETTING_MUSICLIBRARY_IMPORT = "musiclibrary.import";
const std::string CSettings::SETTING_MUSICPLAYER_AUTOPLAYNEXTITEM = "musicplayer.autoplaynextitem";
const std::string CSettings::SETTING_MUSICPLAYER_QUEUEBYDEFAULT = "musicplayer.queuebydefault";
const std::string CSettings::SETTING_MUSICPLAYER_SEEKSTEPS = "musicplayer.seeksteps";
const std::string CSettings::SETTING_MUSICPLAYER_SEEKDELAY = "musicplayer.seekdelay";
const std::string CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE = "musicplayer.replaygaintype";
const std::string CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP = "musicplayer.replaygainpreamp";
const std::string CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP = "musicplayer.replaygainnogainpreamp";
const std::string CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING = "musicplayer.replaygainavoidclipping";
const std::string CSettings::SETTING_MUSICPLAYER_CROSSFADE = "musicplayer.crossfade";
const std::string CSettings::SETTING_MUSICPLAYER_CROSSFADEALBUMTRACKS = "musicplayer.crossfadealbumtracks";
const std::string CSettings::SETTING_MUSICPLAYER_VISUALISATION = "musicplayer.visualisation";
const std::string CSettings::SETTING_MUSICFILES_USETAGS = "musicfiles.usetags";
const std::string CSettings::SETTING_MUSICFILES_TRACKFORMAT = "musicfiles.trackformat";
const std::string CSettings::SETTING_MUSICFILES_NOWPLAYINGTRACKFORMAT = "musicfiles.nowplayingtrackformat";
const std::string CSettings::SETTING_MUSICFILES_LIBRARYTRACKFORMAT = "musicfiles.librarytrackformat";
const std::string CSettings::SETTING_MUSICFILES_FINDREMOTETHUMBS = "musicfiles.findremotethumbs";
const std::string CSettings::SETTING_AUDIOCDS_AUTOACTION = "audiocds.autoaction";
const std::string CSettings::SETTING_AUDIOCDS_USECDDB = "audiocds.usecddb";
const std::string CSettings::SETTING_AUDIOCDS_RECORDINGPATH = "audiocds.recordingpath";
const std::string CSettings::SETTING_AUDIOCDS_TRACKPATHFORMAT = "audiocds.trackpathformat";
const std::string CSettings::SETTING_AUDIOCDS_ENCODER = "audiocds.encoder";
const std::string CSettings::SETTING_AUDIOCDS_SETTINGS = "audiocds.settings";
const std::string CSettings::SETTING_AUDIOCDS_EJECTONRIP = "audiocds.ejectonrip";
const std::string CSettings::SETTING_MYMUSIC_SONGTHUMBINVIS = "mymusic.songthumbinvis";
const std::string CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW = "mymusic.defaultlibview";
const std::string CSettings::SETTING_PICTURES_USETAGS = "pictures.usetags";
const std::string CSettings::SETTING_PICTURES_GENERATETHUMBS = "pictures.generatethumbs";
const std::string CSettings::SETTING_PICTURES_SHOWVIDEOS = "pictures.showvideos";
const std::string CSettings::SETTING_PICTURES_DISPLAYRESOLUTION = "pictures.displayresolution";
const std::string CSettings::SETTING_SLIDESHOW_STAYTIME = "slideshow.staytime";
const std::string CSettings::SETTING_SLIDESHOW_DISPLAYEFFECTS = "slideshow.displayeffects";
const std::string CSettings::SETTING_SLIDESHOW_SHUFFLE = "slideshow.shuffle";
const std::string CSettings::SETTING_SLIDESHOW_HIGHQUALITYDOWNSCALING = "slideshow.highqualitydownscaling";
const std::string CSettings::SETTING_WEATHER_CURRENTLOCATION = "weather.currentlocation";
const std::string CSettings::SETTING_WEATHER_ADDON = "weather.addon";
const std::string CSettings::SETTING_WEATHER_ADDONSETTINGS = "weather.addonsettings";
const std::string CSettings::SETTING_SERVICES_DEVICENAME = "services.devicename";
const std::string CSettings::SETTING_SERVICES_UPNP = "services.upnp";
const std::string CSettings::SETTING_SERVICES_UPNPSERVER = "services.upnpserver";
const std::string CSettings::SETTING_SERVICES_UPNPANNOUNCE = "services.upnpannounce";
const std::string CSettings::SETTING_SERVICES_UPNPLOOKFOREXTERNALSUBTITLES = "services.upnplookforexternalsubtitles";
const std::string CSettings::SETTING_SERVICES_UPNPCONTROLLER = "services.upnpcontroller";
const std::string CSettings::SETTING_SERVICES_UPNPRENDERER = "services.upnprenderer";
const std::string CSettings::SETTING_SERVICES_WEBSERVER = "services.webserver";
const std::string CSettings::SETTING_SERVICES_WEBSERVERPORT = "services.webserverport";
const std::string CSettings::SETTING_SERVICES_WEBSERVERUSERNAME = "services.webserverusername";
const std::string CSettings::SETTING_SERVICES_WEBSERVERPASSWORD = "services.webserverpassword";
const std::string CSettings::SETTING_SERVICES_WEBSKIN = "services.webskin";
const std::string CSettings::SETTING_SERVICES_ESENABLED = "services.esenabled";
const std::string CSettings::SETTING_SERVICES_ESPORT = "services.esport";
const std::string CSettings::SETTING_SERVICES_ESPORTRANGE = "services.esportrange";
const std::string CSettings::SETTING_SERVICES_ESMAXCLIENTS = "services.esmaxclients";
const std::string CSettings::SETTING_SERVICES_ESALLINTERFACES = "services.esallinterfaces";
const std::string CSettings::SETTING_SERVICES_ESINITIALDELAY = "services.esinitialdelay";
const std::string CSettings::SETTING_SERVICES_ESCONTINUOUSDELAY = "services.escontinuousdelay";
const std::string CSettings::SETTING_SERVICES_ZEROCONF = "services.zeroconf";
const std::string CSettings::SETTING_SERVICES_AIRPLAY = "services.airplay";
const std::string CSettings::SETTING_SERVICES_AIRPLAYVOLUMECONTROL = "services.airplayvolumecontrol";
const std::string CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD = "services.useairplaypassword";
const std::string CSettings::SETTING_SERVICES_AIRPLAYPASSWORD = "services.airplaypassword";
const std::string CSettings::SETTING_SERVICES_AIRPLAYVIDEOSUPPORT = "services.airplayvideosupport";
const std::string CSettings::SETTING_SMB_WINSSERVER = "smb.winsserver";
const std::string CSettings::SETTING_SMB_WORKGROUP = "smb.workgroup";
const std::string CSettings::SETTING_VIDEOSCREEN_MONITOR = "videoscreen.monitor";
const std::string CSettings::SETTING_VIDEOSCREEN_SCREEN = "videoscreen.screen";
const std::string CSettings::SETTING_VIDEOSCREEN_RESOLUTION = "videoscreen.resolution";
const std::string CSettings::SETTING_VIDEOSCREEN_SCREENMODE = "videoscreen.screenmode";
const std::string CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN = "videoscreen.fakefullscreen";
const std::string CSettings::SETTING_VIDEOSCREEN_BLANKDISPLAYS = "videoscreen.blankdisplays";
const std::string CSettings::SETTING_VIDEOSCREEN_STEREOSCOPICMODE = "videoscreen.stereoscopicmode";
const std::string CSettings::SETTING_VIDEOSCREEN_PREFEREDSTEREOSCOPICMODE = "videoscreen.preferedstereoscopicmode";
const std::string CSettings::SETTING_VIDEOSCREEN_NOOFBUFFERS = "videoscreen.noofbuffers";
const std::string CSettings::SETTING_VIDEOSCREEN_3DLUT = "videoscreen.cms3dlut";
const std::string CSettings::SETTING_VIDEOSCREEN_DISPLAYPROFILE = "videoscreen.displayprofile";
const std::string CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION = "videoscreen.guicalibration";
const std::string CSettings::SETTING_VIDEOSCREEN_TESTPATTERN = "videoscreen.testpattern";
const std::string CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE = "videoscreen.limitedrange";
const std::string CSettings::SETTING_VIDEOSCREEN_FRAMEPACKING = "videoscreen.framepacking";
const std::string CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE = "audiooutput.audiodevice";
const std::string CSettings::SETTING_AUDIOOUTPUT_CHANNELS = "audiooutput.channels";
const std::string CSettings::SETTING_AUDIOOUTPUT_CONFIG = "audiooutput.config";
const std::string CSettings::SETTING_AUDIOOUTPUT_SAMPLERATE = "audiooutput.samplerate";
const std::string CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX = "audiooutput.stereoupmix";
const std::string CSettings::SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME = "audiooutput.maintainoriginalvolume";
const std::string CSettings::SETTING_AUDIOOUTPUT_PROCESSQUALITY = "audiooutput.processquality";
const std::string CSettings::SETTING_AUDIOOUTPUT_ATEMPOTHRESHOLD = "audiooutput.atempothreshold";
const std::string CSettings::SETTING_AUDIOOUTPUT_STREAMSILENCE = "audiooutput.streamsilence";
const std::string CSettings::SETTING_AUDIOOUTPUT_STREAMNOISE = "audiooutput.streamnoise";
const std::string CSettings::SETTING_AUDIOOUTPUT_DSPADDONSENABLED = "audiooutput.dspaddonsenabled";
const std::string CSettings::SETTING_AUDIOOUTPUT_DSPSETTINGS = "audiooutput.dspsettings";
const std::string CSettings::SETTING_AUDIOOUTPUT_DSPRESETDB = "audiooutput.dspresetdb";
const std::string CSettings::SETTING_AUDIOOUTPUT_GUISOUNDMODE = "audiooutput.guisoundmode";
const std::string CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH = "audiooutput.passthrough";
const std::string CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE = "audiooutput.passthroughdevice";
const std::string CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH = "audiooutput.ac3passthrough";
const std::string CSettings::SETTING_AUDIOOUTPUT_AC3TRANSCODE = "audiooutput.ac3transcode";
const std::string CSettings::SETTING_AUDIOOUTPUT_EAC3PASSTHROUGH = "audiooutput.eac3passthrough";
const std::string CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH = "audiooutput.dtspassthrough";
const std::string CSettings::SETTING_AUDIOOUTPUT_TRUEHDPASSTHROUGH = "audiooutput.truehdpassthrough";
const std::string CSettings::SETTING_AUDIOOUTPUT_DTSHDPASSTHROUGH = "audiooutput.dtshdpassthrough";
const std::string CSettings::SETTING_AUDIOOUTPUT_VOLUMESTEPS = "audiooutput.volumesteps";
const std::string CSettings::SETTING_INPUT_PERIPHERALS = "input.peripherals";
const std::string CSettings::SETTING_INPUT_PERIPHERALLIBRARIES = "input.peripherallibraries";
const std::string CSettings::SETTING_INPUT_ENABLEMOUSE = "input.enablemouse";
const std::string CSettings::SETTING_INPUT_ASKNEWCONTROLLERS = "input.asknewcontrollers";
const std::string CSettings::SETTING_INPUT_CONTROLLERCONFIG = "input.controllerconfig";
const std::string CSettings::SETTING_INPUT_RUMBLENOTIFY = "input.rumblenotify";
const std::string CSettings::SETTING_INPUT_TESTRUMBLE = "input.testrumble";
const std::string CSettings::SETTING_INPUT_CONTROLLERPOWEROFF = "input.controllerpoweroff";
const std::string CSettings::SETTING_INPUT_APPLEREMOTEMODE = "input.appleremotemode";
const std::string CSettings::SETTING_INPUT_APPLEREMOTEALWAYSON = "input.appleremotealwayson";
const std::string CSettings::SETTING_INPUT_APPLEREMOTESEQUENCETIME = "input.appleremotesequencetime";
const std::string CSettings::SETTING_NETWORK_USEHTTPPROXY = "network.usehttpproxy";
const std::string CSettings::SETTING_NETWORK_HTTPPROXYTYPE = "network.httpproxytype";
const std::string CSettings::SETTING_NETWORK_HTTPPROXYSERVER = "network.httpproxyserver";
const std::string CSettings::SETTING_NETWORK_HTTPPROXYPORT = "network.httpproxyport";
const std::string CSettings::SETTING_NETWORK_HTTPPROXYUSERNAME = "network.httpproxyusername";
const std::string CSettings::SETTING_NETWORK_HTTPPROXYPASSWORD = "network.httpproxypassword";
const std::string CSettings::SETTING_NETWORK_BANDWIDTH = "network.bandwidth";
const std::string CSettings::SETTING_POWERMANAGEMENT_DISPLAYSOFF = "powermanagement.displaysoff";
const std::string CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME = "powermanagement.shutdowntime";
const std::string CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE = "powermanagement.shutdownstate";
const std::string CSettings::SETTING_POWERMANAGEMENT_WAKEONACCESS = "powermanagement.wakeonaccess";
const std::string CSettings::SETTING_POWERMANAGEMENT_WAITFORNETWORK = "powermanagement.waitfornetwork";
const std::string CSettings::SETTING_DEBUG_SHOWLOGINFO = "debug.showloginfo";
const std::string CSettings::SETTING_DEBUG_EXTRALOGGING = "debug.extralogging";
const std::string CSettings::SETTING_DEBUG_SETEXTRALOGLEVEL = "debug.setextraloglevel";
const std::string CSettings::SETTING_DEBUG_SCREENSHOTPATH = "debug.screenshotpath";
const std::string CSettings::SETTING_EVENTLOG_ENABLED = "eventlog.enabled";
const std::string CSettings::SETTING_EVENTLOG_ENABLED_NOTIFICATIONS = "eventlog.enablednotifications";
const std::string CSettings::SETTING_EVENTLOG_SHOW = "eventlog.show";
const std::string CSettings::SETTING_MASTERLOCK_LOCKCODE = "masterlock.lockcode";
const std::string CSettings::SETTING_MASTERLOCK_STARTUPLOCK = "masterlock.startuplock";
const std::string CSettings::SETTING_MASTERLOCK_MAXRETRIES = "masterlock.maxretries";
const std::string CSettings::SETTING_CACHE_HARDDISK = "cache.harddisk";
const std::string CSettings::SETTING_CACHEVIDEO_DVDROM = "cachevideo.dvdrom";
const std::string CSettings::SETTING_CACHEVIDEO_LAN = "cachevideo.lan";
const std::string CSettings::SETTING_CACHEVIDEO_INTERNET = "cachevideo.internet";
const std::string CSettings::SETTING_CACHEAUDIO_DVDROM = "cacheaudio.dvdrom";
const std::string CSettings::SETTING_CACHEAUDIO_LAN = "cacheaudio.lan";
const std::string CSettings::SETTING_CACHEAUDIO_INTERNET = "cacheaudio.internet";
const std::string CSettings::SETTING_CACHEDVD_DVDROM = "cachedvd.dvdrom";
const std::string CSettings::SETTING_CACHEDVD_LAN = "cachedvd.lan";
const std::string CSettings::SETTING_CACHEUNKNOWN_INTERNET = "cacheunknown.internet";
const std::string CSettings::SETTING_SYSTEM_PLAYLISTSPATH = "system.playlistspath";
const std::string CSettings::SETTING_ADDONS_AUTOUPDATES = "general.addonupdates";
const std::string CSettings::SETTING_ADDONS_NOTIFICATIONS = "general.addonnotifications";
const std::string CSettings::SETTING_ADDONS_SHOW_RUNNING = "addons.showrunning";
const std::string CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES = "addons.unknownsources";
const std::string CSettings::SETTING_ADDONS_MANAGE_DEPENDENCIES = "addons.managedependencies";
const std::string CSettings::SETTING_GENERAL_ADDONFOREIGNFILTER = "general.addonforeignfilter";
const std::string CSettings::SETTING_GENERAL_ADDONBROKENFILTER = "general.addonbrokenfilter";
const std::string CSettings::SETTING_SOURCE_VIDEOS = "source.videos";
const std::string CSettings::SETTING_SOURCE_MUSIC = "source.music";
const std::string CSettings::SETTING_SOURCE_PICTURES = "source.pictures";
const std::string CSettings::SETTING_GAMES_KEYBOARD_PLAYERS = "gameskeyboard.keyboardplayers";
const std::string CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_1 = "gameskeyboard.keyboardplayerconfig1";
const std::string CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_2 = "gameskeyboard.keyboardplayerconfig2";
const std::string CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_3 = "gameskeyboard.keyboardplayerconfig3";
const std::string CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_4 = "gameskeyboard.keyboardplayerconfig4";
const std::string CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_5 = "gameskeyboard.keyboardplayerconfig5";
const std::string CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_6 = "gameskeyboard.keyboardplayerconfig6";
const std::string CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_7 = "gameskeyboard.keyboardplayerconfig7";
const std::string CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_8 = "gameskeyboard.keyboardplayerconfig8";
const std::string CSettings::SETTING_GAMES_ENABLE = "gamesgeneral.enable";
const std::string CSettings::SETTING_GAMES_ENABLEREWIND = "gamesgeneral.enablerewind";
const std::string CSettings::SETTING_GAMES_REWINDTIME = "gamesgeneral.rewindtime";

bool CSettings::Initialize()
{
  CSingleLock lock(m_critical);
  if (m_initialized)
    return false;

  // register custom setting types
  InitializeSettingTypes();
  // register custom setting controls
  InitializeControls();

  // option fillers and conditions need to be
  // initialized before the setting definitions
  InitializeOptionFillers();
  InitializeConditions();

  // load the settings definitions
  if (!InitializeDefinitions())
    return false;

  GetSettingsManager()->SetInitialized();

  InitializeISettingsHandlers();
  InitializeISubSettings();
  InitializeISettingCallbacks();

  m_initialized = true;

  return true;
}

bool CSettings::Load()
{
  return Load(CProfilesManager::GetInstance().GetSettingsFile());
}

bool CSettings::Load(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  bool updated = false;
  if (!XFILE::CFile::Exists(file) || !xmlDoc.LoadFile(file) ||
      !LoadValuesFromXml(xmlDoc, updated))
  {
    CLog::Log(LOGERROR, "CSettings: unable to load settings from %s, creating new default settings", file.c_str());
    if (!Reset())
      return false;

    if (!Load(file))
      return false;
  }
  // if the settings had to be updated, we need to save the changes
  else if (updated)
    return Save(file);

  return true;
}

bool CSettings::Save()
{
  return Save(CProfilesManager::GetInstance().GetSettingsFile());
}

bool CSettings::Save(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  if (!SaveValuesToXml(xmlDoc))
    return false;

  return xmlDoc.SaveFile(file);
}

bool CSettings::LoadSetting(const TiXmlNode *node, const std::string &settingId)
{
  return GetSettingsManager()->LoadSetting(node, settingId);
}

bool CSettings::GetBool(const std::string& id) const
{
  // Backward compatibility (skins use this setting)
  if (StringUtils::EqualsNoCase(id, "lookandfeel.enablemouse"))
    return CSettingsBase::GetBool(CSettings::SETTING_INPUT_ENABLEMOUSE);

  return CSettingsBase::GetBool(id);
}

bool CSettings::Initialize(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
  {
    CLog::Log(LOGERROR, "CSettings: error loading settings definition from %s, Line %d\n%s", file.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  CLog::Log(LOGDEBUG, "CSettings: loaded settings definition from %s", file.c_str());
  
  return InitializeDefinitionsFromXml(xmlDoc);
}

bool CSettings::InitializeDefinitions()
{
  if (!Initialize(SETTINGS_XML_FOLDER "settings.xml"))
  {
    CLog::Log(LOGFATAL, "Unable to load settings definitions");
    return false;
  }
#if defined(TARGET_WINDOWS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "win32.xml") && !Initialize(SETTINGS_XML_FOLDER "win32.xml"))
    CLog::Log(LOGFATAL, "Unable to load win32-specific settings definitions");
#elif defined(TARGET_ANDROID)
  if (CFile::Exists(SETTINGS_XML_FOLDER "android.xml") && !Initialize(SETTINGS_XML_FOLDER "android.xml"))
    CLog::Log(LOGFATAL, "Unable to load android-specific settings definitions");
#if defined(HAS_LIBAMCODEC)
  if (aml_present() && CFile::Exists(SETTINGS_XML_FOLDER "aml-android.xml") && !Initialize(SETTINGS_XML_FOLDER "aml-android.xml"))
    CLog::Log(LOGFATAL, "Unable to load aml-android-specific settings definitions");
#endif // defined(HAS_LIBAMCODEC)
#elif defined(TARGET_RASPBERRY_PI)
  if (CFile::Exists(SETTINGS_XML_FOLDER "rbp.xml") && !Initialize(SETTINGS_XML_FOLDER "rbp.xml"))
    CLog::Log(LOGFATAL, "Unable to load rbp-specific settings definitions");
  if (g_RBP.RaspberryPiVersion() > 1 && CFile::Exists(SETTINGS_XML_FOLDER "rbp2.xml") && !Initialize(SETTINGS_XML_FOLDER "rbp2.xml"))
    CLog::Log(LOGFATAL, "Unable to load rbp2-specific settings definitions");
#elif defined(TARGET_FREEBSD)
  if (CFile::Exists(SETTINGS_XML_FOLDER "freebsd.xml") && !Initialize(SETTINGS_XML_FOLDER "freebsd.xml"))
    CLog::Log(LOGFATAL, "Unable to load freebsd-specific settings definitions");
#elif defined(HAS_IMXVPU)
  if (CFile::Exists(SETTINGS_XML_FOLDER "imx6.xml") && !Initialize(SETTINGS_XML_FOLDER "imx6.xml"))
    CLog::Log(LOGFATAL, "Unable to load imx6-specific settings definitions");
#elif defined(TARGET_LINUX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "linux.xml") && !Initialize(SETTINGS_XML_FOLDER "linux.xml"))
    CLog::Log(LOGFATAL, "Unable to load linux-specific settings definitions");
#if defined(HAS_LIBAMCODEC)
  if (aml_present() && CFile::Exists(SETTINGS_XML_FOLDER "aml-linux.xml") && !Initialize(SETTINGS_XML_FOLDER "aml-linux.xml"))
    CLog::Log(LOGFATAL, "Unable to load aml-linux-specific settings definitions");
#endif // defined(HAS_LIBAMCODEC)
#elif defined(TARGET_DARWIN)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin.xml"))
    CLog::Log(LOGFATAL, "Unable to load darwin-specific settings definitions");
#if defined(TARGET_DARWIN_OSX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_osx.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_osx.xml"))
    CLog::Log(LOGFATAL, "Unable to load osx-specific settings definitions");
#elif defined(TARGET_DARWIN_IOS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_ios.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_ios.xml"))
    CLog::Log(LOGFATAL, "Unable to load ios-specific settings definitions");
#endif
#endif

  // load any custom visibility and default values before loading the special
  // appliance.xml so that appliances are able to overwrite even those values
  InitializeVisibility();
  InitializeDefaults();

  if (CFile::Exists(SETTINGS_XML_FOLDER "appliance.xml") && !Initialize(SETTINGS_XML_FOLDER "appliance.xml"))
    CLog::Log(LOGFATAL, "Unable to load appliance-specific settings definitions");

  return true;
}

void CSettings::InitializeSettingTypes()
{
  GetSettingsManager()->RegisterSettingType("addon", this);
  GetSettingsManager()->RegisterSettingType("date", this);
  GetSettingsManager()->RegisterSettingType("path", this);
  GetSettingsManager()->RegisterSettingType("time", this);
}

void CSettings::InitializeControls()
{
  GetSettingsManager()->RegisterSettingControl("toggle", this);
  GetSettingsManager()->RegisterSettingControl("spinner", this);
  GetSettingsManager()->RegisterSettingControl("edit", this);
  GetSettingsManager()->RegisterSettingControl("button", this);
  GetSettingsManager()->RegisterSettingControl("list", this);
  GetSettingsManager()->RegisterSettingControl("slider", this);
  GetSettingsManager()->RegisterSettingControl("range", this);
  GetSettingsManager()->RegisterSettingControl("title", this);
}

void CSettings::InitializeVisibility()
{
  // hide some settings if necessary
#if defined(TARGET_DARWIN)
  std::shared_ptr<CSettingString> timezonecountry = std::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_TIMEZONECOUNTRY));
  std::shared_ptr<CSettingString> timezone = std::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_TIMEZONE));

  if (CDarwinUtils::GetIOSVersion() >= 4.3)
  {
    timezonecountry->SetRequirementsMet(false);
    timezone->SetRequirementsMet(false);
  }
#endif
}

void CSettings::InitializeDefaults()
{
  // set some default values if necessary
#if defined(TARGET_POSIX)
  std::shared_ptr<CSettingString> timezonecountry = std::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_TIMEZONECOUNTRY));
  std::shared_ptr<CSettingString> timezone = std::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_TIMEZONE));

  if (timezonecountry->IsVisible())
    timezonecountry->SetDefault(g_timezone.GetCountryByTimezone(g_timezone.GetOSConfiguredTimezone()));
  if (timezone->IsVisible())
    timezone->SetDefault(g_timezone.GetOSConfiguredTimezone());
#endif // defined(TARGET_POSIX)

#if defined(TARGET_WINDOWS)
  #if defined(HAS_DX)
  std::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_MUSICPLAYER_VISUALISATION))->SetDefault("visualization.milkdrop");
  #endif

  #if !defined(HAS_GL)
  // We prefer a fake fullscreen mode (window covering the screen rather than dedicated fullscreen)
  // as it works nicer with switching to other applications. However on some systems vsync is broken
  // when we do this (eg non-Aero on ATI in particular) and on others (AppleTV) we can't get XBMC to
  // the front
  if (g_sysinfo.IsAeroDisabled())
    std::static_pointer_cast<CSettingBool>(GetSettingsManager()->GetSetting(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN))->SetDefault(false);
  #endif
#endif

#if !defined(TARGET_WINDOWS)
  std::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE))->SetDefault(CServiceBroker::GetActiveAE().GetDefaultDevice(false));
  std::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE))->SetDefault(CServiceBroker::GetActiveAE().GetDefaultDevice(true));
#endif

  if (g_application.IsStandAlone())
    std::static_pointer_cast<CSettingInt>(GetSettingsManager()->GetSetting(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE))->SetDefault(POWERSTATE_SHUTDOWN);

  g_powerManager.SetDefaults();
}

void CSettings::InitializeOptionFillers()
{
  // register setting option fillers
#ifdef HAS_DVD_DRIVE
  GetSettingsManager()->RegisterSettingOptionsFiller("audiocdactions", MEDIA_DETECT::CAutorun::SettingOptionAudioCdActionsFiller);
#endif
  GetSettingsManager()->RegisterSettingOptionsFiller("aequalitylevels", ActiveAE::CActiveAESettings::SettingOptionsAudioQualityLevelsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("audiodevices", ActiveAE::CActiveAESettings::SettingOptionsAudioDevicesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("audiodevicespassthrough", ActiveAE::CActiveAESettings::SettingOptionsAudioDevicesPassthroughFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("audiostreamsilence", ActiveAE::CActiveAESettings::SettingOptionsAudioStreamsilenceFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("charsets", CCharsetConverter::SettingOptionsCharsetsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("fonts", GUIFontManager::SettingOptionsFontsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("languagenames", CLangInfo::SettingOptionsLanguageNamesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("refreshchangedelays", CDisplaySettings::SettingOptionsRefreshChangeDelaysFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("refreshrates", CDisplaySettings::SettingOptionsRefreshRatesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("regions", CLangInfo::SettingOptionsRegionsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("shortdateformats", CLangInfo::SettingOptionsShortDateFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("longdateformats", CLangInfo::SettingOptionsLongDateFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("timeformats", CLangInfo::SettingOptionsTimeFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("24hourclockformats", CLangInfo::SettingOptions24HourClockFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("speedunits", CLangInfo::SettingOptionsSpeedUnitsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("temperatureunits", CLangInfo::SettingOptionsTemperatureUnitsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("rendermethods", CBaseRenderer::SettingOptionsRenderMethodsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("resolutions", CDisplaySettings::SettingOptionsResolutionsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("screens", CDisplaySettings::SettingOptionsScreensFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("stereoscopicmodes", CDisplaySettings::SettingOptionsStereoscopicModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("preferedstereoscopicviewmodes", CDisplaySettings::SettingOptionsPreferredStereoscopicViewModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("monitors", CDisplaySettings::SettingOptionsMonitorsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsmodes", CDisplaySettings::SettingOptionsCmsModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmswhitepoints", CDisplaySettings::SettingOptionsCmsWhitepointsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsprimaries", CDisplaySettings::SettingOptionsCmsPrimariesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsgammamodes", CDisplaySettings::SettingOptionsCmsGammaModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("videoseeksteps", CSeekHandler::SettingOptionsSeekStepsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("shutdownstates", CPowerManager::SettingOptionsShutdownStatesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("startupwindows", ADDON::CSkinInfo::SettingOptionsStartupWindowsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("audiostreamlanguages", CLangInfo::SettingOptionsAudioStreamLanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("subtitlestreamlanguages", CLangInfo::SettingOptionsSubtitleStreamLanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("subtitledownloadlanguages", CLangInfo::SettingOptionsSubtitleDownloadlanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("iso6391languages", CLangInfo::SettingOptionsISO6391LanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skincolors", ADDON::CSkinInfo::SettingOptionsSkinColorsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skinfonts", ADDON::CSkinInfo::SettingOptionsSkinFontsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skinthemes", ADDON::CSkinInfo::SettingOptionsSkinThemesFiller);
#ifdef TARGET_LINUX
  GetSettingsManager()->RegisterSettingOptionsFiller("timezonecountries", CLinuxTimezone::SettingOptionsTimezoneCountriesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("timezones", CLinuxTimezone::SettingOptionsTimezonesFiller);
#endif
  GetSettingsManager()->RegisterSettingOptionsFiller("keyboardlayouts", CKeyboardLayoutManager::SettingOptionsKeyboardLayoutsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("loggingcomponents", CAdvancedSettings::SettingOptionsLoggingComponentsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("pvrrecordmargins", PVR::CPVRSettings::MarginTimeFiller);
}

void CSettings::UninitializeOptionFillers()
{
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiocdactions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiocdencoders");
  GetSettingsManager()->UnregisterSettingOptionsFiller("aequalitylevels");
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiodevices");
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiodevicespassthrough");
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiostreamsilence");
  GetSettingsManager()->UnregisterSettingOptionsFiller("charsets");
  GetSettingsManager()->UnregisterSettingOptionsFiller("fontheights");
  GetSettingsManager()->UnregisterSettingOptionsFiller("fonts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("languagenames");
  GetSettingsManager()->UnregisterSettingOptionsFiller("refreshchangedelays");
  GetSettingsManager()->UnregisterSettingOptionsFiller("refreshrates");
  GetSettingsManager()->UnregisterSettingOptionsFiller("regions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("shortdateformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("longdateformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("timeformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("24hourclockformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("speedunits");
  GetSettingsManager()->UnregisterSettingOptionsFiller("temperatureunits");
  GetSettingsManager()->UnregisterSettingOptionsFiller("rendermethods");
  GetSettingsManager()->UnregisterSettingOptionsFiller("resolutions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("screens");
  GetSettingsManager()->UnregisterSettingOptionsFiller("stereoscopicmodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("preferedstereoscopicviewmodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("monitors");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmsmodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmswhitepoints");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmsprimaries");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmsgammamodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("videoseeksteps");
  GetSettingsManager()->UnregisterSettingOptionsFiller("shutdownstates");
  GetSettingsManager()->UnregisterSettingOptionsFiller("startupwindows");
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiostreamlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("subtitlestreamlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("subtitledownloadlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("iso6391languages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skincolors");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skinfonts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skinthemes");
#if defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterSettingOptionsFiller("timezonecountries");
  GetSettingsManager()->UnregisterSettingOptionsFiller("timezones");
#endif // defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterSettingOptionsFiller("verticalsyncs");
  GetSettingsManager()->UnregisterSettingOptionsFiller("keyboardlayouts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("pvrrecordmargins");
}

void CSettings::InitializeConditions()
{
  CSettingConditions::Initialize();

  // add basic conditions
  const std::set<std::string> &simpleConditions = CSettingConditions::GetSimpleConditions();
  for (std::set<std::string>::const_iterator itCondition = simpleConditions.begin(); itCondition != simpleConditions.end(); ++itCondition)
    GetSettingsManager()->AddCondition(*itCondition);

  // add more complex conditions
  const std::map<std::string, SettingConditionCheck> &complexConditions = CSettingConditions::GetComplexConditions();
  for (std::map<std::string, SettingConditionCheck>::const_iterator itCondition = complexConditions.begin(); itCondition != complexConditions.end(); ++itCondition)
    GetSettingsManager()->AddCondition(itCondition->first, itCondition->second);
}

void CSettings::InitializeISettingsHandlers()
{
  // register ISettingsHandler implementations
  // The order of these matters! Handlers are processed in the order they were registered.
  GetSettingsManager()->RegisterSettingsHandler(&g_advancedSettings);
  GetSettingsManager()->RegisterSettingsHandler(&CMediaSourceSettings::GetInstance());
  GetSettingsManager()->RegisterSettingsHandler(&CPlayerCoreFactory::GetInstance());
  GetSettingsManager()->RegisterSettingsHandler(&CProfilesManager::GetInstance());
#ifdef HAS_UPNP
  GetSettingsManager()->RegisterSettingsHandler(&CUPnPSettings::GetInstance());
#endif
  GetSettingsManager()->RegisterSettingsHandler(&CWakeOnAccess::GetInstance());
  GetSettingsManager()->RegisterSettingsHandler(&CRssManager::GetInstance());
  GetSettingsManager()->RegisterSettingsHandler(&g_langInfo);
  GetSettingsManager()->RegisterSettingsHandler(&g_application);
#if defined(TARGET_LINUX) && !defined(TARGET_ANDROID) && !defined(__UCLIBC__)
  GetSettingsManager()->RegisterSettingsHandler(&g_timezone);
#endif
  GetSettingsManager()->RegisterSettingsHandler(&CMediaSettings::GetInstance());
}

void CSettings::UninitializeISettingsHandlers()
{
  // unregister ISettingCallback implementations
  GetSettingsManager()->UnregisterCallback(&CEventLog::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_advancedSettings);
  GetSettingsManager()->UnregisterCallback(&CMediaSettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CDisplaySettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CSeekHandler::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CStereoscopicsManager::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_application);
  GetSettingsManager()->UnregisterCallback(&g_audioManager);
  GetSettingsManager()->UnregisterCallback(&g_charsetConverter);
  GetSettingsManager()->UnregisterCallback(&g_graphicsContext);
  GetSettingsManager()->UnregisterCallback(&g_langInfo);
  GetSettingsManager()->UnregisterCallback(&CInputManager::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CNetworkServices::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_passwordManager);
  GetSettingsManager()->UnregisterCallback(&CRssManager::GetInstance());
  GetSettingsManager()->UnregisterCallback(&ADDON::CRepositoryUpdater::GetInstance());
#if defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterCallback(&g_timezone);
#endif // defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterCallback(&g_weatherManager);
  GetSettingsManager()->UnregisterCallback(&CServiceBroker::GetPeripherals());
#if defined(TARGET_DARWIN_OSX)
  GetSettingsManager()->UnregisterCallback(&XBMCHelper::GetInstance());
#endif
  GetSettingsManager()->UnregisterCallback(&CWakeOnAccess::GetInstance());
}

void CSettings::InitializeISubSettings()
{
  // register ISubSettings implementations
  GetSettingsManager()->RegisterSubSettings(&g_application);
  GetSettingsManager()->RegisterSubSettings(&CDisplaySettings::GetInstance());
  GetSettingsManager()->RegisterSubSettings(&CMediaSettings::GetInstance());
  GetSettingsManager()->RegisterSubSettings(&CSkinSettings::GetInstance());
  GetSettingsManager()->RegisterSubSettings(&g_sysinfo);
  GetSettingsManager()->RegisterSubSettings(&CViewStateSettings::GetInstance());
}

void CSettings::UninitializeISubSettings()
{
  // unregister ISubSettings implementations
  GetSettingsManager()->UnregisterSubSettings(&g_application);
  GetSettingsManager()->UnregisterSubSettings(&CDisplaySettings::GetInstance());
  GetSettingsManager()->UnregisterSubSettings(&CMediaSettings::GetInstance());
  GetSettingsManager()->UnregisterSubSettings(&CSkinSettings::GetInstance());
  GetSettingsManager()->UnregisterSubSettings(&g_sysinfo);
  GetSettingsManager()->UnregisterSubSettings(&CViewStateSettings::GetInstance());
}

void CSettings::InitializeISettingCallbacks()
{
  // register any ISettingCallback implementations
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_EVENTLOG_SHOW);
  GetSettingsManager()->RegisterCallback(&CEventLog::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_DEBUG_SHOWLOGINFO);
  settingSet.insert(CSettings::SETTING_DEBUG_EXTRALOGGING);
  settingSet.insert(CSettings::SETTING_DEBUG_SETEXTRALOGLEVEL);
  GetSettingsManager()->RegisterCallback(&g_advancedSettings, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_CLEANUP);
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_EXPORT);
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_IMPORT);
  settingSet.insert(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_FLATTENTVSHOWS);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_GROUPMOVIESETS);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_CLEANUP);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_IMPORT);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_EXPORT);
  GetSettingsManager()->RegisterCallback(&CMediaSettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_SCREEN);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_RESOLUTION);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_SCREENMODE);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_MONITOR);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_PREFEREDSTEREOSCOPICMODE);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_3DLUT);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_DISPLAYPROFILE);
  GetSettingsManager()->RegisterCallback(&CDisplaySettings::GetInstance(), settingSet);
  
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_VIDEOPLAYER_SEEKDELAY);
  settingSet.insert(CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS);
  settingSet.insert(CSettings::SETTING_MUSICPLAYER_SEEKDELAY);
  settingSet.insert(CSettings::SETTING_MUSICPLAYER_SEEKSTEPS);
  GetSettingsManager()->RegisterCallback(&CSeekHandler::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_STEREOSCOPICMODE);
  GetSettingsManager()->RegisterCallback(&CStereoscopicsManager::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_CONFIG);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_SAMPLERATE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_CHANNELS);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_PROCESSQUALITY);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_ATEMPOTHRESHOLD);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_GUISOUNDMODE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_AC3TRANSCODE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_EAC3PASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_TRUEHDPASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_DTSHDPASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_STREAMSILENCE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_STREAMNOISE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_DSPADDONSENABLED);
  settingSet.insert(CSettings::SETTING_LOOKANDFEEL_SKIN);
  settingSet.insert(CSettings::SETTING_LOOKANDFEEL_SKINSETTINGS);
  settingSet.insert(CSettings::SETTING_LOOKANDFEEL_FONT);
  settingSet.insert(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
  settingSet.insert(CSettings::SETTING_LOOKANDFEEL_SKINCOLORS);
  settingSet.insert(CSettings::SETTING_LOOKANDFEEL_SKINZOOM);
  settingSet.insert(CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP);
  settingSet.insert(CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP);
  settingSet.insert(CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE);
  settingSet.insert(CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING);
  settingSet.insert(CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT);
  settingSet.insert(CSettings::SETTING_SCREENSAVER_MODE);
  settingSet.insert(CSettings::SETTING_SCREENSAVER_PREVIEW);
  settingSet.insert(CSettings::SETTING_SCREENSAVER_SETTINGS);
  settingSet.insert(CSettings::SETTING_AUDIOCDS_SETTINGS);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_TESTPATTERN);
  settingSet.insert(CSettings::SETTING_VIDEOPLAYER_USEAMCODEC);
  settingSet.insert(CSettings::SETTING_VIDEOPLAYER_USEMEDIACODEC);
  settingSet.insert(CSettings::SETTING_VIDEOPLAYER_USEMEDIACODECSURFACE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_VOLUMESTEPS);
  settingSet.insert(CSettings::SETTING_SOURCE_VIDEOS);
  settingSet.insert(CSettings::SETTING_SOURCE_MUSIC);
  settingSet.insert(CSettings::SETTING_SOURCE_PICTURES);
  GetSettingsManager()->RegisterCallback(&g_application, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN);
  GetSettingsManager()->RegisterCallback(&g_audioManager, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_SUBTITLES_CHARSET);
  settingSet.insert(CSettings::SETTING_LOCALE_CHARSET);
  GetSettingsManager()->RegisterCallback(&g_charsetConverter, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN);
  GetSettingsManager()->RegisterCallback(&g_graphicsContext, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_LOCALE_AUDIOLANGUAGE);
  settingSet.insert(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE);
  settingSet.insert(CSettings::SETTING_LOCALE_LANGUAGE);
  settingSet.insert(CSettings::SETTING_LOCALE_COUNTRY);
  settingSet.insert(CSettings::SETTING_LOCALE_SHORTDATEFORMAT);
  settingSet.insert(CSettings::SETTING_LOCALE_LONGDATEFORMAT);
  settingSet.insert(CSettings::SETTING_LOCALE_TIMEFORMAT);
  settingSet.insert(CSettings::SETTING_LOCALE_USE24HOURCLOCK);
  settingSet.insert(CSettings::SETTING_LOCALE_TEMPERATUREUNIT);
  settingSet.insert(CSettings::SETTING_LOCALE_SPEEDUNIT);
  GetSettingsManager()->RegisterCallback(&g_langInfo, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_INPUT_ENABLEMOUSE);
  GetSettingsManager()->RegisterCallback(&CInputManager::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_SERVICES_WEBSERVER);
  settingSet.insert(CSettings::SETTING_SERVICES_WEBSERVERPORT);
  settingSet.insert(CSettings::SETTING_SERVICES_WEBSERVERUSERNAME);
  settingSet.insert(CSettings::SETTING_SERVICES_WEBSERVERPASSWORD);
  settingSet.insert(CSettings::SETTING_SERVICES_ZEROCONF);
  settingSet.insert(CSettings::SETTING_SERVICES_AIRPLAY);
  settingSet.insert(CSettings::SETTING_SERVICES_AIRPLAYVOLUMECONTROL);
  settingSet.insert(CSettings::SETTING_SERVICES_AIRPLAYVIDEOSUPPORT);
  settingSet.insert(CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD);
  settingSet.insert(CSettings::SETTING_SERVICES_AIRPLAYPASSWORD);
  settingSet.insert(CSettings::SETTING_SERVICES_UPNP);
  settingSet.insert(CSettings::SETTING_SERVICES_UPNPSERVER);
  settingSet.insert(CSettings::SETTING_SERVICES_UPNPRENDERER);
  settingSet.insert(CSettings::SETTING_SERVICES_UPNPCONTROLLER);
  settingSet.insert(CSettings::SETTING_SERVICES_ESENABLED);
  settingSet.insert(CSettings::SETTING_SERVICES_ESPORT);
  settingSet.insert(CSettings::SETTING_SERVICES_ESALLINTERFACES);
  settingSet.insert(CSettings::SETTING_SERVICES_ESINITIALDELAY);
  settingSet.insert(CSettings::SETTING_SERVICES_ESCONTINUOUSDELAY);
  settingSet.insert(CSettings::SETTING_SMB_WINSSERVER);
  settingSet.insert(CSettings::SETTING_SMB_WORKGROUP);
  GetSettingsManager()->RegisterCallback(&CNetworkServices::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_MASTERLOCK_LOCKCODE);
  GetSettingsManager()->RegisterCallback(&g_passwordManager, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_LOOKANDFEEL_RSSEDIT);
  GetSettingsManager()->RegisterCallback(&CRssManager::GetInstance(), settingSet);

#if defined(TARGET_LINUX)
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_LOCALE_TIMEZONE);
  settingSet.insert(CSettings::SETTING_LOCALE_TIMEZONECOUNTRY);
  GetSettingsManager()->RegisterCallback(&g_timezone, settingSet);
#endif

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_WEATHER_ADDON);
  settingSet.insert(CSettings::SETTING_WEATHER_ADDONSETTINGS);
  GetSettingsManager()->RegisterCallback(&g_weatherManager, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_INPUT_PERIPHERALS);
  settingSet.insert(CSettings::SETTING_INPUT_PERIPHERALLIBRARIES);
  settingSet.insert(CSettings::SETTING_INPUT_CONTROLLERCONFIG);
  settingSet.insert(CSettings::SETTING_INPUT_TESTRUMBLE);
  settingSet.insert(CSettings::SETTING_LOCALE_LANGUAGE);
  GetSettingsManager()->RegisterCallback(&CServiceBroker::GetPeripherals(), settingSet);

#if defined(TARGET_DARWIN_OSX)
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_INPUT_APPLEREMOTEMODE);
  settingSet.insert(CSettings::SETTING_INPUT_APPLEREMOTEALWAYSON);
  GetSettingsManager()->RegisterCallback(&XBMCHelper::GetInstance(), settingSet);
#endif

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_ADDONS_AUTOUPDATES);
  GetSettingsManager()->RegisterCallback(&ADDON::CRepositoryUpdater::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_ADDONS_SHOW_RUNNING);
  settingSet.insert(CSettings::SETTING_ADDONS_MANAGE_DEPENDENCIES);
  settingSet.insert(CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES);
  GetSettingsManager()->RegisterCallback(&ADDON::CAddonSystemSettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_POWERMANAGEMENT_WAKEONACCESS);
  GetSettingsManager()->RegisterCallback(&CWakeOnAccess::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_GAMES_KEYBOARD_PLAYERS);
  settingSet.insert(CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_1);
  settingSet.insert(CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_2);
  settingSet.insert(CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_3);
  settingSet.insert(CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_4);
  settingSet.insert(CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_5);
  settingSet.insert(CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_6);
  settingSet.insert(CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_7);
  settingSet.insert(CSettings::SETTING_GAMES_KEYBOARD_PLAYERCONFIG_8);
  settingSet.insert(CSettings::SETTING_GAMES_ENABLEREWIND);
  settingSet.insert(CSettings::SETTING_GAMES_REWINDTIME);
  GetSettingsManager()->RegisterCallback(&GAME::CGameSettings::GetInstance(), settingSet);

#ifdef HAVE_LIBBLURAY
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_DISC_PLAYBACK);
  GetSettingsManager()->RegisterCallback(&CDiscSettings::GetInstance(), settingSet);
#endif
}

void CSettings::UninitializeISettingCallbacks()
{
  GetSettingsManager()->UnregisterCallback(&CEventLog::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_advancedSettings);
  GetSettingsManager()->UnregisterCallback(&CMediaSettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CDisplaySettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CSeekHandler::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CStereoscopicsManager::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_application);
  GetSettingsManager()->UnregisterCallback(&g_audioManager);
  GetSettingsManager()->UnregisterCallback(&g_charsetConverter);
  GetSettingsManager()->UnregisterCallback(&g_graphicsContext);
  GetSettingsManager()->UnregisterCallback(&g_langInfo);
  GetSettingsManager()->UnregisterCallback(&CInputManager::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CNetworkServices::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_passwordManager);
  GetSettingsManager()->UnregisterCallback(&CRssManager::GetInstance());
  GetSettingsManager()->UnregisterCallback(&ADDON::CRepositoryUpdater::GetInstance());
  GetSettingsManager()->UnregisterCallback(&GAME::CGameSettings::GetInstance());
#if defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterCallback(&g_timezone);
#endif // defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterCallback(&g_weatherManager);
GetSettingsManager()->UnregisterCallback(&CServiceBroker::GetPeripherals());
#if defined(TARGET_DARWIN_OSX)
  GetSettingsManager()->UnregisterCallback(&XBMCHelper::GetInstance());
#endif
  GetSettingsManager()->UnregisterCallback(&CWakeOnAccess::GetInstance());
#ifdef HAVE_LIBBLURAY
  GetSettingsManager()->UnregisterCallback(&CDiscSettings::GetInstance());
#endif
}

bool CSettings::Reset()
{
  std::string settingsFile = CProfilesManager::GetInstance().GetSettingsFile();
  // try to delete the settings file
  if (XFILE::CFile::Exists(settingsFile, false) && !XFILE::CFile::Delete(settingsFile))
    CLog::Log(LOGWARNING, "Unable to delete old settings file at %s", settingsFile.c_str());
  
  // unload any loaded settings
  Unload();

  // try to save the default settings
  if (!Save())
  {
    CLog::Log(LOGWARNING, "Failed to save the default settings to %s", settingsFile.c_str());
    return false;
  }

  return true;
}
