#pragma once
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

#include <set>
#include <string>
#include <vector>

#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "utils/GlobalsHandling.h"

class CVariant;

class TiXmlElement;
namespace ADDON
{
  class IAddon;
}

class DatabaseSettings
{
public:
  void Reset()
  {
    type.clear();
    host.clear();
    port.clear();
    user.clear();
    pass.clear();
    name.clear();
    key.clear();
    cert.clear();
    ca.clear();
    capath.clear();
    ciphers.clear();
    compression = false;
  };
  std::string type;
  std::string host;
  std::string port;
  std::string user;
  std::string pass;
  std::string name;
  std::string key;
  std::string cert;
  std::string ca;
  std::string capath;
  std::string ciphers;
  bool compression;
};

struct TVShowRegexp
{
  bool byDate;
  std::string regexp;
  int defaultSeason;
  TVShowRegexp(bool d, const std::string& r, int s = 1):
    regexp(r)
  {
    byDate = d;
    defaultSeason = s;
  }
};

struct RefreshOverride
{
  float fpsmin;
  float fpsmax;

  float refreshmin;
  float refreshmax;

  bool  fallback;
};


struct RefreshVideoLatency
{
  float refreshmin;
  float refreshmax;

  float delay;
};

struct StagefrightConfig
{
  int useAVCcodec;
  int useHEVCcodec;
  int useVC1codec;
  int useVPXcodec;
  int useMP4codec;
  int useMPEG2codec;
  bool useSwRenderer;
  bool useInputDTS;
};

typedef std::vector<TVShowRegexp> SETTINGS_TVSHOWLIST;

class CAdvancedSettings : public ISettingCallback, public ISettingsHandler
{
  public:
    CAdvancedSettings();

    static CAdvancedSettings* getInstance();

    virtual void OnSettingsLoaded();
    virtual void OnSettingsUnloaded();

    virtual void OnSettingChanged(const CSetting *setting);

    void Initialize();
    bool Initialized() { return m_initialized; };
    void AddSettingsFile(const std::string &filename);
    bool Load();
    void Clear();

    static void GetCustomTVRegexps(TiXmlElement *pRootElement, SETTINGS_TVSHOWLIST& settings);
    static void GetCustomRegexps(TiXmlElement *pRootElement, std::vector<std::string> &settings);
    static void GetCustomExtensions(TiXmlElement *pRootElement, std::string& extensions);

    bool CanLogComponent(int component) const;
    static void SettingOptionsLoggingComponentsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

    int m_audioHeadRoom;
    float m_ac3Gain;
    std::string m_audioDefaultPlayer;
    float m_audioPlayCountMinimumPercent;
    bool m_dvdplayerIgnoreDTSinWAV;
    float m_limiterHold;
    float m_limiterRelease;

    bool  m_omxHWAudioDecode;
    bool  m_omxDecodeStartWithValidFrame;

    float m_videoSubsDelayRange;
    float m_videoAudioDelayRange;
    bool m_videoUseTimeSeeking;
    int m_videoTimeSeekForward;
    int m_videoTimeSeekBackward;
    int m_videoTimeSeekForwardBig;
    int m_videoTimeSeekBackwardBig;
    int m_videoPercentSeekForward;
    int m_videoPercentSeekBackward;
    int m_videoPercentSeekForwardBig;
    int m_videoPercentSeekBackwardBig;
    std::vector<int> m_seekSteps;
    std::string m_videoPPFFmpegDeint;
    std::string m_videoPPFFmpegPostProc;
    bool m_videoVDPAUtelecine;
    bool m_videoVDPAUdeintSkipChromaHD;
    bool m_musicUseTimeSeeking;
    int m_musicTimeSeekForward;
    int m_musicTimeSeekBackward;
    int m_musicTimeSeekForwardBig;
    int m_musicTimeSeekBackwardBig;
    int m_musicPercentSeekForward;
    int m_musicPercentSeekBackward;
    int m_musicPercentSeekForwardBig;
    int m_musicPercentSeekBackwardBig;
    int m_videoBlackBarColour;
    int m_videoIgnoreSecondsAtStart;
    float m_videoIgnorePercentAtEnd;
    float m_audioApplyDrc;
    bool m_useFfmpegVda;

    int   m_videoVDPAUScaling;
    bool  m_videoVAAPIforced;
    float m_videoNonLinStretchRatio;
    bool  m_videoEnableHighQualityHwScalers;
    float m_videoAutoScaleMaxFps;
    std::vector<RefreshOverride> m_videoAdjustRefreshOverrides;
    std::vector<RefreshVideoLatency> m_videoRefreshLatency;
    float m_videoDefaultLatency;
    bool m_videoDisableBackgroundDeinterlace;
    int  m_videoCaptureUseOcclusionQuery;
    bool m_DXVACheckCompatibility;
    bool m_DXVACheckCompatibilityPresent;
    bool m_DXVAForceProcessorRenderer;
    bool m_DXVANoDeintProcForProgressive;
    bool m_DXVAAllowHqScaling;
    int  m_videoFpsDetect;
    int  m_videoBusyDialogDelay_ms;
    StagefrightConfig m_stagefrightConfig;
    bool m_mediacodecForceSoftwareRendring;

