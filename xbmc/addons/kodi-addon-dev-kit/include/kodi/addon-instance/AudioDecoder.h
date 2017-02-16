#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../AddonBase.h"

#include <stdint.h>
#ifdef BUILD_KODI_ADDON
#include "../AEChannelData.h"
#else
#include "cores/AudioEngine/Utils/AEChannelData.h"
#endif

namespace kodi { namespace addon { class CInstanceAudioDecoder; }}

extern "C"
{

  typedef struct AddonProps_AudioDecoder
  {
    int dummy;
  } AddonProps_AudioDecoder;

  typedef struct AddonToKodiFuncTable_AudioDecoder
  {
    void* kodiInstance;
  } AddonToKodiFuncTable_AudioDecoder;

  typedef struct KodiToAddonFuncTable_AudioDecoder
  {
    bool (__cdecl* Init) (kodi::addon::CInstanceAudioDecoder* addonInstance,
                          const char* file, unsigned int filecache,
                          int* channels, int* samplerate,
                          int* bitspersample, int64_t* totaltime,
                          int* bitrate, AEDataFormat* format,
                          const AEChannel** info);
    int  (__cdecl* ReadPCM) (kodi::addon::CInstanceAudioDecoder* addonInstance, uint8_t* buffer, int size, int* actualsize);
    int64_t  (__cdecl* Seek) (kodi::addon::CInstanceAudioDecoder* addonInstance, int64_t time);
    bool (__cdecl* ReadTag) (kodi::addon::CInstanceAudioDecoder* addonInstance,
                             const char* file, char* title,
                             char* artist, int* length);
    int  (__cdecl* TrackCount) (kodi::addon::CInstanceAudioDecoder* addonInstance, const char* file);
  } KodiToAddonFuncTable_AudioDecoder;

  typedef struct AddonInstance_AudioDecoder
  {
    AddonProps_AudioDecoder props;
    AddonToKodiFuncTable_AudioDecoder toKodi;
    KodiToAddonFuncTable_AudioDecoder toAddon;
  } AddonInstance_AudioDecoder;

} /* extern "C" */

namespace kodi
{
namespace addon
{

  class CInstanceAudioDecoder : public IAddonInstance
  {
  public:
    //==========================================================================
    /// @brief Class constructor
    ///
    /// @param[in] instance             The from Kodi given instance given be
    ///                                 add-on CreateInstance call with instance
    ///                                 id ADDON_INSTANCE_AUDIODECODER.
    CInstanceAudioDecoder(KODI_HANDLE instance)
      : IAddonInstance(ADDON_INSTANCE_AUDIODECODER)
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
    virtual bool Init(std::string filename, unsigned int filecache,
                      int& channels, int& samplerate,
                      int& bitspersample, int64_t& totaltime,
                      int& bitrate, AEDataFormat& format,
                      std::vector<AEChannel>& channellist)=0;
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
    virtual int ReadPCM(uint8_t* buffer, int size, int& actualsize)=0;
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
    /// @param[out] title               Title of file
    /// @param[out] artist              Artist of file
    /// @param[out] length              Length of file
    /// @return                         True on success, false on failure
    ///
    virtual bool ReadTag(std::string file, std::string& title, std::string& artist, int& length) { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    /// @ingroup cpp_kodi_addon_audiodecoder
    /// @brief Get number of tracks in a file
    ///
    /// @param[in] file                 File to read tag for
    /// @return                         Number of tracks in file
    ///
    virtual int TrackCount(std::string file) { return 1; }
    //--------------------------------------------------------------------------

  private:
    void SetAddonStruct(KODI_HANDLE instance)
    {
      if (instance == nullptr)
        throw std::logic_error("kodi::addon::CInstanceAudioDecoder: Creation with empty addon structure not allowed, table must be given from Kodi!");

      m_instanceData = static_cast<AddonInstance_AudioDecoder*>(instance);

      m_instanceData->toAddon.Init = ADDON_Init;
      m_instanceData->toAddon.ReadPCM = ADDON_ReadPCM;
      m_instanceData->toAddon.Seek = ADDON_Seek;
      m_instanceData->toAddon.ReadTag = ADDON_ReadTag;
      m_instanceData->toAddon.TrackCount = ADDON_TrackCount;
    }

    inline static bool ADDON_Init(CInstanceAudioDecoder* addonInstance, const char* file, unsigned int filecache,
                                  int* channels, int* samplerate,
                                  int* bitspersample, int64_t* totaltime,
                                  int* bitrate, AEDataFormat* format,
                                  const AEChannel** info)
    {
      addonInstance->m_channelList.clear();
      bool ret = addonInstance->Init(file, filecache, *channels,
                                                           *samplerate, *bitspersample,
                                                           *totaltime, *bitrate, *format,
                                                           addonInstance->m_channelList);
      if (!addonInstance->m_channelList.empty())
      {
        if (addonInstance->m_channelList.back() != AE_CH_NULL)
          addonInstance->m_channelList.push_back(AE_CH_NULL);
        *info = addonInstance->m_channelList.data();
      }
      else
        *info = nullptr;
      return ret;
    }

    inline static int ADDON_ReadPCM(CInstanceAudioDecoder* addonInstance, uint8_t* buffer, int size, int* actualsize)
    {
      return addonInstance->ReadPCM(buffer, size, *actualsize);
    }

    inline static int64_t ADDON_Seek(CInstanceAudioDecoder* addonInstance, int64_t time)
    {
      return addonInstance->Seek(time);
    }

    inline static bool ADDON_ReadTag(CInstanceAudioDecoder* addonInstance, const char* file, char* title, char* artist, int* length)
    {
      std::string intTitle;
      std::string intArtist;
      memset(title, 0, ADDON_STANDARD_STRING_LENGTH_SMALL);
      memset(artist, 0, ADDON_STANDARD_STRING_LENGTH_SMALL);
      bool ret = addonInstance->ReadTag(file, intTitle, intArtist, *length);
      if (ret)
      {
        strncpy(title, intTitle.c_str(), ADDON_STANDARD_STRING_LENGTH_SMALL-1);
        strncpy(artist, intArtist.c_str(), ADDON_STANDARD_STRING_LENGTH_SMALL-1);
      }
      return ret;
    }

    inline static int ADDON_TrackCount(CInstanceAudioDecoder* addonInstance, const char* file)
    {
      return addonInstance->TrackCount(file);
    }

    std::vector<AEChannel> m_channelList;
    AddonInstance_AudioDecoder* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
