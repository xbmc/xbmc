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

#include "../AddonBase.h"

#define AE_DSP_STREAM_MAX_STREAMS               8
#define AE_DSP_STREAM_MAX_MODES                 32

/*!
 * @file Addon.h
 * @section sec1 Basic audio dsp addon interface description
 * @author Team Kodi
 * @date 10. May 2014
 * @version 0.1.5
 *
 * @subsection sec1_1 General
 * @li The basic support on the addon is supplied with the
 * AE_DSP_ADDON_CAPABILITIES data which becomes asked over
 * GetCapabilities(...), further the addon must register his available
 * modes on startup with the RegisterMode(...) callback function.
 * If one of this two points is not set the addon becomes
 * ignored for the chain step.
 *
 * @subsection sec1_2 Processing
 * @li On start of new stream the addon becomes called with StreamCreate(...)
 * to check about given values that it support it basically and can create
 * his structure, if nothing is supported it can return AE_DSP_ERROR_IGNORE_ME.
 *
 * @li As next step StreamIsModeSupported(...) becomes called for every
 * available and enabled modes, is separated due to more as one available mode
 * on the addon is possible, if the mode is not supported it can also be return
 * AE_DSP_ERROR_IGNORE_ME.
 *   - If mode is a resample mode and returns no error it becomes asked with
 *     InputResampleSampleRate(...) or OutputResampleSampleRate(...) (relevant
 *     to his type) about his given sample rate.
 *   - About the from user selected master processing mode the related addon
 *     becomes called now with MasterProcessSetMode(...) to handle it's
 *     selectionon the addon given by the own addon type identifier or by
 *     KODI's useddatabase id, also the currently used stream type (e.g.
 *     Music or Video) is send.
 *     - If the addon supports only one master mode it can ignore this function
 *       and return always AE_DSP_ERROR_NO_ERROR.
 *     - If the master mode is set the addon becomes asked about the from him
 *       given output channel layout related to up- or downmix modes, if
 *       nothing becomes changed on the layout it can return -1.
 *     - The MasterProcessSetMode(...) is also called if from user a another
 *       mode becomes selected.
 *
 * @li Then as last step shortly before the first process call becomes executed
 * the addon is called one time with StreamInitialize(...) to inform that
 * processing is started on the given settings.
 *   - This function becomes also called on all add-ons if the master process
 *     becomes changed.
 *   - Also every process after StreamInitialize on the addon mode becomes asked
 *     with _..._ProcessNeededSamplesize(...) about required memory size for the
 *     output of his data, if no other size is required it can return 0.
 *
 * @li From now the processing becomes handled for the different steps with
 * _..._Process(...).
 *   - Further it becomes asked with _..._GetDelay(...) about his processing
 *     time as float value in seconds, needed for video and audio alignment.
 *
 * @li On the end of the processing if the source becomes stopped the
 * StreamDestroy(...) function becomes called on all active processing add-ons.
 *
 * @note
 * The StreamCreate(...) can be becomes called for a new stream before the
 * previous was closed with StreamDestroy(...) ! To have a speed improve.
 */

namespace kodi { namespace addon { class CInstanceAudioDSP; }}

