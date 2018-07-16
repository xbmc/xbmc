/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class TiXmlNode;

/*!
  * \brief Interface for classes that can map buttons to Kodi actions
  */
class IButtonMapper
{
public:
  virtual ~IButtonMapper() = default;

  virtual void MapActions(int windowId, const TiXmlNode *pDevice) = 0;

  virtual void Clear() = 0;
};
