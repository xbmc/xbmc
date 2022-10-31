/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MediaSource.h"
#include "utils/LabelFormatter.h"
#include "utils/SortUtils.h"

#include <vector>

class CViewState; // forward
class CFileItemList;

namespace PLAYLIST
{
using Id = int;
} // namespace PLAYLIST

class CGUIViewState
{
public:
  virtual ~CGUIViewState();
  static CGUIViewState* GetViewState(int windowId, const CFileItemList& items);

  void SetViewAsControl(int viewAsControl);
  void SaveViewAsControl(int viewAsControl);
  int GetViewAsControl() const;

  bool ChooseSortMethod();
  SortDescription SetNextSortMethod(int direction = 1);
  void SetCurrentSortMethod(int method);
  SortDescription GetSortMethod() const;
  bool HasMultipleSortMethods() const;
  int GetSortMethodLabel() const;
  int GetSortOrderLabel() const;
  void GetSortMethodLabelMasks(LABEL_MASKS& masks) const;

  std::vector<SortDescription> GetSortDescriptions() const;

  SortOrder SetNextSortOrder();
  SortOrder GetSortOrder() const;

  virtual bool HideExtensions();
  virtual bool HideParentDirItems();
  virtual bool DisableAddSourceButtons();

  virtual PLAYLIST::Id GetPlaylist() const;
  const std::string& GetPlaylistDirectory();
  void SetPlaylistDirectory(const std::string& strDirectory);
  bool IsCurrentPlaylistDirectory(const std::string& strDirectory);
  virtual bool AutoPlayNextItem();

  virtual std::string GetLockType();
  virtual std::string GetExtensions();
  virtual VECSOURCES& GetSources();

protected:
  explicit CGUIViewState(const CFileItemList& items);  // no direct object creation, use GetViewState()

  virtual void SaveViewState() = 0;
  virtual void SaveViewToDb(const std::string &path, int windowID, CViewState *viewState = NULL);
  void LoadViewState(const std::string &path, int windowID);

  void AddLiveTVSources();

  /*! \brief Add the sort order defined in a smartplaylist
   Defaults to SORT_METHOD_PLAYLIST_ORDER if no order is defined.
   \param items the list of items for the view state.
   \param label_mask the label masks for formatting items.
   */
  void AddPlaylistOrder(const CFileItemList& items, const LABEL_MASKS& label_masks);

  void AddSortMethod(SortBy sortBy, int buttonLabel, const LABEL_MASKS &labelMasks, SortAttribute sortAttributes = SortAttributeNone, SortOrder sortOrder = SortOrderNone);
  void AddSortMethod(SortBy sortBy, SortAttribute sortAttributes, int buttonLabel, const LABEL_MASKS &labelMasks, SortOrder sortOrder = SortOrderNone);
  void AddSortMethod(SortDescription sortDescription, int buttonLabel, const LABEL_MASKS &labelMasks);
  void SetSortMethod(SortBy sortBy, SortOrder sortOrder = SortOrderNone);
  void SetSortMethod(SortDescription sortDescription);
  void SetSortOrder(SortOrder sortOrder);

  bool AutoPlayNextVideoItem() const;

  const CFileItemList& m_items;

  int m_currentViewAsControl;
  PLAYLIST::Id m_playlist;

  std::vector<GUIViewSortDetails> m_sortMethods;
  int m_currentSortMethod;

  static VECSOURCES m_sources;
  static std::string m_strPlaylistDirectory;
};

class CGUIViewStateGeneral : public CGUIViewState
{
public:
  explicit CGUIViewStateGeneral(const CFileItemList& items);

protected:
  void SaveViewState() override { }
};

class CGUIViewStateFromItems : public CGUIViewState
{
public:
  explicit CGUIViewStateFromItems(const CFileItemList& items);
  bool AutoPlayNextItem() override;

protected:
  void SaveViewState() override;
};

class CGUIViewStateLibrary : public CGUIViewState
{
public:
  explicit CGUIViewStateLibrary(const CFileItemList& items);

protected:
  void SaveViewState() override;
};
