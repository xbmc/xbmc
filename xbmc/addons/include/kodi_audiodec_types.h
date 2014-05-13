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

#pragma once

#include <stdint.h>
#ifdef BUILD_KODI_ADDON
#include "kodi/AEChannelData.h"
#else
#include "cores/AudioEngine/Utils/AEChannelData.h"
#endif

extern "C"
{
  struct AUDIODEC_INFO
  {
    int dummy;
  };

  struct AUDIODEC_PROPS
  {
    int dummy;
  };

  struct AudioDecoder
  {
    //! \brief Initialize a decoder
    //! \param file The file to read
    //! \param filecache The file cache size
    //! \param channels Number of channels in output stream
    //! \param samplerate Samplerate of output stream
    //! \param bitspersample Bits per sample in output stream
    //! \param totaltime Total time for stream
    //! \param bitrate Average bitrate of input stream
    //! \param format Data format for output stream
    //! \param info Channel mapping for output stream
    //! \return Context of output stream
    //! \sa ICodec::Init
    void* (__cdecl* Init) (const char* file, unsigned int filecache,
                           int* channels, int* samplerate,
                           int* bitspersample, int64_t* totaltime,
                           int* bitrate, AEDataFormat* format,
                           const AEChannel** info);

    //! \brief Produce some noise
    //! \param context Context of output stream
    //! \param buffer Output buffer
    //! \param size Size of output buffer
    //! \param actualsize Actual number of bytes written to output buffer
    //! \return 0 on success, -1 on end of stream, 1 on failure
    //! \sa ICodec::ReadPCM
    int  (__cdecl* ReadPCM) (void* context, uint8_t* buffer, int size, int* actualsize);


    //! \brief Seek in output stream
    //! \param context Context of output stream
    //! \param time Time position to seek to in milliseconds
    //! \return Time position seek ended up on
    //! \sa ICodec::Seek
    int64_t  (__cdecl* Seek) (void* context, int64_t time);

    //! \brief Read tag of a file
    //! \param file File to read tag for
    //! \param title Title of file
    //! \param artist Artist of file
    //! \param length Length of file
    //! \return True on success, false on failure
    //! \sa IMusicInfoTagLoader::ReadTag
    bool (__cdecl* ReadTag)(const char* file, char* title,
                            char* artist, int* length);

    //! \brief Get number of tracks in a file
    //! \param file File to read tag for
    //! \return Number of tracks in file
    //! \sa CMusicFileDirectory
    int  (__cdecl* TrackCount) (const char* file);

    //! \brief Close down an output stream
    //! \param context Context of stream
    //! \return True on success, false on failure
    //! \sa ICodec::DeInit
    bool (__cdecl* DeInit)(void* context);
  };
}
