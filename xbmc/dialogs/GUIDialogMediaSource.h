#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "guilib/GUIDialog.h"

class CFileItemList;
class CMediaSource;

class CGUIDialogMediaSource :
      public CGUIDialog
{
public:
  CGUIDialogMediaSource(void);
  virtual ~CGUIDialogMediaSource(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnDeinitWindow(int nextWindowID);
  virtual bool OnBack(int actionID);
  static bool ShowAndAddMediaSource(const std::string &type);
  static bool ShowAndEditMediaSource(const std::string &type, const CMediaSource &share);
  static bool ShowAndEditMediaSource(const std::string &type, const std::string &share);

  bool IsConfirmed() const { return m_confirmed; };

  void SetShare(const CMediaSource &share);
  void SetTypeOfMedia(const std::string &type, bool editNotAdd = false);
protected:
  void OnPathBrowse(int item);
  void OnPath(int item);
  void OnPathAdd();
  void OnPathRemove(int item);
  void OnOK();
  void OnCancel();
  void UpdateButtons();
  int GetSelectedItem();
  void HighlightItem(int item);

  std::vector<std::string> GetPaths() const;

  std::string m_type;
  std::string m_name;
  CFileItemList* m_paths;
  bool m_confirmed;
  bool m_bNameChanged;
};
