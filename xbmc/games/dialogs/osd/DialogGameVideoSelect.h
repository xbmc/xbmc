/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <memory>

class CFileItemList;
class CGUIViewControl;

namespace KODI
{
namespace RETRO
{
class CGUIGameVideoHandle;
}

namespace GAME
{
/*!
 * \ingroup games
 */
class CDialogGameVideoSelect : public CGUIDialog
{
public:
  ~CDialogGameVideoSelect() override;

  // implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

  // implementation of CGUIWindow via CGUIDialog
  void FrameMove() override;
  void OnDeinitWindow(int nextWindowID) override;

protected:
  CDialogGameVideoSelect(int windowId);

  // implementation of CGUIWindow via CGUIDialog
  void OnWindowUnload() override;
  void OnWindowLoaded() override;
  void OnInitWindow() override;

  // Video select interface
  virtual std::string GetHeading() = 0;
  virtual void PreInit() = 0;
  virtual void GetItems(CFileItemList& items) = 0;
  virtual void OnItemFocus(unsigned int index) = 0;
  virtual unsigned int GetFocusedItem() const = 0;
  virtual void PostExit() = 0;
  // override this to do something when an item is selected
  virtual bool OnClickAction() { return false; }
  // override this to do something when an item's context menu is opened
  virtual bool OnMenuAction() { return false; }
  // override this to do something when an item is overwritten with a new savestate
  virtual bool OnOverwriteAction() { return false; }
  // override this to do something when an item is renamed
  virtual bool OnRenameAction() { return false; }
  // override this to do something when an item is deleted
  virtual bool OnDeleteAction() { return false; }

  // GUI functions
  void RefreshList();
  void OnDescriptionChange(const std::string& description);

  std::shared_ptr<RETRO::CGUIGameVideoHandle> m_gameVideoHandle;

private:
  void Update();
  void Clear();

  void SaveSettings();

  void RegisterDialog();
  void UnregisterDialog();

  std::unique_ptr<CGUIViewControl> m_viewControl;
  std::unique_ptr<CFileItemList> m_vecItems;
};
} // namespace GAME
} // namespace KODI
