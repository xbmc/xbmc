#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>

#include "guilib/GUIDialog.h"

class CFileItemList;
class CMediaSource;

class CGUIDialogMediaSource :
      public CGUIDialog
{
public:
  CGUIDialogMediaSource(void);
  ~CGUIDialogMediaSource(void) override;
  bool OnMessage(CGUIMessage& message) override;
  void OnDeinitWindow(int nextWindowID) override;
  bool OnBack(int actionID) override;
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
  std::string GetUniqueMediaSourceName();
  static void OnMediaSourceChanged(const std::string& type, const std::string& oldName, const CMediaSource& share);

  std::vector<std::string> GetPaths() const;

  std::string m_type;
  std::string m_name;
  CFileItemList* m_paths;
  bool m_confirmed;
  bool m_bNameChanged;
};