extern "C" {

  typedef void* ADSPHANDLE;

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
    AE_DSP_PRSNT_CH_FL        = 1<<AE_DSP_CH_FL,
    AE_DSP_PRSNT_CH_FR        = 1<<AE_DSP_CH_FR,
    AE_DSP_PRSNT_CH_FC        = 1<<AE_DSP_CH_FC,
    AE_DSP_PRSNT_CH_LFE       = 1<<AE_DSP_CH_LFE,
    AE_DSP_PRSNT_CH_BL        = 1<<AE_DSP_CH_BL,
    AE_DSP_PRSNT_CH_BR        = 1<<AE_DSP_CH_BR,
    AE_DSP_PRSNT_CH_FLOC      = 1<<AE_DSP_CH_FLOC,
    AE_DSP_PRSNT_CH_FROC      = 1<<AE_DSP_CH_FROC,
    AE_DSP_PRSNT_CH_BC        = 1<<AE_DSP_CH_BC,
    AE_DSP_PRSNT_CH_SL        = 1<<AE_DSP_CH_SL,
    AE_DSP_PRSNT_CH_SR        = 1<<AE_DSP_CH_SR,
    AE_DSP_PRSNT_CH_TFL       = 1<<AE_DSP_CH_TFL,
    AE_DSP_PRSNT_CH_TFR       = 1<<AE_DSP_CH_TFR,
    AE_DSP_PRSNT_CH_TFC       = 1<<AE_DSP_CH_TFC,
    AE_DSP_PRSNT_CH_TC        = 1<<AE_DSP_CH_TC,
    AE_DSP_PRSNT_CH_TBL       = 1<<AE_DSP_CH_TBL,
    AE_DSP_PRSNT_CH_TBR       = 1<<AE_DSP_CH_TBR,
    AE_DSP_PRSNT_CH_TBC       = 1<<AE_DSP_CH_TBC,
    AE_DSP_PRSNT_CH_BLOC      = 1<<AE_DSP_CH_BLOC,
    AE_DSP_PRSNT_CH_BROC      = 1<<AE_DSP_CH_BROC
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
    AE_DSP_PRSNT_ASTREAM_BASIC    = 1<<AE_DSP_ASTREAM_BASIC,
    AE_DSP_PRSNT_ASTREAM_MUSIC    = 1<<AE_DSP_ASTREAM_MUSIC,
    AE_DSP_PRSNT_ASTREAM_MOVIE    = 1<<AE_DSP_ASTREAM_MOVIE,
    AE_DSP_PRSNT_ASTREAM_GAME     = 1<<AE_DSP_ASTREAM_GAME,
    AE_DSP_PRSNT_ASTREAM_APP      = 1<<AE_DSP_ASTREAM_APP,
    AE_DSP_PRSNT_ASTREAM_PHONE    = 1<<AE_DSP_ASTREAM_PHONE,
    AE_DSP_PRSNT_ASTREAM_MESSAGE  = 1<<AE_DSP_ASTREAM_MESSAGE,
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
      AE_DSP_MODE_TYPE  iModeType;                                        /*!< @brief (required) the processong mode type, see AE_DSP_MODE_TYPE */
      char              strModeName[ADDON_STANDARD_STRING_LENGTH];        /*!< @brief (required) the addon name of the mode, used on KODI's logs  */

      unsigned int      iModeNumber;                                      /*!< @brief (required) number of this mode on the add-on, is used on process functions with value "mode_id" */
      unsigned int      iModeSupportTypeFlags;                            /*!< @brief (required) flags about supported input types for this mode, see AE_DSP_ASTREAM_PRESENT */
      bool              bHasSettingsDialog;                               /*!< @brief (required) if setting dialog(s) are available it must be set to true */
      bool              bIsDisabled;                                      /*!< @brief (optional) true if this mode is marked as disabled and not enabled default, only relevant for master processes, all other types always disabled as default */

      unsigned int      iModeName;                                        /*!< @brief (required) the name id of the mode for this hook in g_localizeStrings */
      unsigned int      iModeSetupName;                                   /*!< @brief (optional) the name id of the mode inside settings for this hook in g_localizeStrings */
      unsigned int      iModeDescription;                                 /*!< @brief (optional) the description id of the mode for this hook in g_localizeStrings */
      unsigned int      iModeHelp;                                        /*!< @brief (optional) help string id for inside DSP settings dialog of the mode for this hook in g_localizeStrings */

      char              strOwnModeImage[ADDON_STANDARD_STRING_LENGTH];    /*!< @brief (optional) flag image for the mode */
      char              strOverrideModeImage[ADDON_STANDARD_STRING_LENGTH];/*!< @brief (optional) image to override KODI Image for the mode, eg. Dolby Digital with Dolby Digital Ex (only used on master modes) */
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
   * @brief Properties passed to the Create() method of an add-on.
   */
  typedef struct AddonProps_AudioDSP
  {
    const char* strUserPath;                    /*!< @brief path to the user profile */
    const char* strAddonPath;                   /*!< @brief path to this add-on */
  } AddonProps_AudioDSP;

  typedef struct AddonToKodiFuncTable_AudioDSP
  {
    void* kodiInstance;
    void (*add_menu_hook)(void* kodiInstance, AE_DSP_MENUHOOK *hook);
    void (*remove_menu_hook)(void* kodiInstance, AE_DSP_MENUHOOK *hook);
    void (*register_mode)(void* kodiInstance, AE_DSP_MODES::AE_DSP_MODE *mode);
    void (*unregister_mode)(void* kodiInstance, AE_DSP_MODES::AE_DSP_MODE *mode);

    ADSPHANDLE (*SoundPlay_GetHandle)(void* kodiInstance, const char *filename);
    void (*SoundPlay_ReleaseHandle)(void* kodiInstance, ADSPHANDLE handle);
    void (*SoundPlay_Play)(void* kodiInstance, ADSPHANDLE handle);
    void (*SoundPlay_Stop)(void* kodiInstance, ADSPHANDLE handle);
    bool (*SoundPlay_IsPlaying)(void* kodiInstance, ADSPHANDLE handle);
    void (*SoundPlay_SetChannel)(void* kodiInstance, ADSPHANDLE handle, AE_DSP_CHANNEL channel);
    AE_DSP_CHANNEL (*SoundPlay_GetChannel)(void* kodiInstance, ADSPHANDLE handle);
    void (*SoundPlay_SetVolume)(void* kodiInstance, ADSPHANDLE handle, float volume);
    float (*SoundPlay_GetVolume)(void* kodiInstance, ADSPHANDLE handle);
  } AddonToKodiFuncTable_AudioDSP;

  struct AddonInstance_AudioDSP;
  typedef struct KodiToAddonFuncTable_AudioDSP
  {
    kodi::addon::CInstanceAudioDSP* addonInstance;
    void (__cdecl* get_capabilities)(AddonInstance_AudioDSP const* addonInstance, AE_DSP_ADDON_CAPABILITIES*);
    const char* (__cdecl* get_dsp_name)(AddonInstance_AudioDSP const* addonInstance);
    const char* (__cdecl* get_dsp_version)(AddonInstance_AudioDSP const* addonInstance);
    AE_DSP_ERROR (__cdecl* menu_hook)(AddonInstance_AudioDSP const* addonInstance, const AE_DSP_MENUHOOK*, const AE_DSP_MENUHOOK_DATA*);

    AE_DSP_ERROR (__cdecl* stream_create)(AddonInstance_AudioDSP const* addonInstance, const AE_DSP_SETTINGS*, const AE_DSP_STREAM_PROPERTIES*, ADDON_HANDLE);
    AE_DSP_ERROR (__cdecl* stream_destroy)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE);
    AE_DSP_ERROR (__cdecl* stream_is_mode_supported)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, AE_DSP_MODE_TYPE, unsigned int, int);
    AE_DSP_ERROR (__cdecl* stream_initialize)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, const AE_DSP_SETTINGS*);

    bool (__cdecl* input_process)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, const float**, unsigned int);

    unsigned int (__cdecl* input_resample_process_needed_samplesize)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE);
    unsigned int (__cdecl* input_resample_process)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, const float**, float**, unsigned int);
    float (__cdecl* input_resample_get_delay)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE);
    int (__cdecl* input_resample_samplerate)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE);

    unsigned int (__cdecl* pre_process_needed_samplesize)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, unsigned int);
    float (__cdecl* pre_process_get_delay)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, unsigned int);
    unsigned int (__cdecl* pre_process)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, unsigned int, const float**, float**, unsigned int);

    AE_DSP_ERROR (__cdecl* master_process_set_mode)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, AE_DSP_STREAMTYPE, unsigned int, int);
    unsigned int (__cdecl* master_process_needed_samplesize)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE);
    float (__cdecl* master_process_get_delay)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE);
    int (__cdecl* master_process_get_out_channels)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, unsigned long*);
    unsigned int (__cdecl* master_process)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, const float**, float**, unsigned int);
    const char* (__cdecl* master_process_get_stream_info_string)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE);

    unsigned int (__cdecl* post_process_needed_samplesize)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, unsigned int);
    float (__cdecl* post_process_get_delay)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, unsigned int);
    unsigned int (__cdecl* post_process)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, unsigned int, const float**, float**, unsigned int);

    unsigned int (__cdecl* output_resample_process_needed_samplesize)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE);
    unsigned int (__cdecl* output_resample_process)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE, const float**, float**, unsigned int);
    float (__cdecl* output_resample_get_delay)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE);
    int (__cdecl* output_resample_samplerate)(AddonInstance_AudioDSP const* addonInstance, const ADDON_HANDLE);
  } KodiToAddonFuncTable_AudioDSP;

  typedef struct AddonInstance_AudioDSP
  {
    AddonProps_AudioDSP props;
    AddonToKodiFuncTable_AudioDSP toKodi;
    KodiToAddonFuncTable_AudioDSP toAddon;
  } AddonInstance_AudioDSP;

} /* extern "C" */

