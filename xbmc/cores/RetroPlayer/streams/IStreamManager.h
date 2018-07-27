/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
