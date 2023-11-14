/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief Input callbacks
 *
 * @todo Remove this file when Game API is updated for input polling
 */
class IGameInputCallback
{
public:
  virtual ~IGameInputCallback() = default;

  /*!
   * \brief Return true if the input source accepts input
   *
   * \return True if input should be processed, false otherwise
   */
  virtual bool AcceptsInput() const = 0;

  /*!
   * \brief Poll the input source for input
   */
  virtual void PollInput() = 0;
};
} // namespace GAME
} // namespace KODI
