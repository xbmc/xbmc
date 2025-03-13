/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/AddonBase.h"

#include <memory>

/*
* CAddonProvider
* IUnknown implementation to retrieve sub-addons from already active addons
* See Inputstream.cpp/h for an explaric use case
*/

namespace ADDON
{
class CAddonInfo;
typedef std::shared_ptr<CAddonInfo> AddonInfoPtr;

class IAddonProvider
{
public:
  virtual ~IAddonProvider() = default;
  enum class InstanceType
  {
    INPUTSTREAM,
    VIDEOCODEC
  };
  virtual void GetAddonInstance(InstanceType instance_type,
                                ADDON::AddonInfoPtr& addonInfo,
                                KODI_HANDLE& parentInstance) = 0;
};

} // namespace ADDON