namespace kodi {
namespace addon {

  class CInstanceAudioDSP : public IAddonInstance
  {
  public:
    //==========================================================================
    /// @brief Class constructor
    ///
    CInstanceAudioDSP()
      : IAddonInstance(ADDON_INSTANCE_ADSP)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceAudioDSP: Creation of more as one in single instance way is not allowed!");

      SetAddonStruct(CAddonBase::m_interface->firstKodiInstance);
      CAddonBase::m_interface->globalSingleInstance = this;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    /// @brief Class constructor
    ///
    /// @param[in] instance             The from Kodi given instance given be
    ///                                 add-on CreateInstance call with instance
    ///                                 id ADDON_INSTANCE_ADSP.
    ///
    CInstanceAudioDSP(KODI_HANDLE instance)
      : IAddonInstance(ADDON_INSTANCE_ADSP)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceAudioDSP: Creation of multiple together with single instance way is not allowed!");

      SetAddonStruct(instance);
    }
    //--------------------------------------------------------------------------

    /*! @name Audio DSP add-on methods */
    //@{
    //==========================================================================
    ///
    /// @brief Get the list of features that this add-on provides.
    /// Called by KODI to query the add-ons capabilities.
    /// Used to check which options should be presented in the DSP, which methods
    /// to call, etc.
    /// All capabilities that the add-on supports should be set to true.
    /// @param capabilities The add-ons capabilities.
    /// @remarks Valid implementation required.
    ///
    virtual void GetCapabilities(AE_DSP_ADDON_CAPABILITIES& capabilities) = 0;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @return The name reported by the back end that will be displayed in the
    /// UI.
    /// @remarks Valid implementation required.
    ///
    virtual std::string GetDSPName() = 0;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @return The version string reported by the back end that will be displayed
    /// in the UI.
    /// @remarks Valid implementation required.
    ///
    virtual std::string GetDSPVersion() = 0;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Call one of the menu hooks (if supported).
    /// Supported AE_DSP_MENUHOOK instances have to be added in ADDON_Create(),
    /// by calling AddMenuHook() on the callback.
    /// @param menuhook The hook to call.
    /// @param item The selected item for which the hook was called.
    /// @return AE_DSP_ERROR_NO_ERROR if the hook was called successfully.
    /// @remarks Optional. Return AE_DSP_ERROR_NOT_IMPLEMENTED if this add-on
    /// won't provide this function.
    ///
    virtual AE_DSP_ERROR MenuHook(const AE_DSP_MENUHOOK& menuhook, const AE_DSP_MENUHOOK_DATA& item) { return AE_DSP_ERROR_NOT_IMPLEMENTED; }
    //--------------------------------------------------------------------------
    //@}

