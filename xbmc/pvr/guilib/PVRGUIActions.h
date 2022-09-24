/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/guilib/PVRGUIActionsRecordings.h"
#include "pvr/guilib/PVRGUIActionsTimers.h"
#include "pvr/guilib/PVRGUIActionsUtils.h"

namespace PVR
{
class CPVRGUIActions : public CPVRGUIActionsRecordings,
                       public CPVRGUIActionsTimers,
                       public CPVRGUIActionsUtils
{
public:
  CPVRGUIActions() = default;
  virtual ~CPVRGUIActions() = default;
};

} // namespace PVR
