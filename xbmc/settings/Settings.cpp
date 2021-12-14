/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Settings.h"
#include "Application.h"
#include "Autorun.h"
#include "LangInfo.h"
#include "Util.h"
#include "addons/AddonSystemSettings.h"
#include "addons/Skin.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "filesystem/File.h"
#include "guilib/GUIFontManager.h"
#include "guilib/StereoscopicsManager.h"
#include "GUIPassword.h"
#include "input/KeyboardLayoutManager.h"
#if defined(TARGET_POSIX)
#include "platform/posix/PosixTimezone.h"
#endif // defined(TARGET_POSIX)
#include "network/upnp/UPnPSettings.h"
#include "network/WakeOnAccess.h"
#if defined(TARGET_DARWIN_OSX)
#include "platform/darwin/osx/XBMCHelper.h"
#endif // defined(TARGET_DARWIN_OSX)
#if defined(TARGET_DARWIN_TVOS)
#include "platform/darwin/tvos/TVOSSettingsHandler.h"
#endif // defined(TARGET_DARWIN_TVOS)
#if defined(TARGET_DARWIN_EMBEDDED)
#include "SettingAddon.h"
#endif
#include "DiscSettings.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"
#include "powermanagement/PowerTypes.h"
#include "profiles/ProfileManager.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingConditions.h"
#include "settings/SettingsComponent.h"
#include "settings/SkinSettings.h"
#include "settings/SubtitlesSettings.h"
#include "settings/lib/SettingsManager.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "utils/RssManager.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"
#include "view/ViewStateSettings.h"

#define SETTINGS_XML_FOLDER "special://xbmc/system/settings/"

using namespace KODI;
using namespace XFILE;

