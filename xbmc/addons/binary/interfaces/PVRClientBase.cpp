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

#include "PVRClientBase.h"

namespace PVR
{

const char *CPVRClientBase::ToString(const PVRError error)
{
  switch (error)
  {
  case PVRError_NO_ERROR:
    return "no error";
  case PVRError_NOT_IMPLEMENTED:
    return "not implemented";
  case PVRError_SERVER_ERROR:
    return "server error";
  case PVRError_SERVER_TIMEOUT:
    return "server timeout";
  case PVRError_RECORDING_RUNNING:
    return "recording already running";
  case PVRError_ALREADY_PRESENT:
    return "already present";
  case PVRError_REJECTED:
    return "rejected by the backend";
  case PVRError_INVALID_PARAMETERS:
    return "invalid parameters for this method";
  case PVRError_FAILED:
    return "the command failed";
  case PVRError_UNKNOWN:
  default:
    return "unknown error";
  }
}

};
