/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/ISubSettings.h"
#include "settings/SettingControl.h"
#include "settings/SettingCreator.h"
#include "settings/SettingsBase.h"

#include <string>

class CSettingList;
class TiXmlNode;

/*!
 \brief Wrapper around CSettingsManager responsible for properly setting up
 the settings manager and registering all the callbacks, handlers and custom
 setting types.
 \sa CSettingsManager
 */
class CSettings : public CSettingsBase, public CSettingCreator, public CSettingControlCreator
                , private ISubSettings
{
public:
  static constexpr auto SETTING_LOOKANDFEEL_SKIN = "lookandfeel.skin";
  static constexpr auto SETTING_LOOKANDFEEL_SKINSETTINGS = "lookandfeel.skinsettings";
  static constexpr auto SETTING_LOOKANDFEEL_SKINTHEME = "lookandfeel.skintheme";
  static constexpr auto SETTING_LOOKANDFEEL_SKINCOLORS = "lookandfeel.skincolors";
  static constexpr auto SETTING_LOOKANDFEEL_FONT = "lookandfeel.font";
  static constexpr auto SETTING_LOOKANDFEEL_SKINZOOM = "lookandfeel.skinzoom";
  static constexpr auto SETTING_LOOKANDFEEL_STARTUPACTION = "lookandfeel.startupaction";
  static constexpr auto SETTING_LOOKANDFEEL_STARTUPWINDOW = "lookandfeel.startupwindow";
  static constexpr auto SETTING_LOOKANDFEEL_SOUNDSKIN = "lookandfeel.soundskin";
  static constexpr auto SETTING_LOOKANDFEEL_ENABLERSSFEEDS = "lookandfeel.enablerssfeeds";
  static constexpr auto SETTING_LOOKANDFEEL_RSSEDIT = "lookandfeel.rssedit";
  static constexpr auto SETTING_LOOKANDFEEL_STEREOSTRENGTH = "lookandfeel.stereostrength";
  static constexpr auto SETTING_LOCALE_LANGUAGE = "locale.language";
  static constexpr auto SETTING_LOCALE_COUNTRY = "locale.country";
  static constexpr auto SETTING_LOCALE_CHARSET = "locale.charset";
  static constexpr auto SETTING_LOCALE_KEYBOARDLAYOUTS = "locale.keyboardlayouts";
  static constexpr auto SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT = "locale.activekeyboardlayout";
  static constexpr auto SETTING_LOCALE_SHORTDATEFORMAT = "locale.shortdateformat";
  static constexpr auto SETTING_LOCALE_LONGDATEFORMAT = "locale.longdateformat";
  static constexpr auto SETTING_LOCALE_TIMEFORMAT = "locale.timeformat";
  static constexpr auto SETTING_LOCALE_USE24HOURCLOCK = "locale.use24hourclock";
  static constexpr auto SETTING_LOCALE_TEMPERATUREUNIT = "locale.temperatureunit";
  static constexpr auto SETTING_LOCALE_SPEEDUNIT = "locale.speedunit";
  static constexpr auto SETTING_FILELISTS_SHOWPARENTDIRITEMS = "filelists.showparentdiritems";
  static constexpr auto SETTING_FILELISTS_SHOWEXTENSIONS = "filelists.showextensions";
  static constexpr auto SETTING_FILELISTS_IGNORETHEWHENSORTING = "filelists.ignorethewhensorting";
  static constexpr auto SETTING_FILELISTS_ALLOWFILEDELETION = "filelists.allowfiledeletion";
  static constexpr auto SETTING_FILELISTS_SHOWADDSOURCEBUTTONS = "filelists.showaddsourcebuttons";
  static constexpr auto SETTING_FILELISTS_SHOWHIDDEN = "filelists.showhidden";
  static constexpr auto SETTING_SCREENSAVER_MODE = "screensaver.mode";
  static constexpr auto SETTING_SCREENSAVER_SETTINGS = "screensaver.settings";
  static constexpr auto SETTING_SCREENSAVER_PREVIEW = "screensaver.preview";
  static constexpr auto SETTING_SCREENSAVER_TIME = "screensaver.time";
  static constexpr auto SETTING_SCREENSAVER_DISABLEFORAUDIO = "screensaver.disableforaudio";
  static constexpr auto SETTING_SCREENSAVER_USEDIMONPAUSE = "screensaver.usedimonpause";
  static constexpr auto SETTING_WINDOW_WIDTH = "window.width";
  static constexpr auto SETTING_WINDOW_HEIGHT = "window.height";
  static constexpr auto SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS = "videolibrary.showunwatchedplots";
  static constexpr auto SETTING_VIDEOLIBRARY_ACTORTHUMBS = "videolibrary.actorthumbs";
  static constexpr auto SETTING_MYVIDEOS_FLATTEN = "myvideos.flatten";
  static constexpr auto SETTING_VIDEOLIBRARY_FLATTENTVSHOWS = "videolibrary.flattentvshows";
  static constexpr auto SETTING_VIDEOLIBRARY_TVSHOWSSELECTFIRSTUNWATCHEDITEM =
      "videolibrary.tvshowsselectfirstunwatcheditem";
  static constexpr auto SETTING_VIDEOLIBRARY_TVSHOWSINCLUDEALLSEASONSANDSPECIALS =
      "videolibrary.tvshowsincludeallseasonsandspecials";
  static constexpr auto SETTING_VIDEOLIBRARY_SHOWALLITEMS = "videolibrary.showallitems";
  static constexpr auto SETTING_VIDEOLIBRARY_GROUPMOVIESETS = "videolibrary.groupmoviesets";
  static constexpr auto SETTING_VIDEOLIBRARY_GROUPSINGLEITEMSETS =
      "videolibrary.groupsingleitemsets";
  static constexpr auto SETTING_VIDEOLIBRARY_UPDATEONSTARTUP = "videolibrary.updateonstartup";
  static constexpr auto SETTING_VIDEOLIBRARY_BACKGROUNDUPDATE = "videolibrary.backgroundupdate";
  static constexpr auto SETTING_VIDEOLIBRARY_CLEANUP = "videolibrary.cleanup";
  static constexpr auto SETTING_VIDEOLIBRARY_EXPORT = "videolibrary.export";
  static constexpr auto SETTING_VIDEOLIBRARY_IMPORT = "videolibrary.import";
  static constexpr auto SETTING_VIDEOLIBRARY_SHOWEMPTYTVSHOWS = "videolibrary.showemptytvshows";
  static constexpr auto SETTING_VIDEOLIBRARY_MOVIESETSFOLDER = "videolibrary.moviesetsfolder";
  static constexpr auto SETTING_VIDEOLIBRARY_ARTWORK_LEVEL = "videolibrary.artworklevel";
  static constexpr auto SETTING_VIDEOLIBRARY_MOVIEART_WHITELIST = "videolibrary.movieartwhitelist";
  static constexpr auto SETTING_VIDEOLIBRARY_TVSHOWART_WHITELIST =
      "videolibrary.tvshowartwhitelist";
  static constexpr auto SETTING_VIDEOLIBRARY_EPISODEART_WHITELIST =
      "videolibrary.episodeartwhitelist";
  static constexpr auto SETTING_VIDEOLIBRARY_MUSICVIDEOART_WHITELIST =
      "videolibrary.musicvideoartwhitelist";
  static constexpr auto SETTING_VIDEOLIBRARY_SHOWPERFORMERS =
      "videolibrary.musicvideosallperformers";
  static constexpr auto SETTING_VIDEOLIBRARY_IGNOREVIDEOVERSIONS =
      "videolibrary.ignorevideoversions";
  static constexpr auto SETTING_VIDEOLIBRARY_IGNOREVIDEOEXTRAS = "videolibrary.ignorevideoextras";
  static constexpr auto SETTING_VIDEOLIBRARY_SHOWVIDEOVERSIONSASFOLDER =
      "videolibrary.showvideoversionsasfolder";
  static constexpr auto SETTING_LOCALE_AUDIOLANGUAGE = "locale.audiolanguage";
  static constexpr auto SETTING_VIDEOPLAYER_PREFERDEFAULTFLAG = "videoplayer.preferdefaultflag";
  static constexpr auto SETTING_VIDEOPLAYER_AUTOPLAYNEXTITEM = "videoplayer.autoplaynextitem";
  static constexpr auto SETTING_VIDEOPLAYER_SEEKSTEPS = "videoplayer.seeksteps";
  static constexpr auto SETTING_VIDEOPLAYER_SEEKDELAY = "videoplayer.seekdelay";
  static constexpr auto SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE = "videoplayer.adjustrefreshrate";
  static constexpr auto SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK = "videoplayer.usedisplayasclock";
  static constexpr auto SETTING_VIDEOPLAYER_ERRORINASPECT = "videoplayer.errorinaspect";
  static constexpr auto SETTING_VIDEOPLAYER_STRETCH43 = "videoplayer.stretch43";
  static constexpr auto SETTING_VIDEOPLAYER_TELETEXTENABLED = "videoplayer.teletextenabled";
  static constexpr auto SETTING_VIDEOPLAYER_TELETEXTSCALE = "videoplayer.teletextscale";
  static constexpr auto SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE =
      "videoplayer.stereoscopicplaybackmode";
  static constexpr auto SETTING_VIDEOPLAYER_QUITSTEREOMODEONSTOP =
      "videoplayer.quitstereomodeonstop";
  static constexpr auto SETTING_VIDEOPLAYER_RENDERMETHOD = "videoplayer.rendermethod";
  static constexpr auto SETTING_VIDEOPLAYER_HQSCALERS = "videoplayer.hqscalers";
  static constexpr auto SETTING_VIDEOPLAYER_USESUPERRESOLUTION = "videoplayer.usesuperresolution";
  static constexpr auto SETTING_VIDEOPLAYER_HIGHPRECISIONPROCESSING = "videoplayer.highprecision";
  static constexpr auto SETTING_VIDEOPLAYER_USEMEDIACODEC = "videoplayer.usemediacodec";
  static constexpr auto SETTING_VIDEOPLAYER_USEMEDIACODECSURFACE =
      "videoplayer.usemediacodecsurface";
  static constexpr auto SETTING_VIDEOPLAYER_USEVDPAU = "videoplayer.usevdpau";
  static constexpr auto SETTING_VIDEOPLAYER_USEVDPAUMIXER = "videoplayer.usevdpaumixer";
  static constexpr auto SETTING_VIDEOPLAYER_USEVDPAUMPEG2 = "videoplayer.usevdpaumpeg2";
  static constexpr auto SETTING_VIDEOPLAYER_USEVDPAUMPEG4 = "videoplayer.usevdpaumpeg4";
  static constexpr auto SETTING_VIDEOPLAYER_USEVDPAUVC1 = "videoplayer.usevdpauvc1";
  static constexpr auto SETTING_VIDEOPLAYER_USEDXVA2 = "videoplayer.usedxva2";
  static constexpr auto SETTING_VIDEOPLAYER_USEVTB = "videoplayer.usevtb";
  static constexpr auto SETTING_VIDEOPLAYER_USEPRIMEDECODER = "videoplayer.useprimedecoder";
  static constexpr auto SETTING_VIDEOPLAYER_USESTAGEFRIGHT = "videoplayer.usestagefright";
  static constexpr auto SETTING_VIDEOPLAYER_LIMITGUIUPDATE = "videoplayer.limitguiupdate";
  static constexpr auto SETTING_VIDEOPLAYER_SUPPORTMVC = "videoplayer.supportmvc";
  static constexpr auto SETTING_VIDEOPLAYER_CONVERTDOVI = "videoplayer.convertdovi";
  static constexpr auto SETTING_VIDEOPLAYER_ALLOWEDHDRFORMATS = "videoplayer.allowedhdrformats";
  static constexpr auto SETTING_MYVIDEOS_SELECTACTION = "myvideos.selectaction";
  static constexpr auto SETTING_MYVIDEOS_SELECTDEFAULTVERSION = "myvideos.selectdefaultversion";
  static constexpr auto SETTING_MYVIDEOS_PLAYACTION = "myvideos.playaction";
  static constexpr auto SETTING_MYVIDEOS_USETAGS = "myvideos.usetags";
  static constexpr auto SETTING_MYVIDEOS_EXTRACTFLAGS = "myvideos.extractflags";
  static constexpr auto SETTING_MYVIDEOS_EXTRACTCHAPTERTHUMBS = "myvideos.extractchapterthumbs";
  static constexpr auto SETTING_MYVIDEOS_REPLACELABELS = "myvideos.replacelabels";
  static constexpr auto SETTING_MYVIDEOS_EXTRACTTHUMB = "myvideos.extractthumb";
  static constexpr auto SETTING_MYVIDEOS_STACKVIDEOS = "myvideos.stackvideos";
  static constexpr auto SETTING_LOCALE_SUBTITLELANGUAGE = "locale.subtitlelanguage";
  static constexpr auto SETTING_SUBTITLES_PARSECAPTIONS = "subtitles.parsecaptions";
  static constexpr auto SETTING_SUBTITLES_CAPTIONSALIGN = "subtitles.captionsalign";
  static constexpr auto SETTING_SUBTITLES_ALIGN = "subtitles.align";
  static constexpr auto SETTING_SUBTITLES_STEREOSCOPICDEPTH = "subtitles.stereoscopicdepth";
  static constexpr auto SETTING_SUBTITLES_FONTNAME = "subtitles.fontname";
  static constexpr auto SETTING_SUBTITLES_FONTSIZE = "subtitles.fontsize";
  static constexpr auto SETTING_SUBTITLES_STYLE = "subtitles.style";
  static constexpr auto SETTING_SUBTITLES_COLOR = "subtitles.colorpick";
  static constexpr auto SETTING_SUBTITLES_BORDERSIZE = "subtitles.bordersize";
  static constexpr auto SETTING_SUBTITLES_BORDERCOLOR = "subtitles.bordercolorpick";
  static constexpr auto SETTING_SUBTITLES_OPACITY = "subtitles.opacity";
  static constexpr auto SETTING_SUBTITLES_BLUR = "subtitles.blur";
  static constexpr auto SETTING_SUBTITLES_BACKGROUNDTYPE = "subtitles.backgroundtype";
  static constexpr auto SETTING_SUBTITLES_SHADOWCOLOR = "subtitles.shadowcolor";
  static constexpr auto SETTING_SUBTITLES_SHADOWOPACITY = "subtitles.shadowopacity";
  static constexpr auto SETTING_SUBTITLES_SHADOWSIZE = "subtitles.shadowsize";
  static constexpr auto SETTING_SUBTITLES_BGCOLOR = "subtitles.bgcolorpick";
  static constexpr auto SETTING_SUBTITLES_BGOPACITY = "subtitles.bgopacity";
  static constexpr auto SETTING_SUBTITLES_MARGINVERTICAL = "subtitles.marginvertical";
  static constexpr auto SETTING_SUBTITLES_CHARSET = "subtitles.charset";
  static constexpr auto SETTING_SUBTITLES_OVERRIDEFONTS = "subtitles.overridefonts";
  static constexpr auto SETTING_SUBTITLES_OVERRIDESTYLES = "subtitles.overridestyles";
  static constexpr auto SETTING_SUBTITLES_LANGUAGES = "subtitles.languages";
  static constexpr auto SETTING_SUBTITLES_STORAGEMODE = "subtitles.storagemode";
  static constexpr auto SETTING_SUBTITLES_CUSTOMPATH = "subtitles.custompath";
  static constexpr auto SETTING_SUBTITLES_PAUSEONSEARCH = "subtitles.pauseonsearch";
  static constexpr auto SETTING_SUBTITLES_DOWNLOADFIRST = "subtitles.downloadfirst";
  static constexpr auto SETTING_SUBTITLES_TV = "subtitles.tv";
  static constexpr auto SETTING_SUBTITLES_MOVIE = "subtitles.movie";
  static constexpr auto SETTING_DVDS_AUTORUN = "dvds.autorun";
  static constexpr auto SETTING_DVDS_PLAYERREGION = "dvds.playerregion";
  static constexpr auto SETTING_DVDS_AUTOMENU = "dvds.automenu";
  static constexpr auto SETTING_DISC_PLAYBACK = "disc.playback";
  static constexpr auto SETTING_BLURAY_PLAYERREGION = "bluray.playerregion";
  static constexpr auto SETTING_ACCESSIBILITY_AUDIOVISUAL = "accessibility.audiovisual";
  static constexpr auto SETTING_ACCESSIBILITY_AUDIOHEARING = "accessibility.audiohearing";
  static constexpr auto SETTING_ACCESSIBILITY_SUBHEARING = "accessibility.subhearing";
  static constexpr auto SETTING_SCRAPERS_MOVIESDEFAULT = "scrapers.moviesdefault";
  static constexpr auto SETTING_SCRAPERS_TVSHOWSDEFAULT = "scrapers.tvshowsdefault";
  static constexpr auto SETTING_SCRAPERS_MUSICVIDEOSDEFAULT = "scrapers.musicvideosdefault";
  static constexpr auto SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL =
      "pvrmanager.preselectplayingchannel";
  static constexpr auto SETTING_PVRMANAGER_BACKENDCHANNELGROUPSORDER =
      "pvrmanager.backendchannelgroupsorder";
  static constexpr auto SETTING_PVRMANAGER_BACKENDCHANNELORDER = "pvrmanager.backendchannelorder";
  static constexpr auto SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS =
      "pvrmanager.usebackendchannelnumbers";
  static constexpr auto SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERSALWAYS =
      "pvrmanager.usebackendchannelnumbersalways";
  static constexpr auto SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE =
      "pvrmanager.startgroupchannelnumbersfromone";
  static constexpr auto SETTING_PVRMANAGER_CLIENTPRIORITIES = "pvrmanager.clientpriorities";
  static constexpr auto SETTING_PVRMANAGER_CHANNELMANAGER = "pvrmanager.channelmanager";
  static constexpr auto SETTING_PVRMANAGER_GROUPMANAGER = "pvrmanager.groupmanager";
  static constexpr auto SETTING_PVRMANAGER_CHANNELSCAN = "pvrmanager.channelscan";
  static constexpr auto SETTING_PVRMANAGER_RESETDB = "pvrmanager.resetdb";
  static constexpr auto SETTING_PVRMANAGER_ADDONS = "pvrmanager.addons";
  static constexpr auto SETTING_PVRMENU_DISPLAYCHANNELINFO = "pvrmenu.displaychannelinfo";
  static constexpr auto SETTING_PVRMENU_CLOSECHANNELOSDONSWITCH = "pvrmenu.closechannelosdonswitch";
  static constexpr auto SETTING_PVRMENU_ICONPATH = "pvrmenu.iconpath";
  static constexpr auto SETTING_PVRMENU_SEARCHICONS = "pvrmenu.searchicons";
  static constexpr auto SETTING_EPG_PAST_DAYSTODISPLAY = "epg.pastdaystodisplay";
  static constexpr auto SETTING_EPG_FUTURE_DAYSTODISPLAY = "epg.futuredaystodisplay";
  static constexpr auto SETTING_EPG_SELECTACTION = "epg.selectaction";
  static constexpr auto SETTING_EPG_HIDENOINFOAVAILABLE = "epg.hidenoinfoavailable";
  static constexpr auto SETTING_EPG_EPGUPDATE = "epg.epgupdate";
  static constexpr auto SETTING_EPG_PREVENTUPDATESWHILEPLAYINGTV =
      "epg.preventupdateswhileplayingtv";
  static constexpr auto SETTING_EPG_RESETEPG = "epg.resetepg";
  static constexpr auto SETTING_PVRPLAYBACK_SWITCHTOFULLSCREENCHANNELTYPES =
      "pvrplayback.switchtofullscreenchanneltypes";
  static constexpr auto SETTING_PVRPLAYBACK_SIGNALQUALITY = "pvrplayback.signalquality";
  static constexpr auto SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH =
      "pvrplayback.confirmchannelswitch";
  static constexpr auto SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT = "pvrplayback.channelentrytimeout";
  static constexpr auto SETTING_PVRPLAYBACK_DELAYMARKLASTWATCHED =
      "pvrplayback.delaymarklastwatched";
  static constexpr auto SETTING_PVRPLAYBACK_FPS = "pvrplayback.fps";
  static constexpr auto SETTING_PVRPLAYBACK_AUTOPLAYNEXTPROGRAMME =
      "pvrplayback.autoplaynextprogramme";
  static constexpr auto SETTING_PVRRECORD_INSTANTRECORDACTION = "pvrrecord.instantrecordaction";
  static constexpr auto SETTING_PVRRECORD_INSTANTRECORDTIME = "pvrrecord.instantrecordtime";
  static constexpr auto SETTING_PVRRECORD_MARGINSTART = "pvrrecord.marginstart";
  static constexpr auto SETTING_PVRRECORD_MARGINEND = "pvrrecord.marginend";
  static constexpr auto SETTING_PVRRECORD_TIMERNOTIFICATIONS = "pvrrecord.timernotifications";
  static constexpr auto SETTING_PVRRECORD_GROUPRECORDINGS = "pvrrecord.grouprecordings";
  static constexpr auto SETTING_PVRREMINDERS_AUTOCLOSEDELAY = "pvrreminders.autoclosedelay";
  static constexpr auto SETTING_PVRREMINDERS_AUTORECORD = "pvrreminders.autorecord";
  static constexpr auto SETTING_PVRREMINDERS_AUTOSWITCH = "pvrreminders.autoswitch";
  static constexpr auto SETTING_PVRPOWERMANAGEMENT_ENABLED = "pvrpowermanagement.enabled";
  static constexpr auto SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME =
      "pvrpowermanagement.backendidletime";
  static constexpr auto SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD = "pvrpowermanagement.setwakeupcmd";
  static constexpr auto SETTING_PVRPOWERMANAGEMENT_PREWAKEUP = "pvrpowermanagement.prewakeup";
  static constexpr auto SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUP = "pvrpowermanagement.dailywakeup";
  static constexpr auto SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME =
      "pvrpowermanagement.dailywakeuptime";
  static constexpr auto SETTING_PVRPARENTAL_ENABLED = "pvrparental.enabled";
  static constexpr auto SETTING_PVRPARENTAL_PIN = "pvrparental.pin";
  static constexpr auto SETTING_PVRPARENTAL_DURATION = "pvrparental.duration";
  static constexpr auto SETTING_PVRCLIENT_MENUHOOK = "pvrclient.menuhook";
  static constexpr auto SETTING_PVRTIMERS_HIDEDISABLEDTIMERS = "pvrtimers.hidedisabledtimers";
  static constexpr auto SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS =
      "musiclibrary.showcompilationartists";
  static constexpr auto SETTING_MUSICLIBRARY_SHOWDISCS = "musiclibrary.showdiscs";
  static constexpr auto SETTING_MUSICLIBRARY_USEORIGINALDATE = "musiclibrary.useoriginaldate";
  static constexpr auto SETTING_MUSICLIBRARY_USEARTISTSORTNAME = "musiclibrary.useartistsortname";
  static constexpr auto SETTING_MUSICLIBRARY_DOWNLOADINFO = "musiclibrary.downloadinfo";
  static constexpr auto SETTING_MUSICLIBRARY_ARTISTSFOLDER = "musiclibrary.artistsfolder";
  static constexpr auto SETTING_MUSICLIBRARY_PREFERONLINEALBUMART =
      "musiclibrary.preferonlinealbumart";
  static constexpr auto SETTING_MUSICLIBRARY_ARTWORKLEVEL = "musiclibrary.artworklevel";
  static constexpr auto SETTING_MUSICLIBRARY_USEALLLOCALART = "musiclibrary.usealllocalart";
  static constexpr auto SETTING_MUSICLIBRARY_USEALLREMOTEART = "musiclibrary.useallremoteart";
  static constexpr auto SETTING_MUSICLIBRARY_ARTISTART_WHITELIST =
      "musiclibrary.artistartwhitelist";
  static constexpr auto SETTING_MUSICLIBRARY_ALBUMART_WHITELIST = "musiclibrary.albumartwhitelist";
  static constexpr auto SETTING_MUSICLIBRARY_MUSICTHUMBS = "musiclibrary.musicthumbs";
  static constexpr auto SETTING_MUSICLIBRARY_ALBUMSSCRAPER = "musiclibrary.albumsscraper";
  static constexpr auto SETTING_MUSICLIBRARY_ARTISTSSCRAPER = "musiclibrary.artistsscraper";
  static constexpr auto SETTING_MUSICLIBRARY_OVERRIDETAGS = "musiclibrary.overridetags";
  static constexpr auto SETTING_MUSICLIBRARY_SHOWALLITEMS = "musiclibrary.showallitems";
  static constexpr auto SETTING_MUSICLIBRARY_UPDATEONSTARTUP = "musiclibrary.updateonstartup";
  static constexpr auto SETTING_MUSICLIBRARY_BACKGROUNDUPDATE = "musiclibrary.backgroundupdate";
  static constexpr auto SETTING_MUSICLIBRARY_CLEANUP = "musiclibrary.cleanup";
  static constexpr auto SETTING_MUSICLIBRARY_EXPORT = "musiclibrary.export";
  static constexpr auto SETTING_MUSICLIBRARY_EXPORT_FILETYPE = "musiclibrary.exportfiletype";
  static constexpr auto SETTING_MUSICLIBRARY_EXPORT_FOLDER = "musiclibrary.exportfolder";
  static constexpr auto SETTING_MUSICLIBRARY_EXPORT_ITEMS = "musiclibrary.exportitems";
  static constexpr auto SETTING_MUSICLIBRARY_EXPORT_UNSCRAPED = "musiclibrary.exportunscraped";
  static constexpr auto SETTING_MUSICLIBRARY_EXPORT_OVERWRITE = "musiclibrary.exportoverwrite";
  static constexpr auto SETTING_MUSICLIBRARY_EXPORT_ARTWORK = "musiclibrary.exportartwork";
  static constexpr auto SETTING_MUSICLIBRARY_EXPORT_SKIPNFO = "musiclibrary.exportskipnfo";
  static constexpr auto SETTING_MUSICLIBRARY_IMPORT = "musiclibrary.import";
  static constexpr auto SETTING_MUSICPLAYER_AUTOPLAYNEXTITEM = "musicplayer.autoplaynextitem";
  static constexpr auto SETTING_MUSICPLAYER_QUEUEBYDEFAULT = "musicplayer.queuebydefault";
  static constexpr auto SETTING_MUSICPLAYER_SEEKSTEPS = "musicplayer.seeksteps";
  static constexpr auto SETTING_MUSICPLAYER_SEEKDELAY = "musicplayer.seekdelay";
  static constexpr auto SETTING_MUSICPLAYER_REPLAYGAINTYPE = "musicplayer.replaygaintype";
  static constexpr auto SETTING_MUSICPLAYER_REPLAYGAINPREAMP = "musicplayer.replaygainpreamp";
  static constexpr auto SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP =
      "musicplayer.replaygainnogainpreamp";
  static constexpr auto SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING =
      "musicplayer.replaygainavoidclipping";
  static constexpr auto SETTING_MUSICPLAYER_CROSSFADE = "musicplayer.crossfade";
  static constexpr auto SETTING_MUSICPLAYER_CROSSFADEALBUMTRACKS =
      "musicplayer.crossfadealbumtracks";
  static constexpr auto SETTING_MUSICPLAYER_VISUALISATION = "musicplayer.visualisation";
  static constexpr auto SETTING_MUSICFILES_SELECTACTION = "musicfiles.selectaction";
  static constexpr auto SETTING_MUSICFILES_USETAGS = "musicfiles.usetags";
  static constexpr auto SETTING_MUSICFILES_TRACKFORMAT = "musicfiles.trackformat";
  static constexpr auto SETTING_MUSICFILES_NOWPLAYINGTRACKFORMAT =
      "musicfiles.nowplayingtrackformat";
  static constexpr auto SETTING_MUSICFILES_LIBRARYTRACKFORMAT = "musicfiles.librarytrackformat";
  static constexpr auto SETTING_MUSICFILES_FINDREMOTETHUMBS = "musicfiles.findremotethumbs";
  static constexpr auto SETTING_AUDIOCDS_AUTOACTION = "audiocds.autoaction";
  static constexpr auto SETTING_AUDIOCDS_USECDDB = "audiocds.usecddb";
  static constexpr auto SETTING_AUDIOCDS_RECORDINGPATH = "audiocds.recordingpath";
  static constexpr auto SETTING_AUDIOCDS_TRACKPATHFORMAT = "audiocds.trackpathformat";
  static constexpr auto SETTING_AUDIOCDS_ENCODER = "audiocds.encoder";
  static constexpr auto SETTING_AUDIOCDS_SETTINGS = "audiocds.settings";
  static constexpr auto SETTING_AUDIOCDS_EJECTONRIP = "audiocds.ejectonrip";
  static constexpr auto SETTING_MYMUSIC_SONGTHUMBINVIS = "mymusic.songthumbinvis";
  static constexpr auto SETTING_MYMUSIC_DEFAULTLIBVIEW = "mymusic.defaultlibview";
  static constexpr auto SETTING_PICTURES_USETAGS = "pictures.usetags";
  static constexpr auto SETTING_PICTURES_GENERATETHUMBS = "pictures.generatethumbs";
  static constexpr auto SETTING_PICTURES_SHOWVIDEOS = "pictures.showvideos";
  static constexpr auto SETTING_PICTURES_DISPLAYRESOLUTION = "pictures.displayresolution";
  static constexpr auto SETTING_SLIDESHOW_STAYTIME = "slideshow.staytime";
  static constexpr auto SETTING_SLIDESHOW_DISPLAYEFFECTS = "slideshow.displayeffects";
  static constexpr auto SETTING_SLIDESHOW_SHUFFLE = "slideshow.shuffle";
  static constexpr auto SETTING_SLIDESHOW_HIGHQUALITYDOWNSCALING =
      "slideshow.highqualitydownscaling";
  static constexpr auto SETTING_WEATHER_CURRENTLOCATION = "weather.currentlocation";
  static constexpr auto SETTING_WEATHER_ADDON = "weather.addon";
  static constexpr auto SETTING_WEATHER_ADDONSETTINGS = "weather.addonsettings";
  static constexpr auto SETTING_SERVICES_DEVICENAME = "services.devicename";
  static constexpr auto SETTING_SERVICES_DEVICEUUID = "services.deviceuuid";
  static constexpr auto SETTING_SERVICES_UPNP = "services.upnp";
  static constexpr auto SETTING_SERVICES_UPNPSERVER = "services.upnpserver";
  static constexpr auto SETTING_SERVICES_UPNPANNOUNCE = "services.upnpannounce";
  static constexpr auto SETTING_SERVICES_UPNPLOOKFOREXTERNALSUBTITLES =
      "services.upnplookforexternalsubtitles";
  static constexpr auto SETTING_SERVICES_UPNPCONTROLLER = "services.upnpcontroller";
  static constexpr auto SETTING_SERVICES_UPNPPLAYERVOLUMESYNC = "services.upnpplayervolumesync";
  static constexpr auto SETTING_SERVICES_UPNPRENDERER = "services.upnprenderer";
  static constexpr auto SETTING_SERVICES_WEBSERVER = "services.webserver";
  static constexpr auto SETTING_SERVICES_WEBSERVERPORT = "services.webserverport";
  static constexpr auto SETTING_SERVICES_WEBSERVERAUTHENTICATION =
      "services.webserverauthentication";
  static constexpr auto SETTING_SERVICES_WEBSERVERUSERNAME = "services.webserverusername";
  static constexpr auto SETTING_SERVICES_WEBSERVERPASSWORD = "services.webserverpassword";
  static constexpr auto SETTING_SERVICES_WEBSERVERSSL = "services.webserverssl";
  static constexpr auto SETTING_SERVICES_WEBSKIN = "services.webskin";
  static constexpr auto SETTING_SERVICES_ESENABLED = "services.esenabled";
  static constexpr auto SETTING_SERVICES_ESPORT = "services.esport";
  static constexpr auto SETTING_SERVICES_ESPORTRANGE = "services.esportrange";
  static constexpr auto SETTING_SERVICES_ESMAXCLIENTS = "services.esmaxclients";
  static constexpr auto SETTING_SERVICES_ESALLINTERFACES = "services.esallinterfaces";
  static constexpr auto SETTING_SERVICES_ESINITIALDELAY = "services.esinitialdelay";
  static constexpr auto SETTING_SERVICES_ESCONTINUOUSDELAY = "services.escontinuousdelay";
  static constexpr auto SETTING_SERVICES_ZEROCONF = "services.zeroconf";
  static constexpr auto SETTING_SERVICES_AIRPLAY = "services.airplay";
  static constexpr auto SETTING_SERVICES_AIRPLAYVOLUMECONTROL = "services.airplayvolumecontrol";
  static constexpr auto SETTING_SERVICES_USEAIRPLAYPASSWORD = "services.useairplaypassword";
  static constexpr auto SETTING_SERVICES_AIRPLAYPASSWORD = "services.airplaypassword";
  static constexpr auto SETTING_SERVICES_AIRPLAYVIDEOSUPPORT = "services.airplayvideosupport";
  static constexpr auto SETTING_SMB_WINSSERVER = "smb.winsserver";
  static constexpr auto SETTING_SMB_WORKGROUP = "smb.workgroup";
  static constexpr auto SETTING_SMB_MINPROTOCOL = "smb.minprotocol";
  static constexpr auto SETTING_SMB_MAXPROTOCOL = "smb.maxprotocol";
  static constexpr auto SETTING_SMB_LEGACYSECURITY = "smb.legacysecurity";
  static constexpr auto SETTING_SMB_CHUNKSIZE = "smb.chunksize";
  static constexpr auto SETTING_SERVICES_WSDISCOVERY = "services.wsdiscovery";
  static constexpr auto SETTING_VIDEOSCREEN_MONITOR = "videoscreen.monitor";
  static constexpr auto SETTING_VIDEOSCREEN_SCREEN = "videoscreen.screen";
  static constexpr auto SETTING_VIDEOSCREEN_WHITELIST = "videoscreen.whitelist";
  static constexpr auto SETTING_VIDEOSCREEN_RESOLUTION = "videoscreen.resolution";
  static constexpr auto SETTING_VIDEOSCREEN_SCREENMODE = "videoscreen.screenmode";
  static constexpr auto SETTING_VIDEOSCREEN_FAKEFULLSCREEN = "videoscreen.fakefullscreen";
  static constexpr auto SETTING_VIDEOSCREEN_BLANKDISPLAYS = "videoscreen.blankdisplays";
  static constexpr auto SETTING_VIDEOSCREEN_STEREOSCOPICMODE = "videoscreen.stereoscopicmode";
  static constexpr auto SETTING_VIDEOSCREEN_PREFEREDSTEREOSCOPICMODE =
      "videoscreen.preferedstereoscopicmode";
  static constexpr auto SETTING_VIDEOSCREEN_NOOFBUFFERS = "videoscreen.noofbuffers";
  static constexpr auto SETTING_VIDEOSCREEN_3DLUT = "videoscreen.cms3dlut";
  static constexpr auto SETTING_VIDEOSCREEN_DISPLAYPROFILE = "videoscreen.displayprofile";
  static constexpr auto SETTING_VIDEOSCREEN_GUICALIBRATION = "videoscreen.guicalibration";
  static constexpr auto SETTING_VIDEOSCREEN_TESTPATTERN = "videoscreen.testpattern";
  static constexpr auto SETTING_VIDEOSCREEN_LIMITEDRANGE = "videoscreen.limitedrange";
  static constexpr auto SETTING_VIDEOSCREEN_FRAMEPACKING = "videoscreen.framepacking";
  static constexpr auto SETTING_VIDEOSCREEN_10BITSURFACES = "videoscreen.10bitsurfaces";
  static constexpr auto SETTING_VIDEOSCREEN_USESYSTEMSDRPEAKLUMINANCE =
      "videoscreen.usesystemsdrpeakluminance";
  static constexpr auto SETTING_VIDEOSCREEN_GUISDRPEAKLUMINANCE = "videoscreen.guipeakluminance";
  static constexpr auto SETTING_VIDEOSCREEN_DITHER = "videoscreen.dither";
  static constexpr auto SETTING_VIDEOSCREEN_DITHERDEPTH = "videoscreen.ditherdepth";
  static constexpr auto SETTING_AUDIOOUTPUT_AUDIODEVICE = "audiooutput.audiodevice";
  static constexpr auto SETTING_AUDIOOUTPUT_CHANNELS = "audiooutput.channels";
  static constexpr auto SETTING_AUDIOOUTPUT_CONFIG = "audiooutput.config";
  static constexpr auto SETTING_AUDIOOUTPUT_SAMPLERATE = "audiooutput.samplerate";
  static constexpr auto SETTING_AUDIOOUTPUT_STEREOUPMIX = "audiooutput.stereoupmix";
  static constexpr auto SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME =
      "audiooutput.maintainoriginalvolume";
  static constexpr auto SETTING_AUDIOOUTPUT_PROCESSQUALITY = "audiooutput.processquality";
  static constexpr auto SETTING_AUDIOOUTPUT_ATEMPOTHRESHOLD = "audiooutput.atempothreshold";
  static constexpr auto SETTING_AUDIOOUTPUT_STREAMSILENCE = "audiooutput.streamsilence";
  static constexpr auto SETTING_AUDIOOUTPUT_STREAMNOISE = "audiooutput.streamnoise";
  static constexpr auto SETTING_AUDIOOUTPUT_GUISOUNDMODE = "audiooutput.guisoundmode";
  static constexpr auto SETTING_AUDIOOUTPUT_GUISOUNDVOLUME = "audiooutput.guisoundvolume";
  static constexpr auto SETTING_AUDIOOUTPUT_PASSTHROUGH = "audiooutput.passthrough";
  static constexpr auto SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE = "audiooutput.passthroughdevice";
  static constexpr auto SETTING_AUDIOOUTPUT_AC3PASSTHROUGH = "audiooutput.ac3passthrough";
  static constexpr auto SETTING_AUDIOOUTPUT_AC3TRANSCODE = "audiooutput.ac3transcode";
  static constexpr auto SETTING_AUDIOOUTPUT_EAC3PASSTHROUGH = "audiooutput.eac3passthrough";
  static constexpr auto SETTING_AUDIOOUTPUT_DTSPASSTHROUGH = "audiooutput.dtspassthrough";
  static constexpr auto SETTING_AUDIOOUTPUT_TRUEHDPASSTHROUGH = "audiooutput.truehdpassthrough";
  static constexpr auto SETTING_AUDIOOUTPUT_DTSHDPASSTHROUGH = "audiooutput.dtshdpassthrough";
  static constexpr auto SETTING_AUDIOOUTPUT_DTSHDCOREFALLBACK = "audiooutput.dtshdcorefallback";
  static constexpr auto SETTING_AUDIOOUTPUT_VOLUMESTEPS = "audiooutput.volumesteps";
  static constexpr auto SETTING_INPUT_PERIPHERALS = "input.peripherals";
  static constexpr auto SETTING_INPUT_PERIPHERALLIBRARIES = "input.peripherallibraries";
  static constexpr auto SETTING_INPUT_ENABLEMOUSE = "input.enablemouse";
  static constexpr auto SETTING_INPUT_ASKNEWCONTROLLERS = "input.asknewcontrollers";
  static constexpr auto SETTING_INPUT_CONTROLLERCONFIG = "input.controllerconfig";
  static constexpr auto SETTING_INPUT_RUMBLENOTIFY = "input.rumblenotify";
  static constexpr auto SETTING_INPUT_TESTRUMBLE = "input.testrumble";
  static constexpr auto SETTING_INPUT_CONTROLLERPOWEROFF = "input.controllerpoweroff";
  static constexpr auto SETTING_INPUT_APPLEREMOTEMODE = "input.appleremotemode";
  static constexpr auto SETTING_INPUT_APPLEREMOTEALWAYSON = "input.appleremotealwayson";
  static constexpr auto SETTING_INPUT_APPLEREMOTESEQUENCETIME = "input.appleremotesequencetime";
  static constexpr auto SETTING_INPUT_SIRIREMOTEIDLETIMERENABLED = "input.siriremoteidletimerenabled";
  static constexpr auto SETTING_INPUT_SIRIREMOTEIDLETIME = "input.siriremoteidletime";
  static constexpr auto SETTING_INPUT_SIRIREMOTEHORIZONTALSENSITIVITY =
      "input.siriremotehorizontalsensitivity";
  static constexpr auto SETTING_INPUT_SIRIREMOTEVERTICALSENSITIVITY =
      "input.siriremoteverticalsensitivity";
  static constexpr auto SETTING_INPUT_TVOSUSEKODIKEYBOARD = "input.tvosusekodikeyboard";
  static constexpr auto SETTING_NETWORK_USEHTTPPROXY = "network.usehttpproxy";
  static constexpr auto SETTING_NETWORK_HTTPPROXYTYPE = "network.httpproxytype";
  static constexpr auto SETTING_NETWORK_HTTPPROXYSERVER = "network.httpproxyserver";
  static constexpr auto SETTING_NETWORK_HTTPPROXYPORT = "network.httpproxyport";
  static constexpr auto SETTING_NETWORK_HTTPPROXYUSERNAME = "network.httpproxyusername";
  static constexpr auto SETTING_NETWORK_HTTPPROXYPASSWORD = "network.httpproxypassword";
  static constexpr auto SETTING_NETWORK_BANDWIDTH = "network.bandwidth";
  static constexpr auto SETTING_POWERMANAGEMENT_DISPLAYSOFF = "powermanagement.displaysoff";
  static constexpr auto SETTING_POWERMANAGEMENT_SHUTDOWNTIME = "powermanagement.shutdowntime";
  static constexpr auto SETTING_POWERMANAGEMENT_SHUTDOWNSTATE = "powermanagement.shutdownstate";
  static constexpr auto SETTING_POWERMANAGEMENT_WAKEONACCESS = "powermanagement.wakeonaccess";
  static constexpr auto SETTING_POWERMANAGEMENT_WAITFORNETWORK = "powermanagement.waitfornetwork";
  static constexpr auto SETTING_DEBUG_SHOWLOGINFO = "debug.showloginfo";
  static constexpr auto SETTING_DEBUG_EXTRALOGGING = "debug.extralogging";
  static constexpr auto SETTING_DEBUG_SETEXTRALOGLEVEL = "debug.setextraloglevel";
  static constexpr auto SETTING_DEBUG_SCREENSHOTPATH = "debug.screenshotpath";
  static constexpr auto SETTING_DEBUG_SHARE_LOG = "debug.sharelog";
  static constexpr auto SETTING_EVENTLOG_ENABLED = "eventlog.enabled";
  static constexpr auto SETTING_EVENTLOG_ENABLED_NOTIFICATIONS = "eventlog.enablednotifications";
  static constexpr auto SETTING_EVENTLOG_SHOW = "eventlog.show";
  static constexpr auto SETTING_MASTERLOCK_LOCKCODE = "masterlock.lockcode";
  static constexpr auto SETTING_MASTERLOCK_STARTUPLOCK = "masterlock.startuplock";
  static constexpr auto SETTING_MASTERLOCK_MAXRETRIES = "masterlock.maxretries";
  static constexpr auto SETTING_CACHE_HARDDISK = "cache.harddisk";
  static constexpr auto SETTING_CACHEVIDEO_DVDROM = "cachevideo.dvdrom";
  static constexpr auto SETTING_CACHEVIDEO_LAN = "cachevideo.lan";
  static constexpr auto SETTING_CACHEVIDEO_INTERNET = "cachevideo.internet";
  static constexpr auto SETTING_CACHEAUDIO_DVDROM = "cacheaudio.dvdrom";
  static constexpr auto SETTING_CACHEAUDIO_LAN = "cacheaudio.lan";
  static constexpr auto SETTING_CACHEAUDIO_INTERNET = "cacheaudio.internet";
  static constexpr auto SETTING_CACHEDVD_DVDROM = "cachedvd.dvdrom";
  static constexpr auto SETTING_CACHEDVD_LAN = "cachedvd.lan";
  static constexpr auto SETTING_CACHEUNKNOWN_INTERNET = "cacheunknown.internet";
  static constexpr auto SETTING_SYSTEM_PLAYLISTSPATH = "system.playlistspath";
  static constexpr auto SETTING_ADDONS_AUTOUPDATES = "general.addonupdates";
  static constexpr auto SETTING_ADDONS_NOTIFICATIONS = "general.addonnotifications";
  static constexpr auto SETTING_ADDONS_SHOW_RUNNING = "addons.showrunning";
  static constexpr auto SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES = "addons.unknownsources";
  static constexpr auto SETTING_ADDONS_UPDATEMODE = "addons.updatemode";
  static constexpr auto SETTING_ADDONS_MANAGE_DEPENDENCIES = "addons.managedependencies";
  static constexpr auto SETTING_ADDONS_REMOVE_ORPHANED_DEPENDENCIES =
      "addons.removeorphaneddependencies";
  static constexpr auto SETTING_GENERAL_ADDONFOREIGNFILTER = "general.addonforeignfilter";
  static constexpr auto SETTING_GENERAL_ADDONBROKENFILTER = "general.addonbrokenfilter";
  static constexpr auto SETTING_SOURCE_VIDEOS = "source.videos";
  static constexpr auto SETTING_SOURCE_MUSIC = "source.music";
  static constexpr auto SETTING_SOURCE_PICTURES = "source.pictures";
  static constexpr auto SETTING_FILECACHE_BUFFERMODE = "filecache.buffermode";
  static constexpr auto SETTING_FILECACHE_MEMORYSIZE = "filecache.memorysize"; // in MBytes
  static constexpr auto SETTING_FILECACHE_READFACTOR = "filecache.readfactor"; // as integer (x100)
  static constexpr auto SETTING_FILECACHE_CHUNKSIZE = "filecache.chunksize"; // in Bytes

  // values for SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS
  static const int VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_MOVIES = 0;
  static const int VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_TVSHOWEPISODES = 1;
  static const int VIDEOLIBRARY_THUMB_SHOW_UNWATCHED_EPISODE = 2;
  // values for SETTING_VIDEOLIBRARY_ARTWORK_LEVEL
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_ALL = 0;
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_BASIC = 1;
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_CUSTOM = 2;
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_NONE = 3;

  // values for SETTING_MUSICLIBRARY_ARTWORKLEVEL
  static const int MUSICLIBRARY_ARTWORK_LEVEL_ALL = 0;
  static const int MUSICLIBRARY_ARTWORK_LEVEL_BASIC = 1;
  static const int MUSICLIBRARY_ARTWORK_LEVEL_CUSTOM = 2;
  static const int MUSICLIBRARY_ARTWORK_LEVEL_NONE = 3;

  // values for SETTING_VIDEOPLAYER_AUTOPLAYNEXTITEM
  static constexpr int SETTING_AUTOPLAYNEXT_MUSICVIDEOS = 0;
  static constexpr int SETTING_AUTOPLAYNEXT_TVSHOWS = 1;
  static constexpr int SETTING_AUTOPLAYNEXT_EPISODES = 2;
  static constexpr int SETTING_AUTOPLAYNEXT_MOVIES = 3;
  static constexpr int SETTING_AUTOPLAYNEXT_UNCATEGORIZED = 4;

  // values for SETTING_VIDEOPLAYER_ALLOWEDHDRFORMATS
  static const int VIDEOPLAYER_ALLOWED_HDR_TYPE_DOLBY_VISION = 0;
  static const int VIDEOPLAYER_ALLOWED_HDR_TYPE_HDR10PLUS = 1;

  /*!
   \brief Creates a new settings wrapper around a new settings manager.

   For access to the "global" settings wrapper the static GetInstance() method should
   be used.
   */
  CSettings() = default;
  ~CSettings() override = default;

  CSettingsManager* GetSettingsManager() const { return m_settingsManager; }

  // specialization of CSettingsBase
  bool Initialize() override;

  /*!
   \brief Registers the given ISubSettings implementation.

   \param subSettings ISubSettings implementation
   */
  void RegisterSubSettings(ISubSettings* subSettings);
  /*!
   \brief Unregisters the given ISubSettings implementation.

   \param subSettings ISubSettings implementation
   */
  void UnregisterSubSettings(ISubSettings* subSettings);

  // implementations of CSettingsBase
  bool Load() override;
  bool Save() override;

  /*!
   \brief Loads setting values from the given (XML) file.

   \param file Path to an XML file containing setting values
   \return True if the setting values were successfully loaded, false otherwise
   */
  bool Load(const std::string &file);
  /*!
  \brief Loads setting values from the given XML element.

  \param root XML element containing setting values
  \return True if the setting values were successfully loaded, false otherwise
  */
  bool Load(const TiXmlElement* root);
  /*!
   \brief Loads setting values from the given XML element.

   \param root XML element containing setting values
   \param hide Whether to hide the loaded settings or not
   \return True if the setting values were successfully loaded, false otherwise
   */
  bool LoadHidden(const TiXmlElement *root) { return CSettingsBase::LoadHiddenValuesFromXml(root); }

  /*!
   \brief Saves the setting values to the given (XML) file.

   \param file Path to an XML file
   \return True if the setting values were successfully saved, false otherwise
   */
  bool Save(const std::string &file);
  /*!
   \brief Saves the setting values to the given XML node.

   \param root XML node
   \return True if the setting values were successfully saved, false otherwise
   */
  bool Save(TiXmlNode* root) const override;

  /*!
   \brief Loads the setting being represented by the given XML node with the
   given identifier.

   \param node XML node representing the setting to load
   \param settingId Setting identifier
   \return True if the setting was successfully loaded from the given XML node, false otherwise
   */
  bool LoadSetting(const TiXmlNode *node, const std::string &settingId);

  // overwrite (not override) from CSettingsBase
  bool GetBool(const std::string& id) const;

  /*!
   \brief Clears the complete settings.

   This removes all initialized settings, groups, categories and sections and
   returns to the uninitialized state. Any registered callbacks or
   implementations stay registered.
   */
  void Clear() override;

protected:
  // specializations of CSettingsBase
  void InitializeSettingTypes() override;
  void InitializeControls() override;
  void InitializeOptionFillers() override;
  void UninitializeOptionFillers() override;
  void InitializeConditions() override;
  void UninitializeConditions() override;
  void InitializeDefaults() override;
  void InitializeISettingsHandlers() override;
  void UninitializeISettingsHandlers() override;
  void InitializeISubSettings() override;
  void UninitializeISubSettings() override;
  void InitializeISettingCallbacks() override;
  void UninitializeISettingCallbacks() override;

  // implementation of CSettingsBase
  bool InitializeDefinitions() override;

private:
  CSettings(const CSettings&) = delete;
  CSettings const& operator=(CSettings const&) = delete;

  bool Load(const TiXmlElement* root, bool& updated);

  // implementation of ISubSettings
  bool Load(const TiXmlNode* settings) override;

  bool Initialize(const std::string &file);
  bool Reset();

  std::set<ISubSettings*> m_subSettings;
};
