#pragma once
/*
 *      Copyright (C) 2005-2014 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../definitions.hpp"
#include "../.internal/AddonLib_internal.hpp"

API_NAMESPACE

namespace KodiAPI
{
namespace AudioEngine
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AudioEngine_General General
  /// \ingroup CPP_KodiAPI_AudioEngine
  /// @{
  /// @brief <b>Allow use of binary classes and function to use on add-on's</b>
  ///
  /// Permits the use of the required functions of the add-on to Kodi. This class
  /// also contains some functions to the control.
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref kodi/api3/audioengine/General.hpp "#include <kodi/api3/audioengine/General.hpp>"
  /// be included to enjoy it.
  ///

  namespace General
  {
    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AudioEngine_General
    /// @brief Add or replace a menu hook for the context menu for this add-on
    ///
    /// Inserted hooks becomes called from Kodi on audio DSP system with
    /// <b><tt>AE_DSP_ERROR CallMenuHook(const AE_DSP_MENUHOOK& menuhook, const AE_DSP_MENUHOOK_DATA &item);</tt></b>
    ///
    /// Menu hooks that are available in the menus while playing a stream via this add-on.
    ///
    /// @param[in] hook The hook to add
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// Here structure shows the necessary values the Kodi must be handed over
    /// around a menu by the audio DSP system to add.
    /// ~~~~~~~~~~~~~{.cpp}
    /// /*
    ///  * Menu hooks that are available in the controls of audio DSP system on
    ///  * settings and on OSD.
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct AE_DSP_MENUHOOK
    /// {
    ///   unsigned int        iHookId;                /* (required) this hook's identifier */
    ///   unsigned int        iLocalizedStringId;     /* (required) the id of the label for this hook in g_localizeStrings */
    ///   AE_DSP_MENUHOOK_CAT category;               /* (required) category of menu hook */
    ///   unsigned int        iRelevantModeId;        /* (required) except category AE_DSP_MENUHOOK_SETTING and AE_DSP_MENUHOOK_ALL must be the related mode id present here */
    ///   bool                bNeedPlayback;          /* (required) set to true if menu hook need playback and active processing */
    /// } ATTRIBUTE_PACKED AE_DSP_MENUHOOK;
    /// ~~~~~~~~~~~~~
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Audio DSP menu hook categories</b>
    ///
    /// Used to identify on AE_DSP_MENUHOOK given add-on related skin dialog/windows.
    /// Except AE_DSP_MENUHOOK_ALL and AE_DSP_MENUHOOK_SETTING are the menus available
    /// from DSP playback dialogue which can be opened over KODI file context menu and over
    /// button on full screen OSD window.
    ///
    /// Menu hook AE_DSP_MENUHOOK_SETTING is available from DSP processing setup dialogue.
    /// |  enum code:                    | Id | Description:                             |
    /// |-------------------------------:|:--:|:-----------------------------------------|
    /// | AE_DSP_MENUHOOK_UNKNOWN        | -1 | Unknown menu hook                        |
    /// | AE_DSP_MENUHOOK_ALL            | 0  | All categories                           |
    /// | AE_DSP_MENUHOOK_PRE_PROCESS    | 1  | For pre processing                       |
    /// | AE_DSP_MENUHOOK_MASTER_PROCESS | 2  | For master processing                    |
    /// | AE_DSP_MENUHOOK_POST_PROCESS   | 3  | For post processing                      |
    /// | AE_DSP_MENUHOOK_RESAMPLE       | 4  | For re sample                            |
    /// | AE_DSP_MENUHOOK_MISCELLANEOUS  | 5  | For miscellaneous dialogues              |
    /// | AE_DSP_MENUHOOK_INFORMATION    | 6  | Dialogue to show processing information  |
    /// | AE_DSP_MENUHOOK_SETTING        | 7  | For settings                             |
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Code example:</b>
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/audioengine/General.hpp>
    ///
    /// ...
    /// AE_DSP_MENUHOOK hook;
    /// hook.iHookId            = ID_MENU_SPEAKER_GAIN_SETUP;
    /// hook.category           = AE_DSP_MENUHOOK_POST_PROCESS;
    /// hook.iLocalizedStringId = 30011;
    /// hook.iRelevantModeId    = ID_POST_PROCESS_SPEAKER_CORRECTION;
    /// hook.bNeedPlayback      = false;
    /// KodiAPI::AudioEngine::General::AddMenuHook(&hook);
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void AddDSPMenuHook(AE_DSP_MENUHOOK* hook);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_AudioEngine_General
    /// @brief Remove a menu hook for the context menu for this add-on
    /// @param[in] hook The hook to remove
    ///
    void RemoveDSPMenuHook(AE_DSP_MENUHOOK* hook);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_AudioEngine_General
    /// @brief Add or replace mode information inside audio dsp database.
    ///
    /// Becomes identifier written inside mode to iModeID if it was 0 (undefined)
    ///
    /// @param[in] mode The master mode to add or update inside database
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Audio DSP mode information:</b>
    ///
    /// Used to get all available modes for current input stream
    ///
    /// ~~~~~~~~~~~~~{.cpp}
    /// /*
    ///  * Audio DSP mode information
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct AE_DSP_MODES
    /// {
    ///   unsigned int iModesCount;                                             /* (required) count of how much modes are in AE_DSP_MODES */
    ///   struct AE_DSP_MODE
    ///   {
    ///     int               iUniqueDBModeId;                                  /* (required) the inside add-on used identifier for the mode, set by KODI's audio DSP database */
    ///     AE_DSP_MODE_TYPE  iModeType;                                        /* (required) the processong mode type, see AE_DSP_MODE_TYPE */
    ///     char              strModeName[AE_DSP_ADDON_STRING_LENGTH];          /* (required) the addon name of the mode, used on KODI's logs  */
    ///
    ///     unsigned int      iModeNumber;                                      /* (required) number of this mode on the add-on, is used on process functions with value "mode_id" */
    ///     unsigned int      iModeSupportTypeFlags;                            /* (required) flags about supported input types for this mode, see AE_DSP_ASTREAM_PRESENT */
    ///     bool              bHasSettingsDialog;                               /* (required) if setting dialog(s) are available it must be set to true */
    ///     bool              bIsDisabled;                                      /* (optional) true if this mode is marked as disabled and not enabled default, only relevant
    ///                                                                          * for master processes, all other types always disabled as default */
    ///     unsigned int      iModeName;                                        /* (required) the name id of the mode for this hook in g_localizeStrings */
    ///     unsigned int      iModeSetupName;                                   /* (optional) the name id of the mode inside settings for this hook in g_localizeStrings */
    ///     unsigned int      iModeDescription;                                 /* (optional) the description id of the mode for this hook in g_localizeStrings */
    ///     unsigned int      iModeHelp;                                        /* (optional) help string id for inside DSP settings dialog of the mode for
    ///                                                                          * this hook in g_localizeStrings */
    ///     char              strOwnModeImage[AE_DSP_ADDON_STRING_LENGTH];      /* (optional) flag image for the mode */
    ///     char              strOverrideModeImage[AE_DSP_ADDON_STRING_LENGTH]; /* (optional) image to override KODI Image for the mode, eg. Dolby Digital with
    ///                                                                          * Dolby Digital Ex (only used on master modes) */
    ///   } mode[AE_DSP_STREAM_MAX_MODES];                                      /* Modes array storage */
    /// } ATTRIBUTE_PACKED AE_DSP_MODES;
    /// ~~~~~~~~~~~~~
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Code example:</b>
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/audioengine/General.hpp>
    ///
    /// struct AE_DSP_MODES::AE_DSP_MODE m_ModeInfoStruct;
    ///
    /// ...
    /// std::string imagePath;
    ///
    /// m_ModeInfoStruct.iUniqueDBModeId        = -1;         // set by RegisterMode
    /// m_ModeInfoStruct.iModeType              = AE_DSP_MODE_TYPE_MASTER_PROCESS;
    /// m_ModeInfoStruct.iModeNumber            = ID_MASTER_PROCESS_FREE_SURROUND;
    /// m_ModeInfoStruct.bHasSettingsDialog     = true;
    /// m_ModeInfoStruct.iModeDescription       = 30002;
    /// m_ModeInfoStruct.iModeHelp              = 30003;
    /// m_ModeInfoStruct.iModeName              = 30000;
    /// m_ModeInfoStruct.iModeSetupName         = 30001;
    /// m_ModeInfoStruct.iModeSupportTypeFlags  = AE_DSP_PRSNT_ASTREAM_BASIC | AE_DSP_PRSNT_ASTREAM_MUSIC | AE_DSP_PRSNT_ASTREAM_MOVIE;
    /// m_ModeInfoStruct.bIsDisabled            = false;
    ///
    /// strncpy(m_ModeInfoStruct.strModeName, "Free Surround", sizeof(m_ModeInfoStruct.strModeName) - 1);
    /// imagePath = g_strAddonPath;
    /// imagePath += "/resources/skins/Confluence/media/adsp-freesurround.png";
    /// strncpy(m_ModeInfoStruct.strOwnModeImage, imagePath.c_str(), sizeof(m_ModeInfoStruct.strOwnModeImage) - 1);
    /// memset(m_ModeInfoStruct.strOverrideModeImage, 0, sizeof(m_ModeInfoStruct.strOwnModeImage)); // unused
    /// CAddonLib::RegisterMode(&m_ModeInfoStruct);
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void RegisterDSPMode(AE_DSP_MODES::AE_DSP_MODE* mode);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_AudioEngine_General
    /// @brief Remove a mode from audio dsp database
    /// @param[in] mode The Mode to remove
    ///
    void UnregisterDSPMode(AE_DSP_MODES::AE_DSP_MODE* mode);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_AudioEngine_General
    /// @brief Get the current sink data format
    ///
    /// @param[out] sinkFormat Current sink data format. For more details see AudioEngineFormat.
    /// @return Returns true on success, else false.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Audio DSP mode information:</b>
    ///
    /// Used to get all available modes for current input stream
    ///
    /// ~~~~~~~~~~~~~{.cpp}
    /// /*
    ///  * Audio engine format information
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct AudioEngineFormat
    /// {
    ///   enum AEDataFormat m_dataFormat;           /* The stream's data format (eg, AE_FMT_S16LE) */
    ///   unsigned int      m_sampleRate;           /* The stream's sample rate (eg, 48000) */
    ///   unsigned int      m_encodedRate;          /* The encoded streams sample rate if a bitstream, otherwise undefined */
    ///   unsigned int      m_channelCount;         /* The amount of used speaker channels */
    ///   enum AEChannel    m_channels[AE_CH_MAX];  /* The stream's channel layout */
    ///   unsigned int      m_frames;               /* The number of frames per period */
    ///   unsigned int      m_frameSamples;         /* The number of samples in one frame */
    ///   unsigned int      m_frameSize;            /* The size of one frame in bytes */
    ///
    ///   /* Function to compare the format structure with another */
    ///   bool compareFormat(const AudioEngineFormat *fmt);
    /// } AudioEngineFormat;
    /// ~~~~~~~~~~~~~
    ///
    bool GetCurrentSinkFormat(AudioEngineFormat &sinkFormat);
    //--------------------------------------------------------------------------
  };
  /// @}

}; /* namespace AudioEngine */
}; /* namespace KodiAPI */

END_NAMESPACE()
