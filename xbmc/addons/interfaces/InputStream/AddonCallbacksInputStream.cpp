/*
 *      Copyright (C) 2012-2016 Team XBMC
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

#include "addons/Addon.h"
#include "AddonCallbacksInputStream.h"

namespace KodiAPI
{
namespace InputStream
{

CAddonCallbacksInputStream::CAddonCallbacksInputStream(ADDON::CAddon* addon)
  : m_callbacks(new CB_INPUTSTREAMLib)
{
  m_callbacks->FreeDemuxPacket = InputStreamFreeDemuxPacket;
  m_callbacks->AllocateDemuxPacket = InputStreamAllocateDemuxPacket;
}

CAddonCallbacksInputStream::~CAddonCallbacksInputStream()
{
  /* delete the callback table */
  delete m_callbacks;
}

void CAddonCallbacksInputStream::InputStreamFreeDemuxPacket(void *addonData, DemuxPacket* pPacket)
{
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

DemuxPacket* CAddonCallbacksInputStream::InputStreamAllocateDemuxPacket(void *addonData, int iDataSize)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
}

} /* namespace InputStream */
} /* namespace KodiAPI */
