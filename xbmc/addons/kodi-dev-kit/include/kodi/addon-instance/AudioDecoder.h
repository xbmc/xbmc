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
#include "../c-api/addon-instance/audiodecoder.h"

#ifdef __cplusplus

#include <cstring>
#include <stdexcept>

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
class ATTR_DLL_LOCAL AudioDecoderInfoTag
{
public:
  /*! \cond PRIVATE */
  AudioDecoderInfoTag() = default;
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
  /// | **Cover art by path** | `std::string` | @ref AudioDecoderInfoTag::SetCoverArtByPath "SetCoverArtByPath" | @ref AudioDecoderInfoTag::GetCoverArtByPath "GetCoverArtByPath"
  /// | **Cover art by memory** | `std::string` | @ref AudioDecoderInfoTag::SetCoverArtByMem "SetCoverArtByMem" | @ref AudioDecoderInfoTag::GetCoverArtByMem "GetCoverArtByMem"
  ///

  /// @addtogroup cpp_kodi_addon_audiodecoder_Defs_AudioDecoderInfoTag
  ///@{

  /// @brief Set the title from music as string on info tag.
  void SetTitle(const std::string& title) { m_title = title; }

  /// @brief Get title name
  std::string GetTitle() const { return m_title; }

  /// @brief Set artist name
  void SetArtist(const std::string& artist) { m_artist = artist; }

  /// @brief Get artist name
  std::string GetArtist() const { return m_artist; }

  /// @brief Set album name
  void SetAlbum(const std::string& album) { m_album = album; }

  /// @brief Set album name
  std::string GetAlbum() const { return m_album; }

  /// @brief Set album artist name
  void SetAlbumArtist(const std::string& albumArtist) { m_album_artist = albumArtist; }

  /// @brief Get album artist name
  std::string GetAlbumArtist() const { return m_album_artist; }

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
  std::string GetMediaType() const { return m_media_type; }

  /// @brief Set genre name from music as string if present.
  void SetGenre(const std::string& genre) { m_genre = genre; }

  /// @brief Get genre name from music as string if present.
  std::string GetGenre() const { return m_genre; }

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
  std::string GetDiscSubtitle() const { return m_disc_subtitle; }

  /// @brief Set disks amount quantity (if present) from music info as integer.
  void SetDiscTotal(int discTotal) { m_disc_total = discTotal; }

  /// @brief Get disks amount quantity (if present)
  int GetDiscTotal() const { return m_disc_total; }

  /// @brief Set release date as string from music info (if present).\n
  /// [ISO8601](https://en.wikipedia.org/wiki/ISO_8601) date YYYY, YYYY-MM or YYYY-MM-DD
  void SetReleaseDate(const std::string& releaseDate) { m_release_date = releaseDate; }

  /// @brief Get release date as string from music info (if present).
  std::string GetReleaseDate() const { return m_release_date; }

  /// @brief Set string from lyrics.
  void SetLyrics(const std::string& lyrics) { m_lyrics = lyrics; }

  /// @brief Get string from lyrics.
  std::string GetLyrics() const { return m_lyrics; }

  /// @brief Set related stream samplerate.
  void SetSamplerate(int samplerate) { m_samplerate = samplerate; }

  /// @brief Get related stream samplerate.
  int GetSamplerate() const { return m_samplerate; }

  /// @brief Set related stream channels amount.
  void SetChannels(int channels) { m_channels = channels; }

  /// @brief Get related stream channels amount.
  int GetChannels() const { return m_channels; }

  /// @brief Set related stream bitrate.
  void SetBitrate(int bitrate) { m_bitrate = bitrate; }

  /// @brief Get related stream bitrate.
  int GetBitrate() const { return m_bitrate; }

  /// @brief Set additional information comment (if present).
  void SetComment(const std::string& comment) { m_comment = comment; }

  /// @brief Get additional information comment (if present).
  std::string GetComment() const { return m_comment; }

  /// @brief Set cover art image by path.
  ///
  /// @param[in] path Image position path
  ///
  /// @note Cannot be combined with @ref SetCoverArtByMem and @ref GetCoverArtByMem.
  void SetCoverArtByPath(const std::string& path) { m_cover_art_path = path; }

