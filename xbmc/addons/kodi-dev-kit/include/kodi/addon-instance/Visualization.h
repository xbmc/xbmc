/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/visualization.h"
#include "../gui/renderHelper.h"

#ifdef __cplusplus
namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_visualization_Defs_VisualizationTrack class VisualizationTrack
/// @ingroup cpp_kodi_addon_visualization_Defs
/// @brief **Info tag data structure**\n
/// Representation of available information of processed audio file.
///
/// This is used to store all the necessary data of audio stream and to have on
/// e.g. GUI for information.
///
/// Called from @ref kodi::addon::CInstanceVisualization::UpdateTrack() with the
/// information of the currently-playing song.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_visualization_Defs_VisualizationTrack_Help
///
///@{
class VisualizationTrack
{
  /*! \cond PRIVATE */
  friend class CInstanceVisualization;
  /*! \endcond */

public:
  /*! \cond PRIVATE */
  VisualizationTrack() = default;
  VisualizationTrack(const VisualizationTrack& tag) { *this = tag; }

  VisualizationTrack& operator=(const VisualizationTrack& right)
  {
    if (&right == this)
      return *this;

    m_title = right.m_title;
    m_artist = right.m_artist;
    m_album = right.m_album;
    m_albumArtist = right.m_albumArtist;
    m_genre = right.m_genre;
    m_comment = right.m_comment;
    m_lyrics = right.m_lyrics;

    m_trackNumber = right.m_trackNumber;
    m_discNumber = right.m_discNumber;
    m_duration = right.m_duration;
    m_year = right.m_year;
    m_rating = right.m_rating;
    return *this;
  }
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_visualization_Defs_VisualizationTrack_Help Value Help
  /// @ingroup cpp_kodi_addon_visualization_Defs_VisualizationTrack
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_visualization_Defs_VisualizationTrack :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Title of the current song.** | `std::string` | @ref VisualizationTrack::SetTitle "SetTitle" | @ref VisualizationTrack::GetTitle "GetTitle"
  /// | **Artist names, as a single string** | `std::string` | @ref VisualizationTrack::SetArtist "SetArtist" | @ref VisualizationTrack::GetArtist "GetArtist"
  /// | **Album that the current song is from.** | `std::string` | @ref VisualizationTrack::SetAlbum "SetAlbum" | @ref VisualizationTrack::GetAlbum "GetAlbum"
  /// | **Album artist names, as a single string** | `std::string` | @ref VisualizationTrack::SetAlbumArtist "SetAlbumArtist" | @ref VisualizationTrack::GetAlbumArtist "GetAlbumArtist"
  /// | **The genre name from the music tag, if present** | `std::string` | @ref VisualizationTrack::SetGenre "SetGenre" | @ref VisualizationTrack::GetGenre "GetGenre"
  /// | **Duration of the current song, in seconds** | `int` | @ref VisualizationTrack::SetDuration "SetDuration" | @ref VisualizationTrack::GetDuration "GetDuration"
  /// | **Track number of the current song** | `int` | @ref VisualizationTrack::SetTrack "SetTrack" | @ref VisualizationTrack::GetTrack "GetTrack"
  /// | **Disc number of the current song stored in the ID tag info** | `int` | @ref VisualizationTrack::SetDisc "SetDisc" | @ref VisualizationTrack::GetDisc "GetDisc"
  /// | **Year that the current song was released** | `int` | @ref VisualizationTrack::SetYear "SetYear" | @ref VisualizationTrack::GetYear "GetYear"
  /// | **Lyrics of the current song, if available** | `std::string` | @ref VisualizationTrack::SetLyrics "SetLyrics" | @ref VisualizationTrack::GetLyrics "GetLyrics"
  /// | **The user-defined rating of the current song** | `int` | @ref VisualizationTrack::SetRating "SetRating" | @ref VisualizationTrack::GetRating "GetRating"
  /// | **Comment of the current song stored in the ID tag info** | `std::string` | @ref VisualizationTrack::SetComment "SetComment" | @ref VisualizationTrack::GetComment "GetComment"
  ///

