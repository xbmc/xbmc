#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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
    int (*write) (void* kodiInstance, uint8_t* data, int len);
    int64_t (*seek)(void* kodiInstance, int64_t pos, int whence);
  } AddonToKodiFuncTable_AudioEncoder;

  typedef struct KodiToAddonFuncTable_AudioEncoder
  {
    bool (__cdecl* Start) (kodi::addon::CInstanceAudioEncoder* addonInstance, int iInChannels, int iInRate, int iInBits,
                           const char* title, const char* artist,
                           const char* albumartist, const char* album,
                           const char* year, const char* track,
                           const char* genre, const char* comment,
                           int iTrackLength);
    int  (__cdecl* Encode) (kodi::addon::CInstanceAudioEncoder* addonInstance, int nNumBytesRead, uint8_t* pbtStream);
    bool (__cdecl* Finish) (kodi::addon::CInstanceAudioEncoder* addonInstance);
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
    CInstanceAudioEncoder(KODI_HANDLE instance)
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
    /// \param[in] iInChannels          Number of channels
    /// \param[in] iInRate              Sample rate of input data
    /// \param[in] iInBits              Bits per sample in input data
    /// \param[in] title                The title of the song
    /// \param[in] artist               The artist of the song
    /// \param[in] albumartist          The albumartist of the song
    /// \param[in] year                 The year of the song
    /// \param[in] track                The track number of the song
    /// \param[in] genre                The genre of the song
    /// \param[in] comment              A comment to attach to the song
    /// \param[in] iTrackLength         Total track length in seconds
    /// \return                         True on success, false on failure.
    ///
    virtual bool Start(int iInChannels,
                       int iInRate,
                       int iInBits,
                       std::string title,
                       std::string artist,
                       std::string albumartist,
                       std::string album,
                       std::string year,
                       std::string track,
                       std::string genre,
                       std::string comment,
                       int iTrackLength)=0;
    //--------------------------------------------------------------------------

    //==========================================================================
    /// \brief Encode a chunk of audio (**required**)
    ///
    /// \param[in] addonInstance        Encoder context from Create.
    /// \param[in] nNumBytesRead        Number of bytes in input buffer
    /// \param[in] pbtStream            the input buffer
    /// \return                         Number of bytes consumed
    ///
    virtual int Encode(int nNumBytesRead, uint8_t* pbtStream)=0;
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
    /// \param[in] len                  Size in bytes to be written.
    /// \return                         The total number of bytes 
    ///                                 successfully written is returned.
    int Write(uint8_t* data, int len)
    {
      return m_instanceData->toKodi.write(m_instanceData->toKodi.kodiInstance, data, len);
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

      m_instanceData->toAddon.Start = ADDON_Start;
      m_instanceData->toAddon.Encode = ADDON_Encode;
      m_instanceData->toAddon.Finish = ADDON_Finish;
    }

    inline static bool ADDON_Start(CInstanceAudioEncoder* addonInstance, int iInChannels, int iInRate, int iInBits,
                     const char* title, const char* artist,
                     const char* albumartist, const char* album,
                     const char* year, const char* track,
                     const char* genre, const char* comment,
                     int iTrackLength)
    {
      return addonInstance->Start(iInChannels,
                                  iInRate,
                                  iInBits,
                                  title,
                                  artist,
                                  albumartist,
                                  album,
                                  year,
                                  track,
                                  genre,
                                  comment,
                                  iTrackLength);
    }

    inline static int ADDON_Encode(CInstanceAudioEncoder* addonInstance, int nNumBytesRead, uint8_t* pbtStream)
    {
      return addonInstance->Encode(nNumBytesRead, pbtStream);
    }

    inline static bool ADDON_Finish(CInstanceAudioEncoder* addonInstance)
    {
      return addonInstance->Finish();
    }

    AddonInstance_AudioEncoder* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
