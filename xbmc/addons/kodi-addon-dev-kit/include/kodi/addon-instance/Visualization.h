/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*
 * Parts with a comment named "internal" are only used inside header and not
 * used or accessed direct during add-on development!
 */

#include "../AddonBase.h"
#include "../gui/renderHelper.h"

namespace kodi { namespace addon { class CInstanceVisualization; }}

extern "C"
{

struct AddonInstance_Visualization;

typedef enum VIS_ACTION : unsigned int /* internal */
{
  VIS_ACTION_NONE = 0,
  VIS_ACTION_NEXT_PRESET,
  VIS_ACTION_PREV_PRESET,
  VIS_ACTION_LOAD_PRESET,
  VIS_ACTION_RANDOM_PRESET,
  VIS_ACTION_LOCK_PRESET,
  VIS_ACTION_RATE_PRESET_PLUS,
  VIS_ACTION_RATE_PRESET_MINUS,
  VIS_ACTION_UPDATE_ALBUMART,
  VIS_ACTION_UPDATE_TRACK
} VIS_ACTION;

struct VIS_INFO /* internal */
{
  bool bWantsFreq;
  int iSyncDelay;
};

typedef struct AddonProps_Visualization /* internal */
{
  void *device;
  int x;
  int y;
  int width;
  int height;
  float pixelRatio;
  const char *name;
  const char *presets;
  const char *profile;
} AddonProps_Visualization;

typedef struct AddonToKodiFuncTable_Visualization /* internal */
{
  KODI_HANDLE kodiInstance;
  void (__cdecl* transfer_preset) (void* kodiInstance, const char* preset);
  void (__cdecl* clear_presets) (void* kodiInstance);
} AddonToKodiFuncTable_Visualization;

typedef struct KodiToAddonFuncTable_Visualization /* internal */
{
  kodi::addon::CInstanceVisualization* addonInstance;
  bool (__cdecl* start)(const AddonInstance_Visualization* instance, int channels, int samples_per_sec, int bits_per_sample, const char* song_name);
  void (__cdecl* stop)(const AddonInstance_Visualization* instance);
  void (__cdecl* audio_data)(const AddonInstance_Visualization* instance, const float* audio_data, int audio_data_length, float *freq_data, int freq_data_length);
  bool (__cdecl* is_dirty)(const AddonInstance_Visualization* instance);
  void (__cdecl* render)(const AddonInstance_Visualization* instance);
  void (__cdecl* get_info)(const AddonInstance_Visualization* instance, VIS_INFO *info);
  bool (__cdecl* on_action)(const AddonInstance_Visualization* instance, VIS_ACTION action, const void *param);
  unsigned int (__cdecl *get_presets)(const AddonInstance_Visualization* instance);
  int (__cdecl *get_active_preset)(const AddonInstance_Visualization* instance);
  bool (__cdecl* is_locked)(const AddonInstance_Visualization* instance);
} KodiToAddonFuncTable_Visualization;

typedef struct AddonInstance_Visualization /* internal */
{
  AddonProps_Visualization* props;
  AddonToKodiFuncTable_Visualization* toKodi;
  KodiToAddonFuncTable_Visualization* toAddon;
} AddonInstance_Visualization;

//============================================================================
/// \defgroup cpp_kodi_addon_visualization_VisTrack class VisTrack
/// \ingroup cpp_kodi_addon_visualization
/// @brief **Visualization track information structure**
///
/// Called from kodi::addon::CInstanceVisualization::UpdateTrack() with the
/// information of the currently-playing song.
///
//@{
struct VisTrack
{
  /// @brief Title of the current song.
  const char *title;

  /// @brief Artist names, as a single string
  const char *artist;

  /// @brief Album that the current song is from.
  const char *album;

  /// @brief Album artist names, as a single string
  const char *albumArtist;

  /// @brief The genre name from the music tag, if present.
  const char *genre;

  /// @brief Comment of the current song stored in the ID tag info.
  const char *comment;

  /// @brief Lyrics of the current song, if available.
  const char *lyrics;

  const char *reserved1;
  const char *reserved2;

  /// @brief Track number of the current song.
  int trackNumber;

  /// @brief Disc number of the current song stored in the ID tag info.
  int discNumber;

  /// @brief Duration of the current song, in seconds.
  int duration;

  /// @brief Year that the current song was released.
  int year;

  /// @brief The user-defined rating of the current song.
  int rating;

  int reserved3;
  int reserved4;
};
//@}
//----------------------------------------------------------------------------

} /* extern "C" */

namespace kodi
{
namespace addon
{

