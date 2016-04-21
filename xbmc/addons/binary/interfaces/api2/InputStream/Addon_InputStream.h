#pragma once
/*
 *      Copyright (C) 2012-2016 Team Kodi
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

struct DemuxPacket;

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;
struct kodi_codec;

namespace InputStream
{
extern "C"
{

  class CAddOnInputStream
  {
  public:
    static void Init(struct CB_AddOnLib *interfaces);

    static void get_codec_by_name(void* addonData, const char* strCodecName, kodi_codec* codec);

    /*!
     * @brief Allocate a demux packet. Free with FreeDemuxPacket
     * @param addonData A pointer to the add-on.
     * @param iDataSize The size of the data that will go into the packet
     * @return The allocated packet.
     */
    static DemuxPacket* allocate_demux_packet(void* addonData, int iDataSize);

    /*!
     * @brief Free a packet that was allocated with AllocateDemuxPacket
     * @param addonData A pointer to the add-on.
     * @param pPacket The packet to free.
     */
    static void free_demux_packet(void* addonData, DemuxPacket* pPacket);
  };

} /* extern "C" */
} /* namespace InputStream */

} /* namespace KodiAPI */
} /* namespace V2 */