  /// @brief Get cover art image path.
  ///
  /// @return Image position path
  ///
  /// @note Only be available if set before by @ref SetCoverArtByPath.
  /// Cannot be combined with @ref SetCoverArtByMem and @ref GetCoverArtByMem.
  ///
  std::string GetCoverArtByPath() const { return m_cover_art_path; }

  /// @brief Set cover art image by memory.
  ///
  /// @param[in] data Image data
  /// @param[in] size Image data size
  /// @param[in] mimetype Image format mimetype
  ///    Possible mimetypes:
  ///     - "image/jpeg"
  ///     - "image/png"
  ///     - "image/gif"
  ///     - "image/bmp"
  ///
  void SetCoverArtByMem(const uint8_t* data, size_t size, const std::string& mimetype)
  {
    if (size > 0)
    {
      m_cover_art_mem_mimetype = mimetype;
      m_cover_art_mem.resize(size);
      m_cover_art_mem.assign(data, data + size);
    }
  }

  /// @brief Get cover art data by memory.
  ///
  /// @param[out] size Stored size about art image
  /// @param[in] mimetype Related image mimetype to stored data
  /// @return Image data
  ///
  /// @note This only works if @ref SetCoverArtByMem was used before
  ///
  /// @warning This function is not thread safe and related data should never be
  /// changed by @ref SetCoverArtByMem, if data from here is used without copy!
  const uint8_t* GetCoverArtByMem(size_t& size, std::string& mimetype) const
  {
    if (!m_cover_art_mem.empty())
    {
      mimetype = m_cover_art_mem_mimetype;
      size = m_cover_art_mem.size();
      return m_cover_art_mem.data();
    }
    else
    {
      size = 0;
      return nullptr;
    }
  }

