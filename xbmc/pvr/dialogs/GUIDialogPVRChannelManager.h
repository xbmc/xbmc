#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "dialogs/GUIDialogContextMenu.h"
#include "GUIViewControl.h"

namespace PVR
{
  class CGUIDialogPVRChannelManager : public CGUIDialog
  {
  public:
    CGUIDialogPVRChannelManager(void);
    virtual ~CGUIDialogPVRChannelManager(void);
    virtual bool OnMessage(CGUIMessage& message);
    virtual bool OnAction(const CAction& action);
    virtual void OnWindowLoaded();
    virtual void OnWindowUnload();
    virtual bool HasListItems() const { return true; };
    virtual CFileItemPtr GetCurrentListItem(int offset = 0);

  protected:
    virtual bool OnPopupMenu(int iItem);
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);

  private:
    void Clear();
    void Update();
    void SaveList();
    void Renumber();
    void SetData(int iItem);
    bool m_bIsRadio;
    bool m_bMovingMode;
    bool m_bContainsChanges;

    int m_iSelected;
    CFileItemList* m_channelItems;
    CGUIViewControl m_viewControl;
  };
}
