#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

namespace PVR
{

  /*!
   * @brief PVR add-on error codes used on Kodi
   */
  typedef enum
  {
    PVRError_NO_ERROR           = 0,  /*!< no error occurred */
    PVRError_UNKNOWN            = -1, /*!< an unknown error occurred */
    PVRError_NOT_IMPLEMENTED    = -2, /*!< the method that Kodi called is not implemented by the add-on */
    PVRError_SERVER_ERROR       = -3, /*!< the backend reported an error, or the add-on isn't connected */
    PVRError_SERVER_TIMEOUT     = -4, /*!< the command was sent to the backend, but the response timed out */
    PVRError_REJECTED           = -5, /*!< the command was rejected by the backend */
    PVRError_ALREADY_PRESENT    = -6, /*!< the requested item can not be added, because it's already present */
    PVRError_INVALID_PARAMETERS = -7, /*!< the parameters of the method that was called are invalid for this operation */
    PVRError_RECORDING_RUNNING  = -8, /*!< a recording is running, so the timer can't be deleted without doing a forced delete */
    PVRError_FAILED             = -9, /*!< the command failed */
  } PVRError;

  class CPVRClientBase
  {
  public:
    static const char *ToString(const PVRError error);
  };

}