    std::string m_videoDefaultPlayer;
    std::string m_videoDefaultDVDPlayer;
    float m_videoPlayCountMinimumPercent;

    float m_slideshowBlackBarCompensation;
    float m_slideshowZoomAmount;
    float m_slideshowPanAmount;

    int m_songInfoDuration;
    int m_logLevel;
    int m_logLevelHint;
    bool m_extraLogEnabled;
    int m_extraLogLevels;
    std::string m_cddbAddress;

    //airtunes + airplay
    int m_airTunesPort;
    int m_airPlayPort;

    bool m_handleMounting;

    bool m_fullScreenOnMovieStart;
    std::string m_cachePath;
    std::string m_videoCleanDateTimeRegExp;
    std::vector<std::string> m_videoCleanStringRegExps;
    std::vector<std::string> m_videoExcludeFromListingRegExps;
    std::vector<std::string> m_moviesExcludeFromScanRegExps;
    std::vector<std::string> m_tvshowExcludeFromScanRegExps;
    std::vector<std::string> m_audioExcludeFromListingRegExps;
    std::vector<std::string> m_audioExcludeFromScanRegExps;
    std::vector<std::string> m_pictureExcludeFromListingRegExps;
    std::vector<std::string> m_videoStackRegExps;
    std::vector<std::string> m_folderStackRegExps;
    std::vector<std::string> m_trailerMatchRegExps;
    SETTINGS_TVSHOWLIST m_tvshowEnumRegExps;
    std::string m_tvshowMultiPartEnumRegExp;
    typedef std::vector< std::pair<std::string, std::string> > StringMapping;
    StringMapping m_pathSubstitutions;
    int m_remoteDelay; ///< \brief number of remote messages to ignore before repeating
    float m_controllerDeadzone;

    bool m_playlistAsFolders;
    bool m_detectAsUdf;

    unsigned int m_fanartRes; ///< \brief the maximal resolution to cache fanart at (assumes 16x9)
    unsigned int m_imageRes;  ///< \brief the maximal resolution to cache images at (assumes 16x9)
    /*! \brief the maximal size to cache thumbs at, assuming square
     Used for actual thumbs (eg bookmark thumbs, picture thumbs) rather than cover art which uses m_imageRes instead
     */
    unsigned int GetThumbSize() const { return m_imageRes / 2; };
    bool m_useDDSFanart;

    int m_sambaclienttimeout;
    std::string m_sambadoscodepage;
    bool m_sambastatfiles;

    bool m_bHTTPDirectoryStatFilesize;

    bool m_bFTPThumbs;

    std::string m_musicThumbs;
    std::string m_fanartImages;

    int m_iMusicLibraryRecentlyAddedItems;
    bool m_bMusicLibraryAllItemsOnBottom;
    bool m_bMusicLibraryAlbumsSortByArtistThenYear;
    bool m_bMusicLibraryCleanOnUpdate;
    std::string m_strMusicLibraryAlbumFormat;
    std::string m_strMusicLibraryAlbumFormatRight;
    bool m_prioritiseAPEv2tags;
    std::string m_musicItemSeparator;
    std::string m_videoItemSeparator;
    std::vector<std::string> m_musicTagsFromFileFilters;

    bool m_bVideoLibraryAllItemsOnBottom;
    int m_iVideoLibraryRecentlyAddedItems;
    bool m_bVideoLibraryHideEmptySeries;
    bool m_bVideoLibraryCleanOnUpdate;
    bool m_bVideoLibraryUseFastHash;
    bool m_bVideoLibraryExportAutoThumbs;
    bool m_bVideoLibraryImportWatchedState;
    bool m_bVideoLibraryImportResumePoint;

    bool m_bVideoScannerIgnoreErrors;
    int m_iVideoLibraryDateAdded;

    std::set<std::string> m_vecTokens;

    int m_iEpgLingerTime;           // minutes
    int m_iEpgUpdateCheckInterval;  // seconds
    int m_iEpgCleanupInterval;      // seconds
    int m_iEpgActiveTagCheckInterval; // seconds
    int m_iEpgRetryInterruptedUpdateInterval; // seconds
    int m_iEpgUpdateEmptyTagsInterval; // seconds
    bool m_bEpgDisplayUpdatePopup;
    bool m_bEpgDisplayIncrementalUpdatePopup;

    // EDL Commercial Break
    bool m_bEdlMergeShortCommBreaks;
    int m_iEdlMaxCommBreakLength;   // seconds
    int m_iEdlMinCommBreakLength;   // seconds
    int m_iEdlMaxCommBreakGap;      // seconds
    int m_iEdlMaxStartGap;          // seconds
    int m_iEdlCommBreakAutowait;    // seconds
    int m_iEdlCommBreakAutowind;    // seconds

