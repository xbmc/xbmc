/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*
* CAddonProvider
* IUnknown implementation to retrieve sub-addons from already active addons
* See Inputstream.cpp/h for an explaric use case
*/

namespace kodi { namespace addon { class IAddonInstance; } }

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
      INSTANCE_VIDEOCODEC
    };
    virtual void getAddonInstance(INSTANCE_TYPE instance_type, ADDON::BinaryAddonBasePtr& addonBase, kodi::addon::IAddonInstance*& parentInstance) = 0;
  };

  } //Namespace