//! @todo: remove in c++17
constexpr const char* CSettings::SETTING_LOOKANDFEEL_SKIN;
constexpr const char* CSettings::SETTING_LOOKANDFEEL_SKINSETTINGS;
constexpr const char* CSettings::SETTING_LOOKANDFEEL_SKINTHEME;
constexpr const char* CSettings::SETTING_LOOKANDFEEL_SKINCOLORS;
constexpr const char* CSettings::SETTING_LOOKANDFEEL_FONT;
constexpr const char* CSettings::SETTING_LOOKANDFEEL_SKINZOOM;
constexpr const char* CSettings::SETTING_LOOKANDFEEL_STARTUPACTION;
constexpr const char* CSettings::SETTING_LOOKANDFEEL_STARTUPWINDOW;
constexpr const char* CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN;
constexpr const char* CSettings::SETTING_LOOKANDFEEL_ENABLERSSFEEDS;
constexpr const char* CSettings::SETTING_LOOKANDFEEL_RSSEDIT;
constexpr const char* CSettings::SETTING_LOOKANDFEEL_STEREOSTRENGTH;
constexpr const char* CSettings::SETTING_LOCALE_LANGUAGE;
constexpr const char* CSettings::SETTING_LOCALE_COUNTRY;
constexpr const char* CSettings::SETTING_LOCALE_CHARSET;
constexpr const char* CSettings::SETTING_LOCALE_KEYBOARDLAYOUTS;
constexpr const char* CSettings::SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT;
constexpr const char* CSettings::SETTING_LOCALE_TIMEZONECOUNTRY;
constexpr const char* CSettings::SETTING_LOCALE_TIMEZONE;
constexpr const char* CSettings::SETTING_LOCALE_SHORTDATEFORMAT;
constexpr const char* CSettings::SETTING_LOCALE_LONGDATEFORMAT;
constexpr const char* CSettings::SETTING_LOCALE_TIMEFORMAT;
constexpr const char* CSettings::SETTING_LOCALE_USE24HOURCLOCK;
constexpr const char* CSettings::SETTING_LOCALE_TEMPERATUREUNIT;
constexpr const char* CSettings::SETTING_LOCALE_SPEEDUNIT;
constexpr const char* CSettings::SETTING_FILELISTS_SHOWPARENTDIRITEMS;
constexpr const char* CSettings::SETTING_FILELISTS_SHOWEXTENSIONS;
constexpr const char* CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING;
constexpr const char* CSettings::SETTING_FILELISTS_ALLOWFILEDELETION;
constexpr const char* CSettings::SETTING_FILELISTS_SHOWADDSOURCEBUTTONS;
constexpr const char* CSettings::SETTING_FILELISTS_SHOWHIDDEN;
constexpr const char* CSettings::SETTING_SCREENSAVER_MODE;
constexpr const char* CSettings::SETTING_SCREENSAVER_SETTINGS;
constexpr const char* CSettings::SETTING_SCREENSAVER_PREVIEW;
constexpr const char* CSettings::SETTING_SCREENSAVER_TIME;
constexpr const char* CSettings::SETTING_SCREENSAVER_USEMUSICVISINSTEAD;
constexpr const char* CSettings::SETTING_SCREENSAVER_USEDIMONPAUSE;
constexpr const char* CSettings::SETTING_WINDOW_WIDTH;
constexpr const char* CSettings::SETTING_WINDOW_HEIGHT;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_ACTORTHUMBS;
constexpr const char* CSettings::SETTING_MYVIDEOS_FLATTEN;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_FLATTENTVSHOWS;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_TVSHOWSSELECTFIRSTUNWATCHEDITEM;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_TVSHOWSINCLUDEALLSEASONSANDSPECIALS;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_SHOWALLITEMS;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_GROUPMOVIESETS;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_GROUPSINGLEITEMSETS;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_UPDATEONSTARTUP;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_BACKGROUNDUPDATE;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_CLEANUP;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_EXPORT;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_IMPORT;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_SHOWEMPTYTVSHOWS;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_MOVIEART_WHITELIST;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_TVSHOWART_WHITELIST;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_EPISODEART_WHITELIST;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_MUSICVIDEOART_WHITELIST;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_ARTSETTINGS_UPDATED;
constexpr const char* CSettings::SETTING_VIDEOLIBRARY_SHOWPERFORMERS;
constexpr const char* CSettings::SETTING_LOCALE_AUDIOLANGUAGE;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_PREFERDEFAULTFLAG;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_AUTOPLAYNEXTITEM;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_SEEKDELAY;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_ERRORINASPECT;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_STRETCH43;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_TELETEXTENABLED;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_TELETEXTSCALE;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_QUITSTEREOMODEONSTOP;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_HQSCALERS;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USEMEDIACODEC;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USEMEDIACODECSURFACE;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USEVDPAU;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USEVDPAUMIXER;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USEVDPAUMPEG2;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USEVDPAUMPEG4;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USEVDPAUVC1;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USEDXVA2;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USEVTB;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USEPRIMEDECODER;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_USESTAGEFRIGHT;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_LIMITGUIUPDATE;
constexpr const char* CSettings::SETTING_VIDEOPLAYER_SUPPORTMVC;
constexpr const char* CSettings::SETTING_MYVIDEOS_SELECTACTION;
constexpr const char* CSettings::SETTING_MYVIDEOS_USETAGS;
constexpr const char* CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS;
constexpr const char* CSettings::SETTING_MYVIDEOS_EXTRACTCHAPTERTHUMBS;
constexpr const char* CSettings::SETTING_MYVIDEOS_REPLACELABELS;
constexpr const char* CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB;
constexpr const char* CSettings::SETTING_MYVIDEOS_STACKVIDEOS;
constexpr const char* CSettings::SETTING_LOCALE_SUBTITLELANGUAGE;
constexpr const char* CSettings::SETTING_SUBTITLES_PARSECAPTIONS;
constexpr const char* CSettings::SETTING_SUBTITLES_CAPTIONSALIGN;
constexpr const char* CSettings::SETTING_SUBTITLES_ALIGN;
constexpr const char* CSettings::SETTING_SUBTITLES_STEREOSCOPICDEPTH;
constexpr const char* CSettings::SETTING_SUBTITLES_FONT;
constexpr const char* CSettings::SETTING_SUBTITLES_FONTSIZE;
constexpr const char* CSettings::SETTING_SUBTITLES_STYLE;
constexpr const char* CSettings::SETTING_SUBTITLES_COLOR;
constexpr const char* CSettings::SETTING_SUBTITLES_BORDERSIZE;
constexpr const char* CSettings::SETTING_SUBTITLES_BORDERCOLOR;
constexpr const char* CSettings::SETTING_SUBTITLES_BGCOLOR;
constexpr const char* CSettings::SETTING_SUBTITLES_BGOPACITY;
constexpr const char* CSettings::SETTING_SUBTITLES_BLUR;
constexpr const char* CSettings::SETTING_SUBTITLES_BACKGROUNDTYPE;
constexpr const char* CSettings::SETTING_SUBTITLES_SHADOWCOLOR;
constexpr const char* CSettings::SETTING_SUBTITLES_SHADOWOPACITY;
constexpr const char* CSettings::SETTING_SUBTITLES_SHADOWSIZE;
constexpr const char* CSettings::SETTING_SUBTITLES_CHARSET;
constexpr const char* CSettings::SETTING_SUBTITLES_OVERRIDEFONTS;
constexpr const char* CSettings::SETTING_SUBTITLES_OVERRIDESTYLES;
constexpr const char* CSettings::SETTING_SUBTITLES_LANGUAGES;
constexpr const char* CSettings::SETTING_SUBTITLES_STORAGEMODE;
constexpr const char* CSettings::SETTING_SUBTITLES_CUSTOMPATH;
constexpr const char* CSettings::SETTING_SUBTITLES_PAUSEONSEARCH;
constexpr const char* CSettings::SETTING_SUBTITLES_DOWNLOADFIRST;
constexpr const char* CSettings::SETTING_SUBTITLES_TV;
constexpr const char* CSettings::SETTING_SUBTITLES_MOVIE;
constexpr const char* CSettings::SETTING_DVDS_AUTORUN;
constexpr const char* CSettings::SETTING_DVDS_PLAYERREGION;
constexpr const char* CSettings::SETTING_DVDS_AUTOMENU;
constexpr const char* CSettings::SETTING_DISC_PLAYBACK;
constexpr const char* CSettings::SETTING_BLURAY_PLAYERREGION;
constexpr const char* CSettings::SETTING_ACCESSIBILITY_AUDIOVISUAL;
constexpr const char* CSettings::SETTING_ACCESSIBILITY_AUDIOHEARING;
constexpr const char* CSettings::SETTING_ACCESSIBILITY_SUBHEARING;
constexpr const char* CSettings::SETTING_SCRAPERS_MOVIESDEFAULT;
constexpr const char* CSettings::SETTING_SCRAPERS_TVSHOWSDEFAULT;
constexpr const char* CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT;
constexpr const char* CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL;
constexpr const char* CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS;
constexpr const char* CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER;
constexpr const char* CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS;
constexpr const char* CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERSALWAYS;
constexpr const char* CSettings::SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE;
constexpr const char* CSettings::SETTING_PVRMANAGER_CLIENTPRIORITIES;
constexpr const char* CSettings::SETTING_PVRMANAGER_CHANNELMANAGER;
constexpr const char* CSettings::SETTING_PVRMANAGER_GROUPMANAGER;
constexpr const char* CSettings::SETTING_PVRMANAGER_CHANNELSCAN;
constexpr const char* CSettings::SETTING_PVRMANAGER_RESETDB;
constexpr const char* CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO;
constexpr const char* CSettings::SETTING_PVRMENU_CLOSECHANNELOSDONSWITCH;
constexpr const char* CSettings::SETTING_PVRMENU_ICONPATH;
constexpr const char* CSettings::SETTING_PVRMENU_SEARCHICONS;
constexpr const char* CSettings::SETTING_EPG_PAST_DAYSTODISPLAY;
constexpr const char* CSettings::SETTING_EPG_FUTURE_DAYSTODISPLAY;
constexpr const char* CSettings::SETTING_EPG_SELECTACTION;
constexpr const char* CSettings::SETTING_EPG_HIDENOINFOAVAILABLE;
constexpr const char* CSettings::SETTING_EPG_EPGUPDATE;
constexpr const char* CSettings::SETTING_EPG_PREVENTUPDATESWHILEPLAYINGTV;
constexpr const char* CSettings::SETTING_EPG_RESETEPG;
constexpr const char* CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREENCHANNELTYPES;
constexpr const char* CSettings::SETTING_PVRPLAYBACK_SIGNALQUALITY;
constexpr const char* CSettings::SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH;
constexpr const char* CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT;
constexpr const char* CSettings::SETTING_PVRPLAYBACK_DELAYMARKLASTWATCHED;
constexpr const char* CSettings::SETTING_PVRPLAYBACK_FPS;
constexpr const char* CSettings::SETTING_PVRRECORD_INSTANTRECORDACTION;
constexpr const char* CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME;
constexpr const char* CSettings::SETTING_PVRRECORD_MARGINSTART;
constexpr const char* CSettings::SETTING_PVRRECORD_MARGINEND;
constexpr const char* CSettings::SETTING_PVRRECORD_TIMERNOTIFICATIONS;
constexpr const char* CSettings::SETTING_PVRRECORD_GROUPRECORDINGS;
constexpr const char* CSettings::SETTING_PVRREMINDERS_AUTOCLOSEDELAY;
constexpr const char* CSettings::SETTING_PVRREMINDERS_AUTORECORD;
constexpr const char* CSettings::SETTING_PVRREMINDERS_AUTOSWITCH;
constexpr const char* CSettings::SETTING_PVRPOWERMANAGEMENT_ENABLED;
constexpr const char* CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME;
constexpr const char* CSettings::SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD;
constexpr const char* CSettings::SETTING_PVRPOWERMANAGEMENT_PREWAKEUP;
constexpr const char* CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUP;
constexpr const char* CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME;
constexpr const char* CSettings::SETTING_PVRPARENTAL_ENABLED;
constexpr const char* CSettings::SETTING_PVRPARENTAL_PIN;
constexpr const char* CSettings::SETTING_PVRPARENTAL_DURATION;
constexpr const char* CSettings::SETTING_PVRCLIENT_MENUHOOK;
constexpr const char* CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_SHOWDISCS;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_USEARTISTSORTNAME;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_DOWNLOADINFO;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_PREFERONLINEALBUMART;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_ARTWORKLEVEL;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_USEALLLOCALART;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_USEALLREMOTEART;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_ARTISTART_WHITELIST;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_ALBUMART_WHITELIST;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_MUSICTHUMBS;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_ARTSETTINGS_UPDATED;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_OVERRIDETAGS;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_SHOWALLITEMS;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_UPDATEONSTARTUP;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_BACKGROUNDUPDATE;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_CLEANUP;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_EXPORT;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_FILETYPE;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_ITEMS;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_UNSCRAPED;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_OVERWRITE;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_ARTWORK;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_SKIPNFO;
constexpr const char* CSettings::SETTING_MUSICLIBRARY_IMPORT;
constexpr const char* CSettings::SETTING_MUSICPLAYER_AUTOPLAYNEXTITEM;
constexpr const char* CSettings::SETTING_MUSICPLAYER_QUEUEBYDEFAULT;
constexpr const char* CSettings::SETTING_MUSICPLAYER_SEEKSTEPS;
constexpr const char* CSettings::SETTING_MUSICPLAYER_SEEKDELAY;
constexpr const char* CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE;
constexpr const char* CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP;
constexpr const char* CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP;
constexpr const char* CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING;
constexpr const char* CSettings::SETTING_MUSICPLAYER_CROSSFADE;
constexpr const char* CSettings::SETTING_MUSICPLAYER_CROSSFADEALBUMTRACKS;
constexpr const char* CSettings::SETTING_MUSICPLAYER_VISUALISATION;
constexpr const char* CSettings::SETTING_MUSICFILES_SELECTACTION;
constexpr const char* CSettings::SETTING_MUSICFILES_USETAGS;
constexpr const char* CSettings::SETTING_MUSICFILES_TRACKFORMAT;
constexpr const char* CSettings::SETTING_MUSICFILES_NOWPLAYINGTRACKFORMAT;
constexpr const char* CSettings::SETTING_MUSICFILES_LIBRARYTRACKFORMAT;
constexpr const char* CSettings::SETTING_MUSICFILES_FINDREMOTETHUMBS;
constexpr const char* CSettings::SETTING_AUDIOCDS_AUTOACTION;
constexpr const char* CSettings::SETTING_AUDIOCDS_USECDDB;
constexpr const char* CSettings::SETTING_AUDIOCDS_RECORDINGPATH;
constexpr const char* CSettings::SETTING_AUDIOCDS_TRACKPATHFORMAT;
constexpr const char* CSettings::SETTING_AUDIOCDS_ENCODER;
constexpr const char* CSettings::SETTING_AUDIOCDS_SETTINGS;
constexpr const char* CSettings::SETTING_AUDIOCDS_EJECTONRIP;
constexpr const char* CSettings::SETTING_MYMUSIC_SONGTHUMBINVIS;
constexpr const char* CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW;
constexpr const char* CSettings::SETTING_PICTURES_USETAGS;
constexpr const char* CSettings::SETTING_PICTURES_GENERATETHUMBS;
constexpr const char* CSettings::SETTING_PICTURES_SHOWVIDEOS;
constexpr const char* CSettings::SETTING_PICTURES_DISPLAYRESOLUTION;
constexpr const char* CSettings::SETTING_SLIDESHOW_STAYTIME;
constexpr const char* CSettings::SETTING_SLIDESHOW_DISPLAYEFFECTS;
constexpr const char* CSettings::SETTING_SLIDESHOW_SHUFFLE;
constexpr const char* CSettings::SETTING_SLIDESHOW_HIGHQUALITYDOWNSCALING;
constexpr const char* CSettings::SETTING_WEATHER_CURRENTLOCATION;
constexpr const char* CSettings::SETTING_WEATHER_ADDON;
constexpr const char* CSettings::SETTING_WEATHER_ADDONSETTINGS;
constexpr const char* CSettings::SETTING_SERVICES_DEVICENAME;
constexpr const char* CSettings::SETTING_SERVICES_DEVICEUUID;
constexpr const char* CSettings::SETTING_SERVICES_UPNP;
constexpr const char* CSettings::SETTING_SERVICES_UPNPSERVER;
constexpr const char* CSettings::SETTING_SERVICES_UPNPANNOUNCE;
constexpr const char* CSettings::SETTING_SERVICES_UPNPLOOKFOREXTERNALSUBTITLES;
constexpr const char* CSettings::SETTING_SERVICES_UPNPCONTROLLER;
constexpr const char* CSettings::SETTING_SERVICES_UPNPRENDERER;
constexpr const char* CSettings::SETTING_SERVICES_WEBSERVER;
constexpr const char* CSettings::SETTING_SERVICES_WEBSERVERPORT;
constexpr const char* CSettings::SETTING_SERVICES_WEBSERVERAUTHENTICATION;
constexpr const char* CSettings::SETTING_SERVICES_WEBSERVERUSERNAME;
constexpr const char* CSettings::SETTING_SERVICES_WEBSERVERPASSWORD;
constexpr const char* CSettings::SETTING_SERVICES_WEBSERVERSSL;
constexpr const char* CSettings::SETTING_SERVICES_WEBSKIN;
constexpr const char* CSettings::SETTING_SERVICES_ESENABLED;
constexpr const char* CSettings::SETTING_SERVICES_ESPORT;
constexpr const char* CSettings::SETTING_SERVICES_ESPORTRANGE;
constexpr const char* CSettings::SETTING_SERVICES_ESMAXCLIENTS;
constexpr const char* CSettings::SETTING_SERVICES_ESALLINTERFACES;
constexpr const char* CSettings::SETTING_SERVICES_ESINITIALDELAY;
constexpr const char* CSettings::SETTING_SERVICES_ESCONTINUOUSDELAY;
constexpr const char* CSettings::SETTING_SERVICES_ZEROCONF;
constexpr const char* CSettings::SETTING_SERVICES_AIRPLAY;
constexpr const char* CSettings::SETTING_SERVICES_AIRPLAYVOLUMECONTROL;
constexpr const char* CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD;
constexpr const char* CSettings::SETTING_SERVICES_AIRPLAYPASSWORD;
constexpr const char* CSettings::SETTING_SERVICES_AIRPLAYVIDEOSUPPORT;
constexpr const char* CSettings::SETTING_SMB_WINSSERVER;
constexpr const char* CSettings::SETTING_SMB_WORKGROUP;
constexpr const char* CSettings::SETTING_SMB_MINPROTOCOL;
constexpr const char* CSettings::SETTING_SMB_MAXPROTOCOL;
constexpr const char* CSettings::SETTING_SMB_LEGACYSECURITY;
constexpr const char* CSettings::SETTING_SERVICES_WSDISCOVERY;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_MONITOR;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_SCREEN;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_WHITELIST;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_RESOLUTION;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_SCREENMODE;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_BLANKDISPLAYS;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_STEREOSCOPICMODE;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_PREFEREDSTEREOSCOPICMODE;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_NOOFBUFFERS;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_3DLUT;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_DISPLAYPROFILE;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_TESTPATTERN;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_FRAMEPACKING;
constexpr const char* CSettings::SETTING_VIDEOSCREEN_10BITSURFACES;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_CHANNELS;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_CONFIG;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_SAMPLERATE;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_PROCESSQUALITY;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_ATEMPOTHRESHOLD;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_STREAMSILENCE;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_STREAMNOISE;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_GUISOUNDMODE;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_AC3TRANSCODE;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_EAC3PASSTHROUGH;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_TRUEHDPASSTHROUGH;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_DTSHDPASSTHROUGH;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_DTSHDCOREFALLBACK;
constexpr const char* CSettings::SETTING_AUDIOOUTPUT_VOLUMESTEPS;
constexpr const char* CSettings::SETTING_INPUT_PERIPHERALS;
constexpr const char* CSettings::SETTING_INPUT_PERIPHERALLIBRARIES;
constexpr const char* CSettings::SETTING_INPUT_ENABLEMOUSE;
constexpr const char* CSettings::SETTING_INPUT_ASKNEWCONTROLLERS;
constexpr const char* CSettings::SETTING_INPUT_CONTROLLERCONFIG;
constexpr const char* CSettings::SETTING_INPUT_RUMBLENOTIFY;
constexpr const char* CSettings::SETTING_INPUT_TESTRUMBLE;
constexpr const char* CSettings::SETTING_INPUT_CONTROLLERPOWEROFF;
constexpr const char* CSettings::SETTING_INPUT_APPLEREMOTEMODE;
constexpr const char* CSettings::SETTING_INPUT_APPLEREMOTEALWAYSON;
constexpr const char* CSettings::SETTING_INPUT_APPLEREMOTESEQUENCETIME;
constexpr const char* CSettings::SETTING_INPUT_SIRIREMOTEIDLETIMERENABLED;
constexpr const char* CSettings::SETTING_INPUT_SIRIREMOTEIDLETIME;
constexpr const char* CSettings::SETTING_INPUT_SIRIREMOTEHORIZONTALSENSITIVITY;
constexpr const char* CSettings::SETTING_INPUT_SIRIREMOTEVERTICALSENSITIVITY;
constexpr const char* CSettings::SETTING_INPUT_TVOSUSEKODIKEYBOARD;
constexpr const char* CSettings::SETTING_NETWORK_USEHTTPPROXY;
constexpr const char* CSettings::SETTING_NETWORK_HTTPPROXYTYPE;
constexpr const char* CSettings::SETTING_NETWORK_HTTPPROXYSERVER;
constexpr const char* CSettings::SETTING_NETWORK_HTTPPROXYPORT;
constexpr const char* CSettings::SETTING_NETWORK_HTTPPROXYUSERNAME;
constexpr const char* CSettings::SETTING_NETWORK_HTTPPROXYPASSWORD;
constexpr const char* CSettings::SETTING_NETWORK_BANDWIDTH;
constexpr const char* CSettings::SETTING_POWERMANAGEMENT_DISPLAYSOFF;
constexpr const char* CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME;
constexpr const char* CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE;
constexpr const char* CSettings::SETTING_POWERMANAGEMENT_WAKEONACCESS;
constexpr const char* CSettings::SETTING_POWERMANAGEMENT_WAITFORNETWORK;
constexpr const char* CSettings::SETTING_DEBUG_SHOWLOGINFO;
constexpr const char* CSettings::SETTING_DEBUG_EXTRALOGGING;
constexpr const char* CSettings::SETTING_DEBUG_SETEXTRALOGLEVEL;
constexpr const char* CSettings::SETTING_DEBUG_SCREENSHOTPATH;
constexpr const char* CSettings::SETTING_DEBUG_SHARE_LOG;
constexpr const char* CSettings::SETTING_EVENTLOG_ENABLED;
constexpr const char* CSettings::SETTING_EVENTLOG_ENABLED_NOTIFICATIONS;
constexpr const char* CSettings::SETTING_EVENTLOG_SHOW;
constexpr const char* CSettings::SETTING_MASTERLOCK_LOCKCODE;
constexpr const char* CSettings::SETTING_MASTERLOCK_STARTUPLOCK;
constexpr const char* CSettings::SETTING_MASTERLOCK_MAXRETRIES;
constexpr const char* CSettings::SETTING_CACHE_HARDDISK;
constexpr const char* CSettings::SETTING_CACHEVIDEO_DVDROM;
constexpr const char* CSettings::SETTING_CACHEVIDEO_LAN;
constexpr const char* CSettings::SETTING_CACHEVIDEO_INTERNET;
constexpr const char* CSettings::SETTING_CACHEAUDIO_DVDROM;
constexpr const char* CSettings::SETTING_CACHEAUDIO_LAN;
constexpr const char* CSettings::SETTING_CACHEAUDIO_INTERNET;
constexpr const char* CSettings::SETTING_CACHEDVD_DVDROM;
constexpr const char* CSettings::SETTING_CACHEDVD_LAN;
constexpr const char* CSettings::SETTING_CACHEUNKNOWN_INTERNET;
constexpr const char* CSettings::SETTING_SYSTEM_PLAYLISTSPATH;
constexpr const char* CSettings::SETTING_ADDONS_AUTOUPDATES;
constexpr const char* CSettings::SETTING_ADDONS_NOTIFICATIONS;
constexpr const char* CSettings::SETTING_ADDONS_SHOW_RUNNING;
constexpr const char* CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES;
constexpr const char* CSettings::SETTING_ADDONS_UPDATEMODE;
constexpr const char* CSettings::SETTING_ADDONS_MANAGE_DEPENDENCIES;
constexpr const char* CSettings::SETTING_ADDONS_REMOVE_ORPHANED_DEPENDENCIES;
constexpr const char* CSettings::SETTING_GENERAL_ADDONFOREIGNFILTER;
constexpr const char* CSettings::SETTING_GENERAL_ADDONBROKENFILTER;
constexpr const char* CSettings::SETTING_SOURCE_VIDEOS;
constexpr const char* CSettings::SETTING_SOURCE_MUSIC;
constexpr const char* CSettings::SETTING_SOURCE_PICTURES;
constexpr const char* CSettings::SETTING_OSD_AUTOCLOSEVIDEOOSD;
constexpr const char* CSettings::SETTING_OSD_AUTOCLOSEVIDEOOSDTIME;
//! @todo: remove in c++17

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