  ///@}

private:
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
  int m_bitrate{0};
  std::string m_comment;
  std::string m_cover_art_path;
  std::string m_cover_art_mem_mimetype;
  std::vector<uint8_t> m_cover_art_mem;
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
///     tags="true"
///     library_@PLATFORM@="@LIBRARY_FILENAME@">
///     <support>
///       <extension name=".2sf">
///         <description>30100</description>
///         <icon>resources/file_format_music_sound.png</icon>
///       </extension>
///       <extension name=".mini2sf"/>
///     </support>
///   </extension>
///   <extension point="xbmc.addon.metadata">
///     <summary lang="en_GB">My audio decoder addon addon</summary>
///     <description lang="en_GB">My audio decoder addon description</description>
///     <platform>@PLATFORM@</platform>
///   </extension>
/// </addon>
/// ~~~~~~~~~~~~~
///
/// Description to audio decoder related addon.xml values:
/// | Name                                                  | Description
/// |:------------------------------------------------------|----------------------------------------
/// | <b>`point`</b>                                        | Addon type specification<br>At all addon types and for this kind always <b>"kodi.audiodecoder"</b>.
/// | <b>`library_@PLATFORM@`</b>                           | Sets the used library name, which is automatically set by cmake at addon build.
/// | <b>`name`</b>                                         | The name of the decoder used in Kodi for display.
/// | <b>`tags`</b>                                         | Boolean to point out that addon can bring own information to replayed file, if <b>`false`</b> only the file name is used as info.<br>If <b>`true`</b>, @ref CInstanceAudioDecoder::ReadTag is used and must be implemented.
/// | <b>`tracks`</b>                                       | Boolean to in inform one file can contains several different streams.
/// | <b>`<support><extension name="..." /></support>`</b>  | The file extensions / styles supported by this addon.\nOptional can be with `<description>` and `<icon>`additional info added where used for list views in Kodi.
/// | <b>`<support><mimetype name="..." /></support>`</b>   | A stream URL mimetype where can be used to force to this addon.\nOptional can be with ``<description>` and `<icon>`additional info added where used for list views in Kodi.
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
///   CMyAudioDecoder(const kodi::addon::IInstanceInfo& instance);
///
///   bool Init(const std::string& filename, unsigned int filecache,
///             int& channels, int& samplerate,
///             int& bitspersample, int64_t& totaltime,
///             int& bitrate, AudioEngineDataFormat& format,
///             std::vector<AudioEngineChannel>& channellist) override;
///   int ReadPCM(uint8_t* buffer, int size, int& actualsize) override;
/// };
///
/// CMyAudioDecoder::CMyAudioDecoder(const kodi::addon::IInstanceInfo& instance)
///   : kodi::addon::CInstanceAudioDecoder(instance)
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
///   return AUDIODECODER_READ_SUCCESS;
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
///   if (instance.IsType(ADDON_INSTANCE_AUDIODECODER))
///   {
///     kodi::Log(ADDON_LOG_INFO, "Creating my audio decoder");
///     hdl = new CMyAudioDecoder(instance);
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
class ATTR_DLL_LOCAL CInstanceAudioDecoder : public IAddonInstance
{
public:
  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Audio decoder class constructor used to support multiple instance
  /// types.
  ///
  /// @param[in] instance The instance value given to
  ///                     <b>`kodi::addon::CAddonBase::CreateInstance(...)`</b>.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Here's example about the use of this:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// class CMyAudioDecoder : public kodi::addon::CInstanceAudioDecoder
  /// {
  /// public:
  ///   CMyAudioDecoder(KODI_HANDLE instance)
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
  ///   kodi::Log(ADDON_LOG_INFO, "Creating my audio decoder");
  ///   hdl = new CMyAudioDecoder(instance);
  ///   return ADDON_STATUS_OK;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  explicit CInstanceAudioDecoder(const kodi::addon::IInstanceInfo& instance)
    : IAddonInstance(instance)
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceAudioDecoder: Creation of multiple together "
                             "with single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Checks addon support given file path.
  ///
  /// @param[in] filename The file to read
  /// @return true if successfully done and supported, otherwise false
  ///
  /// @note Optional to add, as default becomes `true` used.
  ///
  virtual bool SupportsFile(const std::string& filename) { return true; }
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
  /// @param[out] channellist Channel mapping for output stream, see
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
  /// @return @copydetails cpp_kodi_addon_audiodecoder_Defs_AUDIODECODER_READ_RETURN
  ///
  virtual int ReadPCM(uint8_t* buffer, size_t size, size_t& actualsize) = 0;
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

  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Static auxiliary function to read the track number used from the
  /// given path.
  ///
  /// If track number is not found in file name, the originally given file
  /// name is returned, track number then remains at "0".
  ///
  /// @param[in] name The value specified in addon.xml extension under `name="???"`
  /// @param[in] trackPath The full path to evaluate
  /// @param[out] track The track number read out in the path, 0 if not
  ///                   identified as a track path.
  /// @return Path to the associated file
  ///
  inline static std::string GetTrack(const std::string& name,
                                     const std::string& trackPath,
                                     int& track)
  {
    /*
     * get the track name from path
     */
    track = 0;
    std::string toLoad(trackPath);
    const std::string ext = "." + name + KODI_ADDON_AUDIODECODER_TRACK_EXT;
    if (toLoad.find(ext) != std::string::npos)
    {
      size_t iStart = toLoad.rfind('-') + 1;
      track = atoi(toLoad.substr(iStart, toLoad.size() - iStart - ext.size()).c_str());
      //  The directory we are in, is the file
      //  that contains the bitstream to play,
      //  so extract it
      size_t slash = trackPath.rfind('\\');
      if (slash == std::string::npos)
        slash = trackPath.rfind('/');
      toLoad = trackPath.substr(0, slash);
    }

    return toLoad;
  }
  //--------------------------------------------------------------------------

private:
  void SetAddonStruct(KODI_ADDON_INSTANCE_STRUCT* instance)
  {
    instance->hdl = this;
    instance->audiodecoder->toAddon->supports_file = ADDON_supports_file;
    instance->audiodecoder->toAddon->init = ADDON_init;
    instance->audiodecoder->toAddon->read_pcm = ADDON_read_pcm;
    instance->audiodecoder->toAddon->seek = ADDON_seek;
    instance->audiodecoder->toAddon->read_tag = ADDON_read_tag;
    instance->audiodecoder->toAddon->track_count = ADDON_track_count;
  }

  inline static bool ADDON_supports_file(const KODI_ADDON_AUDIODECODER_HDL hdl, const char* file)
  {
    return static_cast<CInstanceAudioDecoder*>(hdl)->SupportsFile(file);
  }

