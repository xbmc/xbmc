/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../AudioEngine.h"
#include "../c-api/addon-instance/audio_decoder.h"

#ifdef __cplusplus
namespace kodi
{
namespace addon
{

class CInstanceAudioDecoder;

//==============================================================================
/// @defgroup cpp_kodi_addon_audiodecoder_Defs_AudioDecoderInfoTag class AudioDecoderInfoTag
/// @ingroup cpp_kodi_addon_audiodecoder_Defs
/// @brief **Info tag data structure**\n
/// Representation of available information of processed audio file.
///
/// This is used to store all the necessary data of audio stream and to have on
/// e.g. GUI for information.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_audiodecoder_Defs_AudioDecoderInfoTag_Help
///
///@{
class AudioDecoderInfoTag : public CStructHdl<AudioDecoderInfoTag, AUDIO_DECODER_INFO_TAG>
{
  /*! \cond PRIVATE */
  friend class CInstanceAudioDecoder;
  /*! \endcond */

public:
  /*! \cond PRIVATE */
  AudioDecoderInfoTag() { memset(m_cStructure, 0, sizeof(AUDIO_DECODER_INFO_TAG)); }
  AudioDecoderInfoTag(const AudioDecoderInfoTag& tag) : CStructHdl(tag) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_audiodecoder_Defs_AudioDecoderInfoTag_Help Value Help
  /// @ingroup cpp_kodi_addon_audiodecoder_Defs_AudioDecoderInfoTag
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_audiodecoder_Defs_AudioDecoderInfoTag :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Title** | `std::string` | @ref AudioDecoderInfoTag::SetTitle "SetTitle" | @ref AudioDecoderInfoTag::GetTitle "GetTitle"
  /// | **Artist** | `std::string` | @ref AudioDecoderInfoTag::SetArtist "SetArtist" | @ref AudioDecoderInfoTag::GetArtist "GetArtist"
  /// | **Album** | `std::string` | @ref AudioDecoderInfoTag::SetAlbum "SetAlbum" | @ref AudioDecoderInfoTag::GetAlbum "GetAlbum"
  /// | **Album artist** | `std::string` | @ref AudioDecoderInfoTag::SetAlbumArtist "SetAlbumArtist" | @ref AudioDecoderInfoTag::GetAlbumArtist "GetAlbumArtist"
  /// | **Media type** | `std::string` | @ref AudioDecoderInfoTag::SetMediaType "SetMediaType" | @ref AudioDecoderInfoTag::GetMediaType "GetMediaType"
  /// | **Genre** | `std::string` | @ref AudioDecoderInfoTag::SetGenre "SetGenre" | @ref AudioDecoderInfoTag::GetGenre "GetGenre"
  /// | **Duration** | `int` | @ref AudioDecoderInfoTag::SetDuration "SetDuration" | @ref AudioDecoderInfoTag::GetDuration "GetDuration"
  /// | **Track number** | `int` | @ref AudioDecoderInfoTag::SetTrack "SetTrack" | @ref AudioDecoderInfoTag::GetTrack "GetTrack"
  /// | **Disc number** | `int` | @ref AudioDecoderInfoTag::SetDisc "SetDisc" | @ref AudioDecoderInfoTag::GetDisc "GetDisc"
  /// | **Disc subtitle name** | `std::string` | @ref AudioDecoderInfoTag::SetDiscSubtitle "SetDiscSubtitle" | @ref AudioDecoderInfoTag::GetDiscSubtitle "GetDiscSubtitle"
  /// | **Disc total amount** | `int` | @ref AudioDecoderInfoTag::SetDiscTotal "SetDiscTotal" | @ref AudioDecoderInfoTag::GetDiscTotal "GetDiscTotal"
  /// | **Release date** | `std::string` | @ref AudioDecoderInfoTag::SetReleaseDate "SetReleaseDate" | @ref AudioDecoderInfoTag::GetReleaseDate "GetReleaseDate"
  /// | **Lyrics** | `std::string` | @ref AudioDecoderInfoTag::SetLyrics "SetLyrics" | @ref AudioDecoderInfoTag::GetLyrics "GetLyrics"
  /// | **Samplerate** | `int` | @ref AudioDecoderInfoTag::SetSamplerate "SetSamplerate" | @ref AudioDecoderInfoTag::GetSamplerate "GetSamplerate"
  /// | **Channels amount** | `int` | @ref AudioDecoderInfoTag::SetChannels "SetChannels" | @ref AudioDecoderInfoTag::GetChannels "GetChannels"
  /// | **Bitrate** | `int` | @ref AudioDecoderInfoTag::SetBitrate "SetBitrate" | @ref AudioDecoderInfoTag::GetBitrate "GetBitrate"
  /// | **Comment text** | `std::string` | @ref AudioDecoderInfoTag::SetComment "SetComment" | @ref AudioDecoderInfoTag::GetComment "GetComment"
  ///

