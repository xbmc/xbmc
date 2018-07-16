/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>

#include "SortUtils.h"

class ISortable
{
protected:
  /* make sure nobody deletes a pointer to this class */
  ~ISortable() = default;

public:
  virtual void ToSortable(SortItem& sortable, Field field) const = 0;
};
