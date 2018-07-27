/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

  mutable CCriticalSection m_currentSelectionMutex;
};

}
}
}
