/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace tinyxml2
{
class XMLNode;
}

class IXmlDeserializable
{
public:
  virtual ~IXmlDeserializable() = default;

  virtual bool Deserialize(const tinyxml2::XMLNode* node) = 0;
};
