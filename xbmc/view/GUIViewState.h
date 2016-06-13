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

#include <vector>

#include "utils/LabelFormatter.h"
#include "utils/SortUtils.h"
#include "MediaSource.h"

class CViewState; // forward
class CFileItemList;

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

  SortOrder SetNextSortOrder();
  SortOrder GetSortOrder() const;

  virtual bool HideExtensions();
  virtual bool HideParentDirItems();
  virtual bool DisableAddSourceButtons();

  virtual int GetPlaylist();
  const std::string& GetPlaylistDirectory();
  void SetPlaylistDirectory(const std::string& strDirectory);
  bool IsCurrentPlaylistDirectory(const std::string& strDirectory);
  virtual bool AutoPlayNextItem();

  virtual std::string GetLockType();
  virtual std::string GetExtensions();
  virtual VECSOURCES& GetSources();

protected:
  CGUIViewState(const CFileItemList& items);  // no direct object creation, use GetViewState()

  virtual void SaveViewState() = 0;
  virtual void SaveViewToDb(const std::string &path, int windowID, CViewState *viewState = NULL);
  void LoadViewState(const std::string &path, int windowID);
  
  /*! \brief Add the addons source for the given content type, if the user has suitable addons
   \param content the type of addon content desired
   \param label the name of the addons source
   \param thumb the skin image to use as the icon
   */
  void AddAddonsSource(const std::string &content, const std::string &label, const std::string& thumb);
  void AddLiveTVSources();

  /*! \brief Add the sort order defined in a smartplaylist
   Defaults to SORT_METHOD_PLAYLIST_ORDER if no order is defined.
   \param items the list of items for the view state.
   \param label_mask the label masks for formatting items.
   */
  void AddPlaylistOrder(const CFileItemList &items, LABEL_MASKS label_masks);

  void AddSortMethod(SortBy sortBy, int buttonLabel, const LABEL_MASKS &labelMasks, SortAttribute sortAttributes = SortAttributeNone, SortOrder sortOrder = SortOrderNone);
  void AddSortMethod(SortBy sortBy, SortAttribute sortAttributes, int buttonLabel, const LABEL_MASKS &labelMasks, SortOrder sortOrder = SortOrderNone);
  void AddSortMethod(SortDescription sortDescription, int buttonLabel, const LABEL_MASKS &labelMasks);
  void SetSortMethod(SortBy sortBy, SortOrder sortOrder = SortOrderNone);
  void SetSortMethod(SortDescription sortDescription);
  void SetSortOrder(SortOrder sortOrder);

  const CFileItemList& m_items;

  int m_currentViewAsControl;
  int m_playlist;

  std::vector<GUIViewSortDetails> m_sortMethods;
  int m_currentSortMethod;

  static VECSOURCES m_sources;
  static std::string m_strPlaylistDirectory;
};

class CGUIViewStateGeneral : public CGUIViewState
{
public:
  CGUIViewStateGeneral(const CFileItemList& items);

protected:
  virtual void SaveViewState() { }
};

class CGUIViewStateFromItems : public CGUIViewState
{
public:
  CGUIViewStateFromItems(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateLibrary : public CGUIViewState
{
public:
  CGUIViewStateLibrary(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};