    int m_curlconnecttimeout;
    int m_curllowspeedtime;
    int m_curlretries;
    bool m_curlDisableIPV6;

    bool m_fullScreen;
    bool m_startFullScreen;
    bool m_showExitButton; /* Ideal for appliances to hide a 'useless' button */
    bool m_canWindowed;
    bool m_splashImage;
    bool m_alwaysOnTop;  /* makes xbmc to run always on top .. osx/win32 only .. */
    int m_playlistRetries;
    int m_playlistTimeout;
    bool m_GLRectangleHack;
    int m_iSkipLoopFilter;
    float m_ForcedSwapTime; /* if nonzero, set's the explicit time in ms to allocate for buffer swap */

    bool m_AllowD3D9Ex;
    bool m_ForceD3D9Ex;
    bool m_AllowDynamicTextures;
    unsigned int m_RestrictCapsMask;
    float m_sleepBeforeFlip; ///< if greather than zero, XBMC waits for raster to be this amount through the frame prior to calling the flip
    bool m_bVirtualShares;

    float m_karaokeSyncDelayCDG; // seems like different delay is needed for CDG and MP3s
    float m_karaokeSyncDelayLRC;
    bool m_karaokeChangeGenreForKaraokeSongs;
    bool m_karaokeKeepDelay; // store user-changed song delay in the database
    int m_karaokeStartIndex; // auto-assign numbering start from this value
    bool m_karaokeAlwaysEmptyOnCdgs; // always have empty background on CDG files
    bool m_karaokeUseSongSpecificBackground; // use song-specific video or image if available instead of default
    std::string m_karaokeDefaultBackgroundType; // empty string or "vis", "image" or "video"
    std::string m_karaokeDefaultBackgroundFilePath; // only for "image" or "video" types above

    std::string m_cpuTempCmd;
    std::string m_gpuTempCmd;

    /* PVR/TV related advanced settings */
    int m_iPVRTimeCorrection;     /*!< @brief correct all times (epg tags, timer tags, recording tags) by this amount of minutes. defaults to 0. */
    int m_iPVRInfoToggleInterval; /*!< @brief if there are more than 1 pvr gui info item available (e.g. multiple recordings active at the same time), use this toggle delay in milliseconds. defaults to 3000. */
    int m_iPVRMinVideoCacheLevel;      /*!< @brief cache up to this level in the video buffer buffer before resuming playback if the buffers run dry */
    int m_iPVRMinAudioCacheLevel;      /*!< @brief cache up to this level in the audio buffer before resuming playback if the buffers run dry */
    bool m_bPVRCacheInDvdPlayer; /*!< @brief true to use "CACHESTATE_PVR" in CDVDPlayer (default) */
    bool m_bPVRChannelIconsAutoScan; /*!< @brief automatically scan user defined folder for channel icons when loading internal channel groups */
    bool m_bPVRAutoScanIconsUserSet; /*!< @brief mark channel icons populated by auto scan as "user set" */
    int m_iPVRNumericChannelSwitchTimeout; /*!< @brief time in ms before the numeric dialog auto closes when confirmchannelswitch is disabled */

    DatabaseSettings m_databaseMusic; // advanced music database setup
    DatabaseSettings m_databaseVideo; // advanced video database setup
    DatabaseSettings m_databaseTV;    // advanced tv database setup
    DatabaseSettings m_databaseEpg;   /*!< advanced EPG database setup */

    bool m_guiVisualizeDirtyRegions;
    int  m_guiAlgorithmDirtyRegions;
    int  m_guiDirtyRegionNoFlipTimeout;
    unsigned int m_addonPackageFolderSize;

    unsigned int m_cacheMemBufferSize;
    unsigned int m_networkBufferMode;
    float m_readBufferFactor;

    bool m_jsonOutputCompact;
    unsigned int m_jsonTcpPort;

    bool m_enableMultimediaKeys;
    std::vector<std::string> m_settingsFiles;
    void ParseSettingsFile(const std::string &file);

    float GetDisplayLatency(float refreshrate);
    bool m_initialized;

    //! \brief Returns a list of music extension for filtering in the GUI
    std::string GetMusicExtensions() const;

    void SetDebugMode(bool debug);

    // runtime settings which cannot be set from advancedsettings.xml
    std::string m_pictureExtensions;
    std::string m_videoExtensions;
    std::string m_discStubExtensions;
    std::string m_subtitlesExtensions;

    std::string m_stereoscopicregex_3d;
    std::string m_stereoscopicregex_sbs;
    std::string m_stereoscopicregex_tab;

    /*!< @brief position behavior of ass subtitiles when setting "subtitle position on screen" set to "fixed"
    True to show at the fixed position set in video calibration
    False to show at the bottom of video (default) */
    bool m_videoAssFixedWorks;

    std::string m_logFolder;

    std::string m_userAgent;

  private:
    std::string m_musicExtensions;
    void setExtraLogLevel(const std::vector<CVariant> &components);
};

XBMC_GLOBAL(CAdvancedSettings,g_advancedSettings);
