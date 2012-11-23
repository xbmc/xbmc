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

void CGUIWindowMediaFilterView::PopulateSublist(CPlexFilterPtr filter)
{
  CFileItemList sublist;
  if (!filter->GetSublist(sublist))
    return;

  CGUIControlGroupList* list = (CGUIControlGroupList*)GetControl(FILTER_SUBLIST);
  if (!list)
    return;

  list->ClearAll();

  CGUIRadioButtonControl* radioButton = (CGUIRadioButtonControl*)GetControl(FILTER_SUBLIST_BUTTON);
  if (!radioButton)
    return;
  radioButton->SetVisible(false);

  CGUILabelControl* headerLabel = (CGUILabelControl*)GetControl(FILTER_SUBLIST_LABEL);
  if (headerLabel)
    headerLabel->SetLabel(filter->GetFilterString());

  for (int i = 0; i < sublist.Size(); i++)
  {
    CFileItemPtr item = sublist.Get(i);
    CGUIRadioButtonControl* sublistItem = new CGUIRadioButtonControl(*radioButton);
    sublistItem->SetLabel(item->GetLabel());
    sublistItem->SetVisible(true);
    sublistItem->AllocResources();
    sublistItem->SetID(FILTER_SUBLIST_BUTTONS_START + i);

    list->AddControl(sublistItem);
  }

  SET_CONTROL_VISIBLE(FILTER_SUBLIST);
}

void CGUIWindowMediaFilterView::OnInitWindow()
{
  CGUIWindowVideoNav::OnInitWindow();
  SET_CONTROL_HIDDEN(FILTER_SUBLIST);
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
              PopulateSublist(pr.second);
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
    }
      break;
  }

  return ret;
}

bool CGUIWindowMediaFilterView::Update(const CStdString &strDirectory, bool updateFilterPath)
{
  return Update(strDirectory, updateFilterPath, true);
}

bool CGUIWindowMediaFilterView::Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFilters)
{
  bool ret;
  CFileItemList tmpItems;
  CPlexDirectory dir(true, true);
  tmpItems.SetPath(strDirectory);

  /* Ok this is kind of wastefull */
  if (dir.GetDirectory(strDirectory, tmpItems))
  {
    if (tmpItems.IsPlexMediaServer() && tmpItems.GetContent() == "secondary")
    {
      CStdString url;
      url = PlexUtils::AppendPathToURL(strDirectory, "all");

      if (m_appliedFilters.size() > 0)
      {
        vector<string> filterValues;
        pair<CStdString, string> stpair;
        BOOST_FOREACH(stpair, m_appliedFilters)
          filterValues.push_back(stpair.second);

        CStdString optionList = StringUtils::Join(filterValues, "&");
        url += "?" + optionList;
      }

      if (!m_appliedSort.empty())
      {
        CStdString sortStr;
        sortStr.Format("sort=%s:%s", m_appliedSort, m_sortDirectionAsc ? "asc" : "desc");

        if (url.Find('?') != -1)
          url += "&" + sortStr;
        else
          url += "?" + sortStr;
      }

      m_baseUrl = strDirectory;

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
      if (tmpItems.Size() == 1 && g_advancedSettings.m_bCollapseSingleSeason)
      {
        CFileItemPtr season = tmpItems.Get(0);
        CStdString url = season->GetPath();
        return CGUIWindowVideoNav::Update(url, updateFilterPath);
      }
    }
  }

  return CGUIWindowVideoNav::Update(strDirectory, updateFilterPath);
}
