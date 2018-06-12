/*
 *      Copyright (C) 2017 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <wayland-client-protocol.hpp>

#include <string>
#include <vector>

#include "threads/CriticalSection.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CConnection;

/**
 * Retrieve and accept selection (clipboard) offers on the data device of a seat
 */
class CSeatSelection
{
public:
  CSeatSelection(CConnection& connection, wayland::seat_t const& seat);
  std::string GetSelectionText() const;

private:
  wayland::data_device_t m_dataDevice;
  wayland::data_offer_t m_currentOffer;
  mutable wayland::data_offer_t m_currentSelection;

  std::vector<std::string> m_mimeTypeOffers;
  std::string m_matchedMimeType;

  CCriticalSection m_currentSelectionMutex;
};

}
}
}