  //============================================================================
  ///
  /// \addtogroup cpp_kodi_addon_visualization
  /// @brief \cpp_class{ kodi::addon::CInstanceVisualization }
  /// **Visualization add-on instance**
  ///
  /// [Music visualization](https://en.wikipedia.org/wiki/Music_visualization),
  /// or music visualisation, is a feature in Kodi that generates animated
  /// imagery based on a piece of music. The imagery is usually generated and
  /// rendered in real time synchronized to the music.
  ///
  /// Visualization techniques range from simple ones (e.g., a simulation of an
  /// oscilloscope display) to elaborate ones, which often include a plurality
  /// of composited effects. The changes in the music's loudness and frequency
  /// spectrum are among the properties used as input to the visualization.
  ///
  /// Include the header \ref Visualization.h "#include <kodi/addon-instance/Visualization.h>"
  /// to use this class.
  ///
  /// This interface allows the creation of visualizations for Kodi, based upon
  /// **DirectX** or/and **OpenGL** rendering with `C++` code.
  ///
  /// Additionally, there are several \ref cpp_kodi_addon_visualization_CB "other functions"
  /// available in which the child class can ask about the current hardware,
  /// including the device, display and several other parts.
  ///
  /// --------------------------------------------------------------------------
  ///
  ///
  /// **Here is an example of the minimum required code to start a visualization:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/addon-instance/Visualization.h>
  ///
  /// class CMyVisualization : public kodi::addon::CAddonBase,
  ///                          public kodi::addon::CInstanceVisualization
  /// {
  /// public:
  ///   CMyVisualization();
  ///
  ///   bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) override;
  ///   void AudioData(const float* audioData, int audioDataLength, float* freqData, int freqDataLength) override;
  ///   void Render() override;
  /// };
  ///
  /// CMyVisualization::CMyVisualization()
  /// {
  ///   ...
  /// }
  ///
  /// bool CMyVisualization::Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName)
  /// {
  ///   ...
  ///   return true;
  /// }
  ///
  /// void CMyVisualization::AudioData(const float* audioData, int audioDataLength, float* freqData, int freqDataLength)
  /// {
  ///   ...
  /// }
  ///
  /// void CMyVisualization::Render()
  /// {
  ///   ...
  /// }
  ///
  /// ADDONCREATOR(CMyVisualization)
  /// ~~~~~~~~~~~~~
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  ///
  /// **Here is another example where the visualization is used together with
  /// other instance types:**
  ///
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/addon-instance/Visualization.h>
  ///
  /// class CMyVisualization : public ::kodi::addon::CInstanceVisualization
  /// {
  /// public:
  ///   CMyVisualization(KODI_HANDLE instance);
  ///
  ///   bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) override;
  ///   void AudioData(const float* audioData, int audioDataLength, float* freqData, int freqDataLength) override;
  ///   void Render() override;
  /// };
  ///
  /// CMyVisualization::CMyVisualization(KODI_HANDLE instance)
  ///   : CInstanceVisualization(instance)
  /// {
  ///   ...
  /// }
  ///
  /// bool CMyVisualization::Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName)
  /// {
  ///   ...
  ///   return true;
  /// }
  ///
  /// void CMyVisualization::AudioData(const float* audioData, int audioDataLength, float* freqData, int freqDataLength)
  /// {
  ///   ...
  /// }
  ///
  /// void CMyVisualization::Render()
  /// {
  ///   ...
  /// }
  ///
  ///
  /// /*----------------------------------------------------------------------*/
  ///
  /// class CMyAddon : public ::kodi::addon::CAddonBase
  /// {
  /// public:
  ///   CMyAddon() { }
  ///   ADDON_STATUS CreateInstance(int instanceType,
  ///                               std::string instanceID,
  ///                               KODI_HANDLE instance,
  ///                               KODI_HANDLE& addonInstance) override;
  /// };
  ///
  /// /* If you use only one instance in your add-on, can be instanceType and
  ///  * instanceID ignored */
  /// ADDON_STATUS CMyAddon::CreateInstance(int instanceType,
  ///                                       std::string instanceID,
  ///                                       KODI_HANDLE instance,
  ///                                       KODI_HANDLE& addonInstance)
  /// {
  ///   if (instanceType == ADDON_INSTANCE_VISUALIZATION)
  ///   {
  ///     kodi::Log(ADDON_LOG_NOTICE, "Creating my Visualization");
  ///     addonInstance = new CMyVisualization(instance);
  ///     return ADDON_STATUS_OK;
  ///   }
  ///   else if (...)
  ///   {
  ///     ...
  ///   }
  ///   return ADDON_STATUS_UNKNOWN;
  /// }
  ///
  /// ADDONCREATOR(CMyAddon)
  /// ~~~~~~~~~~~~~
  ///
  /// The destruction of the example class `CMyVisualization` is called from
  /// Kodi's header. Manually deleting the add-on instance is not required.
  ///
  //----------------------------------------------------------------------------
  class CInstanceVisualization : public IAddonInstance
  {
  public:
    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Visualization class constructor
    ///
    /// Used by an add-on that only supports visualizations.
    ///
    CInstanceVisualization()
      : IAddonInstance(ADDON_INSTANCE_VISUALIZATION)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceVisualization: Cannot create multiple instances of add-on.");

