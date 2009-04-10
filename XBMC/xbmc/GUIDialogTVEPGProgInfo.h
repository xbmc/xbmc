#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogSettings.h"
#include "GUIListItem.h"
#include "DateTime.h"

#include "GUIDialog.h"
#include "GUIListItem.h"

class CFileItem;

class CGUIDialogTVEPGProgInfo : public CGUIDialog
{
public:
  CGUIDialogTVEPGProgInfo(void);
  virtual ~CGUIDialogTVEPGProgInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  void SetProgInfo(const CFileItem *item);

  virtual CFileItemPtr GetCurrentListItem(int offset = 0) { return m_progItem; }

  virtual bool HasListItems() const { return true; };

protected:
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);

  CFileItemPtr        m_progItem;
  bool                m_viewDescription;
};

