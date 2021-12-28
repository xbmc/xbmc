/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/audioencoder.h"

#ifdef __cplusplus

#include <stdexcept>

namespace kodi
{
namespace addon
{

class CInstanceAudioEncoder;

//==============================================================================
/// @defgroup cpp_kodi_addon_audioencoder_Defs_AudioEncoderInfoTag class AudioEncoderInfoTag
/// @ingroup cpp_kodi_addon_audioencoder_Defs
/// @brief **Info tag data structure**\n
/// Representation of available information of processed audio file.
///
/// This is used to get all the necessary data of audio stream and to have on
/// created files by encoders.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_audioencoder_Defs_AudioEncoderInfoTag_Help
///
///@{
class ATTR_DLL_LOCAL AudioEncoderInfoTag
{
public:
  /*! \cond PRIVATE */
  AudioEncoderInfoTag() = default;
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_audioencoder_Defs_AudioEncoderInfoTag_Help Value Help
  /// @ingroup cpp_kodi_addon_audioencoder_Defs_AudioEncoderInfoTag
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_audioencoder_Defs_AudioEncoderInfoTag :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Title** | `std::string` | @ref AudioEncoderInfoTag::SetTitle "SetTitle" | @ref AudioEncoderInfoTag::GetTitle "GetTitle"
  /// | **Artist** | `std::string` | @ref AudioEncoderInfoTag::SetArtist "SetArtist" | @ref AudioEncoderInfoTag::GetArtist "GetArtist"
  /// | **Album** | `std::string` | @ref AudioEncoderInfoTag::SetAlbum "SetAlbum" | @ref AudioEncoderInfoTag::GetAlbum "GetAlbum"
  /// | **Album artist** | `std::string` | @ref AudioEncoderInfoTag::SetAlbumArtist "SetAlbumArtist" | @ref AudioEncoderInfoTag::GetAlbumArtist "GetAlbumArtist"
  /// | **Media type** | `std::string` | @ref AudioEncoderInfoTag::SetMediaType "SetMediaType" | @ref AudioEncoderInfoTag::GetMediaType "GetMediaType"
  /// | **Genre** | `std::string` | @ref AudioEncoderInfoTag::SetGenre "SetGenre" | @ref AudioEncoderInfoTag::GetGenre "GetGenre"
  /// | **Duration** | `int` | @ref AudioEncoderInfoTag::SetDuration "SetDuration" | @ref AudioEncoderInfoTag::GetDuration "GetDuration"
  /// | **Track number** | `int` | @ref AudioEncoderInfoTag::SetTrack "SetTrack" | @ref AudioEncoderInfoTag::GetTrack "GetTrack"
  /// | **Disc number** | `int` | @ref AudioEncoderInfoTag::SetDisc "SetDisc" | @ref AudioEncoderInfoTag::GetDisc "GetDisc"
  /// | **Disc subtitle name** | `std::string` | @ref AudioEncoderInfoTag::SetDiscSubtitle "SetDiscSubtitle" | @ref AudioEncoderInfoTag::GetDiscSubtitle "GetDiscSubtitle"
  /// | **Disc total amount** | `int` | @ref AudioEncoderInfoTag::SetDiscTotal "SetDiscTotal" | @ref AudioEncoderInfoTag::GetDiscTotal "GetDiscTotal"
  /// | **Release date** | `std::string` | @ref AudioEncoderInfoTag::SetReleaseDate "SetReleaseDate" | @ref AudioEncoderInfoTag::GetReleaseDate "GetReleaseDate"
  /// | **Lyrics** | `std::string` | @ref AudioEncoderInfoTag::SetLyrics "SetLyrics" | @ref AudioEncoderInfoTag::GetLyrics "GetLyrics"
  /// | **Samplerate** | `int` | @ref AudioEncoderInfoTag::SetSamplerate "SetSamplerate" | @ref AudioEncoderInfoTag::GetSamplerate "GetSamplerate"
  /// | **Channels amount** | `int` | @ref AudioEncoderInfoTag::SetChannels "SetChannels" | @ref AudioEncoderInfoTag::GetChannels "GetChannels"
  /// | **Bits per sample** | `int` | @ref AudioEncoderInfoTag::SetBitsPerSample "SetBitsPerSample" | @ref AudioEncoderInfoTag::GetBitsPerSample "GetBitsPerSample"
  /// | **Track length** | `int` | @ref AudioEncoderInfoTag::SetTrackLength "SetTrackLength" | @ref AudioEncoderInfoTag::GetTrackLength "GetTrackLength"
  /// | **Comment text** | `std::string` | @ref AudioEncoderInfoTag::SetComment "SetComment" | @ref AudioEncoderInfoTag::GetComment "GetComment"
  ///
  /// @addtogroup cpp_kodi_addon_audioencoder_Defs_AudioEncoderInfoTag
  ///@{

