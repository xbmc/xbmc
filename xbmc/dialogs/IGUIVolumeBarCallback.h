/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
 * \brief Interface to expose properties to the volume bar dialog
 */
class IGUIVolumeBarCallback
{
public:
  ~IGUIVolumeBarCallback() = default;

  /*!
   * \brief Return true if the callback is active in the GUI
   *
   * If a registered callback is shown in the GUI, the volume bar is disabled
   * until no more callbacks are shown.
   *
   * \return True if the callback is active in the GUI, false otherwise
   */
  virtual bool IsShown() const = 0;
};
