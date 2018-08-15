/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CKey;

/// \addtogroup input
/// \{

class IInputEventSource
{
public:
  virtual ~IInputEventSource() = default;

  /*!
   * \brief Try to get a keypress from an external source
   * \param frameTime The current frametime
   * \param key The fetched key
   * \return True when a keypress was fetched, false otherwise
   */
  virtual bool GetNextKeypress(float frameTime, CKey &key) { return false; }
};

/// \}