  /// @addtogroup cpp_kodi_addon_visualization_Defs_VisualizationTrack
  ///@{

  /// @brief Set title of the current song.
  void SetTitle(const std::string& title) { m_title = title; }

  /// @brief Get title of the current song.
  const std::string& GetTitle() const { return m_title; }

  /// @brief Set artist names, as a single string-
  void SetArtist(const std::string& artist) { m_artist = artist; }

  /// @brief Get artist names, as a single string-
  const std::string& GetArtist() const { return m_artist; }

  /// @brief Set Album that the current song is from.
  void SetAlbum(const std::string& album) { m_album = album; }

  /// @brief Get Album that the current song is from.
  const std::string& GetAlbum() const { return m_album; }

  /// @brief Set album artist names, as a single stringalbum artist name
  void SetAlbumArtist(const std::string& albumArtist) { m_albumArtist = albumArtist; }

  /// @brief Get album artist names, as a single string-
  const std::string& GetAlbumArtist() const { return m_albumArtist; }

  /// @brief Set genre name from music as string if present.
  void SetGenre(const std::string& genre) { m_genre = genre; }

  /// @brief Get genre name from music as string if present.
  const std::string& GetGenre() const { return m_genre; }

  /// @brief Set the duration of music as integer from info.
  void SetDuration(int duration) { m_duration = duration; }

  /// @brief Get the duration of music as integer from info.
  int GetDuration() const { return m_duration; }

  /// @brief Set track number (if present) from music info as integer.
  void SetTrack(int trackNumber) { m_trackNumber = trackNumber; }

  /// @brief Get track number (if present).
  int GetTrack() const { return m_trackNumber; }

  /// @brief Set disk number (if present) from music info as integer.
  void SetDisc(int discNumber) { m_discNumber = discNumber; }

  /// @brief Get disk number (if present)
  int GetDisc() const { return m_discNumber; }

  /// @brief Set year that the current song was released.
  void SetYear(int year) { m_year = year; }

  /// @brief Get year that the current song was released.
  int GetYear() const { return m_year; }

  /// @brief Set string from lyrics.
  void SetLyrics(const std::string& lyrics) { m_lyrics = lyrics; }

  /// @brief Get string from lyrics.
  const std::string& GetLyrics() const { return m_lyrics; }

  /// @brief Set the user-defined rating of the current song.
  void SetRating(int rating) { m_rating = rating; }

  /// @brief Get the user-defined rating of the current song.
  int GetRating() const { return m_rating; }

  /// @brief Set additional information comment (if present).
  void SetComment(const std::string& comment) { m_comment = comment; }

  /// @brief Get additional information comment (if present).
  const std::string& GetComment() const { return m_comment; }

  ///@}

private:
  VisualizationTrack(const KODI_ADDON_VISUALIZATION_TRACK* tag)
  {
    if (!tag)
      return;

    m_title = tag->title ? tag->title : "";
    m_artist = tag->artist ? tag->artist : "";
    m_album = tag->album ? tag->album : "";
    m_albumArtist = tag->albumArtist ? tag->albumArtist : "";
    m_genre = tag->genre ? tag->genre : "";
    m_comment = tag->comment ? tag->comment : "";
    m_lyrics = tag->lyrics ? tag->lyrics : "";

    m_trackNumber = tag->trackNumber;
    m_discNumber = tag->discNumber;
    m_duration = tag->duration;
    m_year = tag->year;
    m_rating = tag->rating;
  }

  std::string m_title;
  std::string m_artist;
  std::string m_album;
  std::string m_albumArtist;
  std::string m_genre;
  std::string m_comment;
  std::string m_lyrics;

