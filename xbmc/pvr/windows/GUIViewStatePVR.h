/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "view/GUIViewState.h"

class CFileItemList;

namespace PVR
{
class CGUIViewStatePVR : public CGUIViewState
{
public:
  CGUIViewStatePVR(const int windowId, const CFileItemList& items) : CGUIViewState(items)
  {
    m_windowId = windowId;
  }

protected:
  bool HideParentDirItems() override { return true; }

  int m_windowId;
};

class CGUIViewStateWindowPVRChannels : public CGUIViewStatePVR
{
public:
  CGUIViewStateWindowPVRChannels(const int windowId, const CFileItemList& items);

protected:
  void SaveViewState() override;
};

class CGUIViewStateWindowPVRRecordings : public CGUIViewStatePVR
{
public:
  CGUIViewStateWindowPVRRecordings(const int windowId, const CFileItemList& items);

protected:
  void SaveViewState() override;
  bool HideParentDirItems() override;
};

class CGUIViewStateWindowPVRGuide : public CGUIViewStatePVR
{
public:
  CGUIViewStateWindowPVRGuide(const int windowId, const CFileItemList& items);

protected:
  void SaveViewState() override;
};

class CGUIViewStateWindowPVRTimers : public CGUIViewStatePVR
{
public:
  CGUIViewStateWindowPVRTimers(const int windowId, const CFileItemList& items);

protected:
  void SaveViewState() override;
  bool HideParentDirItems() override;
};

class CGUIViewStateWindowPVRSearch : public CGUIViewStatePVR
{
public:
  CGUIViewStateWindowPVRSearch(const int windowId, const CFileItemList& items);

protected:
  void SaveViewState() override;
  bool HideParentDirItems() override;
};
} // namespace PVR
