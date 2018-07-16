/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CVariant;

class ISerializable
{
protected:
  /* make sure nobody deletes a pointer to this class */
  ~ISerializable() = default;

 public:
  virtual void Serialize(CVariant& value) const = 0;
};