  /// @addtogroup cpp_kodi_addon_audiodecoder_Defs_AudioDecoderInfoTag
  ///@{

  /// @brief Set the title from music as string on info tag.
  void SetTitle(const std::string& title)
  {
    strncpy(m_cStructure->title, title.c_str(), sizeof(m_cStructure->title) - 1);
  }

  /// @brief Get title name
  std::string GetTitle() const { return m_cStructure->title; }

  /// @brief Set artist name
  void SetArtist(const std::string& artist)
  {
    strncpy(m_cStructure->artist, artist.c_str(), sizeof(m_cStructure->artist) - 1);
  }

  /// @brief Get artist name
  std::string GetArtist() const { return m_cStructure->artist; }

  /// @brief Set album name
  void SetAlbum(const std::string& album)
  {
    strncpy(m_cStructure->album, album.c_str(), sizeof(m_cStructure->album) - 1);
  }

  /// @brief Set album name
  std::string GetAlbum() const { return m_cStructure->album; }

  /// @brief Set album artist name
  void SetAlbumArtist(const std::string& albumArtist)
  {
    strncpy(m_cStructure->album_artist, albumArtist.c_str(),
            sizeof(m_cStructure->album_artist) - 1);
  }

  /// @brief Get album artist name
  std::string GetAlbumArtist() const { return m_cStructure->album_artist; }

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
  void SetMediaType(const std::string& mediaType)
  {
    strncpy(m_cStructure->media_type, mediaType.c_str(), sizeof(m_cStructure->media_type) - 1);
  }

  /// @brief Get the media type of the music item.
  std::string GetMediaType() const { return m_cStructure->media_type; }

  /// @brief Set genre name from music as string if present.
  void SetGenre(const std::string& genre)
  {
    strncpy(m_cStructure->genre, genre.c_str(), sizeof(m_cStructure->genre) - 1);
  }

  /// @brief Get genre name from music as string if present.
  std::string GetGenre() const { return m_cStructure->genre; }

  /// @brief Set the duration of music as integer from info.
  void SetDuration(int duration) { m_cStructure->duration = duration; }

  /// @brief Get the duration of music as integer from info.
  int GetDuration() const { return m_cStructure->duration; }

  /// @brief Set track number (if present) from music info as integer.
  void SetTrack(int track) { m_cStructure->track = track; }

  /// @brief Get track number (if present).
  int GetTrack() const { return m_cStructure->track; }

  /// @brief Set disk number (if present) from music info as integer.
  void SetDisc(int disc) { m_cStructure->disc = disc; }

  /// @brief Get disk number (if present)
  int GetDisc() const { return m_cStructure->disc; }

  /// @brief Set disk subtitle name (if present) from music info.
  void SetDiscSubtitle(const std::string& discSubtitle)
  {
    strncpy(m_cStructure->disc_subtitle, discSubtitle.c_str(),
            sizeof(m_cStructure->disc_subtitle) - 1);
  }

  /// @brief Get disk subtitle name (if present) from music info.
  std::string GetDiscSubtitle() const { return m_cStructure->disc_subtitle; }

  /// @brief Set disks amount quantity (if present) from music info as integer.
  void SetDiscTotal(int discTotal) { m_cStructure->disc_total = discTotal; }

  /// @brief Get disks amount quantity (if present)
  int GetDiscTotal() const { return m_cStructure->disc_total; }

  /// @brief Set release date as string from music info (if present).\n
  /// [ISO8601](https://en.wikipedia.org/wiki/ISO_8601) date YYYY, YYYY-MM or YYYY-MM-DD
  void SetReleaseDate(const std::string& releaseDate)
  {
    strncpy(m_cStructure->release_date, releaseDate.c_str(),
            sizeof(m_cStructure->release_date) - 1);
  }

  /// @brief Get release date as string from music info (if present).
  std::string GetReleaseDate() const { return m_cStructure->release_date; }

  /// @brief Set string from lyrics.
  void SetLyrics(const std::string& lyrics)
  {
    strncpy(m_cStructure->lyrics, lyrics.c_str(), sizeof(m_cStructure->lyrics) - 1);
  }

  /// @brief Get string from lyrics.
  std::string GetLyrics() const { return m_cStructure->lyrics; }