  inline static bool ADDON_init(const KODI_ADDON_AUDIODECODER_HDL hdl,
                                const char* file,
                                unsigned int filecache,
                                int* channels,
                                int* samplerate,
                                int* bitspersample,
                                int64_t* totaltime,
                                int* bitrate,
                                AudioEngineDataFormat* format,
                                enum AudioEngineChannel info[AUDIOENGINE_CH_MAX])
  {
    CInstanceAudioDecoder* thisClass = static_cast<CInstanceAudioDecoder*>(hdl);

    std::vector<AudioEngineChannel> channelList;

    bool ret = thisClass->Init(file, filecache, *channels, *samplerate, *bitspersample, *totaltime,
                               *bitrate, *format, channelList);
    if (!channelList.empty())
    {
      if (channelList.back() != AUDIOENGINE_CH_NULL)
        channelList.push_back(AUDIOENGINE_CH_NULL);

      for (unsigned int i = 0; i < channelList.size(); ++i)
      {
        info[i] = channelList[i];
      }
    }
    return ret;
  }

  inline static int ADDON_read_pcm(const KODI_ADDON_AUDIODECODER_HDL hdl,
                                   uint8_t* buffer,
                                   size_t size,
                                   size_t* actualsize)
  {
    return static_cast<CInstanceAudioDecoder*>(hdl)->ReadPCM(buffer, size, *actualsize);
  }

  inline static int64_t ADDON_seek(const KODI_ADDON_AUDIODECODER_HDL hdl, int64_t time)
  {
    return static_cast<CInstanceAudioDecoder*>(hdl)->Seek(time);
  }

  inline static bool ADDON_read_tag(const KODI_ADDON_AUDIODECODER_HDL hdl,
                                    const char* file,
                                    struct KODI_ADDON_AUDIODECODER_INFO_TAG* tag)
  {
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996)
#endif // _WIN32
    kodi::addon::AudioDecoderInfoTag cppTag;
    bool ret = static_cast<CInstanceAudioDecoder*>(hdl)->ReadTag(file, cppTag);
    if (ret)
    {
      tag->title = strdup(cppTag.GetTitle().c_str());
      tag->artist = strdup(cppTag.GetArtist().c_str());
      tag->album = strdup(cppTag.GetAlbum().c_str());
      tag->album_artist = strdup(cppTag.GetAlbumArtist().c_str());
      tag->media_type = strdup(cppTag.GetMediaType().c_str());
      tag->genre = strdup(cppTag.GetGenre().c_str());
      tag->duration = cppTag.GetDuration();
      tag->track = cppTag.GetTrack();
      tag->disc = cppTag.GetDisc();
      tag->disc_subtitle = strdup(cppTag.GetDiscSubtitle().c_str());
      tag->disc_total = cppTag.GetDiscTotal();
      tag->release_date = strdup(cppTag.GetReleaseDate().c_str());
      tag->lyrics = strdup(cppTag.GetLyrics().c_str());
      tag->samplerate = cppTag.GetSamplerate();
      tag->channels = cppTag.GetChannels();
      tag->bitrate = cppTag.GetBitrate();
      tag->comment = strdup(cppTag.GetComment().c_str());
      std::string mimetype;
      size_t size = 0;
      const uint8_t* mem = cppTag.GetCoverArtByMem(size, mimetype);
      if (mem && size > 0)
      {
        tag->cover_art_mem_mimetype = strdup(mimetype.c_str());
        tag->cover_art_mem_size = size;
        tag->cover_art_mem = static_cast<uint8_t*>(malloc(size));
        memcpy(tag->cover_art_mem, mem, size);
      }
      else
      {
        tag->cover_art_path = strdup(cppTag.GetCoverArtByPath().c_str());
      }
    }
    return ret;
#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32
  }

  inline static int ADDON_track_count(const KODI_ADDON_AUDIODECODER_HDL hdl, const char* file)
  {
    return static_cast<CInstanceAudioDecoder*>(hdl)->TrackCount(file);
  }
};

} /* namespace addon */
} /* namespace kodi */
#endif /* __cplusplus */
