/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-addon-dev-kit/include/kodi/AddonBase.h"

#include <memory>

/*
* CAddonProvider
* IUnknown implementation to retrieve sub-addons from already active addons
* See Inputstream.cpp/h for an explaric use case
*/

namespace ADDON
{
  class CBinaryAddonBase;
  typedef std::shared_ptr<CBinaryAddonBase> BinaryAddonBasePtr;

  class IAddonProvider
  {
  public:
    virtual ~IAddonProvider() = default;
    enum INSTANCE_TYPE
    {
      INSTANCE_INPUTSTREAM,
      INSTANCE_VIDEOCODEC
    };
    virtual void getAddonInstance(INSTANCE_TYPE instance_type,
                                  ADDON::BinaryAddonBasePtr& addonBase,
                                  KODI_HANDLE& parentInstance) = 0;
  };

  } //Namespace