  /// @brief Set related stream samplerate.
  void SetSamplerate(int samplerate) { m_cStructure->samplerate = samplerate; }

  /// @brief Get related stream samplerate.
  int GetSamplerate() const { return m_cStructure->samplerate; }

  /// @brief Set related stream channels amount.
  void SetChannels(int channels) { m_cStructure->channels = channels; }

  /// @brief Get related stream channels amount.
  int GetChannels() const { return m_cStructure->channels; }

  /// @brief Set related stream bitrate.
  void SetBitrate(int bitrate) { m_cStructure->bitrate = bitrate; }

  /// @brief Get related stream bitrate.
  int GetBitrate() const { return m_cStructure->bitrate; }

  /// @brief Set additional information comment (if present).
  void SetComment(const std::string& comment)
  {
    strncpy(m_cStructure->comment, comment.c_str(), sizeof(m_cStructure->comment) - 1);
  }

  /// @brief Get additional information comment (if present).
  std::string GetComment() const { return m_cStructure->comment; }

  ///@}

private:
  AudioDecoderInfoTag(const AUDIO_DECODER_INFO_TAG* tag) : CStructHdl(tag) {}
  AudioDecoderInfoTag(AUDIO_DECODER_INFO_TAG* tag) : CStructHdl(tag) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_audiodecoder_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_addon_audiodecoder
/// @brief **Audio decoder add-on instance definition values**\n
/// All audio decoder functions associated data structures.
///
/// Used to exchange the available options between Kodi and addon.\n
/// The groups described here correspond to the groups of functions on audio
/// decoder instance class.
///

//==============================================================================
///
/// @addtogroup cpp_kodi_addon_audiodecoder
/// @brief \cpp_class{ kodi::addon::CInstanceAudioDecoder }
/// **Audio decoder add-on instance**
///
/// For audio decoders as binary add-ons. This class implements a way to handle
/// special types of audio files.
///
/// The add-on handles loading of the source file and outputting the audio stream
/// for consumption by the player.
///
/// The addon.xml defines the capabilities of this add-on.
///
/// @note The option to have multiple instances is possible with audio-decoder
/// add-ons. This is useful, since some playback engines are riddled by global
/// variables, making decoding of multiple streams using the same instance
/// impossible.
///
///
/// ----------------------------------------------------------------------------
///
/// **Here's an example on addon.xml:**
/// ~~~~~~~~~~~~~{.xml}
/// <?xml version="1.0" encoding="UTF-8"?>
/// <addon
///   id="audiodecoder.myspecialnamefor"
///   version="1.0.0"
///   name="My special audio decoder addon"
///   provider-name="Your Name">
///   <requires>@ADDON_DEPENDS@</requires>
///   <extension
///     point="kodi.audiodecoder"
///     name="2sf"
///     extension=".2sf|.mini2sf"
///     tags="true"
///     library_@PLATFORM@="@LIBRARY_FILENAME@"/>
///   <extension point="xbmc.addon.metadata">
///     <summary lang="en_GB">My audio decoder addon addon</summary>
///     <description lang="en_GB">My audio decoder addon description</description>
///     <platform>@PLATFORM@</platform>
///   </extension>
/// </addon>
/// ~~~~~~~~~~~~~
///
/// Description to audio decoder related addon.xml values:
/// | Name                          | Description
/// |:------------------------------|----------------------------------------
/// | <b>`point`</b>                | Addon type specification<br>At all addon types and for this kind always <b>"kodi.audiodecoder"</b>.
/// | <b>`library_@PLATFORM@`</b>   | Sets the used library name, which is automatically set by cmake at addon build.
/// | <b>`extension`</b>            | The file extensions / styles supported by this addon.
/// | <b>`mimetype`</b>             | A stream URL mimetype where can be used to force to this addon.
/// | <b>`name`</b>                 | The name of the decoder used in Kodi for display.
/// | <b>`tags`</b>                 | Boolean to point out that addon can bring own information to replayed file, if <b>`false`</b> only the file name is used as info.<br>If <b>`true`</b>, @ref CInstanceAudioDecoder::ReadTag is used and must be implemented.
/// | <b>`tracks`</b>               | Boolean to in inform one file can contains several different streams.
///
/// --------------------------------------------------------------------------
///
/// **Here is a code example how this addon is used:**
///
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/AudioDecoder.h>
///
/// class CMyAudioDecoder : public kodi::addon::CInstanceAudioDecoder
/// {
/// public:
///   CMyAudioDecoder(KODI_HANDLE instance, const std::string& version);
///
///   bool Init(const std::string& filename, unsigned int filecache,
///             int& channels, int& samplerate,
///             int& bitspersample, int64_t& totaltime,
///             int& bitrate, AudioEngineDataFormat& format,
///             std::vector<AudioEngineChannel>& channellist) override;
///   int ReadPCM(uint8_t* buffer, int size, int& actualsize) override;
/// };
///
/// CMyAudioDecoder::CMyAudioDecoder(KODI_HANDLE instance, const std::string& version)
///   : kodi::addon::CInstanceAudioDecoder(instance, version)
/// {
///   ...
/// }
///
/// bool CMyAudioDecoder::Init(const std::string& filename, unsigned int filecache,
///                            int& channels, int& samplerate,
///                            int& bitspersample, int64_t& totaltime,
///                            int& bitrate, AudioEngineDataFormat& format,
///                            std::vector<AudioEngineChannel>& channellist)
/// {
///   ...
///   return true;
/// }
///
/// int CMyAudioDecoder::ReadPCM(uint8_t* buffer, int size, int& actualsize)
/// {
///   ...
///   return 0;
/// }
///
///
/// //----------------------------------------------------------------------
///
/// class CMyAddon : public kodi::addon::CAddonBase
/// {
/// public:
///   CMyAddon() { }
///   ADDON_STATUS CreateInstance(int instanceType,
///                               const std::string& instanceID,
///                               KODI_HANDLE instance,
///                               const std::string& version,
///                               KODI_HANDLE& addonInstance) override;
/// };
///
/// // If you use only one instance in your add-on, can be instanceType and
/// // instanceID ignored
/// ADDON_STATUS CMyAddon::CreateInstance(int instanceType,
///                                       const std::string& instanceID,
///                                       KODI_HANDLE instance,
///                                       const std::string& version,
///                                       KODI_HANDLE& addonInstance)
/// {
///   if (instanceType == ADDON_INSTANCE_AUDIODECODER)
///   {
///     kodi::Log(ADDON_LOG_INFO, "Creating my audio decoder");
///     addonInstance = new CMyAudioDecoder(instance, version);
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
/// The destruction of the example class `CMyAudioDecoder` is called from
/// Kodi's header. Manually deleting the add-on instance is not required.
///
class ATTRIBUTE_HIDDEN CInstanceAudioDecoder : public IAddonInstance
{
public:
  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Audio decoder class constructor used to support multiple instance
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
  /// class CMyAudioDecoder : public kodi::addon::CInstanceAudioDecoder
  /// {
  /// public:
  ///   CMyAudioDecoder(KODI_HANDLE instance, const std::string& kodiVersion)
  ///     : kodi::addon::CInstanceAudioDecoder(instance, kodiVersion)
  ///   {
  ///      ...
  ///   }
  ///
  ///   ...
  /// };
  ///
  /// ADDON_STATUS CMyAddon::CreateInstance(int instanceType,
  ///                                       const std::string& instanceID,
  ///                                       KODI_HANDLE instance,
  ///                                       const std::string& version,
  ///                                       KODI_HANDLE& addonInstance)
  /// {
  ///   kodi::Log(ADDON_LOG_INFO, "Creating my audio decoder");
  ///   addonInstance = new CMyAudioDecoder(instance, version);
  ///   return ADDON_STATUS_OK;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  explicit CInstanceAudioDecoder(KODI_HANDLE instance, const std::string& kodiVersion = "")
    : IAddonInstance(ADDON_INSTANCE_AUDIODECODER,
                     !kodiVersion.empty() ? kodiVersion
                                          : GetKodiTypeVersion(ADDON_INSTANCE_AUDIODECODER))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceAudioDecoder: Creation of multiple together with single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Initialize a decoder.
  ///
  /// @param[in] filename The file to read
  /// @param[in] filecache The file cache size
  /// @param[out] channels Number of channels in output stream
  /// @param[out] samplerate Samplerate of output stream
  /// @param[out] bitspersample Bits per sample in output stream
  /// @param[out] totaltime Total time for stream
  /// @param[out] bitrate Average bitrate of input stream
  /// @param[out] format Data format for output stream, see
  ///                    @ref cpp_kodi_audioengine_Defs_AudioEngineFormat for
  ///                    available values
  /// @param[out] channellist Channel mapping for output streamm, see
  ///                         @ref cpp_kodi_audioengine_Defs_AudioEngineChannel
  ///                         for available values
  /// @return true if successfully done, otherwise false
  ///
  virtual bool Init(const std::string& filename,
                    unsigned int filecache,
                    int& channels,
                    int& samplerate,
                    int& bitspersample,
                    int64_t& totaltime,
                    int& bitrate,
                    AudioEngineDataFormat& format,
                    std::vector<AudioEngineChannel>& channellist) = 0;
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Produce some noise.
  ///
  /// @param[in] buffer Output buffer
  /// @param[in] size Size of output buffer
  /// @param[out] actualsize Actual number of bytes written to output buffer
  /// @return Return with following possible values:
  /// | Value | Description
  /// |:-----:|:------------
  /// |   0   | on success
  /// |  -1   | on end of stream
  /// |   1   | on failure
  ///
  virtual int ReadPCM(uint8_t* buffer, int size, int& actualsize) = 0;
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Seek in output stream.
  ///
  /// @param[in] time Time position to seek to in milliseconds
  /// @return Time position seek ended up on
  ///
  virtual int64_t Seek(int64_t time) { return time; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Read tag of a file.
  ///
  /// @param[in] file File to read tag for
  /// @param[out] tag Information tag about
  /// @return True on success, false on failure
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_audiodecoder_Defs_AudioDecoderInfoTag_Help
  ///
  virtual bool ReadTag(const std::string& file, kodi::addon::AudioDecoderInfoTag& tag)
  {
    return false;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Get number of tracks in a file.
  ///
  /// @param[in] file File to read tag for
  /// @return Number of tracks in file
  ///
  virtual int TrackCount(const std::string& file) { return 1; }
  //--------------------------------------------------------------------------

private:
  void SetAddonStruct(KODI_HANDLE instance)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstanceAudioDecoder: Creation with empty addon structure not allowed, table must be given from Kodi!");

    m_instanceData = static_cast<AddonInstance_AudioDecoder*>(instance);

    m_instanceData->toAddon->addonInstance = this;
    m_instanceData->toAddon->init = ADDON_Init;
    m_instanceData->toAddon->read_pcm = ADDON_ReadPCM;
    m_instanceData->toAddon->seek = ADDON_Seek;
    m_instanceData->toAddon->read_tag = ADDON_ReadTag;
    m_instanceData->toAddon->track_count = ADDON_TrackCount;
  }

  inline static bool ADDON_Init(const AddonInstance_AudioDecoder* instance,
                                const char* file,
                                unsigned int filecache,
                                int* channels,
                                int* samplerate,
                                int* bitspersample,
                                int64_t* totaltime,
                                int* bitrate,
                                AudioEngineDataFormat* format,
                                const AudioEngineChannel** info)
  {
    CInstanceAudioDecoder* thisClass =
        static_cast<CInstanceAudioDecoder*>(instance->toAddon->addonInstance);

    thisClass->m_channelList.clear();
    bool ret = thisClass->Init(file, filecache, *channels, *samplerate, *bitspersample, *totaltime,
                               *bitrate, *format, thisClass->m_channelList);
    if (!thisClass->m_channelList.empty())
    {
      if (thisClass->m_channelList.back() != AUDIOENGINE_CH_NULL)
        thisClass->m_channelList.push_back(AUDIOENGINE_CH_NULL);
      *info = thisClass->m_channelList.data();
    }
    else
      *info = nullptr;
    return ret;
  }

  inline static int ADDON_ReadPCM(const AddonInstance_AudioDecoder* instance, uint8_t* buffer, int size, int* actualsize)
  {
    return static_cast<CInstanceAudioDecoder*>(instance->toAddon->addonInstance)
        ->ReadPCM(buffer, size, *actualsize);
  }

  inline static int64_t ADDON_Seek(const AddonInstance_AudioDecoder* instance, int64_t time)
  {
    return static_cast<CInstanceAudioDecoder*>(instance->toAddon->addonInstance)->Seek(time);
  }

  inline static bool ADDON_ReadTag(const AddonInstance_AudioDecoder* instance,
                                   const char* file,
                                   struct AUDIO_DECODER_INFO_TAG* tag)
  {
    kodi::addon::AudioDecoderInfoTag cppTag(tag);
    return static_cast<CInstanceAudioDecoder*>(instance->toAddon->addonInstance)
        ->ReadTag(file, cppTag);
  }

  inline static int ADDON_TrackCount(const AddonInstance_AudioDecoder* instance, const char* file)
  {
    return static_cast<CInstanceAudioDecoder*>(instance->toAddon->addonInstance)->TrackCount(file);
  }

  std::vector<AudioEngineChannel> m_channelList;
  AddonInstance_AudioDecoder* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */
#endif /* __cplusplus */
