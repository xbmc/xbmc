/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "addons/kodi-addon-dev-kit/include/kodi/libKODI_guilib.h"

#include "windows/GUIMediaWindow.h"
#include "threads/Event.h"

namespace ADDON
{
  class CAddon;
}

namespace KodiAPI
{
namespace GUI
{

class CGUIAddonWindow : public CGUIMediaWindow
{
friend class CAddonCallbacksGUI;

public:
  CGUIAddonWindow(int id, const std::string& strXML, ADDON::CAddon* addon);
  ~CGUIAddonWindow(void) override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  void AllocResources(bool forceLoad = false) override;
  void FreeResources(bool forceUnLoad = false) override;
  void Render() override;
  void WaitForActionEvent(unsigned int timeout);
  void PulseActionEvent();
  void AddItem(CFileItemPtr fileItem, int itemPosition);
  void RemoveItem(int itemPosition);
  void ClearList();
  CFileItemPtr GetListItem(int position);
  int GetListSize();
  int GetCurrentListPosition();
  void SetCurrentListPosition(int item);
  bool OnClick(int iItem, const std::string &player = "") override;

protected:
  using CGUIMediaWindow::Update;
  void Update();
  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  void SetupShares() override;

  bool (*CBOnInit)(GUIHANDLE cbhdl);
  bool (*CBOnFocus)(GUIHANDLE cbhdl, int controlId);
  bool (*CBOnClick)(GUIHANDLE cbhdl, int controlId);
  bool (*CBOnAction)(GUIHANDLE cbhdl, int);

  GUIHANDLE        m_clientHandle;
  const int m_iWindowId;
  int m_iOldWindowId;
  bool m_bModal;
  bool m_bIsDialog;

private:
  CEvent           m_actionEvent;
  ADDON::CAddon*   m_addon;
  std::string      m_mediaDir;
};

/*\_____________________________________________________________________________
\*/

class CGUIAddonWindowDialog : public CGUIAddonWindow
{
public:
  CGUIAddonWindowDialog(int id, const std::string& strXML, ADDON::CAddon* addon);
  ~CGUIAddonWindowDialog(void) override;

  void            Show(bool show = true);
  bool    OnMessage(CGUIMessage &message) override;
  bool    IsDialogRunning() const override { return m_bRunning; }
  bool    IsDialog() const override { return true;};
  bool    IsModalDialog() const override { return true; };
  bool    IsMediaWindow() const override { return false; };

  void Show_Internal(bool show = true);

private:
  bool             m_bRunning;
};

} /* namespace GUI */
} /* namespace KodiAPI */
