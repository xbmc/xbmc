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

#include "addons/Addon.h"
#include "GUIMediaWindow.h"
#include "utils/Job.h"

class CFileItem;
class CFileItemList;

class CGUIWindowAddonBrowser :
      public CGUIMediaWindow,
      public IJobCallback
{
public:
  CGUIWindowAddonBrowser(void);
  virtual ~CGUIWindowAddonBrowser(void);
  virtual bool OnMessage(CGUIMessage& message);

  // job callback
  void OnJobComplete(unsigned int jobID, bool success, CJob* job);

  static void AddJob(const CStdString& path);
protected:
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnClick(int iItem);
  virtual void UpdateButtons();
};

