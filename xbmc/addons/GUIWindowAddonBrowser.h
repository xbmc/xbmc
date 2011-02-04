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
#include "windows/GUIMediaWindow.h"
#include "pictures/PictureThumbLoader.h"

class CFileItem;
class CFileItemList;

class CGUIWindowAddonBrowser : public CGUIMediaWindow
{
public:
  CGUIWindowAddonBrowser(void);
  virtual ~CGUIWindowAddonBrowser(void);
  virtual bool OnMessage(CGUIMessage& message);

  /*! \brief Popup a selection dialog with a list of addons of the given type
   \param type the type of addon wanted
   \param addonID [out] the addon ID of the selected item
   \param showNone whether there should be a "None" item in the list (defaults to false)
   \return 1 if an addon was selected, 2 if "Get More" was chosen, or 0 if an error occurred or if the selection process was cancelled
   */
  static int SelectAddonID(ADDON::TYPE type, CStdString &addonID, bool showNone = false);

protected:
  /* \brief set label2 of an item based on the Addon.Status property
   \param item the item to update
   */
  void SetItemLabel2(CFileItemPtr item);

  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnClick(int iItem);
  virtual void UpdateButtons();
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual bool Update(const CStdString &strDirectory);
  virtual CStdString GetStartFolder(const CStdString &dir);
private:
  CPictureThumbLoader m_thumbLoader;
};

