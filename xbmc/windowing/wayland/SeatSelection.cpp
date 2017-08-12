/*
 *      Copyright (C) 2017 Team XBMC
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

#include "SeatSelection.h"

#include <poll.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <system_error>

#include "Connection.h"
#include "Registry.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/posix/FileHandle.h"
#include "utils/StringUtils.h"
#include "WinEventsWayland.h"

using namespace KODI::UTILS::POSIX;
using namespace KODI::WINDOWING::WAYLAND;

namespace
{

const std::vector<std::string> MIME_TYPES_PREFERENCE =
{
  "text/plain;charset=utf-8",
  "text/plain;charset=iso-8859-1",
  "text/plain;charset=us-ascii",
  "text/plain"
};

}

CSeatSelection::CSeatSelection(CConnection& connection, wayland::seat_t const& seat)
{
  wayland::data_device_manager_t manager;
  {
    CRegistry registry{connection};
    registry.RequestSingleton(manager, 1, 3, false);
    registry.Bind();
  }

  if (!manager)
  {
    CLog::Log(LOGWARNING, "No data device manager announced by compositor, clipboard will not be available");
    return;
  }

  m_dataDevice = manager.get_data_device(seat);

  // Class is created in response to seat add events - so no events can get lost
  m_dataDevice.on_data_offer() = [this](wayland::data_offer_t offer)
  {
    // We don't know yet whether this is drag-and-drop or selection, so collect
    // MIME types in either case
    m_currentOffer = offer;
    m_mimeTypeOffers.clear();
    m_currentOffer.on_offer() = [this](std::string mime)
    {
      m_mimeTypeOffers.push_back(std::move(mime));
    };
  };
  m_dataDevice.on_selection() = [this](wayland::data_offer_t offer)
  {
    CSingleLock lock(m_currentSelectionMutex);
    m_matchedMimeType.clear();

    if (offer != m_currentOffer)
    {
      // Selection was not previously introduced by offer (could be NULL for example)
      m_currentSelection.proxy_release();
    }
    else
    {
      m_currentSelection = offer;
      std::string offers = StringUtils::Join(m_mimeTypeOffers, ", ");

      // Match MIME type by priority: Find first preferred MIME type that is in the
      // set of offered types
      // Charset is not case-sensitive in MIME type spec, so match case-insensitively
      auto mimeIt = std::find_first_of(MIME_TYPES_PREFERENCE.cbegin(), MIME_TYPES_PREFERENCE.cend(),
                                       m_mimeTypeOffers.cbegin(), m_mimeTypeOffers.cend(),
                                       // static_cast needed for overload resolution
                                       static_cast<bool (*)(std::string const&, std::string const&)> (&StringUtils::EqualsNoCase));
      if (mimeIt != MIME_TYPES_PREFERENCE.cend())
      {
        m_matchedMimeType = *mimeIt;
        CLog::Log(LOGDEBUG, "Chose selection MIME type %s out of offered %s", m_matchedMimeType.c_str(), offers.c_str());
      }
      else
      {
        CLog::Log(LOGDEBUG, "Could not find compatible MIME type for selection data (offered: %s)", offers.c_str());
      }
    }
  };
}

std::string CSeatSelection::GetSelectionText() const
{
  CSingleLock lock(m_currentSelectionMutex);
  if (!m_currentSelection || m_matchedMimeType.empty())
  {
    return "";
  }

  std::array<int, 2> fds;
  if (pipe(fds.data()) != 0)
  {
    CLog::LogF(LOGERROR, "Could not open pipe for selection data transfer: %s", std::strerror(errno));
    return "";
  }

  CFileHandle readFd{fds[0]};
  CFileHandle writeFd{fds[1]};

  m_currentSelection.receive(m_matchedMimeType, writeFd);
  lock.Leave();
  // Make sure the other party gets the request as soon as possible
  CWinEventsWayland::Flush();
  // Fd now gets sent to the other party -> make sure our write end is closed
  // so we get POLLHUP when the other party closes its write fd
  writeFd.reset();

  pollfd fd =
  {
    .fd = readFd,
    .events = POLLIN,
    .revents = 0
  };

  // UI will block in this function when Ctrl+V is pressed, so timeout should be
  // rather short!
  const std::chrono::seconds TIMEOUT{1};
  const std::size_t MAX_SIZE{4096};
  std::array<char, MAX_SIZE> buffer;

  auto start = std::chrono::steady_clock::now();
  std::size_t totalBytesRead{0};

  do
  {
    auto now = std::chrono::steady_clock::now();
    // Do not permit negative timeouts (would cause infinitely long poll)
    auto remainingTimeout = std::max(std::chrono::milliseconds(0), std::chrono::duration_cast<std::chrono::milliseconds> (TIMEOUT - (now - start))).count();
    // poll() for changes until poll signals POLLHUP and the remaining data was read
    int ret{poll(&fd, 1, remainingTimeout)};
    if (ret == 0)
    {
      // Timeout
      CLog::LogF(LOGERROR, "Reading from selection data pipe timed out");
      return "";
    }
    else if (ret < 0 && errno == EINTR)
    {
      continue;
    }
    else if (ret < 0)
    {
      throw std::system_error(errno, std::generic_category(), "Error polling selection pipe");
    }
    else if (fd.revents & POLLNVAL || fd.revents & POLLERR)
    {
      CLog::LogF(LOGERROR, "poll() indicated error on selection pipe");
      return "";
    }
    else if (fd.revents & POLLIN)
    {
      if (totalBytesRead >= buffer.size())
      {
        CLog::LogF(LOGERROR, "Selection data is too big, aborting read");
        return "";
      }
      ssize_t readBytes{read(fd.fd, buffer.data() + totalBytesRead, buffer.size() - totalBytesRead)};
      if (readBytes < 0)
      {
        CLog::LogF(LOGERROR, "read() from selection pipe failed: %s", std::strerror(errno));
        return "";
      }
      totalBytesRead += readBytes;
    }
  }
  while (!(fd.revents & POLLHUP));

  return std::string(buffer.data(), totalBytesRead);
}
