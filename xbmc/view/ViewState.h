/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/SortUtils.h"

#define DEFAULT_VIEW_AUTO (VIEW_TYPE_AUTO << 16)
#define DEFAULT_VIEW_LIST (VIEW_TYPE_LIST << 16)
#define DEFAULT_VIEW_ICONS (VIEW_TYPE_ICON << 16)
#define DEFAULT_VIEW_BIG_ICONS (VIEW_TYPE_BIG_ICON << 16)
#define DEFAULT_VIEW_INFO (VIEW_TYPE_INFO << 16)
#define DEFAULT_VIEW_BIG_INFO (VIEW_TYPE_BIG_INFO << 16)
#define DEFAULT_VIEW_MAX (((VIEW_TYPE_MAX - 1) << 16) | 60)

class CViewState
{
public:
  CViewState(int viewMode, SortBy sortMethod, SortOrder sortOrder, SortAttribute sortAttributes = SortAttributeNone)
  {
    m_viewMode = viewMode;
    m_sortDescription.sortBy = sortMethod;
    m_sortDescription.sortOrder = sortOrder;
    m_sortDescription.sortAttributes = sortAttributes;
  };
  CViewState()
  {
    m_viewMode = 0;
    m_sortDescription.sortBy = SortByLabel;
    m_sortDescription.sortOrder = SortOrderAscending;
  };

  int m_viewMode;
  SortDescription m_sortDescription;
};
