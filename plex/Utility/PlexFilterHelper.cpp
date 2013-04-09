//
//  PlexFilterHelper.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-05.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "PlexFilterHelper.h"
#include "guilib/GUIControlGroupList.h"
#include "GUI/GUIDialogFilterSort.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISpinControlEx.h"
#include "plex/PlexUtils.h"
#include "plex/FileSystem/PlexDirectory.h"
#include "GUIUserMessages.h"
#include "AdvancedSettings.h"
#include "guilib/GUILabelControl.h"
#include "GUI/GUIDialogFilterSort.h"
#include "GUIWindowManager.h"

using namespace XFILE;

typedef pair<CStdString, CPlexFilterPtr> name_filter_pair;

bool CPlexFilterHelper::ApplySort(int ctrlId)
{
  m_appliedSort.clear();

  BOOST_FOREACH(name_filter_pair pr, m_sorts)
  {
    if (pr.second->GetControlID() == ctrlId)
    {
      m_appliedSort = pr.second->GetFilterName();
      m_sortDirectionAsc = ((CGUIRadioButtonControl*)pr.second->GetFilterControl())->IsSelected();
      return true;
    }
  }
  return false;
}

void CPlexFilterHelper::ApplyFilterFromDialog(CPlexFilterPtr filter)
{
  if (m_appliedFilters.find(filter->GetFilterName()) != m_appliedFilters.end())
    m_appliedFilters.erase(filter->GetFilterName());

  if (!filter->GetFilterValue().empty())
    m_appliedFilters[filter->GetFilterName()] = filter->GetFilterValue();

  BOOST_FOREACH(name_filter_pair p, m_filters)
    p.second->SetFilterUrl(GetFilterUrl(p.second->GetFilterName()));
}

bool CPlexFilterHelper::ApplyFilter(int ctrlId)
{
  bool update = false;
  BOOST_FOREACH(name_filter_pair pr, m_filters)
  {
    if (pr.second->GetControlID() == ctrlId)
    {
      if (pr.second->IsBooleanType())
      {
        /* Clear this filter first */
        if (m_appliedFilters.find(pr.second->GetFilterName()) != m_appliedFilters.end())
          m_appliedFilters.erase(pr.second->GetFilterName());

        if (!pr.second->GetFilterValue().empty())
          m_appliedFilters[pr.second->GetFilterName()] = pr.second->GetFilterValue();

        update = true;
        break;
      }
      else
      {
        CGUIDialogFilterSort *dialog = (CGUIDialogFilterSort *)g_windowManager.GetWindow(WINDOW_DIALOG_FILTER_SORT);
        if (!dialog)
          break;
        dialog->SetFilter(pr.second);
        dialog->SetFilterHelper(this);
        dialog->DoModal();
      }
    }
  }
  return update;
}

bool CPlexFilterHelper::FetchFilterSortList(const CStdString& url, const CStdString& filterSort, int type, CFileItemList& list)
{
  CStdString filterUrl;
  CStdString typeStr;
  typeStr.Format("?type=%d", type);

  filterUrl = PlexUtils::AppendPathToURL(url, filterSort);
  filterUrl = PlexUtils::AppendPathToURL(filterUrl, typeStr);

  CPlexDirectory fdir;
  return fdir.GetDirectory(filterUrl, list);
}

