#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "guilib/GUIDialog.h"

struct PVREpgSearchFilter;

class CGUIDialogPVRGuideSearch : public CGUIDialog
{
public:
  CGUIDialogPVRGuideSearch(void);
  virtual ~CGUIDialogPVRGuideSearch(void) {}
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnWindowLoaded();

  void SetFilterData(PVREpgSearchFilter *searchfilter) { m_searchfilter = searchfilter; }
  bool IsConfirmed() const { return m_bConfirmed; }
  bool IsCanceled() const { return m_bCanceled; }
  void OnSearch();

protected:
  void Update();

  bool m_bConfirmed;
  bool m_bCanceled;
  PVREpgSearchFilter *m_searchfilter;
};