    //==========================================================================
    /// @name DSP processing control, used to open and close a stream
    ///  @remarks Valid implementation required.
    ///
    //@{
    ///
    /// @brief Set up Audio DSP with selected audio settings (use the basic
    /// present audio stream data format).
    /// Used to detect available add-ons for present stream, as example stereo
    /// surround upmix not needed on 5.1 audio stream.
    /// @param addonSettings The add-ons audio settings.
    /// @param properties The properties of the currently playing stream.
    /// @param handle On this becomes addon informated about stream id and can set function addresses which need on calls
    /// @return AE_DSP_ERROR_NO_ERROR if the properties were fetched successfully
    /// and data can be performed. AE_DSP_ERROR_IGNORE_ME if format is not
    /// supported, but without fault.
    /// @remarks Valid implementation required.
    ///
    virtual AE_DSP_ERROR StreamCreate(const AE_DSP_SETTINGS& addonSettings, const AE_DSP_STREAM_PROPERTIES& properties, ADDON_HANDLE handle) = 0;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// Remove the selected id from currently used DSP processes
    /// @param handle identification data for stream
    /// @return AE_DSP_ERROR_NO_ERROR if the becomes found and removed
    /// @remarks Valid implementation required.
    ///
    virtual AE_DSP_ERROR StreamDestroy(const ADDON_HANDLE handle) = 0;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Ask the add-on about a requested processing mode that it is
    /// supported on the current stream. Is called about every add-on mode after
    /// successed StreamCreate.
    /// @param handle identification data for stream
    /// @param type The processing mode type, see AE_DSP_MODE_TYPE for definitions
    /// @param mode_id The mode inside add-on which must be performed on call. Id
    /// is set from add-on by iModeNumber on AE_DSP_MODE structure during
    /// RegisterMode callback,
    /// @param unique_db_mode_id The Mode unique id generated from dsp database.
    /// @return AE_DSP_ERROR_NO_ERROR if the properties were fetched successfully
    /// or if the stream is not supported the add-on must return
    /// AE_DSP_ERROR_IGNORE_ME.
    /// @remarks Valid implementation required.
    ///
    virtual AE_DSP_ERROR StreamIsModeSupported(const ADDON_HANDLE handle, AE_DSP_MODE_TYPE type, unsigned int mode_id, int unique_db_mode_id) = 0;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Set up Audio DSP with selected audio settings (detected on data of
    /// first present audio packet)
    /// @param addonSettings The add-ons audio settings.
    /// @return AE_DSP_ERROR_NO_ERROR if the properties were fetched successfully.
    /// @remarks Valid implementation required.
    ///
    virtual AE_DSP_ERROR StreamInitialize(const ADDON_HANDLE handle, const AE_DSP_SETTINGS& addonSettings) = 0;
    //--------------------------------------------------------------------------

    //@}

    /// @name DSP input processing
    ///  @remarks Only used by KODI if bSupportsInputProcess is set to true.
    ///
    //@{
    //==========================================================================
    ///
    /// @brief DSP input processing
    /// Can be used to have unchanged stream..
    /// All DSP add-ons allowed to-do this.
    /// @param handle identification data for stream
    /// @param array_in Pointer to data memory
    /// @param samples Amount of samples inside array_in
    /// @return true if work was OK
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual bool InputProcess(const ADDON_HANDLE handle, const float** array_in, unsigned int samples) { return true; }
    //--------------------------------------------------------------------------
    //@}

    /// @name DSP pre-resampling
    ///  @remarks Only used by KODI if bSupportsInputResample is set to true.
    ///
    //@{
    //==========================================================================
    ///
    /// @brief If the add-on operate with buffered arrays and the output size can
    /// be higher as the input it becomes asked about needed size before any
    /// InputResampleProcess call.
    /// @param handle identification data for stream
    /// @return The needed size of output array or 0 if no changes within it
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual unsigned int InputResampleProcessNeededSamplesize(const ADDON_HANDLE handle) { return 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief DSP re sample processing before master.
    /// Here a high quality resample can be performed.
    /// Only one DSP add-on is allowed to-do this!
    /// @param handle identification data for stream
    /// @param array_in Pointer to input data memory
    /// @param array_out Pointer to output data memory
    /// @param samples Amount of samples inside array_in
    /// @return Amount of samples processed
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual unsigned int InputResampleProcess(const ADDON_HANDLE handle, const float** array_in, float** array_out, unsigned int samples) { return 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Returns the re-sampling generated new sample rate used before the
    /// master process
    /// @param handle identification data for stream
    /// @return The new sample rate
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual int InputResampleSampleRate(const ADDON_HANDLE handle) { return 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Returns the time in seconds that it will take
    /// for the next added packet to be returned to KODI.
    /// @param handle identification data for stream
    /// @return the delay in seconds
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual float InputResampleGetDelay(const ADDON_HANDLE handle) { return 0.0f; }
    //--------------------------------------------------------------------------
    //@}

    /** @name DSP Pre processing
    *  @remarks Only used by KODI if bSupportsPreProcess is set to true.
    */
    //@{
    //==========================================================================
    ///
    /// @brief If the addon operate with buffered arrays and the output size can
    /// be higher as the input it becomes asked about needed size before any
    /// PreProcess call.
    /// @param handle identification data for stream
    /// @param mode_id The mode inside add-on which must be performed on call. Id
    /// is set from add-on by iModeNumber on AE_DSP_MODE structure during
    /// RegisterMode callback and can be defined from add-on as a structure
    /// pointer or anything else what is needed to find it.
    /// @return The needed size of output array or 0 if no changes within it
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual unsigned int PreProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id) { return 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Returns the time in seconds that it will take
    /// for the next added packet to be returned to KODI.
    /// @param handle identification data for stream
    /// @param mode_id The mode inside add-on which must be performed on call. Id
    /// is set from add-on by iModeNumber on AE_DSP_MODE structure during
    /// RegisterMode callback and can be defined from add-on as a structure
    /// pointer or anything else what is needed to find it.
    /// @return the delay in seconds
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual float PreProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id) { return 0.0f; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief DSP preprocessing
    /// All DSP add-ons allowed to-do this.
    /// @param handle identification data for stream
    /// @param mode_id The mode inside add-on which must be performed on call. Id
    /// is set from add-on by iModeNumber on AE_DSP_MODE structure during
    /// RegisterMode callback and can be defined from add-on as a structure
    /// pointer or anything else what is needed to find it.
    /// @param array_in Pointer to input data memory
    /// @param array_out Pointer to output data memory
    /// @param samples Amount of samples inside array_in
    /// @return Amount of samples processed
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual unsigned int PreProcess(const ADDON_HANDLE handle, unsigned int mode_id, const float** array_in, float** array_out, unsigned int samples) { return 0; }
    //--------------------------------------------------------------------------
    //@}

    /** @name DSP Master processing
    *  @remarks Only used by KODI if bSupportsMasterProcess is set to true.
    */
    //@{
    //==========================================================================
    ///
    /// @brief Set the active master process mode
    /// @param handle identification data for stream
    /// @param type Requested stream type for the selected master mode
    /// @param mode_id The Mode identifier.
    /// @param unique_db_mode_id The Mode unique id generated from DSP database.
    /// @return AE_DSP_ERROR_NO_ERROR if the setup was successful
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual AE_DSP_ERROR MasterProcessSetMode(const ADDON_HANDLE handle, AE_DSP_STREAMTYPE type, unsigned int mode_id, int unique_db_mode_id) { return AE_DSP_ERROR_NOT_IMPLEMENTED; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief If the add-on operate with buffered arrays and the output size can
    /// be higher as the input it becomes asked about needed size before any
    /// MasterProcess call.
    /// @param handle identification data for stream
    /// @return The needed size of output array or 0 if no changes within it
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual unsigned int MasterProcessNeededSamplesize(const ADDON_HANDLE handle) { return 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Returns the time in seconds that it will take
    /// for the next added packet to be returned to KODI.
    /// @param handle identification data for stream
    /// @return the delay in seconds
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual float MasterProcessGetDelay(const ADDON_HANDLE handle) { return 0.0f; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Returns the from selected master mode performed channel alignment
    /// @param handle identification data for stream
    /// @retval out_channel_present_flags the exact channel present flags after
    /// performed up-/downmix
    /// @return the amount channels
    /// @remarks Optional. Must be used and set if a channel up- or downmix is
    /// processed from the active master mode
    ///
    virtual int MasterProcessGetOutChannels(const ADDON_HANDLE handle, unsigned long& out_channel_present_flags) { return 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Master processing becomes performed with it
    /// Here a channel up-mix/down-mix for stereo surround sound can be performed
    /// Only one DSP add-on is allowed to-do this!
    /// @param handle identification data for stream
    /// @param array_in Pointer to input data memory
    /// @param array_out Pointer to output data memory
    /// @param samples Amount of samples inside array_in
    /// @return Amount of samples processed
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual unsigned int MasterProcess(const ADDON_HANDLE handle, const float** array_in, float** array_out, unsigned int samples) { return 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// Used to get a information string about the processed work to show on skin
    /// @return A string to show
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual std::string MasterProcessGetStreamInfoString(const ADDON_HANDLE handle) { return ""; }
    //--------------------------------------------------------------------------
    //@}

    /** @name DSP Post processing
    *  @remarks Only used by KODI if bSupportsPostProcess is set to true.
    */
    //@{
    //==========================================================================
    ///
    /// If the add-on operate with buffered arrays and the output size can be
    /// higher as the input it becomes asked about needed size before any
    /// PostProcess call.
    /// @param handle identification data for stream
    /// @param mode_id The mode inside add-on which must be performed on call. Id
    /// is set from add-on by iModeNumber on AE_DSP_MODE structure during
    /// RegisterMode callback, and can be defined from add-on as a structure
    /// pointer or anything else what is needed to find it.
    /// @return The needed size of output array or 0 if no changes within it
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual unsigned int PostProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id) { return 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// Returns the time in seconds that it will take
    /// for the next added packet to be returned to KODI.
    /// @param handle identification data for stream
    /// @param mode_id The mode inside add-on which must be performed on call. Id
    /// is set from add-on by iModeNumber on AE_DSP_MODE structure during
    /// RegisterMode callback, and can be defined from add-on as a structure
    /// pointer or anything else what is needed to find it.
    /// @return the delay in seconds
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual float PostProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id) { return 0.0f; }
    //--------------------------------------------------------------------------

    //==========================================================================

    ///
    /// @brief DSP post processing
    /// On the post processing can be things performed with additional channel
    /// upmix like 6.1 to 7.1
    /// or frequency/volume corrections, speaker distance handling, equalizer... .
    /// All DSP add-ons allowed to-do this.
    /// @param handle identification data for stream
    /// @param mode_id The mode inside add-on which must be performed on call. Id
    /// is set from add-on by iModeNumber on AE_DSP_MODE structure during
    /// RegisterMode callback, and can be defined from add-on as a structure
    /// pointer or anything else what is needed to find it.
    /// @param array_in Pointer to input data memory
    /// @param array_out Pointer to output data memory
    /// @param samples Amount of samples inside array_in
    /// @return Amount of samples processed
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual unsigned int PostProcess(const ADDON_HANDLE handle, unsigned int mode_id, const float** array_in, float** array_out, unsigned int samples) { return 0; }

    //--------------------------------------------------------------------------
    //@}

    /** @name DSP Post re-sampling
    *  @remarks Only used by KODI if bSupportsOutputResample is set to true.
    */
    //@{
    //==========================================================================
    ///
    /// @brief If the add-on operate with buffered arrays and the output size
    /// can be higher as the input
    /// it becomes asked about needed size before any OutputResampleProcess call.
    /// @param handle identification data for stream
    /// @return The needed size of output array or 0 if no changes within it
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual unsigned int OutputResampleProcessNeededSamplesize(const ADDON_HANDLE handle) { return 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Re-sampling after master processing becomes performed with it if
    /// needed, only
    /// one add-on can perform it.
    /// @param handle identification data for stream
    /// @param array_in Pointer to input data memory
    /// @param array_out Pointer to output data memory
    /// @param samples Amount of samples inside array_in
    /// @return Amount of samples processed
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual unsigned int OutputResampleProcess(const ADDON_HANDLE handle, const float** array_in, float** array_out, unsigned int samples) { return 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Returns the re-sampling generated new sample rate used after the
    /// master process.
    /// @param handle identification data for stream
    /// @return The new sample rate
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual int OutputResampleSampleRate(const ADDON_HANDLE handle) { return 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Returns the time in seconds that it will take for the next added
    /// packet to be returned to KODI.
    /// @param handle identification data for stream
    /// @return the delay in seconds
    /// @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
    /// GetCapabilities
    ///
    virtual float OutputResampleGetDelay(const ADDON_HANDLE handle) { return 0.0f; }
    //--------------------------------------------------------------------------
    //@}

    //==========================================================================
    ///
    /// @brief Add or replace a menu hook for the context menu for this add-on
    /// @param hook The hook to add
    ///
    void AddMenuHook(AE_DSP_MENUHOOK* hook)
    {
      return m_instanceData->toKodi.add_menu_hook(m_instanceData->toKodi.kodiInstance, hook);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Remove a menu hook for the context menu for this add-on
    /// @param hook The hook to remove
    ///
    void RemoveMenuHook(AE_DSP_MENUHOOK* hook)
    {
      return m_instanceData->toKodi.remove_menu_hook(m_instanceData->toKodi.kodiInstance, hook);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Add or replace master mode information inside audio dsp database.
    /// Becomes identifier written inside mode to iModeID if it was 0 (undefined)
    /// @param mode The master mode to add or update inside database
    ///
    void RegisterMode(AE_DSP_MODES::AE_DSP_MODE* mode)
    {
      return m_instanceData->toKodi.register_mode(m_instanceData->toKodi.kodiInstance, mode);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @brief Remove a master mode from audio dsp database
    /// @param mode The Mode to remove
    ///
    void UnregisterMode(AE_DSP_MODES::AE_DSP_MODE* mode)
    {
      return m_instanceData->toKodi.unregister_mode(m_instanceData->toKodi.kodiInstance, mode);
    }
    //--------------------------------------------------------------------------

  private:
    void SetAddonStruct(KODI_HANDLE instance)
    {
      if (instance == nullptr)
        throw std::logic_error("kodi::addon::CInstanceAudioDSP: Null pointer instance passed.");

      m_instanceData = static_cast<AddonInstance_AudioDSP*>(instance);

      m_instanceData->toAddon.get_capabilities = ADDON_GetCapabilities;
      m_instanceData->toAddon.get_dsp_name = ADDON_GetDSPName;
      m_instanceData->toAddon.get_dsp_version = ADDON_GetDSPVersion;
      m_instanceData->toAddon.menu_hook = ADDON_MenuHook;

      m_instanceData->toAddon.stream_create = ADDON_StreamCreate;
      m_instanceData->toAddon.stream_destroy = ADDON_StreamDestroy;
      m_instanceData->toAddon.stream_is_mode_supported = ADDON_StreamIsModeSupported;
      m_instanceData->toAddon.stream_initialize = ADDON_StreamInitialize;

      m_instanceData->toAddon.input_process = ADDON_InputProcess;

      m_instanceData->toAddon.input_resample_process_needed_samplesize = ADDON_InputResampleProcessNeededSamplesize;
      m_instanceData->toAddon.input_resample_process = ADDON_InputResampleProcess;
      m_instanceData->toAddon.input_resample_get_delay = ADDON_InputResampleGetDelay;
      m_instanceData->toAddon.input_resample_samplerate = ADDON_InputResampleSampleRate;

      m_instanceData->toAddon.pre_process_needed_samplesize = ADDON_PreProcessNeededSamplesize;
      m_instanceData->toAddon.pre_process_get_delay = ADDON_PreProcessGetDelay;
      m_instanceData->toAddon.pre_process = ADDON_PreProcess;

      m_instanceData->toAddon.master_process_set_mode = ADDON_MasterProcessSetMode;
      m_instanceData->toAddon.master_process_needed_samplesize = ADDON_MasterProcessNeededSamplesize;
      m_instanceData->toAddon.master_process_get_delay = ADDON_MasterProcessGetDelay;
      m_instanceData->toAddon.master_process_get_out_channels = ADDON_MasterProcessGetOutChannels;
      m_instanceData->toAddon.master_process = ADDON_MasterProcess;
      m_instanceData->toAddon.master_process_get_stream_info_string = ADDON_MasterProcessGetStreamInfoString;

      m_instanceData->toAddon.post_process_needed_samplesize = ADDON_PostProcessNeededSamplesize;
      m_instanceData->toAddon.post_process_get_delay = ADDON_PostProcessGetDelay;
      m_instanceData->toAddon.post_process = ADDON_PostProcess;

      m_instanceData->toAddon.output_resample_process_needed_samplesize = ADDON_OutputResampleProcessNeededSamplesize;
      m_instanceData->toAddon.output_resample_process = ADDON_OutputResampleProcess;
      m_instanceData->toAddon.output_resample_get_delay = ADDON_OutputResampleGetDelay;
      m_instanceData->toAddon.output_resample_samplerate = ADDON_OutputResampleSampleRate;
    }

    static inline void ADDON_GetCapabilities(AddonInstance_AudioDSP const* instance, AE_DSP_ADDON_CAPABILITIES *capabilities)
    {
      instance->toAddon.addonInstance->GetCapabilities(*capabilities);
    }

    static inline const char* ADDON_GetDSPName(AddonInstance_AudioDSP const* instance)
    {
      instance->toAddon.addonInstance->m_dspName = instance->toAddon.addonInstance->GetDSPName();
      return instance->toAddon.addonInstance->m_dspName.c_str();
    }

    static inline const char* ADDON_GetDSPVersion(AddonInstance_AudioDSP const* instance)
    {
      instance->toAddon.addonInstance->m_dspVersion = instance->toAddon.addonInstance->GetDSPVersion();
      return instance->toAddon.addonInstance->m_dspVersion.c_str();
    }

    static inline AE_DSP_ERROR ADDON_MenuHook(AddonInstance_AudioDSP const* instance, const AE_DSP_MENUHOOK* menuhook, const AE_DSP_MENUHOOK_DATA* item)
    {
      return instance->toAddon.addonInstance->MenuHook(*menuhook, *item);
    }

    static inline AE_DSP_ERROR ADDON_StreamCreate(AddonInstance_AudioDSP const* instance, const AE_DSP_SETTINGS *addonSettings, const AE_DSP_STREAM_PROPERTIES* properties, ADDON_HANDLE handle)
    {
      return instance->toAddon.addonInstance->StreamCreate(*addonSettings, *properties, handle);
    }

    static inline AE_DSP_ERROR ADDON_StreamDestroy(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle)
    {
      return instance->toAddon.addonInstance->StreamDestroy(handle);
    }

    static inline AE_DSP_ERROR ADDON_StreamIsModeSupported(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, AE_DSP_MODE_TYPE type, unsigned int mode_id, int unique_db_mode_id)
    {
      return instance->toAddon.addonInstance->StreamIsModeSupported(handle, type, mode_id, unique_db_mode_id);
    }

    static inline AE_DSP_ERROR ADDON_StreamInitialize(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, const AE_DSP_SETTINGS *addonSettings)
    {
      return instance->toAddon.addonInstance->StreamInitialize(handle, *addonSettings);
    }

    static inline bool ADDON_InputProcess(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, const float** array_in, unsigned int samples)
    {
      return instance->toAddon.addonInstance->InputProcess(handle, array_in, samples);
    }

    static inline unsigned int ADDON_InputResampleProcessNeededSamplesize(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle)
    {
      return instance->toAddon.addonInstance->InputResampleProcessNeededSamplesize(handle);
    }

    static inline unsigned int ADDON_InputResampleProcess(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, const float** array_in, float** array_out, unsigned int samples)
    {
      return instance->toAddon.addonInstance->InputResampleProcess(handle, array_in, array_out, samples);
    }

    static inline int ADDON_InputResampleSampleRate(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle)
    {
      return instance->toAddon.addonInstance->InputResampleSampleRate(handle);
    }

    static inline float ADDON_InputResampleGetDelay(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle)
    {
      return instance->toAddon.addonInstance->InputResampleGetDelay(handle);
    }

    static inline unsigned int ADDON_PreProcessNeededSamplesize(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, unsigned int mode_id)
    {
      return instance->toAddon.addonInstance->PreProcessNeededSamplesize(handle, mode_id);
    }

    static inline float ADDON_PreProcessGetDelay(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, unsigned int mode_id)
    {
      return instance->toAddon.addonInstance->PreProcessGetDelay(handle, mode_id);
    }

    static inline unsigned int ADDON_PreProcess(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, unsigned int mode_id, const float** array_in, float** array_out, unsigned int samples)
    {
      return instance->toAddon.addonInstance->PreProcess(handle, mode_id, array_in, array_out, samples);
    }

    static inline AE_DSP_ERROR ADDON_MasterProcessSetMode(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, AE_DSP_STREAMTYPE type, unsigned int mode_id, int unique_db_mode_id)
    {
      return instance->toAddon.addonInstance->MasterProcessSetMode(handle, type, mode_id, unique_db_mode_id);
    }

    static inline unsigned int ADDON_MasterProcessNeededSamplesize(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle)
    {
      return instance->toAddon.addonInstance->MasterProcessNeededSamplesize(handle);
    }

    static inline float ADDON_MasterProcessGetDelay(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle)
    {
      return instance->toAddon.addonInstance->MasterProcessGetDelay(handle);
    }

    static inline int ADDON_MasterProcessGetOutChannels(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, unsigned long* out_channel_present_flags)
    {
      return instance->toAddon.addonInstance->MasterProcessGetOutChannels(handle, *out_channel_present_flags);
    }

    static inline unsigned int ADDON_MasterProcess(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, const float** array_in, float** array_out, unsigned int samples)
    {
      return instance->toAddon.addonInstance->MasterProcess(handle, array_in, array_out, samples);
    }

    static inline const char* ADDON_MasterProcessGetStreamInfoString(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle)
    {
      instance->toAddon.addonInstance->m_streamInfoString = instance->toAddon.addonInstance->MasterProcessGetStreamInfoString(handle);
      return instance->toAddon.addonInstance->m_streamInfoString.c_str();
    }

    static inline unsigned int ADDON_PostProcessNeededSamplesize(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, unsigned int mode_id)
    {
      return instance->toAddon.addonInstance->PostProcessNeededSamplesize(handle, mode_id);
    }

    static inline float ADDON_PostProcessGetDelay(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, unsigned int mode_id)
    {
      return instance->toAddon.addonInstance->PostProcessGetDelay(handle, mode_id);
    }

    static inline unsigned int ADDON_PostProcess(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, unsigned int mode_id, const float** array_in, float** array_out, unsigned int samples)
    {
      return instance->toAddon.addonInstance->PostProcess(handle, mode_id, array_in, array_out, samples);
    }

    static inline unsigned int ADDON_OutputResampleProcessNeededSamplesize(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle)
    {
      return instance->toAddon.addonInstance->OutputResampleProcessNeededSamplesize(handle);
    }

    static inline unsigned int ADDON_OutputResampleProcess(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle, const float** array_in, float** array_out, unsigned int samples)
    {
      return instance->toAddon.addonInstance->OutputResampleProcess(handle, array_in, array_out, samples);
    }

    static inline int ADDON_OutputResampleSampleRate(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle)
    {
      return instance->toAddon.addonInstance->OutputResampleSampleRate(handle);
    }

    static inline float ADDON_OutputResampleGetDelay(AddonInstance_AudioDSP const* instance, const ADDON_HANDLE handle)
    {
      return instance->toAddon.addonInstance->OutputResampleGetDelay(handle);
    }

    std::string m_dspName;
    std::string m_dspVersion;
    std::string m_streamInfoString;
    AddonInstance_AudioDSP* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
