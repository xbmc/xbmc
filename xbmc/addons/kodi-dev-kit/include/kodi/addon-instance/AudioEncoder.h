/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/audio_encoder.h"

#ifdef __cplusplus
namespace kodi
{
namespace addon
{

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
/// class ATTRIBUTE_HIDDEN CMyAudioEncoder : public kodi::addon::CInstanceAudioEncoder
/// {
/// public:
///   CMyAudioEncoder(KODI_HANDLE instance, const std::string& kodiVersion)
///     : kodi::addon::CInstanceAudioEncoder(instance, kodiVersion)
///
///   bool Init(const std::string& filename, unsigned int filecache,
///             int& channels, int& samplerate,
///             int& bitspersample, int64_t& totaltime,
///             int& bitrate, AEDataFormat& format,
///             std::vector<AEChannel>& channellist) override;
///   int Encode(int numBytesRead, const uint8_t* pbtStream) override;
///   bool Finish() override; // Optional
/// };
///
/// CMyAudioEncoder::CMyAudioEncoder(KODI_HANDLE instance)
///   : kodi::addon::CInstanceAudioEncoder(instance)
/// {
///   ...
/// }
///
/// bool CMyAudioEncoder::Start(int inChannels,
///                             int inRate,
///                             int inBits,
///                             const std::string& title,
///                             const std::string& artist,
///                             const std::string& albumartist,
///                             const std::string& album,
///                             const std::string& year,
///                             const std::string& track,
///                             const std::string& genre,
///                             const std::string& comment,
///                             int trackLength)
/// {
///   ...
///   return true;
/// }
///
/// int CMyAudioEncoder::Encode(int numBytesRead, const uint8_t* pbtStream)
/// {
///   uint8_t* data = nullptr;
///   int length = 0;
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
///   if (instanceType == ADDON_INSTANCE_AUDIOENCODER)
///   {
///     kodi::Log(ADDON_LOG_INFO, "Creating my audio encoder instance");
///     addonInstance = new CMyAudioEncoder(instance, version);
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
class ATTRIBUTE_HIDDEN CInstanceAudioEncoder : public IAddonInstance
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_addon_audioencoder
  /// @brief Audio encoder class constructor used to support multiple instances.
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
  /// class CMyAudioEncoder : public kodi::addon::CInstanceAudioEncoder
  /// {
  /// public:
  ///   CMyAudioEncoder(KODI_HANDLE instance, const std::string& kodiVersion)
  ///     : kodi::addon::CInstanceAudioEncoder(instance, kodiVersion)
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
  ///   kodi::Log(ADDON_LOG_INFO, "Creating my audio encoder instance");
  ///   addonInstance = new CMyAudioEncoder(instance, version);
  ///   return ADDON_STATUS_OK;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  explicit CInstanceAudioEncoder(KODI_HANDLE instance, const std::string& kodiVersion = "")
    : IAddonInstance(ADDON_INSTANCE_AUDIOENCODER,
                     !kodiVersion.empty() ? kodiVersion
                                          : GetKodiTypeVersion(ADDON_INSTANCE_AUDIOENCODER))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceAudioEncoder: Creation of multiple together "
                             "with single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audioencoder
  /// @brief Start encoder (**required**)
  ///
  /// @param[in] inChannels Number of channels
  /// @param[in] inRate Sample rate of input data
  /// @param[in] inBits Bits per sample in input data
  /// @param[in] title The title of the song
  /// @param[in] artist The artist of the song
  /// @param[in] albumartist The albumartist of the song
  /// @param[in] year The year of the song
  /// @param[in] track The track number of the song
  /// @param[in] genre The genre of the song
  /// @param[in] comment A comment to attach to the song
  /// @param[in] trackLength Total track length in seconds
  /// @return True on success, false on failure.
  ///
  virtual bool Start(int inChannels,
                     int inRate,
                     int inBits,
                     const std::string& title,
                     const std::string& artist,
                     const std::string& albumartist,
                     const std::string& album,
                     const std::string& year,
                     const std::string& track,
                     const std::string& genre,
                     const std::string& comment,
                     int trackLength) = 0;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audioencoder
  /// @brief Encode a chunk of audio (**required**)
  ///
  /// @param[in] numBytesRead Number of bytes in input buffer
  /// @param[in] pbtStream The input buffer
  /// @return Number of bytes consumed
  ///
  virtual int Encode(int numBytesRead, const uint8_t* pbtStream) = 0;
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
  int Write(const uint8_t* data, int length)
  {
    return m_instanceData->toKodi->write(m_instanceData->toKodi->kodiInstance, data, length);
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
  int64_t Seek(int64_t position, int whence = SEEK_SET)
  {
    return m_instanceData->toKodi->seek(m_instanceData->toKodi->kodiInstance, position, whence);
  }
  //----------------------------------------------------------------------------

private:
  void SetAddonStruct(KODI_HANDLE instance)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstanceAudioEncoder: Creation with empty addon "
                             "structure not allowed, table must be given from Kodi!");

    m_instanceData = static_cast<AddonInstance_AudioEncoder*>(instance);
    m_instanceData->toAddon->addonInstance = this;
    m_instanceData->toAddon->start = ADDON_Start;
    m_instanceData->toAddon->encode = ADDON_Encode;
    m_instanceData->toAddon->finish = ADDON_Finish;
  }

  inline static bool ADDON_Start(const AddonInstance_AudioEncoder* instance,
                                 int inChannels,
                                 int inRate,
                                 int inBits,
                                 const char* title,
                                 const char* artist,
                                 const char* albumartist,
                                 const char* album,
                                 const char* year,
                                 const char* track,
                                 const char* genre,
                                 const char* comment,
                                 int trackLength)
  {
    return static_cast<CInstanceAudioEncoder*>(instance->toAddon->addonInstance)
        ->Start(inChannels, inRate, inBits, title, artist, albumartist, album, year, track, genre,
                comment, trackLength);
  }

  inline static int ADDON_Encode(const AddonInstance_AudioEncoder* instance,
                                 int numBytesRead,
                                 const uint8_t* pbtStream)
  {
    return static_cast<CInstanceAudioEncoder*>(instance->toAddon->addonInstance)
        ->Encode(numBytesRead, pbtStream);
  }

  inline static bool ADDON_Finish(const AddonInstance_AudioEncoder* instance)
  {
    return static_cast<CInstanceAudioEncoder*>(instance->toAddon->addonInstance)->Finish();
  }

  AddonInstance_AudioEncoder* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
