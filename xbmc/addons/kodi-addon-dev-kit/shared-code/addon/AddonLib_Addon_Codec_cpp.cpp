/*
 *      Copyright (C) 2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InterProcess.h"
#include "kodi/api2/addon/Codec.hpp"

#include <string>
#include <stdarg.h>

namespace V2
{
namespace KodiAPI
{

namespace AddOn
{

  namespace Codec
  {
    kodi_codec GetCodecByName(const std::string &strCodecName)
    {
      return g_interProcess.GetCodecByName(strCodecName);
    }

    DemuxPacket* AllocateDemuxPacket(int iDataSize)
    {
      return g_interProcess.AllocateDemuxPacket(iDataSize);
    }

    void FreeDemuxPacket(DemuxPacket* pPacket)
    {
      return g_interProcess.FreeDemuxPacket(pPacket);
    }
  }; /* namespace Codec */

}; /* namespace AddOn */

}; /* namespace KodiAPI */
}; /* namespace V2 */
