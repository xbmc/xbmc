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
 * @file kodi_adsp_dll.h
 * @section sec1 Basic audio dsp addon interface description
 * @author Team Kodi
 * @date 10. May 2014
 * @version 0.1.5
 *
 * @subsection sec1_1 General
 * @li The basic support on the addon is supplied with the
 * AE_DSP_ADDON_CAPABILITIES data which becomes asked over
 * GetAddonCapabilities(...), further the addon must register his available
 * modes on startup with the RegisterMode(...) callback function (see
 * libKODI_adsp.h). If one of this two points is not set the addon becomes
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
 *     selection on the addon given by the own addon type identifier or by
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

#include "xbmc_addon_dll.h"
#include "kodi_adsp_types.h"

/*!
 * Functions that the Audio DSP add-on must implement, but some can be empty.
 *
 * The 'remarks' field indicates which methods should be implemented, and which
 * ones are optional.
 */

extern "C"
{
  /*! @name Audio DSP add-on methods */
  //@{
  /*!
   * @brief Get the list of features that this add-on provides.
   * Called by KODI to query the add-ons capabilities.
   * Used to check which options should be presented in the DSP, which methods
   * to call, etc.
   * All capabilities that the add-on supports should be set to true.
   * @param pCapabilities The add-ons capabilities.
   * @return AE_DSP_ERROR_NO_ERROR if the properties were fetched successfully.
   * @remarks Valid implementation required.
   */
  AE_DSP_ERROR GetAddonCapabilities(AE_DSP_ADDON_CAPABILITIES *pCapabilities);

  /*!
   * @return The name reported by the back end that will be displayed in the
   * UI.
   * @remarks Valid implementation required.
   */
  const char* GetDSPName(void);

  /*!
   * @return The version string reported by the back end that will be displayed
   * in the UI.
   * @remarks Valid implementation required.
   */
  const char* GetDSPVersion(void);

  /*!
   * @brief Call one of the menu hooks (if supported).
   * Supported AE_DSP_MENUHOOK instances have to be added in ADDON_Create(),
   * by calling AddMenuHook() on the callback.
   * @param menuhook The hook to call.
   * @param item The selected item for which the hook was called.
   * @return AE_DSP_ERROR_NO_ERROR if the hook was called successfully.
   * @remarks Optional. Return AE_DSP_ERROR_NOT_IMPLEMENTED if this add-on
   * won't provide this function.
   */
  AE_DSP_ERROR CallMenuHook(const AE_DSP_MENUHOOK& menuhook, const AE_DSP_MENUHOOK_DATA &item);
  //@}

  /** @name DSP processing control, used to open and close a stream
   *  @remarks Valid implementation required.
   */
  //@{
  /*!
   * @brief Set up Audio DSP with selected audio settings (use the basic
   * present audio stream data format).
   * Used to detect available add-ons for present stream, as example stereo
   * surround upmix not needed on 5.1 audio stream.
   * @param addonSettings The add-ons audio settings.
   * @param pProperties The properties of the currently playing stream.
   * @param handle On this becomes addon informed about stream id and can set function addresses which need on calls
   * @return AE_DSP_ERROR_NO_ERROR if the properties were fetched successfully
   * and data can be performed. AE_DSP_ERROR_IGNORE_ME if format is not
   * supported, but without fault.
   * @remarks Valid implementation required.
   */
  AE_DSP_ERROR StreamCreate(const AE_DSP_SETTINGS *addonSettings, const AE_DSP_STREAM_PROPERTIES* pProperties, ADDON_HANDLE handle);

  /*!
   * Remove the selected id from currently used DSP processes
   * @param handle identification data for stream
   * @return AE_DSP_ERROR_NO_ERROR if the becomes found and removed
   * @remarks Valid implementation required.
   */
  AE_DSP_ERROR StreamDestroy(const ADDON_HANDLE handle);

  /*!
   * @brief Ask the add-on about a requested processing mode that it is
   * supported on the current stream. Is called about every add-on mode after
   * successed StreamCreate.
   * @param handle identification data for stream
   * @param type The processing mode type, see AE_DSP_MODE_TYPE for definitions
   * @param mode_id The mode inside add-on which must be performed on call. Id
   * is set from add-on by iModeNumber on AE_DSP_MODE structure during
   * RegisterMode callback,
   * @param unique_db_mode_id The Mode unique id generated from dsp database.
   * @return AE_DSP_ERROR_NO_ERROR if the properties were fetched successfully
   * or if the stream is not supported the add-on must return
   * AE_DSP_ERROR_IGNORE_ME.
   * @remarks Valid implementation required.
   */
  AE_DSP_ERROR StreamIsModeSupported(const ADDON_HANDLE handle, AE_DSP_MODE_TYPE type, unsigned int mode_id, int unique_db_mode_id);

  /*!
   * @brief Set up Audio DSP with selected audio settings (detected on data of
   * first present audio packet)
   * @param addonSettings The add-ons audio settings.
   * @return AE_DSP_ERROR_NO_ERROR if the properties were fetched successfully.
   * @remarks Valid implementation required.
   */
  AE_DSP_ERROR StreamInitialize(const ADDON_HANDLE handle, const AE_DSP_SETTINGS *addonSettings);
  //@}

  /** @name DSP input processing
   *  @remarks Only used by KODI if bSupportsInputProcess is set to true.
   */
  //@{
  /*!
   * @brief DSP input processing
   * Can be used to have unchanged stream..
   * All DSP add-ons allowed to-do this.
   * @param handle identification data for stream
   * @param array_in Pointer to data memory
   * @param samples Amount of samples inside array_in
   * @return true if work was OK
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  bool InputProcess(const ADDON_HANDLE handle, const float **array_in, unsigned int samples);
  //@}

  /** @name DSP pre-resampling
   *  @remarks Only used by KODI if bSupportsInputResample is set to true.
   */
  //@{
  /*!
   * @brief If the add-on operate with buffered arrays and the output size can
   * be higher as the input it becomes asked about needed size before any
   * InputResampleProcess call.
   * @param handle identification data for stream
   * @return The needed size of output array or 0 if no changes within it
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  unsigned int InputResampleProcessNeededSamplesize(const ADDON_HANDLE handle);

  /*!
   * @brief DSP re sample processing before master.
   * Here a high quality resample can be performed.
   * Only one DSP add-on is allowed to-do this!
   * @param handle identification data for stream
   * @param array_in Pointer to input data memory
   * @param array_out Pointer to output data memory
   * @param samples Amount of samples inside array_in
   * @return Amount of samples processed
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  unsigned int InputResampleProcess(const ADDON_HANDLE handle, float **array_in, float **array_out, unsigned int samples);

  /*!
   * @brief Returns the re-sampling generated new sample rate used before the
   * master process
   * @param handle identification data for stream
   * @return The new sample rate
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  int InputResampleSampleRate(const ADDON_HANDLE handle);

  /*!
   * @brief Returns the time in seconds that it will take
   * for the next added packet to be returned to KODI.
   * @param handle identification data for stream
   * @return the delay in seconds
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  float InputResampleGetDelay(const ADDON_HANDLE handle);
  //@}

  /** @name DSP Pre processing
   *  @remarks Only used by KODI if bSupportsPreProcess is set to true.
   */
  //@{
  /*!
   * @brief If the addon operate with buffered arrays and the output size can
   * be higher as the input it becomes asked about needed size before any
   * PreProcess call.
   * @param handle identification data for stream
   * @param mode_id The mode inside add-on which must be performed on call. Id
   * is set from add-on by iModeNumber on AE_DSP_MODE structure during
   * RegisterMode callback and can be defined from add-on as a structure
   * pointer or anything else what is needed to find it.
   * @return The needed size of output array or 0 if no changes within it
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  unsigned int PreProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id);

  /*!
   * @brief Returns the time in seconds that it will take
   * for the next added packet to be returned to KODI.
   * @param handle identification data for stream
   * @param mode_id The mode inside add-on which must be performed on call. Id
   * is set from add-on by iModeNumber on AE_DSP_MODE structure during
   * RegisterMode callback and can be defined from add-on as a structure
   * pointer or anything else what is needed to find it.
   * @return the delay in seconds
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  float PreProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id);

  /*!
   * @brief DSP preprocessing
   * All DSP add-ons allowed to-do this.
   * @param handle identification data for stream
   * @param mode_id The mode inside add-on which must be performed on call. Id
   * is set from add-on by iModeNumber on AE_DSP_MODE structure during
   * RegisterMode callback and can be defined from add-on as a structure
   * pointer or anything else what is needed to find it.
   * @param array_in Pointer to input data memory
   * @param array_out Pointer to output data memory
   * @param samples Amount of samples inside array_in
   * @return Amount of samples processed
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  unsigned int PreProcess(const ADDON_HANDLE handle, unsigned int mode_id, float **array_in, float **array_out, unsigned int samples);
  //@}

  /** @name DSP Master processing
   *  @remarks Only used by KODI if bSupportsMasterProcess is set to true.
   */
  //@{
  /*!
   * @brief Set the active master process mode
   * @param handle identification data for stream
   * @param type Requested stream type for the selected master mode
   * @param mode_id The Mode identifier.
   * @param unique_db_mode_id The Mode unique id generated from DSP database.
   * @return AE_DSP_ERROR_NO_ERROR if the setup was successful
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  AE_DSP_ERROR MasterProcessSetMode(const ADDON_HANDLE handle, AE_DSP_STREAMTYPE type, unsigned int mode_id, int unique_db_mode_id);

  /*!
   * @brief If the add-on operate with buffered arrays and the output size can
   * be higher as the input it becomes asked about needed size before any
   * MasterProcess call.
   * @param handle identification data for stream
   * @return The needed size of output array or 0 if no changes within it
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  unsigned int MasterProcessNeededSamplesize(const ADDON_HANDLE handle);

  /*!
   * @brief Returns the time in seconds that it will take
   * for the next added packet to be returned to KODI.
   * @param handle identification data for stream
   * @return the delay in seconds
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  float MasterProcessGetDelay(const ADDON_HANDLE handle);

  /*!
   * @brief Returns the from selected master mode performed channel alignment
   * @param handle identification data for stream
   * @retval out_channel_present_flags the exact channel present flags after
   * performed up-/downmix
   * @return the amount channels
   * @remarks Optional. Must be used and set if a channel up- or downmix is
   * processed from the active master mode
   */
  int MasterProcessGetOutChannels(const ADDON_HANDLE handle, unsigned long &out_channel_present_flags);

  /*!
   * @brief Master processing becomes performed with it
   * Here a channel up-mix/down-mix for stereo surround sound can be performed
   * Only one DSP add-on is allowed to-do this!
   * @param handle identification data for stream
   * @param array_in Pointer to input data memory
   * @param array_out Pointer to output data memory
   * @param samples Amount of samples inside array_in
   * @return Amount of samples processed
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  unsigned int MasterProcess(const ADDON_HANDLE handle, float **array_in, float **array_out, unsigned int samples);

  /*!
   * Used to get a information string about the processed work to show on skin
   * @return A string to show
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  const char *MasterProcessGetStreamInfoString(const ADDON_HANDLE handle);
  //@}

  /** @name DSP Post processing
   *  @remarks Only used by KODI if bSupportsPostProcess is set to true.
   */
  //@{
  /*!
   * If the add-on operate with buffered arrays and the output size can be
   * higher as the input it becomes asked about needed size before any
   * PostProcess call.
   * @param handle identification data for stream
   * @param mode_id The mode inside add-on which must be performed on call. Id
   * is set from add-on by iModeNumber on AE_DSP_MODE structure during
   * RegisterMode callback, and can be defined from add-on as a structure
   * pointer or anything else what is needed to find it.
   * @return The needed size of output array or 0 if no changes within it
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  unsigned int PostProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id);

  /*!
   * Returns the time in seconds that it will take
   * for the next added packet to be returned to KODI.
   * @param handle identification data for stream
   * @param mode_id The mode inside add-on which must be performed on call. Id
   * is set from add-on by iModeNumber on AE_DSP_MODE structure during
   * RegisterMode callback, and can be defined from add-on as a structure
   * pointer or anything else what is needed to find it.
   * @return the delay in seconds
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  float PostProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id);

  /*!
   * @brief DSP post processing
   * On the post processing can be things performed with additional channel
   * upmix like 6.1 to 7.1
   * or frequency/volume corrections, speaker distance handling, equalizer... .
   * All DSP add-ons allowed to-do this.
   * @param handle identification data for stream
   * @param mode_id The mode inside add-on which must be performed on call. Id
   * is set from add-on by iModeNumber on AE_DSP_MODE structure during
   * RegisterMode callback, and can be defined from add-on as a structure
   * pointer or anything else what is needed to find it.
   * @param array_in Pointer to input data memory
   * @param array_out Pointer to output data memory
   * @param samples Amount of samples inside array_in
   * @return Amount of samples processed
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  unsigned int PostProcess(const ADDON_HANDLE handle, unsigned int mode_id, float **array_in, float **array_out, unsigned int samples);
  //@}

  /** @name DSP Post re-sampling
   *  @remarks Only used by KODI if bSupportsOutputResample is set to true.
   */
  //@{
  /*!
   * @brief If the add-on operate with buffered arrays and the output size
   * can be higher as the input
   * it becomes asked about needed size before any OutputResampleProcess call.
   * @param handle identification data for stream
   * @return The needed size of output array or 0 if no changes within it
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  unsigned int OutputResampleProcessNeededSamplesize(const ADDON_HANDLE handle);

  /*!
   * @brief Re-sampling after master processing becomes performed with it if
   * needed, only
   * one add-on can perform it.
   * @param handle identification data for stream
   * @param array_in Pointer to input data memory
   * @param array_out Pointer to output data memory
   * @param samples Amount of samples inside array_in
   * @return Amount of samples processed
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  unsigned int OutputResampleProcess(const ADDON_HANDLE handle, float **array_in, float **array_out, unsigned int samples);

  /*!
   * @brief Returns the re-sampling generated new sample rate used after the
   * master process.
   * @param handle identification data for stream
   * @return The new sample rate
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  int OutputResampleSampleRate(const ADDON_HANDLE handle);

  /*!
   * @brief Returns the time in seconds that it will take for the next added
   * packet to be returned to KODI.
   * @param handle identification data for stream
   * @return the delay in seconds
   * @remarks Optional. Is set by AE_DSP_ADDON_CAPABILITIES and asked with
   * GetAddonCapabilities
   */
  float OutputResampleGetDelay(const ADDON_HANDLE handle);
  //@}

  // function to export the above structure to KODI
  void __declspec(dllexport) get_addon(void* ptr)
  {
    KodiToAddonFuncTable_AudioDSP* pDSP = static_cast<KodiToAddonFuncTable_AudioDSP*>(ptr);

    pDSP->GetAddonCapabilities                  = GetAddonCapabilities;
    pDSP->GetDSPName                            = GetDSPName;
    pDSP->GetDSPVersion                         = GetDSPVersion;
    pDSP->MenuHook                              = CallMenuHook;

    pDSP->StreamCreate                          = StreamCreate;
    pDSP->StreamDestroy                         = StreamDestroy;
    pDSP->StreamIsModeSupported                 = StreamIsModeSupported;
    pDSP->StreamInitialize                      = StreamInitialize;

    pDSP->InputProcess                          = InputProcess;

    pDSP->InputResampleProcessNeededSamplesize  = InputResampleProcessNeededSamplesize;
    pDSP->InputResampleProcess                  = InputResampleProcess;
    pDSP->InputResampleGetDelay                 = InputResampleGetDelay;
    pDSP->InputResampleSampleRate               = InputResampleSampleRate;

    pDSP->PreProcessNeededSamplesize            = PreProcessNeededSamplesize;
    pDSP->PreProcessGetDelay                    = PreProcessGetDelay;
    pDSP->PreProcess                            = PreProcess;

    pDSP->MasterProcessSetMode                  = MasterProcessSetMode;
    pDSP->MasterProcessNeededSamplesize         = MasterProcessNeededSamplesize;
    pDSP->MasterProcessGetDelay                 = MasterProcessGetDelay;
    pDSP->MasterProcessGetOutChannels           = MasterProcessGetOutChannels;
    pDSP->MasterProcess                         = MasterProcess;
    pDSP->MasterProcessGetStreamInfoString      = MasterProcessGetStreamInfoString;

    pDSP->PostProcessNeededSamplesize           = PostProcessNeededSamplesize;
    pDSP->PostProcessGetDelay                   = PostProcessGetDelay;
    pDSP->PostProcess                           = PostProcess;

    pDSP->OutputResampleProcessNeededSamplesize = OutputResampleProcessNeededSamplesize;
    pDSP->OutputResampleProcess                 = OutputResampleProcess;
    pDSP->OutputResampleSampleRate              = OutputResampleSampleRate;
    pDSP->OutputResampleGetDelay                = OutputResampleGetDelay;
  };
};

