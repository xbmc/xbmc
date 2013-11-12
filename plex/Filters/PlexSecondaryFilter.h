#ifndef PLEXPRIMARYFILTER_H
#define PLEXPRIMARYFILTER_H

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

#include "FileItem.h"
#include "Variant.h"
#include "JobManager.h"
#include "UrlOptions.h"

#include <boost/foreach.hpp>

class CPlexSecondaryFilter;
typedef boost::shared_ptr<CPlexSecondaryFilter> CPlexSecondaryFilterPtr;

class CPlexSecondaryFilter : public IJobCallback
{
  public:
    enum SecondaryFilterType {
      FILTER_TYPE_UNKNOWN,
      FILTER_TYPE_INTEGER,
      FILTER_TYPE_STRING,
      FILTER_TYPE_BOOLEAN
    };

    CPlexSecondaryFilter(const std::string &name, const std::string &key, const std::string &title, SecondaryFilterType type);
    static CPlexSecondaryFilterPtr secondaryFilterFromItem(CFileItemPtr item);
    static SecondaryFilterType getFilterTypeFromString(const std::string& typeString);

    std::string getFilterTitle() const { return m_title; }
    std::string getFilterName() const { return m_name; }
    std::string getFilterKey() const { return m_filterKey; }

    std::string getCurrentValue() const;
    std::string getCurrentValueLabel() const;
    std::pair<std::string, std::string> getFilterKeyValue() const;

    SecondaryFilterType getFilterType() const { return m_type; }

    /* for bool values */
    void setSelected(bool selected);
    /* for string and integer values */
    void setSelected(const std::string& value, bool selected);

    /* all values packed with , */
    void setSelected(const std::string& values);

    /* works for all types */
    bool isSelected() const
    {
      if (m_type == FILTER_TYPE_BOOLEAN)
        return m_booleanValue;
      else
        return m_selectedValues.size() > 0;
    }

    bool isSelected(const std::string& value)
    {
      if (m_type == FILTER_TYPE_BOOLEAN)
        return isSelected();

      return std::find(m_selectedValues.begin(), m_selectedValues.end(), value) != m_selectedValues.end();
    }

    bool hasValues() const
    {
      if (m_type == FILTER_TYPE_BOOLEAN) return true;
      return m_filterValues.size() > 0;
    }

    PlexStringPairVector getFilterValues() const
    {
      return m_filterValues;
    }

    void loadValues(const CUrlOptions &options = CUrlOptions());
    void clearFilters()
    {
      if (m_type == FILTER_TYPE_BOOLEAN)
        m_booleanValue = false;
      else
        m_selectedValues.clear();
    }

  private:
    void OnJobComplete(unsigned int jobID, bool success, CJob *job);

    std::string m_name;
    std::string m_filterKey;
    std::string m_title;
    SecondaryFilterType m_type;

    /* selected values contains the selected values for
     * INTEGER and STRING type filters */
    PlexStringVector m_selectedValues;

    /* And this holds the value for a BOOLEAN type filter */
    bool m_booleanValue;

    /* All available values */
    PlexStringPairVector m_filterValues;
    std::string getValueLabel(const std::string &value) const;
};

#endif // PLEXPRIMARYFILTER_H
