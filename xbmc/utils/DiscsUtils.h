/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace UTILS
{
namespace DISCS
{

/*! \brief Abstracts a disc type
*/
enum class DiscType
{
  UNKNOWN, ///< the default value, the application doesn't know what the device is
  DVD, ///< dvd disc
  BLURAY ///< blu-ray disc
};

/*! \brief Abstracts the info the app knows about a disc (type, name, serial)
*/
struct DiscInfo
{
  /*! \brief The disc type, \sa DiscType */
  DiscType type{DiscType::UNKNOWN};
  /*! \brief The disc serial number */
  std::string serial;
  /*! \brief The disc label (equivalent to the mount point label) */
  std::string name;

  /*! \brief Check if the info is empty (e.g. after probing)
    \return true if the info is empty, false otherwise
  */
  bool empty() { return (type == DiscType::UNKNOWN && name.empty() && serial.empty()); }

  /*! \brief Clears all the DiscInfo members
  */
  void clear()
  {
    type = DiscType::UNKNOWN;
    name.clear();
    serial.clear();
  }
};

/*! \brief Try to obtain the disc info (type, name, serial) of a given media path
    \param[in, out] info The disc info struct
    \param mediaPath The disc mediapath (e.g. /dev/cdrom, D\://, etc)
    \return true if getting the disc info was successful
*/
bool GetDiscInfo(DiscInfo& info, const std::string& mediaPath);

/*! \brief Try to probe the provided media path as a DVD
    \param mediaPath The disc mediapath (e.g. /dev/cdrom, D\://, etc)
    \return the DiscInfo for the given media path (might be an empty struct)
*/
DiscInfo ProbeDVDDiscInfo(const std::string& mediaPath);

/*! \brief Try to probe the provided media path as a Bluray
    \param mediaPath The disc mediapath (e.g. /dev/cdrom, D\://, etc)
    \return the DiscInfo for the given media path (might be an empty struct)
*/
DiscInfo ProbeBlurayDiscInfo(const std::string& mediaPath);

} // namespace DISCS
} // namespace UTILS