void CSettings::RegisterSubSettings(ISubSettings* subSettings)
{
  if (subSettings == nullptr)
    return;

  CSingleLock lock(m_critical);
  m_subSettings.insert(subSettings);
}

void CSettings::UnregisterSubSettings(ISubSettings* subSettings)
{
  if (subSettings == nullptr)
    return;

  CSingleLock lock(m_critical);
  m_subSettings.erase(subSettings);
}

bool CSettings::Load()
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  return Load(profileManager->GetSettingsFile());
}

bool CSettings::Load(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  bool updated = false;
  if (!XFILE::CFile::Exists(file) || !xmlDoc.LoadFile(file) ||
      !Load(xmlDoc.RootElement(), updated))
  {
    CLog::Log(LOGERROR, "CSettings: unable to load settings from {}, creating new default settings",
              file);
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

bool CSettings::Load(const TiXmlElement* root)
{
  bool updated = false;
  return Load(root, updated);
}

bool CSettings::Save()
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  return Save(profileManager->GetSettingsFile());
}

bool CSettings::Save(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  if (!SaveValuesToXml(xmlDoc))
    return false;

  TiXmlElement* root = xmlDoc.RootElement();
  if (root == nullptr)
    return false;

  if (!Save(root))
    return false;

  return xmlDoc.SaveFile(file);
}

bool CSettings::Save(TiXmlNode* root) const
{
  CSingleLock lock(m_critical);
  // save any ISubSettings implementations
  for (const auto& subSetting : m_subSettings)
  {
    if (!subSetting->Save(root))
      return false;
  }

  return true;
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

void CSettings::Clear()
{
  CSingleLock lock(m_critical);
  if (!m_initialized)
    return;

  GetSettingsManager()->Clear();

  for (auto& subSetting : m_subSettings)
    subSetting->Clear();

  m_initialized = false;
}

bool CSettings::Load(const TiXmlElement* root, bool& updated)
{
  if (root == nullptr)
    return false;

  if (!CSettingsBase::LoadValuesFromXml(root, updated))
    return false;

  return Load(static_cast<const TiXmlNode*>(root));
}

bool CSettings::Load(const TiXmlNode* settings)
{
  bool ok = true;
  CSingleLock lock(m_critical);
  for (const auto& subSetting : m_subSettings)
    ok &= subSetting->Load(settings);

  return ok;
}

bool CSettings::Initialize(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
  {
    CLog::Log(LOGERROR, "CSettings: error loading settings definition from {}, Line {}\n{}", file,
              xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  CLog::Log(LOGDEBUG, "CSettings: loaded settings definition from {}", file);

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
  if (CFile::Exists(SETTINGS_XML_FOLDER "windows.xml") && !Initialize(SETTINGS_XML_FOLDER "windows.xml"))
    CLog::Log(LOGFATAL, "Unable to load windows-specific settings definitions");
#if defined(TARGET_WINDOWS_DESKTOP)
  if (CFile::Exists(SETTINGS_XML_FOLDER "win32.xml") && !Initialize(SETTINGS_XML_FOLDER "win32.xml"))
    CLog::Log(LOGFATAL, "Unable to load win32-specific settings definitions");
#elif defined(TARGET_WINDOWS_STORE)
  if (CFile::Exists(SETTINGS_XML_FOLDER "win10.xml") && !Initialize(SETTINGS_XML_FOLDER "win10.xml"))
    CLog::Log(LOGFATAL, "Unable to load win10-specific settings definitions");
#endif
#elif defined(TARGET_ANDROID)
  if (CFile::Exists(SETTINGS_XML_FOLDER "android.xml") && !Initialize(SETTINGS_XML_FOLDER "android.xml"))
    CLog::Log(LOGFATAL, "Unable to load android-specific settings definitions");
#elif defined(TARGET_FREEBSD)
  if (CFile::Exists(SETTINGS_XML_FOLDER "freebsd.xml") && !Initialize(SETTINGS_XML_FOLDER "freebsd.xml"))
    CLog::Log(LOGFATAL, "Unable to load freebsd-specific settings definitions");
#elif defined(TARGET_LINUX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "linux.xml") && !Initialize(SETTINGS_XML_FOLDER "linux.xml"))
    CLog::Log(LOGFATAL, "Unable to load linux-specific settings definitions");
#elif defined(TARGET_DARWIN)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin.xml"))
    CLog::Log(LOGFATAL, "Unable to load darwin-specific settings definitions");
#if defined(TARGET_DARWIN_OSX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_osx.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_osx.xml"))
    CLog::Log(LOGFATAL, "Unable to load osx-specific settings definitions");
#elif defined(TARGET_DARWIN_IOS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_ios.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_ios.xml"))
    CLog::Log(LOGFATAL, "Unable to load ios-specific settings definitions");
#elif defined(TARGET_DARWIN_TVOS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_tvos.xml") &&
      !Initialize(SETTINGS_XML_FOLDER "darwin_tvos.xml"))
    CLog::Log(LOGFATAL, "Unable to load tvos-specific settings definitions");
#endif
#endif

#if defined(PLATFORM_SETTINGS_FILE)
  if (CFile::Exists(SETTINGS_XML_FOLDER DEF_TO_STR_VALUE(PLATFORM_SETTINGS_FILE)) && !Initialize(SETTINGS_XML_FOLDER DEF_TO_STR_VALUE(PLATFORM_SETTINGS_FILE)))
    CLog::Log(LOGFATAL, "Unable to load platform-specific settings definitions ({})",
              DEF_TO_STR_VALUE(PLATFORM_SETTINGS_FILE));
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
  GetSettingsManager()->RegisterSettingControl("colorbutton", this);
}

void CSettings::InitializeVisibility()
{
  // hide some settings if necessary
#if defined(TARGET_DARWIN_EMBEDDED)
  std::shared_ptr<CSettingString> timezonecountry = std::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_TIMEZONECOUNTRY));
  std::shared_ptr<CSettingString> timezone = std::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_TIMEZONE));

  timezonecountry->SetRequirementsMet(false);
  timezone->SetRequirementsMet(false);
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
  // We prefer a fake fullscreen mode (window covering the screen rather than dedicated fullscreen)
  // as it works nicer with switching to other applications. However on some systems vsync is broken
  // when we do this (eg non-Aero on ATI in particular) and on others (AppleTV) we can't get XBMC to
  // the front
  if (g_sysinfo.IsAeroDisabled())
  {
    auto setting = GetSettingsManager()->GetSetting(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN);
    if (!setting)
      CLog::Log(LOGERROR, "Failed to load setting for: {}",
                CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN);
    else
      std::static_pointer_cast<CSettingBool>(setting)->SetDefault(false);
  }
