//
//  PlexFilter.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-11-26.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXFILTER_H
#define PLEXFILTER_H

#include "StdString.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "FileSystem/PlexDirectory.h"

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

class CPlexFilter
{
  public:
    CPlexFilter(CFileItemPtr filterItem) : m_filterItem(filterItem)
    {
      m_filterControl = NULL;
    }

    CGUIButtonControl* NewFilterControl(CGUIButtonControl *parent, int id)
    {
      if (IsBooleanType() || !m_filterItem->HasProperty("filterType"))
        m_filterControl = new CGUIRadioButtonControl(*(CGUIRadioButtonControl*)parent);
      else
      {
        m_filterControl = new CGUIButtonControl(*parent);
      }

      m_filterControl->SetLabel(m_filterItem->GetLabel());
      m_filterControl->AllocResources();
      m_filterControl->SetID(id);
      m_filterControl->SetVisible(true);

      return m_filterControl;
    }

    CGUIButtonControl* GetFilterControl() const { return m_filterControl; }

    bool IsStringType() const { return m_filterItem->GetProperty("filterType").asString() == "string"; }
    bool IsIntegerType() const { return m_filterItem->GetProperty("filterType").asString() == "integer"; }
    bool IsBooleanType() const { return m_filterItem->GetProperty("filterType").asString() == "boolean"; }
    bool IsActive() const { return !m_currentValue.empty(); }

    CStdString GetFilterName() const
    {
      if (m_filterItem->HasProperty("filter"))
        return m_filterItem->GetProperty("filter").asString();
      else
        return m_filterItem->GetProperty("unprocessed_key").asString();
    }
    CStdString GetFilterTitle() const { return m_filterItem->GetLabel(); }

    int GetControlID() const
    {
      if (m_filterControl)
        return m_filterControl->GetID();

      return 0;
    }

    CStdString GetFilterValue() const
    {
      CStdString filterValue;
      if (IsBooleanType())
      {
        CGUIRadioButtonControl* radio = (CGUIRadioButtonControl*)m_filterControl;
        if (radio->IsSelected())
          filterValue = "1";
      }
      else
      {
        if (m_currentValue.empty())
          return "";
        filterValue = StringUtils::Join(m_currentValue, ",");
      }

      return filterValue;
    }

    bool GetSublist(CFileItemList& sublist)
    {
      /* FIXME: needs to be made ASync? */
      if (m_sublist.Size() == 0)
        if (!FetchSublist())
          return false;

      sublist.Copy(m_sublist);
      return true;
    }

    void UpdateLabel()
    {
      CStdString newLabel(GetFilterTitle());
      if (m_currentValue.size() > 0)
      {
        newLabel = "[B]" + GetFilterTitle();

        CStdString primaryKey = m_currentValue[0];
        CFileItemList list;
        if (GetSublist(list))
        {
          for (int i = 0; i < list.Size(); i ++)
          {
            CFileItemPtr item = list.Get(i);
            if (item->GetProperty("unprocessed_key").asString() == primaryKey)
            {
              newLabel += ": " + item->GetLabel();
              break;
            }
          }
          if (m_currentValue.size() > 1)
          {
            CStdString plusStr;
            plusStr.Format("(+%d)", m_currentValue.size() - 1);
            newLabel += " " + plusStr;
          }
        }
        newLabel += "[/B]";
      }
      m_filterControl->SetLabel(newLabel);
    }

    void AddCurrentValue(const CStdString& value)
    {
      m_currentValue.push_back(value);
      UpdateLabel();
    }

    void ClearCurrentValue()
    {
      m_currentValue.clear();
      UpdateLabel();
    }

    void RemoveCurrentValue(const CStdString& value)
    {
      m_currentValue.erase(std::remove(m_currentValue.begin(), m_currentValue.end(), value), m_currentValue.end());
      UpdateLabel();
    }

    void SetAppliedFilters(const std::map<CStdString, CStdString> &filters)
    {
      m_appliedFilters = filters;
      m_sublist.Clear();
    }

    bool HasCurrentValue(const CStdString& valueKey)
    {
      bool found = false;
      BOOST_FOREACH(std::string& value, m_currentValue)
      {
        if (value.compare(valueKey) == 0)
        {
          found = true;
          break;
        }
      }
      return found;
    }

  private:
    bool FetchSublist()
    {
      XFILE::CPlexDirectory dir;
      CURL url(m_filterItem->GetProperty("key").asString());

      std::pair<CStdString, CStdString> f;
      BOOST_FOREACH(f, m_appliedFilters)
        url.SetOption(f.first, f.second);
      
      return dir.GetDirectory(url, m_sublist);
    }

    CGUIButtonControl* m_filterControl;
    std::vector<std::string> m_currentValue;
    CFileItemPtr m_filterItem;
    std::map<CStdString, CStdString> m_appliedFilters;

    CFileItemList m_sublist;
};

typedef boost::shared_ptr<CPlexFilter> CPlexFilterPtr;


#endif // PLEXFILTER_H
