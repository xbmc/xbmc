//
//  GUIWindowMediaFilterView.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-11-19.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "GUIWindowMediaFilterView.h"
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

bool CGUIWindowMediaFilterView::FetchFilterSortList(const CStdString& url, const CStdString& filterSort, int type, CFileItemList& list)
{
  CStdString filterUrl;
  CStdString typeStr;
  typeStr.Format("?type=%d", type);

  filterUrl = PlexUtils::AppendPathToURL(url, filterSort);
  filterUrl = PlexUtils::AppendPathToURL(filterUrl, typeStr);

  CPlexDirectory fdir(true, false);
  return fdir.GetDirectory(filterUrl, list);
}

void CGUIWindowMediaFilterView::BuildFilters(const CStdString& baseUrl, int type)
{
  CGUIControlGroupList *filterGroup = (CGUIControlGroupList*)GetControl(FILTER_LIST);
  CGUIControlGroupList *sortGroup = (CGUIControlGroupList*)GetControl(SORT_LIST);

  if (!filterGroup || !sortGroup)
    return;

  filterGroup->ClearAll();
  sortGroup->ClearAll();

  CGUIButtonControl* originalButton = (CGUIButtonControl*)GetControl(FILTER_BUTTON);
  if (!originalButton)
    return;
  originalButton->SetVisible(false);

  CGUIRadioButtonControl* radioButton = (CGUIRadioButtonControl*)GetControl(FILTER_RADIO_BUTTON);
  if (!radioButton)
    return;
  radioButton->SetVisible(false);

  CGUIRadioButtonControl* sortButton = (CGUIRadioButtonControl*)GetControl(SORT_RADIO_BUTTON);
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
    dprintf("FILTER: adding %s to filterGroup", filter->GetFilterName().c_str());
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

typedef pair<CStdString, CPlexFilterPtr> name_filter_pair;

bool CGUIWindowMediaFilterView::OnMessage(CGUIMessage &message)
{
  bool ret = CGUIWindowVideoNav::OnMessage(message);

  switch(message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      bool update = false;
      int ctrlId = message.GetSenderId();
      dprintf("Clicked with CtrlID = %d", ctrlId);
      if (ctrlId < 0 && ctrlId >= FILTER_BUTTONS_START)
      {
        BOOST_FOREACH(name_filter_pair pr, m_filters)
        {
          if (pr.second->GetControlID() == ctrlId)
          {
            dprintf("Clicked filter %s", pr.second->GetFilterName().c_str());

            if (pr.second->IsBooleanType())
            {
              /* Clear this filter first */
              if (m_appliedFilters.find(pr.second->GetFilterName()) != m_appliedFilters.end())
                m_appliedFilters.erase(pr.second->GetFilterName());

              if (!pr.second->GetFilterValue().empty())
                m_appliedFilters[pr.second->GetFilterName()] = pr.second->GetFilterValue();

              update = true;
            }
            else
            {
              CGUIDialogFilterSort *dialog = (CGUIDialogFilterSort *)g_windowManager.GetWindow(WINDOW_DIALOG_FILTER_SORT);
              if (!dialog)
                break;
              dialog->SetFilter(pr.second);
              m_openFilter = pr.second;
              dialog->DoModal();

              break;
            }
          }
        }
      }
      else if (ctrlId < FILTER_BUTTONS_START && ctrlId >= SORT_BUTTONS_START)
      {
        m_appliedSort.clear();

        BOOST_FOREACH(name_filter_pair pr, m_sorts)
        {
          if (pr.second->GetControlID() == ctrlId)
          {
            m_appliedSort = pr.second->GetFilterName();
            m_sortDirectionAsc = ((CGUIRadioButtonControl*)pr.second->GetFilterControl())->IsSelected();
            update = true;
          }
        }
      }

      if (update)
        Update(m_baseUrl, true, false);
    }
      break;

    case GUI_MSG_LOAD_SKIN:
    {
      /* This is called BEFORE the skin is reloaded, so let's save this event to be handled
       * in WINDOW_INIT instead */
      if (IsActive())
        m_returningFromSkinLoad = true;
    }
      break;

    case GUI_MSG_WINDOW_INIT:
    {
      /* If this is a reload event we must make sure to get the filters back */
      if (m_returningFromSkinLoad)
        Update(m_baseUrl, true);
      m_returningFromSkinLoad = false;

      CGUILabelControl *lbl = (CGUILabelControl*)GetControl(FILTER_LABEL);
      if (lbl)
        lbl->SetLabel(g_localizeStrings.Get(44030));

      lbl = (CGUILabelControl*)GetControl(SORT_LABEL);
      if (lbl)
        lbl->SetLabel(g_localizeStrings.Get(44031));
    }
      break;

    case GUI_MSG_UPDATE_FILTERS:
    {
      if (m_openFilter)
      {
        if (m_appliedFilters.find(m_openFilter->GetFilterName()) != m_appliedFilters.end())
          m_appliedFilters.erase(m_openFilter->GetFilterName());

        if (!m_openFilter->GetFilterValue().empty())
          m_appliedFilters[m_openFilter->GetFilterName()] = m_openFilter->GetFilterValue();
      }

      /* This sucks, but "ok" */
      BOOST_FOREACH(name_filter_pair p, m_filters)
        p.second->SetFilterUrl(GetFilterUrl(p.second->GetFilterName()));

      Update(m_baseUrl, true, false);
    }
      break;
  }

  return ret;
}

bool CGUIWindowMediaFilterView::Update(const CStdString &strDirectory, bool updateFilterPath)
{
  return Update(strDirectory, updateFilterPath, true);
}

CStdString CGUIWindowMediaFilterView::GetFilterUrl(const CStdString& exclude, const CStdString& baseUrl) const
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

bool CGUIWindowMediaFilterView::Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFilters)
{
  bool ret;
  CFileItemList tmpItems;
  CPlexDirectory dir(true, true);

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
      if (!m_baseUrl.Equals(strDirectory))
      {
        m_baseUrl = strDirectory;
        m_appliedFilters.clear();
        m_filters.clear();
        m_sorts.clear();
      }

      CStdString url;
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


      ret = CGUIWindowVideoNav::Update(url, updateFilterPath);

      /* Kill the history */
      m_history.ClearPathHistory();
      m_startDirectory = url;

      if (ret && updateFilters)
      {
        int type = 1;
        if (m_vecItems->HasProperty("typeNumber"))
          type = m_vecItems->GetProperty("typeNumber").asInteger();
        BuildFilters(strDirectory, type);
      }

      return ret;
    }
    else if (tmpItems.IsPlexMediaServer() && tmpItems.GetContent() == "seasons")
    {
      m_history.AddPath(m_baseUrl);
      if (tmpItems.GetProperty("totalSize").asInteger() == 1 && g_advancedSettings.m_bCollapseSingleSeason)
      {
        CFileItemPtr season = tmpItems.Get(0);
        CStdString url = season->GetPath();
        return CGUIWindowVideoNav::Update(url, updateFilterPath);
      }
    }
  }

  return CGUIWindowVideoNav::Update(strDirectory, updateFilterPath);
}