#endif

  if (g_application.IsStandAlone())
  {
    auto setting =
        GetSettingsManager()->GetSetting(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE);
    if (!setting)
      CLog::Log(LOGERROR, "Failed to load setting for: {}",
                CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE);
    else
      std::static_pointer_cast<CSettingInt>(setting)->SetDefault(POWERSTATE_SHUTDOWN);
  }

  // Initialize deviceUUID if not already set, used in zeroconf advertisements.
  std::shared_ptr<CSettingString> deviceUUID = std::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_SERVICES_DEVICEUUID));
  if (deviceUUID->GetValue().empty())
  {
    const std::string& uuid = StringUtils::CreateUUID();
    auto setting = GetSettingsManager()->GetSetting(CSettings::SETTING_SERVICES_DEVICEUUID);
    if (!setting)
      CLog::Log(LOGERROR, "Failed to load setting for: {}", CSettings::SETTING_SERVICES_DEVICEUUID);
    else
      std::static_pointer_cast<CSettingString>(setting)->SetValue(uuid);
  }
}

void CSettings::InitializeOptionFillers()
{
  // register setting option fillers
#ifdef HAS_DVD_DRIVE
  GetSettingsManager()->RegisterSettingOptionsFiller("audiocdactions", MEDIA_DETECT::CAutorun::SettingOptionAudioCdActionsFiller);
#endif
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
  GetSettingsManager()->RegisterSettingOptionsFiller("modes", CDisplaySettings::SettingOptionsModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("resolutions", CDisplaySettings::SettingOptionsResolutionsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("screens", CDisplaySettings::SettingOptionsDispModeFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("stereoscopicmodes", CDisplaySettings::SettingOptionsStereoscopicModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("preferedstereoscopicviewmodes", CDisplaySettings::SettingOptionsPreferredStereoscopicViewModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("monitors", CDisplaySettings::SettingOptionsMonitorsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsmodes", CDisplaySettings::SettingOptionsCmsModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmswhitepoints", CDisplaySettings::SettingOptionsCmsWhitepointsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsprimaries", CDisplaySettings::SettingOptionsCmsPrimariesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsgammamodes", CDisplaySettings::SettingOptionsCmsGammaModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("videoseeksteps", CSeekHandler::SettingOptionsSeekStepsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("startupwindows", ADDON::CSkinInfo::SettingOptionsStartupWindowsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("audiostreamlanguages", CLangInfo::SettingOptionsAudioStreamLanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("subtitlestreamlanguages", CLangInfo::SettingOptionsSubtitleStreamLanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("subtitledownloadlanguages", CLangInfo::SettingOptionsSubtitleDownloadlanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("iso6391languages", CLangInfo::SettingOptionsISO6391LanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skincolors", ADDON::CSkinInfo::SettingOptionsSkinColorsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skinfonts", ADDON::CSkinInfo::SettingOptionsSkinFontsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skinthemes", ADDON::CSkinInfo::SettingOptionsSkinThemesFiller);
#ifdef TARGET_LINUX
  GetSettingsManager()->RegisterSettingOptionsFiller("timezonecountries", CPosixTimezone::SettingOptionsTimezoneCountriesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("timezones", CPosixTimezone::SettingOptionsTimezonesFiller);
#endif
  GetSettingsManager()->RegisterSettingOptionsFiller("keyboardlayouts", CKeyboardLayoutManager::SettingOptionsKeyboardLayoutsFiller);
}

void CSettings::UninitializeOptionFillers()
{
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiocdactions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiocdencoders");
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
    GetSettingsManager()->AddDynamicCondition(itCondition->first, itCondition->second);
}

void CSettings::UninitializeConditions()
{
  CSettingConditions::Deinitialize();
}

void CSettings::InitializeISettingsHandlers()
{
  // register ISettingsHandler implementations
  // The order of these matters! Handlers are processed in the order they were registered.
  GetSettingsManager()->RegisterSettingsHandler(&CMediaSourceSettings::GetInstance());
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
  // unregister ISettingsHandler implementations
  GetSettingsManager()->UnregisterSettingsHandler(&CMediaSettings::GetInstance());
#if defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterSettingsHandler(&g_timezone);
#endif // defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterSettingsHandler(&g_application);
  GetSettingsManager()->UnregisterSettingsHandler(&g_langInfo);
  GetSettingsManager()->UnregisterSettingsHandler(&CRssManager::GetInstance());
  GetSettingsManager()->UnregisterSettingsHandler(&CWakeOnAccess::GetInstance());
#ifdef HAS_UPNP
  GetSettingsManager()->UnregisterSettingsHandler(&CUPnPSettings::GetInstance());
#endif
  GetSettingsManager()->UnregisterSettingsHandler(&CMediaSourceSettings::GetInstance());
}

void CSettings::InitializeISubSettings()
{
  // register ISubSettings implementations
  RegisterSubSettings(&g_application);
  RegisterSubSettings(&CDisplaySettings::GetInstance());
  RegisterSubSettings(&CMediaSettings::GetInstance());
  RegisterSubSettings(&CSkinSettings::GetInstance());
  RegisterSubSettings(&g_sysinfo);
  RegisterSubSettings(&CViewStateSettings::GetInstance());
}

void CSettings::UninitializeISubSettings()
{
  // unregister ISubSettings implementations
  UnregisterSubSettings(&g_application);
  UnregisterSubSettings(&CDisplaySettings::GetInstance());
  UnregisterSubSettings(&CMediaSettings::GetInstance());
  UnregisterSubSettings(&CSkinSettings::GetInstance());
  UnregisterSubSettings(&g_sysinfo);
  UnregisterSubSettings(&CViewStateSettings::GetInstance());
}

void CSettings::InitializeISettingCallbacks()
{
  // register any ISettingCallback implementations
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_CLEANUP);
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_EXPORT);
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_IMPORT);
  settingSet.insert(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_FLATTENTVSHOWS);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_GROUPMOVIESETS);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_CLEANUP);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_IMPORT);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_EXPORT);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS);
  GetSettingsManager()->RegisterCallback(&CMediaSettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_SCREEN);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_RESOLUTION);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_SCREENMODE);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_MONITOR);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_PREFEREDSTEREOSCOPICMODE);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_3DLUT);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_DISPLAYPROFILE);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_BLANKDISPLAYS);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_WHITELIST);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_10BITSURFACES);
  GetSettingsManager()->RegisterCallback(&CDisplaySettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_VIDEOPLAYER_SEEKDELAY);
  settingSet.insert(CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS);
  settingSet.insert(CSettings::SETTING_MUSICPLAYER_SEEKDELAY);
  settingSet.insert(CSettings::SETTING_MUSICPLAYER_SEEKSTEPS);
  GetSettingsManager()->RegisterCallback(&g_application.GetAppPlayer().GetSeekHandler(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH);
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
  settingSet.insert(CSettings::SETTING_VIDEOPLAYER_USEMEDIACODEC);
  settingSet.insert(CSettings::SETTING_VIDEOPLAYER_USEMEDIACODECSURFACE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_VOLUMESTEPS);
  settingSet.insert(CSettings::SETTING_SOURCE_VIDEOS);
  settingSet.insert(CSettings::SETTING_SOURCE_MUSIC);
  settingSet.insert(CSettings::SETTING_SOURCE_PICTURES);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN);
  GetSettingsManager()->RegisterCallback(&g_application, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_SUBTITLES_CHARSET);
  settingSet.insert(CSettings::SETTING_LOCALE_CHARSET);
  GetSettingsManager()->RegisterCallback(&g_charsetConverter, settingSet);

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

