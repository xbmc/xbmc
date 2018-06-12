/*
 *      Copyright (C) 2018 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "RetroPlayerStreamTypes.h"

namespace KODI
{
namespace RETRO
{

class IStreamManager
{
public:
  virtual ~IStreamManager() = default;

  /*!
   * \brief Create a stream for gameplay data
   *
   * \param streamType The stream type
   *
   * \return A stream handle, or empty on failure
   */
  virtual StreamPtr CreateStream(StreamType streamType) = 0;

  /*!
   * \brief Free the specified stream
   *
   * \param stream The stream to close
   */
  virtual void CloseStream(StreamPtr stream) = 0;
};

}
}