void CPlexFilterHelper::BuildFilters(const CStdString& baseUrl, EPlexDirectoryType type)
{
  CGUIControlGroupList *filterGroup = (CGUIControlGroupList*)m_mediaWindow->GetControl(FILTER_LIST);
  CGUIControlGroupList *sortGroup = (CGUIControlGroupList*)m_mediaWindow->GetControl(SORT_LIST);

  if (!filterGroup || !sortGroup)
    return;

  filterGroup->ClearAll();
  sortGroup->ClearAll();

  CGUIButtonControl* originalButton = (CGUIButtonControl*)m_mediaWindow->GetControl(FILTER_BUTTON);
  if (!originalButton)
    return;
  originalButton->SetVisible(false);

  CGUIRadioButtonControl* radioButton = (CGUIRadioButtonControl*)m_mediaWindow->GetControl(FILTER_RADIO_BUTTON);
  if (!radioButton)
    return;
  radioButton->SetVisible(false);

  CGUIRadioButtonControl* sortButton = (CGUIRadioButtonControl*)m_mediaWindow->GetControl(SORT_RADIO_BUTTON);
  if (!sortButton)
    return;
  sortButton->SetVisible(false);

  /* Fetch Filters */
  CFileItemList filterItems;
  FetchFilterSortList(baseUrl, "filters", type, filterItems);
  std::map<CStdString, CPlexFilterPtr> newFilters;

  for(int i = 0; i < filterItems.Size(); i++)
  {
    CFileItemPtr item = filterItems.Get(i);
    CPlexFilterPtr filter;
    CStdString filterName = item->GetProperty("filter").asString();
    if (m_filters.find(filterName) == m_filters.end())
      /* No such filter yet */
      filter = CPlexFilterPtr(new CPlexFilter(item->GetLabel(), filterName, item->GetProperty("filterType").asString(), item->GetProperty("key").asString()));
    else
      filter = m_filters[filterName];

    newFilters[filterName] = filter;
    filterGroup->AddControl(filter->NewFilterControl(filter->IsBooleanType() ? radioButton : originalButton, FILTER_BUTTONS_START + i));
    CLog::Log(LOGDEBUG, "CPlexFilterHelper::BuildFilters adding %s to filterGroup", filter->GetFilterName().c_str());
  }

  m_filters = newFilters;

  /* Fetch sorts */
  newFilters.clear();
  CFileItemList sortItems;
  FetchFilterSortList(baseUrl, "sorts", type, sortItems);

  for(int i = 0; i < sortItems.Size(); i++)
  {
    CFileItemPtr item = sortItems.Get(i);
    CPlexFilterPtr filter;
    CStdString sortName = item->GetProperty("unprocessedKey").asString();
    if (m_sorts.find(sortName) == m_sorts.end())
      /* No such filter yet */
      filter = CPlexFilterPtr(new CPlexFilter(item->GetLabel(), sortName, "", sortName));
    else
      filter = m_sorts[sortName];

    newFilters[sortName] = filter;
    sortGroup->AddControl(filter->NewFilterControl(sortButton, SORT_BUTTONS_START + i));
  }

  m_sorts = newFilters;
}

CStdString CPlexFilterHelper::GetFilterUrl(const CStdString& exclude, const CStdString& baseUrl) const
{
  CStdString url(baseUrl);

  if (m_appliedFilters.size() > 0)
  {
    vector<string> filterValues;
    pair<CStdString, string> stpair;
    BOOST_FOREACH(stpair, m_appliedFilters)
    {
      if (!stpair.first.Equals(exclude))
        filterValues.push_back(stpair.second);
    }

    CStdString optionList = StringUtils::Join(filterValues, "&");
    if (url.Find('?') == -1)
      url += "?" + optionList;
    else
      url += "&" + optionList;
  }
  return url;
}

void CPlexFilterHelper::ClearFilters()
{
  m_appliedFilters.clear();
  m_filters.clear();
  m_sorts.clear();
}

CStdString CPlexFilterHelper::GetRealDirectoryUrl(const CStdString& url_, bool& secondary)
{
  CFileItemList tmpItems;
  CPlexDirectory dir;
  CStdString strDirectory(url_);

  secondary = false;

  if (m_mapToSection == strDirectory)
    strDirectory = m_sectionUrl;

  CStdString containerUrl(strDirectory);

  /* we just request the header to see if this is a "secondary" list */
  CStdString offset ="X-Plex-Container-Start=0&X-Plex-Container-Size=1";
  if (containerUrl.Find('?') == -1)
    containerUrl += "?" + offset;
  else
    containerUrl += "&" + offset;
  tmpItems.SetPath(containerUrl);

  /* A bit stupidity here, but we need to request the container twice. Fortunately it's really fast
   * to do it with the offset stuff above */
  if (dir.GetDirectory(containerUrl, tmpItems))
  {
    if (tmpItems.IsPlexMediaServer() && tmpItems.GetContent() == "secondary")
    {
      if (m_sectionUrl != strDirectory)
      {
        m_sectionUrl = strDirectory;
        ClearFilters();
      }

      CStdString url;
      if (tmpItems.GetProperty("HomeVideoSection").asBoolean())
        url = PlexUtils::AppendPathToURL(strDirectory, "folder");
      else
        url = PlexUtils::AppendPathToURL(strDirectory, "all");

      url = GetFilterUrl("", url);

      if (!m_appliedSort.empty())
      {
        CStdString sortStr;
        sortStr.Format("sort=%s:%s", m_appliedSort, m_sortDirectionAsc ? "asc" : "desc");

        if (url.Find('?') != -1)
          url += "&" + sortStr;
        else
          url += "?" + sortStr;
      }

      /* Save this */
      m_mapToSection = url;

      secondary = true;
      return url;
    }
    else if (tmpItems.IsPlexMediaServer() && (tmpItems.GetContent() == "seasons" || tmpItems.GetContent() == "albums"))
    {
      if (tmpItems.GetProperty("totalSize").asInteger() == 1 && g_advancedSettings.m_bCollapseSingleSeason)
      {
        CFileItemPtr season = tmpItems.Get(0);
        CStdString url = season->GetPath();
        return url;
      }
    }
  }
  return strDirectory;
}