#if defined(TARGET_DARWIN_OSX)
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_INPUT_APPLEREMOTEMODE);
  settingSet.insert(CSettings::SETTING_INPUT_APPLEREMOTEALWAYSON);
  GetSettingsManager()->RegisterCallback(&XBMCHelper::GetInstance(), settingSet);
#endif

#if defined(TARGET_DARWIN_TVOS)
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEIDLETIMERENABLED);
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEIDLETIME);
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEHORIZONTALSENSITIVITY);
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEVERTICALSENSITIVITY);
  GetSettingsManager()->RegisterCallback(&CTVOSInputSettings::GetInstance(), settingSet);
#endif

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_ADDONS_SHOW_RUNNING);
  settingSet.insert(CSettings::SETTING_ADDONS_MANAGE_DEPENDENCIES);
  settingSet.insert(CSettings::SETTING_ADDONS_REMOVE_ORPHANED_DEPENDENCIES);
  settingSet.insert(CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES);
  GetSettingsManager()->RegisterCallback(&ADDON::CAddonSystemSettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_POWERMANAGEMENT_WAKEONACCESS);
  GetSettingsManager()->RegisterCallback(&CWakeOnAccess::GetInstance(), settingSet);

#ifdef HAVE_LIBBLURAY
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_DISC_PLAYBACK);
  GetSettingsManager()->RegisterCallback(&CDiscSettings::GetInstance(), settingSet);
#endif
}

void CSettings::UninitializeISettingCallbacks()
{
  GetSettingsManager()->UnregisterCallback(&CMediaSettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CDisplaySettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_application.GetAppPlayer().GetSeekHandler());
  GetSettingsManager()->UnregisterCallback(&g_application);
  GetSettingsManager()->UnregisterCallback(&g_charsetConverter);
  GetSettingsManager()->UnregisterCallback(&g_langInfo);
  GetSettingsManager()->UnregisterCallback(&g_passwordManager);
  GetSettingsManager()->UnregisterCallback(&CRssManager::GetInstance());
#if defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterCallback(&g_timezone);
#endif // defined(TARGET_LINUX)
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
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  const std::string settingsFile = profileManager->GetSettingsFile();

  // try to delete the settings file
  if (XFILE::CFile::Exists(settingsFile, false) && !XFILE::CFile::Delete(settingsFile))
    CLog::Log(LOGWARNING, "Unable to delete old settings file at {}", settingsFile);

  // unload any loaded settings
  Unload();

  // try to save the default settings
  if (!Save())
  {
    CLog::Log(LOGWARNING, "Failed to save the default settings to {}", settingsFile);
    return false;
  }

  return true;
}
