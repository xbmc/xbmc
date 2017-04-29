#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/*!
 * Common data structures shared between KODI and KODI's audio DSP add-ons
 */

#ifndef TARGET_WINDOWS
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

#include <cstddef>

#include "xbmc_addon_types.h"

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#define PRAGMA_PACK 0
#endif
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif

#define AE_DSP_ADDON_STRING_LENGTH              1024

#define AE_DSP_STREAM_MAX_STREAMS               8
#define AE_DSP_STREAM_MAX_MODES                 32

/* current Audio DSP API version */
#define KODI_AE_DSP_API_VERSION                 ADDON_INSTANCE_VERSION_ADSP

/* min. Audio DSP API version */
#define KODI_AE_DSP_MIN_API_VERSION             ADDON_INSTANCE_VERSION_ADSP

#ifdef __cplusplus
extern "C" {
#endif

  typedef unsigned int AE_DSP_STREAM_ID;

  /*!
   * @brief Audio DSP add-on error codes
   */
  typedef enum
  {
    AE_DSP_ERROR_NO_ERROR             = 0,      /*!< @brief no error occurred */
    AE_DSP_ERROR_UNKNOWN              = -1,     /*!< @brief an unknown error occurred */
    AE_DSP_ERROR_IGNORE_ME            = -2,     /*!< @brief the used input stream can not processed and add-on want to ignore */
    AE_DSP_ERROR_NOT_IMPLEMENTED      = -3,     /*!< @brief the method that KODI called is not implemented by the add-on */
    AE_DSP_ERROR_REJECTED             = -4,     /*!< @brief the command was rejected by the DSP */
    AE_DSP_ERROR_INVALID_PARAMETERS   = -5,     /*!< @brief the parameters of the method that was called are invalid for this operation */
    AE_DSP_ERROR_INVALID_SAMPLERATE   = -6,     /*!< @brief the processed samplerate is not supported */
    AE_DSP_ERROR_INVALID_IN_CHANNELS  = -7,     /*!< @brief the processed input channel format is not supported */
    AE_DSP_ERROR_INVALID_OUT_CHANNELS = -8,     /*!< @brief the processed output channel format is not supported */
    AE_DSP_ERROR_FAILED               = -9,     /*!< @brief the command failed */
  } AE_DSP_ERROR;

  /*!
   * @brief The possible DSP channels (used as pointer inside arrays)
   */
  typedef enum
  {
    AE_DSP_CH_INVALID = -1,
    AE_DSP_CH_FL = 0,
    AE_DSP_CH_FR,
    AE_DSP_CH_FC,
    AE_DSP_CH_LFE,
    AE_DSP_CH_BL,
    AE_DSP_CH_BR,
    AE_DSP_CH_FLOC,
    AE_DSP_CH_FROC,
    AE_DSP_CH_BC,
    AE_DSP_CH_SL,
    AE_DSP_CH_SR,
    AE_DSP_CH_TFL,
    AE_DSP_CH_TFR,
    AE_DSP_CH_TFC,
    AE_DSP_CH_TC,
    AE_DSP_CH_TBL,
    AE_DSP_CH_TBR,
    AE_DSP_CH_TBC,
    AE_DSP_CH_BLOC,
    AE_DSP_CH_BROC,

    AE_DSP_CH_MAX
  } AE_DSP_CHANNEL;

  /*!
   * @brief Present channel flags
   */
  typedef enum
  {
    AE_DSP_PRSNT_CH_UNDEFINED = 0,
    AE_DSP_PRSNT_CH_FL        = 1<<0,
    AE_DSP_PRSNT_CH_FR        = 1<<1,
    AE_DSP_PRSNT_CH_FC        = 1<<2,
    AE_DSP_PRSNT_CH_LFE       = 1<<3,
    AE_DSP_PRSNT_CH_BL        = 1<<4,
    AE_DSP_PRSNT_CH_BR        = 1<<5,
    AE_DSP_PRSNT_CH_FLOC      = 1<<6,
    AE_DSP_PRSNT_CH_FROC      = 1<<7,
    AE_DSP_PRSNT_CH_BC        = 1<<8,
    AE_DSP_PRSNT_CH_SL        = 1<<9,
    AE_DSP_PRSNT_CH_SR        = 1<<10,
    AE_DSP_PRSNT_CH_TFL       = 1<<11,
    AE_DSP_PRSNT_CH_TFR       = 1<<12,
    AE_DSP_PRSNT_CH_TFC       = 1<<13,
    AE_DSP_PRSNT_CH_TC        = 1<<14,
    AE_DSP_PRSNT_CH_TBL       = 1<<15,
    AE_DSP_PRSNT_CH_TBR       = 1<<16,
    AE_DSP_PRSNT_CH_TBC       = 1<<17,
    AE_DSP_PRSNT_CH_BLOC      = 1<<18,
    AE_DSP_PRSNT_CH_BROC      = 1<<19
  } AE_DSP_CHANNEL_PRESENT;

  /**
   * @brief The various stream type formats
   * Used for audio DSP processing to know input audio type
   */
  typedef enum
  {
    AE_DSP_ASTREAM_INVALID = -1,
    AE_DSP_ASTREAM_BASIC = 0,
    AE_DSP_ASTREAM_MUSIC,
    AE_DSP_ASTREAM_MOVIE,
    AE_DSP_ASTREAM_GAME,
    AE_DSP_ASTREAM_APP,
    AE_DSP_ASTREAM_PHONE,
    AE_DSP_ASTREAM_MESSAGE,

    AE_DSP_ASTREAM_AUTO,
    AE_DSP_ASTREAM_MAX
  } AE_DSP_STREAMTYPE;

  /*!
   * @brief Add-ons supported audio stream type flags
   * used on master mode information on AE_DSP_MODES to know
   * on which audio stream the master mode is supported
   */
  typedef enum
  {
    AE_DSP_PRSNT_ASTREAM_BASIC    = 1<<0,
    AE_DSP_PRSNT_ASTREAM_MUSIC    = 1<<1,
    AE_DSP_PRSNT_ASTREAM_MOVIE    = 1<<2,
    AE_DSP_PRSNT_ASTREAM_GAME     = 1<<3,
    AE_DSP_PRSNT_ASTREAM_APP      = 1<<4,
    AE_DSP_PRSNT_ASTREAM_MESSAGE  = 1<<5,
    AE_DSP_PRSNT_ASTREAM_PHONE    = 1<<6,
  } AE_DSP_ASTREAM_PRESENT;

  /**
   * @brief The various base type formats
   * Used for audio DSP processing to know input audio source
   */
  typedef enum
  {
    AE_DSP_ABASE_INVALID = -1,
    AE_DSP_ABASE_STEREO = 0,
    AE_DSP_ABASE_MONO,
    AE_DSP_ABASE_MULTICHANNEL,
    AE_DSP_ABASE_AC3,
    AE_DSP_ABASE_EAC3,
    AE_DSP_ABASE_DTS,
    AE_DSP_ABASE_DTSHD_MA,
    AE_DSP_ABASE_DTSHD_HRA,
    AE_DSP_ABASE_TRUEHD,
    AE_DSP_ABASE_MLP,
    AE_DSP_ABASE_FLAC,

    AE_DSP_ABASE_MAX
  } AE_DSP_BASETYPE;


  /**
   * @brief The from KODI in settings requested audio process quality.
   * The KODI internal used quality levels is translated to this values
   * for usage on DSP processing add-ons. Is present on iQualityLevel
   * inside AE_DSP_SETTINGS.
   */
  typedef enum
  {
    AE_DSP_QUALITY_UNKNOWN    = -1,             /*!< @brief  Unset, unknown or incorrect quality level */
    AE_DSP_QUALITY_DEFAULT    =  0,             /*!< @brief  Engine's default quality level */

    /* Basic quality levels */
    AE_DSP_QUALITY_LOW        = 20,             /*!< @brief  Low quality level */
    AE_DSP_QUALITY_MID        = 30,             /*!< @brief  Standard quality level */
    AE_DSP_QUALITY_HIGH       = 50,             /*!< @brief  Best sound processing quality */

    /* Optional quality levels */
    AE_DSP_QUALITY_REALLYHIGH = 100             /*!< @brief  Uncompromising optional quality level, usually with unmeasurable and unnoticeable improvement */
  } AE_DSP_QUALITY;

  /*!
   * @brief Audio DSP menu hook categories.
   * Used to identify on AE_DSP_MENUHOOK given add-on related skin dialog/windows.
   * Except AE_DSP_MENUHOOK_ALL and AE_DSP_MENUHOOK_SETTING are the menus available
   * from DSP playback dialogue which can be opened over KODI file context menu and over
   * button on full screen OSD window.
   *
   * Menu hook AE_DSP_MENUHOOK_SETTING is available from DSP processing setup dialogue.
   */
  typedef enum
  {
    AE_DSP_MENUHOOK_UNKNOWN         =-1,        /*!< @brief unknown menu hook */
    AE_DSP_MENUHOOK_ALL             = 0,        /*!< @brief all categories */
    AE_DSP_MENUHOOK_PRE_PROCESS     = 1,        /*!< @brief for pre processing */
    AE_DSP_MENUHOOK_MASTER_PROCESS  = 2,        /*!< @brief for master processing */
    AE_DSP_MENUHOOK_POST_PROCESS    = 3,        /*!< @brief for post processing */
    AE_DSP_MENUHOOK_RESAMPLE        = 4,        /*!< @brief for re sample */
    AE_DSP_MENUHOOK_MISCELLANEOUS   = 5,        /*!< @brief for miscellaneous dialogues */
    AE_DSP_MENUHOOK_INFORMATION     = 6,        /*!< @brief dialogue to show processing information */
    AE_DSP_MENUHOOK_SETTING         = 7,        /*!< @brief for settings */
  } AE_DSP_MENUHOOK_CAT;

  /*!
   * @brief Menu hooks that are available in the menus while playing a stream via this add-on.
   */
  typedef struct AE_DSP_MENUHOOK
  {
    unsigned int        iHookId;                /*!< @brief (required) this hook's identifier */
    unsigned int        iLocalizedStringId;     /*!< @brief (required) the id of the label for this hook in g_localizeStrings */
    AE_DSP_MENUHOOK_CAT category;               /*!< @brief (required) category of menu hook */
    unsigned int        iRelevantModeId;        /*!< @brief (required) except category AE_DSP_MENUHOOK_SETTING and AE_DSP_MENUHOOK_ALL must be the related mode id present here */
    bool                bNeedPlayback;          /*!< @brief (required) set to true if menu hook need playback and active processing */
  } ATTRIBUTE_PACKED AE_DSP_MENUHOOK;

  /*!
   * @brief Properties passed to the Create() method of an add-on.
   */
  typedef struct AE_DSP_PROPERTIES
  {
    const char* strUserPath;                    /*!< @brief path to the user profile */
    const char* strAddonPath;                   /*!< @brief path to this add-on */
  } AE_DSP_PROPERTIES;

  /*!
   * @brief Audio DSP add-on capabilities. All capabilities are set to "false" as default.
   * If a capability is set to true, then the corresponding methods from kodi_audiodsp_dll.h need to be implemented.
   */
  typedef struct AE_DSP_ADDON_CAPABILITIES
  {
    bool bSupportsInputProcess;                 /*!< @brief true if this add-on provides audio input processing */
    bool bSupportsInputResample;                /*!< @brief true if this add-on provides audio resample before master handling */
    bool bSupportsPreProcess;                   /*!< @brief true if this add-on provides audio pre processing */
    bool bSupportsMasterProcess;                /*!< @brief true if this add-on provides audio master processing */
    bool bSupportsPostProcess;                  /*!< @brief true if this add-on provides audio post processing */
    bool bSupportsOutputResample;               /*!< @brief true if this add-on provides audio re sample after master handling */
  } ATTRIBUTE_PACKED AE_DSP_ADDON_CAPABILITIES;

  /*!
   * @brief Audio processing settings for in and out arrays
   * Send on creation and before first processed audio packet to add-on
   */
  typedef struct AE_DSP_SETTINGS
  {
    AE_DSP_STREAM_ID  iStreamID;                /*!< @brief id of the audio stream packets */
    AE_DSP_STREAMTYPE iStreamType;              /*!< @brief the input stream type source eg, Movie or Music */
    int               iInChannels;              /*!< @brief the amount of input channels */
    unsigned long     lInChannelPresentFlags;   /*!< @brief the exact channel mapping flags of input */
    int               iInFrames;                /*!< @brief the input frame size from KODI */
    unsigned int      iInSamplerate;            /*!< @brief the basic sample rate of the audio packet */
    int               iProcessFrames;           /*!< @brief the processing frame size inside add-on's */
    unsigned int      iProcessSamplerate;       /*!< @brief the sample rate after input resample present in master processing */
    int               iOutChannels;             /*!< @brief the amount of output channels */
    unsigned long     lOutChannelPresentFlags;  /*!< @brief the exact channel mapping flags for output */
    int               iOutFrames;               /*!< @brief the final out frame size for KODI */
    unsigned int      iOutSamplerate;           /*!< @brief the final sample rate of the audio packet */
    bool              bInputResamplingActive;   /*!< @brief if a re-sampling is performed before master processing this flag is set to true */
    bool              bStereoUpmix;             /*!< @brief true if the stereo upmix setting on kodi is set */
    int               iQualityLevel;            /*!< @brief the from KODI selected quality level for signal processing */
    /*!
     * @note about "iProcessSamplerate" and "iProcessFrames" is set from KODI after call of StreamCreate on input re sample add-on, if re-sampling
     * and processing is handled inside the same add-on, this value must be ignored!
     */
  } ATTRIBUTE_PACKED AE_DSP_SETTINGS;

  /*!
   * @brief Stream profile properties
   * Can be used to detect best master processing mode and for post processing methods.
   */
//@{

  /*!
   * @brief Dolby profile types. Given from several formats, e.g. Dolby Digital or TrueHD
   * Used on AE_DSP_PROFILE_AC3_EAC3 and AE_DSP_PROFILE_MLP_TRUEHD
   */
  #define AE_DSP_PROFILE_DOLBY_NONE             0
  #define AE_DSP_PROFILE_DOLBY_SURROUND         1
  #define AE_DSP_PROFILE_DOLBY_PLII             2
  #define AE_DSP_PROFILE_DOLBY_PLIIX            3
  #define AE_DSP_PROFILE_DOLBY_PLIIZ            4
  #define AE_DSP_PROFILE_DOLBY_EX               5
  #define AE_DSP_PROFILE_DOLBY_HEADPHONE        6

  /*!
   * @brief DTS/DTS HD profile types
   * Used on AE_DSP_PROFILE_DTS_DTSHD
   */
  #define AE_DSP_PROFILE_DTS                    0
  #define AE_DSP_PROFILE_DTS_ES                 1
  #define AE_DSP_PROFILE_DTS_96_24              2
  #define AE_DSP_PROFILE_DTS_HD_HRA             3
  #define AE_DSP_PROFILE_DTS_HD_MA              4

  /*!
   * @brief AC3/EAC3 based service types
   * Used on AE_DSP_PROFILE_AC3_EAC3
   */
  #define AE_DSP_SERVICE_TYPE_MAIN              0
  #define AE_DSP_SERVICE_TYPE_EFFECTS           1
  #define AE_DSP_SERVICE_TYPE_VISUALLY_IMPAIRED 2
  #define AE_DSP_SERVICE_TYPE_HEARING_IMPAIRED  3
  #define AE_DSP_SERVICE_TYPE_DIALOGUE          4
  #define AE_DSP_SERVICE_TYPE_COMMENTARY        5
  #define AE_DSP_SERVICE_TYPE_EMERGENCY         6
  #define AE_DSP_SERVICE_TYPE_VOICE_OVER        7
  #define AE_DSP_SERVICE_TYPE_KARAOKE           8

  /*!
   * @brief AC3/EAC3 based room types
   * Present on AE_DSP_PROFILE_AC3_EAC3 and can be used for frequency corrections
   * at post processing, e.g. THX Re-Equalization
   */
  #define AE_DSP_ROOM_TYPE_UNDEFINED            0
  #define AE_DSP_ROOM_TYPE_SMALL                1
  #define AE_DSP_ROOM_TYPE_LARGE                2

  /*!
   * @brief AC3/EAC3 stream profile properties
   */
  //! @todo add handling for it (currently never becomes set)
  typedef struct AE_DSP_PROFILE_AC3_EAC3
  {
    unsigned int iProfile;                      /*!< defined by AE_DSP_PROFILE_DOLBY_* */
    unsigned int iServiceType;                  /*!< defined by AE_DSP_SERVICE_TYPE_* */
    unsigned int iRoomType;                     /*!< defined by AE_DSP_ROOM_TYPE_* (NOTICE: Information about it currently not supported from ffmpeg and must be implemented) */
  } ATTRIBUTE_PACKED AE_DSP_PROFILE_AC3_EAC3;

  /*!
   * @brief MLP/Dolby TrueHD stream profile properties
   */
  //! @todo add handling for it (currently never becomes set)
  typedef struct AE_DSP_PROFILE_MLP_TRUEHD
  {
    unsigned int iProfile;                      /*!< defined by AE_DSP_PROFILE_DOLBY_* */
  } ATTRIBUTE_PACKED AE_DSP_PROFILE_MLP_TRUEHD;

  /*!
   * @brief DTS/DTS HD stream profile properties
   */
  //! @todo add handling for it (currently never becomes set)
  typedef struct AE_DSP_PROFILE_DTS_DTSHD
  {
    unsigned int iProfile;                      /*!< defined by AE_DSP_PROFILE_DTS* */
    bool bSurroundMatrix;                       /*!< if set to true given 2.0 stream is surround encoded */
  } ATTRIBUTE_PACKED AE_DSP_PROFILE_DTS_DTSHD;

  union AE_DSP_PROFILE
  {
    AE_DSP_PROFILE_AC3_EAC3   ac3_eac3;         /*!< Dolby Digital/Digital+ profile data */
    AE_DSP_PROFILE_MLP_TRUEHD mlp_truehd;       /*!< MLP or Dolby TrueHD profile data */
    AE_DSP_PROFILE_DTS_DTSHD  dts_dtshd;        /*!< DTS/DTS-HD profile data */
  };
  //@}

  /*!
   * @brief Audio DSP stream properties
   * Used to check for the DSP add-on that the stream is supported,
   * as example Dolby Digital Ex processing is only required on Dolby Digital with 5.1 layout
   */
  typedef struct AE_DSP_STREAM_PROPERTIES
  {
    AE_DSP_STREAM_ID  iStreamID;                  /*!< @brief stream id of the audio stream packets */
    AE_DSP_STREAMTYPE iStreamType;                /*!< @brief the input stream type source eg, Movie or Music */
    int               iBaseType;                  /*!< @brief the input stream base type source eg, Dolby Digital */
    const char*       strName;                    /*!< @brief the audio stream name */
    const char*       strCodecId;                 /*!< @brief codec id string of the audio stream */
    const char*       strLanguage;                /*!< @brief language id of the audio stream */
    int               iIdentifier;                /*!< @brief audio stream id inside player */
    int               iChannels;                  /*!< @brief amount of basic channels */
    int               iSampleRate;                /*!< @brief sample rate */
    AE_DSP_PROFILE    Profile;                    /*!< @brief current running stream profile data */
  } ATTRIBUTE_PACKED AE_DSP_STREAM_PROPERTIES;

  /*!
   * @brief Audio DSP mode categories
   */
  typedef enum
  {
    AE_DSP_MODE_TYPE_UNDEFINED       = -1,       /*!< @brief undefined type, never be used from add-on! */
    AE_DSP_MODE_TYPE_INPUT_RESAMPLE  = 0,        /*!< @brief for input re sample */
    AE_DSP_MODE_TYPE_PRE_PROCESS     = 1,        /*!< @brief for preprocessing */
    AE_DSP_MODE_TYPE_MASTER_PROCESS  = 2,        /*!< @brief for master processing */
    AE_DSP_MODE_TYPE_POST_PROCESS    = 3,        /*!< @brief for post processing */
    AE_DSP_MODE_TYPE_OUTPUT_RESAMPLE = 4,        /*!< @brief for output re sample */
    AE_DSP_MODE_TYPE_MAX             = 5
  } AE_DSP_MODE_TYPE;

  /*!
   * @brief Audio DSP master mode information
   * Used to get all available modes for current input stream
   */
  typedef struct AE_DSP_MODES
  {
    unsigned int iModesCount;                                             /*!< @brief (required) count of how much modes are in AE_DSP_MODES */
    struct AE_DSP_MODE
    {
      int               iUniqueDBModeId;                                  /*!< @brief (required) the inside add-on used identifier for the mode, set by KODI's audio DSP database */
      AE_DSP_MODE_TYPE  iModeType;                                        /*!< @brief (required) the processing mode type, see AE_DSP_MODE_TYPE */
      char              strModeName[AE_DSP_ADDON_STRING_LENGTH];          /*!< @brief (required) the addon name of the mode, used on KODI's logs  */

      unsigned int      iModeNumber;                                      /*!< @brief (required) number of this mode on the add-on, is used on process functions with value "mode_id" */
      unsigned int      iModeSupportTypeFlags;                            /*!< @brief (required) flags about supported input types for this mode, see AE_DSP_ASTREAM_PRESENT */
      bool              bHasSettingsDialog;                               /*!< @brief (required) if setting dialog(s) are available it must be set to true */
      bool              bIsDisabled;                                      /*!< @brief (optional) true if this mode is marked as disabled and not enabled default, only relevant for master processes, all other types always disabled as default */

      unsigned int      iModeName;                                        /*!< @brief (required) the name id of the mode for this hook in g_localizeStrings */
      unsigned int      iModeSetupName;                                   /*!< @brief (optional) the name id of the mode inside settings for this hook in g_localizeStrings */
      unsigned int      iModeDescription;                                 /*!< @brief (optional) the description id of the mode for this hook in g_localizeStrings */
      unsigned int      iModeHelp;                                        /*!< @brief (optional) help string id for inside DSP settings dialog of the mode for this hook in g_localizeStrings */

      char              strOwnModeImage[AE_DSP_ADDON_STRING_LENGTH];      /*!< @brief (optional) flag image for the mode */
      char              strOverrideModeImage[AE_DSP_ADDON_STRING_LENGTH]; /*!< @brief (optional) image to override KODI Image for the mode, eg. Dolby Digital with Dolby Digital Ex (only used on master modes) */
    } mode[AE_DSP_STREAM_MAX_MODES];                                      /*!< @brief Modes array storage */
  } ATTRIBUTE_PACKED AE_DSP_MODES;

  /*!
   * @brief Audio DSP menu hook data
   */
  typedef struct AE_DSP_MENUHOOK_DATA
  {
    AE_DSP_MENUHOOK_CAT category;                                         /*!< @brief (required) related menuhook data category */
    union data {
      AE_DSP_STREAM_ID  iStreamId;                                        /*!< @brief currently only stream id is used, is used as union to have extension possibility */
    } data;                                                               /*!< @brief related category related data */
  } ATTRIBUTE_PACKED AE_DSP_MENUHOOK_DATA;

  /*!
   * @brief Structure to transfer the methods from kodi_audiodsp_dll.h to KODI
   */
  typedef struct KodiToAddonFuncTable_AudioDSP
  {
    const char*  (__cdecl* GetAudioDSPAPIVersion)                (void);
    const char*  (__cdecl* GetMinimumAudioDSPAPIVersion)         (void);
    const char*  (__cdecl* GetGUIAPIVersion)                     (void);
    const char*  (__cdecl* GetMinimumGUIAPIVersion)              (void);
    AE_DSP_ERROR (__cdecl* GetAddonCapabilities)                 (AE_DSP_ADDON_CAPABILITIES*);
    const char*  (__cdecl* GetDSPName)                           (void);
    const char*  (__cdecl* GetDSPVersion)                        (void);
    AE_DSP_ERROR (__cdecl* MenuHook)                             (const AE_DSP_MENUHOOK&, const AE_DSP_MENUHOOK_DATA&);

    AE_DSP_ERROR (__cdecl* StreamCreate)                         (const AE_DSP_SETTINGS*, const AE_DSP_STREAM_PROPERTIES*, ADDON_HANDLE);
    AE_DSP_ERROR (__cdecl* StreamDestroy)                        (const ADDON_HANDLE);
    AE_DSP_ERROR (__cdecl* StreamIsModeSupported)                (const ADDON_HANDLE, AE_DSP_MODE_TYPE, unsigned int, int);
    AE_DSP_ERROR (__cdecl* StreamInitialize)                     (const ADDON_HANDLE, const AE_DSP_SETTINGS*);

    bool         (__cdecl* InputProcess)                         (const ADDON_HANDLE, const float**, unsigned int);

    unsigned int (__cdecl* InputResampleProcessNeededSamplesize) (const ADDON_HANDLE);
    unsigned int (__cdecl* InputResampleProcess)                 (const ADDON_HANDLE, float**, float**, unsigned int);
    float        (__cdecl* InputResampleGetDelay)                (const ADDON_HANDLE);
    int          (__cdecl* InputResampleSampleRate)              (const ADDON_HANDLE);

    unsigned int (__cdecl* PreProcessNeededSamplesize)           (const ADDON_HANDLE, unsigned int);
    float        (__cdecl* PreProcessGetDelay)                   (const ADDON_HANDLE, unsigned int);
    unsigned int (__cdecl* PreProcess)                           (const ADDON_HANDLE, unsigned int, float**, float**, unsigned int);

    AE_DSP_ERROR (__cdecl* MasterProcessSetMode)                 (const ADDON_HANDLE, AE_DSP_STREAMTYPE, unsigned int, int);
    unsigned int (__cdecl* MasterProcessNeededSamplesize)        (const ADDON_HANDLE);
    float        (__cdecl* MasterProcessGetDelay)                (const ADDON_HANDLE);
    int          (__cdecl* MasterProcessGetOutChannels)          (const ADDON_HANDLE, unsigned long&);
    unsigned int (__cdecl* MasterProcess)                        (const ADDON_HANDLE, float**, float**, unsigned int);
    const char*  (__cdecl* MasterProcessGetStreamInfoString)     (const ADDON_HANDLE);

    unsigned int (__cdecl* PostProcessNeededSamplesize)          (const ADDON_HANDLE, unsigned int);
    float        (__cdecl* PostProcessGetDelay)                  (const ADDON_HANDLE, unsigned int);
    unsigned int (__cdecl* PostProcess)                          (const ADDON_HANDLE, unsigned int, float**, float**, unsigned int);

    unsigned int (__cdecl* OutputResampleProcessNeededSamplesize)(const ADDON_HANDLE);
    unsigned int (__cdecl* OutputResampleProcess)                (const ADDON_HANDLE, float**, float**, unsigned int);
    float        (__cdecl* OutputResampleGetDelay)               (const ADDON_HANDLE);
    int          (__cdecl* OutputResampleSampleRate)             (const ADDON_HANDLE);
  } KodiToAddonFuncTable_AudioDSP;

#ifdef __cplusplus
}
#endif