  int m_trackNumber = 0;
  int m_discNumber = 0;
  int m_duration = 0;
  int m_year = 0;
  int m_rating = 0;
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_visualization_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_addon_visualization
/// @brief **Visualization add-on instance definition values**\n
/// All visualization functions associated data structures.
///
/// Used to exchange the available options between Kodi and addon.
///

//==============================================================================
/// @addtogroup cpp_kodi_addon_visualization
/// @brief \cpp_class{ kodi::addon::CInstanceVisualization }
/// **Visualization add-on instance**\n
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
/// Include the header @ref Visualization.h "#include <kodi/addon-instance/Visualization.h>"
/// to use this class.
///
/// This interface allows the creation of visualizations for Kodi, based upon
/// **DirectX** or/and **OpenGL** rendering with `C++` code.
///
/// Additionally, there are several @ref cpp_kodi_addon_visualization_CB "other functions"
/// available in which the child class can ask about the current hardware,
/// including the device, display and several other parts.
///
/// ----------------------------------------------------------------------------
///
/// **Here's an example on addon.xml:**
/// ~~~~~~~~~~~~~{.xml}
/// <?xml version="1.0" encoding="UTF-8"?>
/// <addon
///   id="visualization.myspecialnamefor"
///   version="1.0.0"
///   name="My special visualization addon"
///   provider-name="Your Name">
///   <requires>@ADDON_DEPENDS@</requires>
///   <extension
///     point="xbmc.player.musicviz"
///     library_@PLATFORM@="@LIBRARY_FILENAME@"/>
///   <extension point="xbmc.addon.metadata">
///     <summary lang="en_GB">My visualization addon addon</summary>
///     <description lang="en_GB">My visualization addon description</description>
///     <platform>@PLATFORM@</platform>
///   </extension>
/// </addon>
/// ~~~~~~~~~~~~~
///
/// Description to visualization related addon.xml values:
/// | Name                          | Description
/// |:------------------------------|----------------------------------------
/// | <b>`point`</b>                | Addon type specification<br>At all addon types and for this kind always <b>"xbmc.player.musicviz"</b>.
/// | <b>`library_@PLATFORM@`</b>   | Sets the used library name, which is automatically set by cmake at addon build.
///
/// --------------------------------------------------------------------------
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
///   bool Start(int channels, int samplesPerSec, int bitsPerSample, const std::string& songName) override;
///   void AudioData(const float* audioData, size_t audioDataLength) override;
///   void Render() override;
/// };
///
/// CMyVisualization::CMyVisualization()
/// {
///   ...
/// }
///
/// bool CMyVisualization::Start(int channels, int samplesPerSec, int bitsPerSample, const std::string& songName)
/// {
///   ...
///   return true;
/// }
///
/// void CMyVisualization::AudioData(const float* audioData, size_t audioDataLength)
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
/// class CMyVisualization : public kodi::addon::CInstanceVisualization
/// {
/// public:
///   CMyVisualization(const kodi::addon::IInstanceInfo& instance);
///
///   bool Start(int channels, int samplesPerSec, int bitsPerSample, const std::string& songName) override;
///   void AudioData(const float* audioData, size_t audioDataLength) override;
///   void Render() override;
/// };
///
/// CMyVisualization::CMyVisualization(const kodi::addon::IInstanceInfo& instance)
///   : kodi::addon::CInstanceAudioDecoder(instance)
/// {
///   ...
/// }
///
/// bool CMyVisualization::Start(int channels, int samplesPerSec, int bitsPerSample, const std::string& songName)
/// {
///   ...
///   return true;
/// }
///
/// void CMyVisualization::AudioData(const float* audioData, size_t audioDataLength)
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
/// //----------------------------------------------------------------------
///
/// class CMyAddon : public kodi::addon::CAddonBase
/// {
/// public:
///   CMyAddon() = default;
///   ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
///                               KODI_ADDON_INSTANCE_HDL& hdl) override;
/// };
///
/// // If you use only one instance in your add-on, can be instanceType and
/// // instanceID ignored
/// ADDON_STATUS CMyAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance,
///                                       KODI_ADDON_INSTANCE_HDL& hdl)
/// {
///   if (instance.IsType(ADDON_INSTANCE_VISUALIZATION))
///   {
///     kodi::Log(ADDON_LOG_INFO, "Creating my visualization");
///     hdl = new CMyVisualization(instance);
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
class ATTR_DLL_LOCAL CInstanceVisualization : public IAddonInstance
{
public:
  //============================================================================
  ///
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Visualization class constructor
  ///
  /// Used by an add-on that only supports visualizations.
  ///
  CInstanceVisualization()
    : IAddonInstance(IInstanceInfo(CPrivateBase::m_interface->firstKodiInstance))
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error(
          "kodi::addon::CInstanceVisualization: Cannot create multiple instances of add-on.");

