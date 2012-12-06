#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

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

  SORT_METHOD SetNextSortMethod(int direction = 1);
  void SetCurrentSortMethod(int method);
  SORT_METHOD GetSortMethod() const;
  int GetSortMethodLabel() const;
  void GetSortMethodLabelMasks(LABEL_MASKS& masks) const;
  void GetSortMethods(std::vector< std::pair<int,int> > &sortMethods) const;

  SortOrder SetNextSortOrder();
  SortOrder GetSortOrder() const { return m_sortOrder; };
  SortOrder GetDisplaySortOrder() const;
  virtual bool HideExtensions();
  virtual bool HideParentDirItems();
  virtual bool DisableAddSourceButtons();
  virtual int GetPlaylist();
  const CStdString& GetPlaylistDirectory();
  void SetPlaylistDirectory(const CStdString& strDirectory);
  bool IsCurrentPlaylistDirectory(const CStdString& strDirectory);
  virtual bool AutoPlayNextItem();
  virtual CStdString GetLockType();
  virtual CStdString GetExtensions();
  virtual VECSOURCES& GetSources();

protected:
  CGUIViewState(const CFileItemList& items);  // no direct object creation, use GetViewState()
  virtual void SaveViewState()=0;
  virtual void SaveViewToDb(const CStdString &path, int windowID, CViewState *viewState = NULL);
  void LoadViewState(const CStdString &path, int windowID);
  
  /*! \brief Add the addons source for the given content type, if the user has suitable addons
   \param content the type of addon content desired
   \param label the name of the addons source
   \param thumb the skin image to use as the icon
   */
  void AddAddonsSource(const CStdString &content, const CStdString &label, const CStdString& thumb);
#if defined(TARGET_ANDROID)
  void AddAndroidSource(const CStdString &content, const CStdString &label, const CStdString& thumb);
#endif
  void AddLiveTVSources();

  /*! \brief Add the sort order defined in a smartplaylist
   Defaults to SORT_METHOD_PLAYLIST_ORDER if no order is defined.
   \param items the list of items for the view state.
   \param label_mask the label masks for formatting items.
   */
  void AddPlaylistOrder(const CFileItemList &items, LABEL_MASKS label_masks);

  void AddSortMethod(SORT_METHOD sortMethod, int buttonLabel, LABEL_MASKS labelmasks);
  void SetSortMethod(SORT_METHOD sortMethod);
  void SetSortOrder(SortOrder sortOrder);
  const CFileItemList& m_items;

  static VECSOURCES m_sources;

  int m_currentViewAsControl;
  int m_playlist;

  std::vector<SORT_METHOD_DETAILS> m_sortMethods;
  int m_currentSortMethod;

  SortOrder m_sortOrder;

  static CStdString m_strPlaylistDirectory;
};

class CGUIViewStateGeneral : public CGUIViewState
{
public:
  CGUIViewStateGeneral(const CFileItemList& items);

protected:
  virtual void SaveViewState() {};
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
