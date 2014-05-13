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
#include "xbmc_addon_dll.h"
#include "kodi_audiodec_types.h"

extern "C"
{
  //! \copydoc AudioDecoder::Init
  void* Init(const char* file, unsigned int filecache, int* channels,
             int* samplerate, int* bitspersample, int64_t* totaltime,
             int* bitrate, AEDataFormat* format, const AEChannel** channelinfo);

  //! \copydoc AudioDecoder::ReadPCM
  int ReadPCM(void* context, uint8_t* buffer, int size, int* actualsize);

  //! \copydoc AudioDecoder::Seek
  int64_t Seek(void* context, int64_t time);

  //! \copydoc AudioDecoder::ReadTag
  bool ReadTag(const char* file, char* title,
               char* artist, int* length);

  //! \copydoc AudioDecoder::TrackCount
  int TrackCount(const char* file);

  //! \copydoc AudioDecoder::DeInit
  bool DeInit(void* context);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct AudioDecoder* pScr)
  {
    pScr->Init = Init;
    pScr->ReadPCM = ReadPCM;
    pScr->Seek = Seek;
    pScr->ReadTag = ReadTag;
    pScr->TrackCount = TrackCount;
    pScr->DeInit = DeInit;
  };
};
