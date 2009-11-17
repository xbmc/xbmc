#pragma once
#include <atlcoll.h>
#include "streams.h"
#include "IGraphBuilder2.h"
#include "FgFilter.h"
class CFGLoader
{
public:
  CFGLoader(IGraphBuilder2* gb,CStdString xbmcPath);
  virtual ~CFGLoader();
  HRESULT LoadConfig(CStdString configFile);
  HRESULT LoadFilterRules(CStdString fileType,CComPtr<IBaseFilter> fileSource,IBaseFilter** pBFSplitter);
protected:
  CComPtr<IGraphBuilder2>  m_pGraphBuilder;
  CStdString               m_xbmcPath;
  CStdString               m_xbmcConfigFilePath;
  GUID                     m_mpcVideoDecGuid;
  CAtlList<CFGFilterFile*> m_configFilter;
};

