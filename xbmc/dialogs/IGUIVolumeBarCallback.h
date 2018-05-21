/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
