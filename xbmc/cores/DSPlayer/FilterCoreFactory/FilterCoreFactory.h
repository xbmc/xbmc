#pragma once

#include "FilterSelectionRule.h"
#include "tinyXML\tinyxml.h"
#include "globalfilterselectionrule.h"
#include "GUIDialogOK.h"
#include "GUIWindowManager.h"

class CFilterCoreFactory
{
public:
  static std::vector<CFGFilterFile *> m_Filters;

  static bool LoadConfiguration(TiXmlElement* pConfig, bool clear);
  static bool GetSourceFilter(const CFileItem& pFileItem, CStdString& filter);
  static bool GetSplitterFilter(const CFileItem& pFileItem, CStdString& filter);
  static bool GetAudioRendererFilter(const CFileItem& pFileItem, CStdString& filter, SStreamInfos* s = NULL);
  static bool GetVideoFilter(const CFileItem& pFileItem, CStdString& filter, bool dxva = false);
  static bool GetAudioFilter(const CFileItem& pFileItem, CStdString& filter, bool dxva = false, SStreamInfos* s = NULL);
  static bool GetExtraFilters(const CFileItem& pFileItem, std::vector<CStdString>& filters, bool dxva = false);

  static CFGFilterFile* GetFilterFromName(const CStdString& filter, bool showError = true);

  static bool SomethingMatch( const CFileItem& pFileItem)
  {
    return (GetGlobalFilterSelectionRule(pFileItem) != NULL);
  }

  static void Destroy()
  {
    while(!m_Filters.empty())
    {
      if (m_Filters.back())
        delete m_Filters.back();
      m_Filters.pop_back();
    }

    while(!m_selecRules.empty())
    {
      if (m_selecRules.back())
        delete m_selecRules.back();
      m_selecRules.pop_back();
    }

    CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__);
  }

private:
  static bool CompareCFGFilterFileToString(CFGFilterFile * f, CStdString s)
  {
    return f->GetInternalName().Equals(s);
  }
  static CGlobalFilterSelectionRule* GetGlobalFilterSelectionRule(const CFileItem& pFileItem);

  static std::vector<CGlobalFilterSelectionRule *> m_selecRules;
};