    SetAddonStruct(CPrivateBase::m_interface->firstKodiInstance);
    CPrivateBase::m_interface->globalSingleInstance = this;
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Visualization class constructor used to support multiple instance
  /// types.
  ///
  /// @param[in] instance The instance value given to
  ///                     <b>`kodi::addon::CAddonBase::CreateInstance(...)`</b>.
  /// @param[in] kodiVersion [opt] Version used in Kodi for this instance, to
  ///                        allow compatibility to older Kodi versions.
  ///
  /// @note Recommended to set <b>`kodiVersion`</b>.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Here's example about the use of this:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// class CMyVisualization : public kodi::addon::CInstanceAudioDecoder
  /// {
  /// public:
  ///   CMyVisualization(const kodi::addon::IInstanceInfo& instance)
  ///     : kodi::addon::CInstanceAudioDecoder(instance)
  ///   {
  ///      ...
  ///   }
  ///
  ///   ...
  /// };
  ///
  /// ADDON_STATUS CMyAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance,
  ///                                       KODI_ADDON_INSTANCE_HDL& hdl)
  /// {
  ///   kodi::Log(ADDON_LOG_INFO, "Creating my visualization");
  ///   hdl = new CMyVisualization(instance);
  ///   return ADDON_STATUS_OK;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  explicit CInstanceVisualization(const IInstanceInfo& instance) : IAddonInstance(instance)
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceVisualization: Creation of multiple together "
                             "with single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Destructor.
  ///
  ~CInstanceVisualization() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Used to notify the visualization that a new song has been started.
  ///
  /// @param[in] channels Number of channels in the stream
  /// @param[in] samplesPerSec Samples per second of stream
  /// @param[in] bitsPerSample Number of bits in one sample
  /// @param[in] songName The name of the currently-playing song
  /// @return true if start successful done
  ///
  virtual bool Start(int channels,
                     int samplesPerSec,
                     int bitsPerSample,
                     const std::string& songName)
  {
    return true;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Used to inform the visualization that the rendering control was
  /// stopped.
  ///
  virtual void Stop() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Pass audio data to the visualization.
  ///
  /// @param[in] audioData The raw audio data
  /// @param[in] audioDataLength Length of the audioData array
  ///
  virtual void AudioData(const float* audioData, size_t audioDataLength) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Used to inform Kodi that the rendered region is dirty and need an
  /// update.
  ///
  /// @return True if dirty
  ///
  virtual bool IsDirty() { return true; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Used to indicate when the add-on should render.
  ///
  virtual void Render() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Used to get the number of buffers from the current visualization.
  ///
  /// @return The number of buffers to delay before calling @ref AudioData()
  ///
  /// @note If this function is not implemented, it will default to 0.
  ///
  virtual int GetSyncDelay() { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Used to get a list of visualization presets the user can select.
  /// from
  ///
  /// @param[out] presets The vector list containing the names of presets that
  ///                     the user can select
  /// @return Return true if successful, or false if there are no presets to
  /// choose from
  ///
  virtual bool GetPresets(std::vector<std::string>& presets) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Get the index of the current preset.
  ///
  /// @return Index number of the current preset
  ///
  virtual int GetActivePreset() { return -1; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Check if the add-on is locked to the current preset.
  ///
  /// @return True if locked to the current preset
  ///
  virtual bool IsLocked() { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Load the previous visualization preset.
  ///
  /// @return Return true if the previous preset was loaded
  ///
  virtual bool PrevPreset() { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Load the next visualization preset.
  ///
  /// @return Return true if the next preset was loaded
  ///
  virtual bool NextPreset() { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Load a visualization preset.
  ///
  /// This function is called after a new preset is selected.
  ///
  /// @param[in] select Preset index to use
  /// @return Return true if the preset is loaded
  ///
  virtual bool LoadPreset(int select) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Switch to a new random preset.
  ///
  /// @return Return true if a random preset was loaded
  ///
  virtual bool RandomPreset() { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Lock the current visualization preset, preventing it from changing.
  ///
  /// @param[in] lockUnlock If set to true, the preset should be locked
  /// @return Return true if the current preset is locked
  ///
  virtual bool LockPreset(bool lockUnlock) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Used to increase/decrease the visualization preset rating.
  ///
  /// @param[in] plusMinus If set to true the rating is increased, otherwise
  ///                      decreased
  /// @return Return true if the rating is modified
  ///
  virtual bool RatePreset(bool plusMinus) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Inform the visualization of the current album art image.
  ///
  /// @param[in] albumart Path to the current album art image
  /// @return Return true if the image is used
  ///
  virtual bool UpdateAlbumart(const std::string& albumart) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief Inform the visualization of the current track's tag information.
  ///
  /// @param[in] track Visualization track information structure
  /// @return Return true if the track information is used
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_visualization_Defs_VisualizationTrack_Help
  ///
  ///-------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  ///
  /// #include <kodi/addon-instance/Visualization.h>
  ///
  /// class CMyVisualization : public kodi::addon::CInstanceVisualization
  /// {
  /// public:
  ///   CMyVisualization(KODI_HANDLE instance, const std::string& version);
  ///
  ///   ...
  ///
  /// private:
  ///   kodi::addon::VisualizationTrack m_runningTrack;
  /// };
  ///
  /// bool CMyVisualization::UpdateTrack(const kodi::addon::VisualizationTrack& track)
  /// {
  ///   m_runningTrack = track;
  ///   return true;
  /// }
  ///
  /// ~~~~~~~~~~~~~
  ///
  virtual bool UpdateTrack(const kodi::addon::VisualizationTrack& track) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_visualization_CB Information functions
  /// @ingroup cpp_kodi_addon_visualization
  /// @brief **To get info about the device, display and several other parts**\n
  /// These are functions to query any values or to transfer them to Kodi.
  ///
  ///@{

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization_CB
  /// @brief To transfer available presets on addon.
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
    m_instanceData->visualization->toKodi->clear_presets(m_instanceData->info->kodi);
    for (const auto& it : presets)
      m_instanceData->visualization->toKodi->transfer_preset(m_instanceData->info->kodi,
                                                             it.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization_CB
  /// @brief Device that represents the display adapter.
  ///
  /// @return A pointer to the used device with @ref cpp_kodi_Defs_HardwareContext "HardwareContext"
  ///
  /// @note This is only available on **DirectX**, It us unused (`nullptr`) on
  /// **OpenGL**
  ///
  ///-------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <d3d11_1.h>
  /// ..
  /// // Note: Device() there is used inside addon child class about
  /// // kodi::addon::CInstanceVisualization
  /// ID3D11DeviceContext1* context = static_cast<ID3D11DeviceContext1*>(kodi::addon::CInstanceVisualization::Device());
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  inline kodi::HardwareContext Device() { return m_props.device; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization_CB
  /// @brief Returns the X position of the rendering window.
  ///
  /// @return The X position, in pixels
  ///
  inline int X() { return m_props.x; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization_CB
  /// @brief Returns the Y position of the rendering window.
  ///
  /// @return The Y position, in pixels
  ///
  inline int Y() { return m_props.y; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization_CB
  /// @brief Returns the width of the rendering window.
  ///
  /// @return The width, in pixels
  ///
  inline int Width() { return m_props.width; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization_CB
  /// @brief Returns the height of the rendering window.
  ///
  /// @return The height, in pixels
  ///
  inline int Height() { return m_props.height; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_visualization_CB
  /// @brief Pixel aspect ratio (often abbreviated PAR) is a ratio that
  /// describes how the width of a pixel compares to the height of that pixel.
  ///
  /// @return The pixel aspect ratio used by the display
  ///
  inline float PixelRatio() { return m_props.pixelRatio; }
  //----------------------------------------------------------------------------

  ///@}

private:
  void SetAddonStruct(KODI_ADDON_INSTANCE_STRUCT* instance)
  {
    m_instanceData = instance;
    m_instanceData->hdl = this;
    m_instanceData->visualization->toAddon->start = ADDON_start;
    m_instanceData->visualization->toAddon->stop = ADDON_stop;
    m_instanceData->visualization->toAddon->audio_data = ADDON_audio_data;
    m_instanceData->visualization->toAddon->is_dirty = ADDON_is_dirty;
    m_instanceData->visualization->toAddon->render = ADDON_render;
    m_instanceData->visualization->toAddon->get_sync_delay = ADDON_get_sync_delay;
    m_instanceData->visualization->toAddon->prev_preset = ADDON_prev_preset;
    m_instanceData->visualization->toAddon->next_preset = ADDON_next_preset;
    m_instanceData->visualization->toAddon->load_preset = ADDON_load_preset;
    m_instanceData->visualization->toAddon->random_preset = ADDON_random_preset;
    m_instanceData->visualization->toAddon->lock_preset = ADDON_lock_preset;
    m_instanceData->visualization->toAddon->rate_preset = ADDON_rate_preset;
    m_instanceData->visualization->toAddon->update_albumart = ADDON_update_albumart;
    m_instanceData->visualization->toAddon->update_track = ADDON_update_track;
    m_instanceData->visualization->toAddon->get_presets = ADDON_get_presets;
    m_instanceData->visualization->toAddon->get_active_preset = ADDON_get_active_preset;
    m_instanceData->visualization->toAddon->is_locked = ADDON_is_locked;

    m_instanceData->visualization->toKodi->get_properties(instance->info->kodi, &m_props);
  }

  inline static bool ADDON_start(const KODI_ADDON_VISUALIZATION_HDL hdl,
                                 int channels,
                                 int samplesPerSec,
                                 int bitsPerSample,
                                 const char* songName)
  {
    CInstanceVisualization* thisClass = static_cast<CInstanceVisualization*>(hdl);
    thisClass->m_renderHelper = kodi::gui::GetRenderHelper();
    return thisClass->Start(channels, samplesPerSec, bitsPerSample, songName);
  }

  inline static void ADDON_stop(const KODI_ADDON_VISUALIZATION_HDL hdl)
  {
    CInstanceVisualization* thisClass = static_cast<CInstanceVisualization*>(hdl);
    thisClass->Stop();
    thisClass->m_renderHelper = nullptr;
  }

  inline static void ADDON_audio_data(const KODI_ADDON_VISUALIZATION_HDL hdl,
                                      const float* audioData,
                                      size_t audioDataLength)
  {
    static_cast<CInstanceVisualization*>(hdl)->AudioData(audioData, audioDataLength);
  }

  inline static bool ADDON_is_dirty(const KODI_ADDON_VISUALIZATION_HDL hdl)
  {
    return static_cast<CInstanceVisualization*>(hdl)->IsDirty();
  }

  inline static void ADDON_render(const KODI_ADDON_VISUALIZATION_HDL hdl)
  {
    CInstanceVisualization* thisClass = static_cast<CInstanceVisualization*>(hdl);
    if (!thisClass->m_renderHelper)
      return;
    thisClass->m_renderHelper->Begin();
    thisClass->Render();
    thisClass->m_renderHelper->End();
  }

  inline static int ADDON_get_sync_delay(const KODI_ADDON_VISUALIZATION_HDL hdl)
  {
    return static_cast<CInstanceVisualization*>(hdl)->GetSyncDelay();
  }

  inline static unsigned int ADDON_get_presets(const KODI_ADDON_VISUALIZATION_HDL hdl)
  {
    CInstanceVisualization* thisClass = static_cast<CInstanceVisualization*>(hdl);
    std::vector<std::string> presets;
    if (thisClass->GetPresets(presets))
    {
      for (const auto& it : presets)
        thisClass->m_instanceData->visualization->toKodi->transfer_preset(
            thisClass->m_instanceData->info->kodi, it.c_str());
    }

    return static_cast<unsigned int>(presets.size());
  }

  inline static int ADDON_get_active_preset(const KODI_ADDON_VISUALIZATION_HDL hdl)
  {
    return static_cast<CInstanceVisualization*>(hdl)->GetActivePreset();
  }

  inline static bool ADDON_prev_preset(const KODI_ADDON_VISUALIZATION_HDL hdl)
  {
    return static_cast<CInstanceVisualization*>(hdl)->PrevPreset();
  }

  inline static bool ADDON_next_preset(const KODI_ADDON_VISUALIZATION_HDL hdl)
  {
    return static_cast<CInstanceVisualization*>(hdl)->NextPreset();
  }

  inline static bool ADDON_load_preset(const KODI_ADDON_VISUALIZATION_HDL hdl, int select)

  {
    return static_cast<CInstanceVisualization*>(hdl)->LoadPreset(select);
  }

  inline static bool ADDON_random_preset(const KODI_ADDON_VISUALIZATION_HDL hdl)
  {
    return static_cast<CInstanceVisualization*>(hdl)->RandomPreset();
  }

  inline static bool ADDON_lock_preset(const KODI_ADDON_VISUALIZATION_HDL hdl)
  {
    CInstanceVisualization* thisClass = static_cast<CInstanceVisualization*>(hdl);
    thisClass->m_presetLockedByUser = !thisClass->m_presetLockedByUser;
    return thisClass->LockPreset(thisClass->m_presetLockedByUser);
  }

  inline static bool ADDON_rate_preset(const KODI_ADDON_VISUALIZATION_HDL hdl, bool plus_minus)
  {
    return static_cast<CInstanceVisualization*>(hdl)->RatePreset(plus_minus);
  }

  inline static bool ADDON_is_locked(const KODI_ADDON_VISUALIZATION_HDL hdl)
  {
    return static_cast<CInstanceVisualization*>(hdl)->IsLocked();
  }

  inline static bool ADDON_update_albumart(const KODI_ADDON_VISUALIZATION_HDL hdl,
                                           const char* albumart)
  {
    return static_cast<CInstanceVisualization*>(hdl)->UpdateAlbumart(albumart);
  }

  inline static bool ADDON_update_track(const KODI_ADDON_VISUALIZATION_HDL hdl,
                                        const KODI_ADDON_VISUALIZATION_TRACK* track)
  {
    VisualizationTrack cppTrack(track);
    return static_cast<CInstanceVisualization*>(hdl)->UpdateTrack(cppTrack);
  }

  std::shared_ptr<kodi::gui::IRenderHelper> m_renderHelper;
  bool m_presetLockedByUser = false;
  KODI_ADDON_INSTANCE_STRUCT* m_instanceData{nullptr};
  KODI_ADDON_VISUALIZATION_PROPS m_props = {};
};

} /* namespace addon */
} /* namespace kodi */
#endif /* __cplusplus */