  /// @brief Set the title from music as string on info tag.
  void SetTitle(const std::string& title) { m_title = title; }

  /// @brief Get title name
  const std::string& GetTitle() const { return m_title; }

  /// @brief Set artist name
  void SetArtist(const std::string& artist) { m_artist = artist; }

  /// @brief Get artist name
  const std::string& GetArtist() const { return m_artist; }

  /// @brief Set album name
  void SetAlbum(const std::string& album) { m_album = album; }

  /// @brief Get album name
  const std::string& GetAlbum() const { return m_album; }

  /// @brief Set album artist name
  void SetAlbumArtist(const std::string& albumArtist) { m_album_artist = albumArtist; }

  /// @brief Get album artist name
  const std::string& GetAlbumArtist() const { return m_album_artist; }

  /// @brief Set the media type of the music item.
  ///
  /// Available strings about media type for music:
  /// | String         | Description                                       |
  /// |---------------:|:--------------------------------------------------|
  /// | artist         | If it is defined as an artist
  /// | album          | If it is defined as an album
  /// | music          | If it is defined as an music
  /// | song           | If it is defined as a song
  ///
  void SetMediaType(const std::string& mediaType) { m_media_type = mediaType; }

  /// @brief Get the media type of the music item.
  const std::string& GetMediaType() const { return m_media_type; }

  /// @brief Set genre name from music as string if present.
  void SetGenre(const std::string& genre) { m_genre = genre; }

  /// @brief Get genre name from music as string if present.
  const std::string& GetGenre() const { return m_genre; }

  /// @brief Set the duration of music as integer from info.
  void SetDuration(int duration) { m_duration = duration; }

  /// @brief Get the duration of music as integer from info.
  int GetDuration() const { return m_duration; }

  /// @brief Set track number (if present) from music info as integer.
  void SetTrack(int track) { m_track = track; }

  /// @brief Get track number (if present).
  int GetTrack() const { return m_track; }

  /// @brief Set disk number (if present) from music info as integer.
  void SetDisc(int disc) { m_disc = disc; }

  /// @brief Get disk number (if present)
  int GetDisc() const { return m_disc; }

  /// @brief Set disk subtitle name (if present) from music info.
  void SetDiscSubtitle(const std::string& discSubtitle) { m_disc_subtitle = discSubtitle; }

  /// @brief Get disk subtitle name (if present) from music info.
  const std::string& GetDiscSubtitle() const { return m_disc_subtitle; }

  /// @brief Set disks amount quantity (if present) from music info as integer.
  void SetDiscTotal(int discTotal) { m_disc_total = discTotal; }

  /// @brief Get disks amount quantity (if present)
  int GetDiscTotal() const { return m_disc_total; }

  /// @brief Set release date as string from music info (if present).\n
  /// [ISO8601](https://en.wikipedia.org/wiki/ISO_8601) date YYYY, YYYY-MM or YYYY-MM-DD
  void SetReleaseDate(const std::string& releaseDate) { m_release_date = releaseDate; }

  /// @brief Get release date as string from music info (if present).
  const std::string& GetReleaseDate() const { return m_release_date; }

  /// @brief Set string from lyrics.
  void SetLyrics(const std::string& lyrics) { m_lyrics = lyrics; }

  /// @brief Get string from lyrics.
  const std::string& GetLyrics() const { return m_lyrics; }

  /// @brief Set related stream samplerate.
  void SetSamplerate(int samplerate) { m_samplerate = samplerate; }

  /// @brief Get related stream samplerate.
  int GetSamplerate() const { return m_samplerate; }

  /// @brief Set related stream channels amount.
  void SetChannels(int channels) { m_channels = channels; }

  /// @brief Get related stream channels amount.
  int GetChannels() const { return m_channels; }

  /// @brief Set related stream bits per sample.
  void SetBitsPerSample(int bits_per_sample) { m_bits_per_sample = bits_per_sample; }

  /// @brief Get related stream bits per sample.
  int GetBitsPerSample() const { return m_bits_per_sample; }

  /// @brief Set related stream track length.
  void SetTrackLength(int track_length) { m_track_length = track_length; }

  /// @brief Get related stream track length.
  int GetTrackLength() const { return m_track_length; }

  /// @brief Set additional information comment (if present).
  void SetComment(const std::string& comment) { m_comment = comment; }

  /// @brief Get additional information comment (if present).
  const std::string& GetComment() const { return m_comment; }

  ///@}

private:
  friend class CInstanceAudioEncoder;

