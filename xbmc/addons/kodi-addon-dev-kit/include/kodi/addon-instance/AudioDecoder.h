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

  void SetTitle(const std::string& title)
  {
    strncpy(m_cStructure->title, title.c_str(), sizeof(m_cStructure->title) - 1);
  }

  std::string GetTitle() const { return m_cStructure->title; }

  void SetArtist(const std::string& artist)
  {
    strncpy(m_cStructure->artist, artist.c_str(), sizeof(m_cStructure->artist) - 1);
  }

  std::string GetArtist() const { return m_cStructure->artist; }

  void SetLength(int length) { m_cStructure->length = length; }

  int GetLength() const { return m_cStructure->length; }

private:
  AudioDecoderInfoTag(const AUDIO_DECODER_INFO_TAG* tag) : CStructHdl(tag) {}
  AudioDecoderInfoTag(AUDIO_DECODER_INFO_TAG* tag) : CStructHdl(tag) {}
};

//==============================================================================
///
/// \addtogroup cpp_kodi_addon_audiodecoder
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
///   <extension
///     point="kodi.audiodecoder"
///     name="2sf"
///     extension=".2sf|.mini2sf"
///     tags="true"
///     library_@PLATFORM@="@LIBRARY_FILENAME@"/>
/// ~~~~~~~~~~~~~
///
/// Description to audio decoder related addon.xml values:
/// | Name                          | Description
/// |:------------------------------|----------------------------------------
/// | <b>`point`</b>                | Addon type specification<br>At all addon types and for this kind always <b>"kodi.audiodecoder"</b>.
/// | <b>`library_@PLATFORM@`</b>   | Sets the used library name, which is automatically set by cmake at addon build.
/// | <b>`name`</b>                 | The name of the decoder used in Kodi for display.
/// | <b>`extension`</b>            | The file extensions / styles supported by this addon.
/// | <b>`tags`</b>                 | Boolean to point out that addon can bring own information to replayed file, if <b>`false`</b> only the file name is used as info.<br>If <b>`true`</b>, \ref CInstanceAudioDecoder::ReadTag is used and must be implemented.
///
/// --------------------------------------------------------------------------
///
/// **Here is a code example how this addon is used:**
///
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/AudioDecoder.h>
///
/// class CMyAudioDecoder : public ::kodi::addon::CInstanceAudioDecoder
/// {
/// public:
///   CMyAudioDecoder(KODI_HANDLE instance);
///
///   bool Init(const std::string& filename, unsigned int filecache,
///             int& channels, int& samplerate,
///             int& bitspersample, int64_t& totaltime,
///             int& bitrate, AEDataFormat& format,
///             std::vector<AEChannel>& channellist) override;
///   int ReadPCM(uint8_t* buffer, int size, int& actualsize) override;
/// };
///
/// CMyAudioDecoder::CMyAudioDecoder(KODI_HANDLE instance)
///   : CInstanceAudioDecoder(instance)
/// {
///   ...
/// }
///
/// bool CMyAudioDecoder::Init(const std::string& filename, unsigned int filecache,
///                            int& channels, int& samplerate,
///                            int& bitspersample, int64_t& totaltime,
///                            int& bitrate, AEDataFormat& format,
///                            std::vector<AEChannel>& channellist)
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
///   if (instanceType == ADDON_INSTANCE_AUDIODECODER)
///   {
///     kodi::Log(ADDON_LOG_NOTICE, "Creating my audio decoder");
///     addonInstance = new CMyAudioDecoder(instance);
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
  /// @brief Class constructor
  ///
  /// @param[in] instance The addon instance class handler given by Kodi
  ///                     at \ref kodi::addon::CAddonBase::CreateInstance(...)
  /// @param[in] kodiVersion [opt] Version used in Kodi for this instance, to
  ///                        allow compatibility to older Kodi versions.
  ///                        @note Recommended to set.
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
  /// @brief Initialize a decoder
  ///
  /// @param[in] filename             The file to read
  /// @param[in] filecache            The file cache size
  /// @param[out] channels            Number of channels in output stream
  /// @param[out] samplerate          Samplerate of output stream
  /// @param[out] bitspersample       Bits per sample in output stream
  /// @param[out] totaltime           Total time for stream
  /// @param[out] bitrate             Average bitrate of input stream
  /// @param[out] format              Data format for output stream
  /// @param[out] channellist         Channel mapping for output stream
  /// @return                         true if successfully done, otherwise
  ///                                 false
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
  /// @brief Produce some noise
  ///
  /// @param[in] buffer               Output buffer
  /// @param[in] size                 Size of output buffer
  /// @param[out] actualsize          Actual number of bytes written to output buffer
  /// @return                         Return with following possible values:
  ///                                 | Value | Description                  |
  ///                                 |:-----:|:-----------------------------|
  ///                                 |   0   | on success
  ///                                 |  -1   | on end of stream
  ///                                 |   1   | on failure
  ///
  virtual int ReadPCM(uint8_t* buffer, int size, int& actualsize) = 0;
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Seek in output stream
  ///
  /// @param[in] time                 Time position to seek to in milliseconds
  /// @return                         Time position seek ended up on
  ///
  virtual int64_t Seek(int64_t time) { return time; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Read tag of a file
  ///
  /// @param[in] file                 File to read tag for
  /// @param[out] tag                 Information tag about
  /// @return                         True on success, false on failure
  ///
  virtual bool ReadTag(const std::string& file, kodi::addon::AudioDecoderInfoTag& tag)
  {
    return false;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_addon_audiodecoder
  /// @brief Get number of tracks in a file
  ///
  /// @param[in] file                 File to read tag for
  /// @return                         Number of tracks in file
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
