/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CFileItemList;

class CAutoSwitch
{
public:

  CAutoSwitch(void);
  virtual ~CAutoSwitch(void);

  static int GetView(const CFileItemList& vecItems);

  static bool ByFolders(const CFileItemList& vecItems);
  static bool ByFiles(bool bHideParentDirItems, const CFileItemList& vecItems);
  static bool ByThumbPercent(bool bHideParentDirItems, int iPercent, const CFileItemList& vecItems);
  static bool ByFileCount(const CFileItemList& vecItems);
  static bool ByFolderThumbPercentage(bool hideParentDirItems, int percent, const CFileItemList &vecItems);
  static float MetadataPercentage(const CFileItemList &vecItems);
protected:

};