  AudioEncoderInfoTag(const struct KODI_ADDON_AUDIOENCODER_INFO_TAG* tag)
  {
    if (tag->title)
      m_title = tag->title;
    if (tag->artist)
      m_artist = tag->artist;
    if (tag->album)
      m_album = tag->album;
    if (tag->album_artist)
      m_album_artist = tag->album_artist;
    if (tag->media_type)
      m_media_type = tag->media_type;
    if (tag->genre)
      m_genre = tag->genre;
    m_duration = tag->duration;
    m_track = tag->track;
    m_disc = tag->disc;
    if (tag->artist)
      m_disc_subtitle = tag->artist;
    m_disc_total = tag->disc_total;
    if (tag->release_date)
      m_release_date = tag->release_date;
    if (tag->lyrics)
      m_lyrics = tag->lyrics;
    m_samplerate = tag->samplerate;
    m_channels = tag->channels;
    m_bits_per_sample = tag->bits_per_sample;
    m_track_length = tag->track_length;
    if (tag->comment)
      m_comment = tag->comment;
  }

  std::string m_title;
  std::string m_artist;
  std::string m_album;
  std::string m_album_artist;
  std::string m_media_type;
  std::string m_genre;
  int m_duration{0};
  int m_track{0};
  int m_disc{0};
  std::string m_disc_subtitle;
  int m_disc_total{0};
  std::string m_release_date;
  std::string m_lyrics;
  int m_samplerate{0};
  int m_channels{0};
  int m_bits_per_sample{0};
  int m_track_length{0};
  std::string m_comment;
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @addtogroup cpp_kodi_addon_audioencoder
/// @brief \cpp_class{ kodi::addon::CInstanceAudioEncoder }
/// **Audio encoder add-on instance.**\n
/// For audio encoders as binary add-ons. This class implements a way to handle
/// the encode of given stream to a new format.
///
/// The addon.xml defines the capabilities of this add-on.
///
///
/// ----------------------------------------------------------------------------
///
/// **Here's an example on addon.xml:**
/// ~~~~~~~~~~~~~{.xml}
///   <extension
///     point="kodi.audioencoder"
///     extension=".flac"
///     library_@PLATFORM@="@LIBRARY_FILENAME@"/>
/// ~~~~~~~~~~~~~
///
/// Description to audio encoder related addon.xml values:
/// | Name                          | Description
/// |:------------------------------|----------------------------------------
/// | <b>`point`</b>                | Addon type specification<br>At all addon types and for this kind always <b>"kodi.audioencoder"</b>.
/// | <b>`library_@PLATFORM@`</b>   | Sets the used library name, which is automatically set by cmake at addon build.
/// | <b>`extension`</b>            | The file extensions / styles supported by this addon.
///
/// --------------------------------------------------------------------------
///
/// --------------------------------------------------------------------------
///
/// **Here is a code example how this addon is used:**
///
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/AudioEncoder.h>
///
/// class ATTR_DLL_LOCAL CMyAudioEncoder : public kodi::addon::CInstanceAudioEncoder
/// {
/// public:
///   CMyAudioEncoder(const kodi::addon::IInstanceInfo& instance);
///
///   bool Start(const kodi::addon::AudioEncoderInfoTag& tag) override;
///   int Encode(int numBytesRead, const uint8_t* pbtStream) override;
///   bool Finish() override; // Optional
/// };
///
/// CMyAudioEncoder::CMyAudioEncoder(const kodi::addon::IInstanceInfo& instance)
///   : kodi::addon::CInstanceAudioEncoder(instance)
/// {
///   ...
/// }
///
/// bool CMyAudioEncoder::Start(const kodi::addon::AudioEncoderInfoTag& tag)
/// {
///   ...
///   return true;
/// }
///
/// ssize_t CMyAudioEncoder::Encode(const uint8_t* pbtStream, size_t numBytesRead)
/// {
///   uint8_t* data = nullptr;
///   size_t length = 0;
///   ...
///   kodi::addon::CInstanceAudioEncoder::Write(data, length);
///
///   return 0;
/// }
///
///
/// bool CMyAudioEncoder::Finish()
/// {
///   ...
///   return true;
/// }
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
///   if (instance.IsType(ADDON_INSTANCE_AUDIOENCODER))
///   {
///     kodi::Log(ADDON_LOG_INFO, "Creating my audio encoder instance");
///     hdl = new CMyAudioEncoder(instance);
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
/// The destruction of the example class `CMyAudioEncoder` is called from
/// Kodi's header. Manually deleting the add-on instance is not required.
///
class ATTR_DLL_LOCAL CInstanceAudioEncoder : public IAddonInstance
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_addon_audioencoder
  /// @brief Audio encoder class constructor used to support multiple instances.
  ///
  /// @param[in] instance The instance value given to
  ///                     <b>`kodi::addon::CAddonBase::CreateInstance(...)`</b>.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Here's example about the use of this:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// class CMyAudioEncoder : public kodi::addon::CInstanceAudioEncoder
  /// {
  /// public:
  ///   CMyAudioEncoder(const kodi::addon::IInstanceInfo& instance)
  ///     : kodi::addon::CInstanceAudioEncoder(instance)
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
  ///   kodi::Log(ADDON_LOG_INFO, "Creating my audio encoder instance");
  ///   hdl = new CMyAudioEncoder(instance);
  ///   return ADDON_STATUS_OK;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  explicit CInstanceAudioEncoder(const kodi::addon::IInstanceInfo& instance)
    : IAddonInstance(instance)
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceAudioEncoder: Creation of multiple together "
                             "with single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audioencoder
  /// @brief Start encoder (**required**)
  ///
  /// @param[in] tag Information tag about
  /// @return True on success, false on failure.
  ///
  virtual bool Start(const kodi::addon::AudioEncoderInfoTag& tag) = 0;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audioencoder
  /// @brief Encode a chunk of audio (**required**)
  ///
  /// @param[in] pbtStream The input buffer
  /// @param[in] numBytesRead Number of bytes in input buffer
  /// @return Number of bytes consumed
  ///
  virtual ssize_t Encode(const uint8_t* pbtStream, size_t numBytesRead) = 0;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audioencoder
  /// @brief Finalize encoding (**optional**)
  ///
  /// @return True on success, false on failure.
  ///
  virtual bool Finish() { return true; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audioencoder
  /// @brief Write block of data
  ///
  /// @param[in] data Pointer to the array of elements to be written
  /// @param[in] length Size in bytes to be written.
  /// @return The total number of bytes successfully written is returned.
  ///
  /// @remarks Only called from addon itself.
  ///
  ssize_t Write(const uint8_t* data, size_t length)
  {
    return m_kodi->write(m_kodi->kodiInstance, data, length);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audioencoder
  /// @brief Set the file's current position.
  ///
  /// The whence argument is optional and defaults to SEEK_SET (0)
  ///
  /// @param[in] position The position that you want to seek to
  /// @param[in] whence [optional] offset relative to\n
  ///                              You can set the value of whence to one
  ///                              of three things:
  ///  |   Value  | int | Description                                        |
  ///  |:--------:|:---:|:---------------------------------------------------|
  ///  | SEEK_SET |  0  | position is relative to the beginning of the file. This is probably what you had in mind anyway, and is the most commonly used value for whence.
  ///  | SEEK_CUR |  1  | position is relative to the current file pointer position. So, in effect, you can say, "Move to my current position plus 30 bytes," or, "move to my current position minus 20 bytes."
  ///  | SEEK_END |  2  | position is relative to the end of the file. Just like SEEK_SET except from the other end of the file. Be sure to use negative values for offset if you want to back up from the end of the file, instead of going past the end into oblivion.
  ///
  /// @return Returns the resulting offset location as measured in bytes from
  ///         the beginning of the file. On error, the value -1 is returned.
  ///
  /// @remarks Only called from addon itself.
  ///
  ssize_t Seek(ssize_t position, int whence = SEEK_SET)
  {
    return m_kodi->seek(m_kodi->kodiInstance, position, whence);
  }
  //----------------------------------------------------------------------------

private:
  void SetAddonStruct(KODI_ADDON_INSTANCE_STRUCT* instance)
  {
    instance->hdl = this;
    instance->audioencoder->toAddon->start = ADDON_start;
    instance->audioencoder->toAddon->encode = ADDON_encode;
    instance->audioencoder->toAddon->finish = ADDON_finish;
    m_kodi = instance->audioencoder->toKodi;
  }

  inline static bool ADDON_start(const KODI_ADDON_AUDIOENCODER_HDL hdl,
                                 const struct KODI_ADDON_AUDIOENCODER_INFO_TAG* tag)
  {
    return static_cast<CInstanceAudioEncoder*>(hdl)->Start(tag);
  }

  inline static ssize_t ADDON_encode(const KODI_ADDON_AUDIOENCODER_HDL hdl,
                                     const uint8_t* pbtStream,
                                     size_t num_bytes_read)
  {
    return static_cast<CInstanceAudioEncoder*>(hdl)->Encode(pbtStream, num_bytes_read);
  }

  inline static bool ADDON_finish(const KODI_ADDON_AUDIOENCODER_HDL hdl)
  {
    return static_cast<CInstanceAudioEncoder*>(hdl)->Finish();
  }

  AddonToKodiFuncTable_AudioEncoder* m_kodi{nullptr};
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
