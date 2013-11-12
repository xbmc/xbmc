#include "PlexSecondaryFilter.h"
#include "Variant.h"
#include <boost/foreach.hpp>
#include "StringUtils.h"
#include "JobManager.h"
#include "PlexJobs.h"

#include "Key.h"
#include "guilib/GUIWindowManager.h"

#include <sstream>

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexSecondaryFilter::CPlexSecondaryFilter(const std::string &name,
                                           const std::string &key,
                                           const std::string &title,
                                           CPlexSecondaryFilter::SecondaryFilterType type)
  : m_name(name), m_filterKey(key), m_title(title), m_type(type), m_booleanValue(false)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexSecondaryFilterPtr CPlexSecondaryFilter::secondaryFilterFromItem(CFileItemPtr item)
{
  if (!item)
    return CPlexSecondaryFilterPtr();

  CPlexSecondaryFilterPtr secondaryFilter
      = CPlexSecondaryFilterPtr(new CPlexSecondaryFilter(item->GetProperty("filter").asString(),
                                                         item->GetProperty("key").asString(),
                                                         item->GetLabel(),
                                                         getFilterTypeFromString(item->GetProperty("filterType").asString())));

  return secondaryFilter;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexSecondaryFilter::SecondaryFilterType CPlexSecondaryFilter::getFilterTypeFromString(const std::string &typeString)
{
  if (typeString == "integer")
    return FILTER_TYPE_INTEGER;
  else if (typeString == "string")
    return FILTER_TYPE_STRING;
  else if (typeString == "boolean")
    return FILTER_TYPE_BOOLEAN;

  return FILTER_TYPE_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string CPlexSecondaryFilter::getCurrentValue() const
{
  if (m_type == FILTER_TYPE_INTEGER ||
      m_type == FILTER_TYPE_STRING)
    return StringUtils::Join(m_selectedValues, ",");
  else if (m_type == FILTER_TYPE_BOOLEAN)
    return m_booleanValue ? "1" : "0";
  return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string CPlexSecondaryFilter::getValueLabel(const std::string &value) const
{
  BOOST_FOREACH(PlexStringPair p, m_filterValues)
  {
    if (p.first == value)
      return p.second;
  }

  return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string CPlexSecondaryFilter::getCurrentValueLabel() const
{
  if (m_type == FILTER_TYPE_BOOLEAN)
    return "";

  if (m_selectedValues.size() == 1)
    return getValueLabel(m_selectedValues.at(0));

  std::stringstream ss;
  ss << getValueLabel(m_selectedValues.at(0));
  ss << " (+" << m_selectedValues.size() - 1 << ")";
  return ss.str();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::pair<std::string, std::string> CPlexSecondaryFilter::getFilterKeyValue() const
{
  std::pair<std::string, std::string> pair;

  pair.first = m_name;
  pair.second = getCurrentValue();

  return pair;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexSecondaryFilter::setSelected(bool selected)
{
  if (m_type != FILTER_TYPE_BOOLEAN)
    return;

  m_booleanValue = selected;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexSecondaryFilter::setSelected(const std::string& value, bool selected)
{
  if (m_type == FILTER_TYPE_BOOLEAN)
    return;

  bool hasValue = std::find(m_selectedValues.begin(), m_selectedValues.end(), value) != m_selectedValues.end();

  if (selected && !hasValue)
    m_selectedValues.push_back(value);
  else if (!selected && hasValue)
    m_selectedValues.erase(std::remove(m_selectedValues.begin(), m_selectedValues.end(), value), m_selectedValues.end());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexSecondaryFilter::setSelected(const std::string &values)
{
  std::vector<std::string> valVec = StringUtils::Split(values, ",");
  if (valVec.size() > 0)
  {
    m_selectedValues.clear();
    m_selectedValues = valVec;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexSecondaryFilter::loadValues(const CUrlOptions &options)
{
  m_filterValues.clear();
  CURL u(getFilterKey());
  u.AddOptions(options);

  u.SetOption("sort", "titleSort:asc");

  if (u.HasOption(getFilterName()))
    u.RemoveOption(getFilterName());

  CJobManager::GetInstance().AddJob(new CPlexDirectoryFetchJob(u), this, CJob::PRIORITY_NORMAL);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexSecondaryFilter::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexDirectoryFetchJob *fjob = static_cast<CPlexDirectoryFetchJob*>(job);
  if (fjob && success)
  {
    for (int i = 0; i < fjob->m_items.Size(); i ++)
    {
      CFileItemPtr item = fjob->m_items.Get(i);
      std::string key = item->GetProperty("unprocessed_key").asString();
      key = CURL::Decode(key);

      PlexStringPair pair = std::make_pair(key, item->GetProperty("title").asString());
      m_filterValues.push_back(pair);
    }

    CGUIMessage msg(GUI_MSG_FILTER_VALUES_LOADED, PLEX_FILTER_MANAGER, 0, 0, 0);
    msg.SetStringParam(getFilterKey());
    g_windowManager.SendThreadMessage(msg, g_windowManager.GetActiveWindow());
  }
}
