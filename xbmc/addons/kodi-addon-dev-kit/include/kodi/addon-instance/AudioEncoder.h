/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"

namespace kodi { namespace addon { class CInstanceAudioEncoder; }}

extern "C"
{

  typedef struct AddonProps_AudioEncoder
  {
    int dummy;
  } AddonProps_AudioEncoder;

  typedef struct AddonToKodiFuncTable_AudioEncoder
  {
    void* kodiInstance;
    int (*write) (void* kodiInstance, const uint8_t* data, int len);
    int64_t (*seek)(void* kodiInstance, int64_t pos, int whence);
  } AddonToKodiFuncTable_AudioEncoder;

  struct AddonInstance_AudioEncoder;
  typedef struct KodiToAddonFuncTable_AudioEncoder
  {
    kodi::addon::CInstanceAudioEncoder* addonInstance;
    bool (__cdecl* start) (const AddonInstance_AudioEncoder* instance, int in_channels, int in_rate, int in_bits,
                           const char* title, const char* artist,
                           const char* albumartist, const char* album,
                           const char* year, const char* track,
                           const char* genre, const char* comment,
                           int track_length);
    int  (__cdecl* encode) (const AddonInstance_AudioEncoder* instance, int num_bytes_read, const uint8_t* pbt_stream);
    bool (__cdecl* finish) (const AddonInstance_AudioEncoder* instance);
  } KodiToAddonFuncTable_AudioEncoder;

  typedef struct AddonInstance_AudioEncoder
  {
    AddonProps_AudioEncoder props;
    AddonToKodiFuncTable_AudioEncoder toKodi;
    KodiToAddonFuncTable_AudioEncoder toAddon;
  } AddonInstance_AudioEncoder;

} /* extern "C" */

namespace kodi
{
namespace addon
{

  class CInstanceAudioEncoder : public IAddonInstance
  {
  public:
    //==========================================================================
    /// @brief Class constructor
    ///
    /// @param[in] instance             The from Kodi given instance given be
    ///                                 add-on CreateInstance call with instance
    ///                                 id ADDON_INSTANCE_AUDIOENCODER.
    explicit CInstanceAudioEncoder(KODI_HANDLE instance)
      : IAddonInstance(ADDON_INSTANCE_AUDIOENCODER)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceAudioEncoder: Creation of multiple together with single instance way is not allowed!");

      SetAddonStruct(instance);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    /// \brief Start encoder (**required**)
    ///
    /// \param[in] inChannels           Number of channels
    /// \param[in] inRate               Sample rate of input data
    /// \param[in] inBits               Bits per sample in input data
    /// \param[in] title                The title of the song
    /// \param[in] artist               The artist of the song
    /// \param[in] albumartist          The albumartist of the song
    /// \param[in] year                 The year of the song
    /// \param[in] track                The track number of the song
    /// \param[in] genre                The genre of the song
    /// \param[in] comment              A comment to attach to the song
    /// \param[in] trackLength          Total track length in seconds
    /// \return                         True on success, false on failure.
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
    //--------------------------------------------------------------------------

    //==========================================================================
    /// \brief Encode a chunk of audio (**required**)
    ///
    /// \param[in] numBytesRead         Number of bytes in input buffer
    /// \param[in] pbtStream            the input buffer
    /// \return                         Number of bytes consumed
    ///
    virtual int Encode(int numBytesRead, const uint8_t* pbtStream) = 0;
    //--------------------------------------------------------------------------

    //==========================================================================
    /// \brief Finalize encoding (**optional**)
    ///
    /// \return                         True on success, false on failure.
    ///
    virtual bool Finish() { return true; }
    //--------------------------------------------------------------------------

    //==========================================================================
    /// \brief Write block of data
    ///
    /// \param[in] data                 Pointer to the array of elements to be
    ///                                 written
    /// \param[in] length               Size in bytes to be written.
    /// \return                         The total number of bytes
    ///                                 successfully written is returned.
    int Write(const uint8_t* data, int length)
    {
      return m_instanceData->toKodi.write(m_instanceData->toKodi.kodiInstance, data, length);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    /// \brief Set the file's current position.
    ///
    /// The whence argument is optional and defaults to SEEK_SET (0)
    ///
    /// \param[in] position             the position that you want to seek to
    /// \param[in] whence               [optional] offset relative to
    ///                                 You can set the value of whence to one
    ///                                 of three things:
    ///  |   Value  | int | Description                                        |
    ///  |:--------:|:---:|:---------------------------------------------------|
    ///  | SEEK_SET |  0  | position is relative to the beginning of the file. This is probably what you had in mind anyway, and is the most commonly used value for whence.
    ///  | SEEK_CUR |  1  | position is relative to the current file pointer position. So, in effect, you can say, "Move to my current position plus 30 bytes," or, "move to my current position minus 20 bytes."
    ///  | SEEK_END |  2  | position is relative to the end of the file. Just like SEEK_SET except from the other end of the file. Be sure to use negative values for offset if you want to back up from the end of the file, instead of going past the end into oblivion.
    ///
    /// \return                         Returns the resulting offset location as
    ///                                 measured in bytes from the beginning of
    ///                                 the file. On error, the value -1 is
    ///                                 returned.
    int64_t Seek(int64_t position, int whence = SEEK_SET)
    {
      return m_instanceData->toKodi.seek(m_instanceData->toKodi.kodiInstance, position, whence);
    }
    //--------------------------------------------------------------------------

  private:
    void SetAddonStruct(KODI_HANDLE instance)
    {
      if (instance == nullptr)
        throw std::logic_error("kodi::addon::CInstanceAudioEncoder: Creation with empty addon structure not allowed, table must be given from Kodi!");

      m_instanceData = static_cast<AddonInstance_AudioEncoder*>(instance);
      m_instanceData->toAddon.addonInstance = this;
      m_instanceData->toAddon.start = ADDON_Start;
      m_instanceData->toAddon.encode = ADDON_Encode;
      m_instanceData->toAddon.finish = ADDON_Finish;
    }

    inline static bool ADDON_Start(const AddonInstance_AudioEncoder* instance, int inChannels, int inRate, int inBits,
                                   const char* title, const char* artist,
                                   const char* albumartist, const char* album,
                                   const char* year, const char* track,
                                   const char* genre, const char* comment,
                                   int trackLength)
    {
      return instance->toAddon.addonInstance->Start(inChannels,
                                                    inRate,
                                                    inBits,
                                                    title,
                                                    artist,
                                                    albumartist,
                                                    album,
                                                    year,
                                                    track,
                                                    genre,
                                                    comment,
                                                    trackLength);
    }

    inline static int ADDON_Encode(const AddonInstance_AudioEncoder* instance, int numBytesRead, const uint8_t* pbtStream)
    {
      return instance->toAddon.addonInstance->Encode(numBytesRead, pbtStream);
    }

    inline static bool ADDON_Finish(const AddonInstance_AudioEncoder* instance)
    {
      return instance->toAddon.addonInstance->Finish();
    }

    AddonInstance_AudioEncoder* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
