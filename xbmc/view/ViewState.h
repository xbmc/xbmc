/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
