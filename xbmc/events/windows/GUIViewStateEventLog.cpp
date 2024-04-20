/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIViewStateEventLog.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "guilib/WindowIDs.h"
#include "view/ViewState.h"
#include "windowing/GraphicContext.h"

CGUIViewStateEventLog::CGUIViewStateEventLog(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%d", "%L", "%d"));  // Label, Date | Label, Date

  SetSortMethod(SortByDate);
  SetViewAsControl(DEFAULT_VIEW_AUTO);

  SetSortOrder(SortOrderDescending);
  LoadViewState(items.GetPath(), WINDOW_EVENT_LOG);
}

void CGUIViewStateEventLog::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_EVENT_LOG);
}

std::string CGUIViewStateEventLog::GetExtensions()
{
  return "";
}