      SetAddonStruct(CAddonBase::m_interface->firstKodiInstance);
      CAddonBase::m_interface->globalSingleInstance = this;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Visualization class constructor used to support multiple instance
    /// types
    ///
    /// @param[in] instance               The instance value given to
    ///                                   <b>`kodi::addon::CAddonBase::CreateInstance(...)`</b>.
    ///
    /// @warning Only use `instance` from the CreateInstance call
    ///
    explicit CInstanceVisualization(KODI_HANDLE instance)
      : IAddonInstance(ADDON_INSTANCE_VISUALIZATION)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceVisualization: Creation of multiple together with single instance way is not allowed!");

      SetAddonStruct(instance);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Destructor
    ///
    ~CInstanceVisualization() override = default;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Used to notify the visualization that a new song has been started
    ///
    /// @param[in] channels             Number of channels in the stream
    /// @param[in] samplesPerSec        Samples per second of stream
    /// @param[in] bitsPerSample        Number of bits in one sample
    /// @param[in] songName             The name of the currently-playing song
    /// @return                         true if start successful done
    ///
    virtual bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) { return true; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Used to inform the visualization that the rendering control was
    /// stopped
    ///
    virtual void Stop() {}
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Pass audio data to the visualization
    ///
    /// @param[in] audioData            The raw audio data
    /// @param[in] audioDataLength      Length of the audioData array
    /// @param[in] freqData             The [FFT](https://en.wikipedia.org/wiki/Fast_Fourier_transform)
    ///                                 of the audio data
    /// @param[in] freqDataLength       Length of frequency data array
    ///
    /// Values **freqData** and **freqDataLength** are used if GetInfo() returns
    /// true for the `wantsFreq` parameter. Otherwise, **freqData** is set to
    /// `nullptr` and **freqDataLength** is `0`.
    ///
    virtual void AudioData(const float* audioData, int audioDataLength, float* freqData, int freqDataLength) {}
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Used to inform Kodi that the rendered region is dirty and need an
    /// update
    ///
    /// @return True if dirty
    ///
    virtual bool IsDirty() { return true; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Used to indicate when the add-on should render
    ///
    virtual void Render() {}
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Used to get the number of buffers from the current visualization
    ///
    /// @param[out] wantsFreq           Indicates whether the add-on wants FFT
    ///                                 data. If set to true, the **freqData**
    ///                                 and **freqDataLength** parameters of
    ///                                 AudioData() are used
    /// @param[out] syncDelay           The number of buffers to delay before
    ///                                 calling AudioData()
    ///
    /// @note If this function is not implemented, it will default to
    /// `wantsFreq` = false and `syncDelay` = 0.
    ///
    virtual void GetInfo(bool& wantsFreq, int& syncDelay) { wantsFreq = false; syncDelay = 0; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Used to get a list of visualization presets the user can select
    /// from
    ///
    /// @param[out] presets             The vector list containing the names of
    ///                                 presets that the user can select
    /// @return                         Return true if successful, or false if
    ///                                 there are no presets to choose from
    ///
    virtual bool GetPresets(std::vector<std::string>& presets) { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Get the index of the current preset
    ///
    /// @return                         Index number of the current preset
    ///
    virtual int GetActivePreset() { return -1; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Check if the add-on is locked to the current preset
    ///
    /// @return                         True if locked to the current preset
    ///
    virtual bool IsLocked() { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Load the previous visualization preset
    ///
    /// @return                 Return true if the previous preset was loaded
    virtual bool PrevPreset() { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Load the next visualization preset
    ///
    /// @return                 Return true if the next preset was loaded
    virtual bool NextPreset() { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Load a visualization preset
    ///
    /// This function is called after a new preset is selected.
    ///
    /// @param[in] select       Preset index to use
    /// @return                 Return true if the preset is loaded
    virtual bool LoadPreset(int select) { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Switch to a new random preset
    ///
    /// @return                 Return true if a random preset was loaded
    virtual bool RandomPreset() { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Lock the current visualization preset, preventing it from changing
    ///
    /// @param[in] lockUnlock   If set to true, the preset should be locked
    /// @return                 Return true if the current preset is locked
    virtual bool LockPreset(bool lockUnlock) { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Used to increase/decrease the visualization preset rating
    ///
    /// @param[in] plusMinus    If set to true the rating is increased, otherwise
    ///                         decreased
    /// @return                 Return true if the rating is modified
    virtual bool RatePreset(bool plusMinus) { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Inform the visualization of the current album art image
    ///
    /// @param[in] albumart     Path to the current album art image
    /// @return                 Return true if the image is used
    virtual bool UpdateAlbumart(std::string albumart) { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization
    /// @brief Inform the visualization of the current track's tag information
    ///
    /// @param[in] track        Visualization track information structure
    /// @return                 Return true if the track information is used
    virtual bool UpdateTrack(const VisTrack &track) { return false; }

    //==========================================================================
    ///
    /// \defgroup cpp_kodi_addon_visualization_CB Information functions
    /// \ingroup cpp_kodi_addon_visualization
    /// @brief **To get info about the device, display and several other parts**
    ///
    //@{

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization_CB
    /// @brief To transfer available presets on addon
    ///
    /// Used if @ref GetPresets not possible to use, e.g. where available presets
    /// are only known during @ref Start call.
    ///
    /// @param[in] presets List to store available presets.
    ///
    /// @note The function should only be called once, if possible
    ///
    inline void TransferPresets(const std::vector<std::string>& presets)
    {
      m_instanceData->toKodi->clear_presets(m_instanceData->toKodi->kodiInstance);
      for (auto it : presets)
        m_instanceData->toKodi->transfer_preset(m_instanceData->toKodi->kodiInstance, it.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization_CB
    /// @brief Device that represents the display adapter
    ///
    /// @return A pointer to the used device
    ///
    /// @note This is only available on **DirectX**, It us unused (`nullptr`) on
    /// **OpenGL**
    ///
    inline void* Device() { return m_instanceData->props->device; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization_CB
    /// @brief Returns the X position of the rendering window
    ///
    /// @return The X position, in pixels
    ///
    inline int X() { return m_instanceData->props->x; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization_CB
    /// @brief Returns the Y position of the rendering window
    ///
    /// @return The Y position, in pixels
    ///
    inline int Y() { return m_instanceData->props->y; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization_CB
    /// @brief Returns the width of the rendering window
    ///
    /// @return The width, in pixels
    ///
    inline int Width() { return m_instanceData->props->width; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization_CB
    /// @brief Returns the height of the rendering window
    ///
    /// @return The height, in pixels
    ///
    inline int Height() { return m_instanceData->props->height; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization_CB
    /// @brief Pixel aspect ratio (often abbreviated PAR) is a ratio that
    /// describes how the width of a pixel compares to the height of that pixel.
    ///
    /// @return The pixel aspect ratio used by the display
    ///
    inline float PixelRatio() { return m_instanceData->props->pixelRatio; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization_CB
    /// @brief Used to get the name of the add-on defined in `addon.xml`
    ///
    /// @return The add-on name
    ///
    inline std::string Name() { return m_instanceData->props->name; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization_CB
    /// @brief Used to get the full path where the add-on is installed
    ///
    /// @return The add-on installation path
    ///
    inline std::string Presets() { return m_instanceData->props->presets; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_visualization_CB
    /// @brief Used to get the full path to the add-on's user profile
    ///
    /// @note The trailing folder (consisting of the add-on's ID) is not created
    /// by default. If it is needed, you must call kodi::vfs::CreateDirectory()
    /// to create the folder.
    ///
    /// @return Path to the user profile
    ///
    inline std::string Profile() { return m_instanceData->props->profile; }
    //--------------------------------------------------------------------------
    //@}

  private:
    void SetAddonStruct(KODI_HANDLE instance)
    {
      if (instance == nullptr)
        throw std::logic_error("kodi::addon::CInstanceVisualization: Null pointer instance passed.");

      m_instanceData = static_cast<AddonInstance_Visualization*>(instance);
      m_instanceData->toAddon->addonInstance = this;
      m_instanceData->toAddon->start = ADDON_Start;
      m_instanceData->toAddon->stop = ADDON_Stop;
      m_instanceData->toAddon->audio_data = ADDON_AudioData;
      m_instanceData->toAddon->is_dirty = ADDON_IsDirty;
      m_instanceData->toAddon->render = ADDON_Render;
      m_instanceData->toAddon->get_info = ADDON_GetInfo;
      m_instanceData->toAddon->on_action = ADDON_OnAction;
      m_instanceData->toAddon->get_presets = ADDON_GetPresets;
      m_instanceData->toAddon->get_active_preset = ADDON_GetActivePreset;
      m_instanceData->toAddon->is_locked = ADDON_IsLocked;
    }

    inline static bool ADDON_Start(const AddonInstance_Visualization* addon, int channels, int samplesPerSec, int bitsPerSample, const char* songName)
    {
      addon->toAddon->addonInstance->m_renderHelper = kodi::gui::GetRenderHelper();
      return addon->toAddon->addonInstance->Start(channels, samplesPerSec, bitsPerSample, songName);
    }

    inline static void ADDON_Stop(const AddonInstance_Visualization* addon)
    {
      addon->toAddon->addonInstance->Stop();
      addon->toAddon->addonInstance->m_renderHelper = nullptr;
    }

    inline static void ADDON_AudioData(const AddonInstance_Visualization* addon, const float* audioData, int audioDataLength, float *freqData, int freqDataLength)
    {
      addon->toAddon->addonInstance->AudioData(audioData, audioDataLength, freqData, freqDataLength);
    }

    inline static bool ADDON_IsDirty(const AddonInstance_Visualization* addon)
    {
      return addon->toAddon->addonInstance->IsDirty();
    }

    inline static void ADDON_Render(const AddonInstance_Visualization* addon)
    {
      if (!addon->toAddon->addonInstance->m_renderHelper)
        return;
      addon->toAddon->addonInstance->m_renderHelper->Begin();
      addon->toAddon->addonInstance->Render();
      addon->toAddon->addonInstance->m_renderHelper->End();
    }

    inline static void ADDON_GetInfo(const AddonInstance_Visualization* addon, VIS_INFO *info)
    {
      addon->toAddon->addonInstance->GetInfo(info->bWantsFreq, info->iSyncDelay);
    }

    inline static bool ADDON_OnAction(const AddonInstance_Visualization* addon, VIS_ACTION action, const void *param)
    {
      switch (action)
      {
        case VIS_ACTION_NEXT_PRESET:
          return addon->toAddon->addonInstance->NextPreset();
        case VIS_ACTION_PREV_PRESET:
          return addon->toAddon->addonInstance->PrevPreset();
        case VIS_ACTION_LOAD_PRESET:
          return addon->toAddon->addonInstance->LoadPreset(*static_cast<const int*>(param));
        case VIS_ACTION_RANDOM_PRESET:
          return addon->toAddon->addonInstance->RandomPreset();
        case VIS_ACTION_LOCK_PRESET:
          addon->toAddon->addonInstance->m_presetLockedByUser = !addon->toAddon->addonInstance->m_presetLockedByUser;
          return addon->toAddon->addonInstance->LockPreset(addon->toAddon->addonInstance->m_presetLockedByUser);
        case VIS_ACTION_RATE_PRESET_PLUS:
          return addon->toAddon->addonInstance->RatePreset(true);
        case VIS_ACTION_RATE_PRESET_MINUS:
          return addon->toAddon->addonInstance->RatePreset(false);
        case VIS_ACTION_UPDATE_ALBUMART:
          return addon->toAddon->addonInstance->UpdateAlbumart(static_cast<const char*>(param));
        case VIS_ACTION_UPDATE_TRACK:
          return addon->toAddon->addonInstance->UpdateTrack(*static_cast<const VisTrack*>(param));
        case VIS_ACTION_NONE:
        default:
          break;
      }
      return false;
    }

    inline static unsigned int ADDON_GetPresets(const AddonInstance_Visualization* addon)
    {
      std::vector<std::string> presets;
      if (addon->toAddon->addonInstance->GetPresets(presets))
      {
        for (auto it : presets)
          addon->toAddon->addonInstance->m_instanceData->toKodi->transfer_preset(addon->toKodi->kodiInstance, it.c_str());
      }

      return static_cast<unsigned int>(presets.size());
    }

    inline static int ADDON_GetActivePreset(const AddonInstance_Visualization* addon)
    {
      return addon->toAddon->addonInstance->GetActivePreset();
    }

    inline static bool ADDON_IsLocked(const AddonInstance_Visualization* addon)
    {
      return addon->toAddon->addonInstance->IsLocked();
    }

    std::shared_ptr<kodi::gui::IRenderHelper> m_renderHelper;
    bool m_presetLockedByUser = false;
    AddonInstance_Visualization* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
