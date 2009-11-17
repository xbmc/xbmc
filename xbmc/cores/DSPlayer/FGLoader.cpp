#include "FGLoader.h"
#include "dshowutil/dshowutil.h"
#include "tinyXML/tinyxml.h"
#include "XMLUtils.h"
#include "charsetconverter.h"
#include "Log.h"
using namespace std;
CFGLoader::CFGLoader(IGraphBuilder2* gb,CStdString xbmcPath)
:m_pGraphBuilder(gb)
,m_xbmcPath(xbmcPath)
{
}

CFGLoader::~CFGLoader()
{
}

HRESULT CFGLoader::LoadConfig(CStdString configFile)
{
  HRESULT hr = S_OK;
  m_xbmcConfigFilePath = configFile;
  m_xbmcPath.Replace("\\","\\\\");
  if (!CFile::Exists(configFile))
    return false;
  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(configFile))
    return false;
  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return false;
  CStdString strFPath;
  CStdString strFGuid;
  CStdString strFileType;
  CStdString strTrimedPath;
  CStdString strTmpFilterName;
  CStdString strTmpFilterType;
  CFGFilterFile* pFGF;
  TiXmlElement *pFilters = graphConfigRoot->FirstChildElement("filters");
  pFilters = pFilters->FirstChildElement("filter");
  while (pFilters)
  {
    strTmpFilterName = pFilters->Attribute("name");
    strTmpFilterType = pFilters->Attribute("type");
    XMLUtils::GetString(pFilters,"path",strFPath);
    if (!CFile::Exists(strFPath))
	{
      strFPath.Format("%s\\\\system\\\\players\\\\dsplayer\\\\%s",m_xbmcPath.c_str(),strFPath.c_str());
	}
	XMLUtils::GetString(pFilters,"guid",strFGuid);
    XMLUtils::GetString(pFilters,"filetype",strFileType);
    pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,L"",MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
    if (strTmpFilterName.Equals("mpcvideodec",false))
      m_mpcVideoDecGuid = DShowUtil::GUIDFromCString(strFGuid);

  if (pFGF)
    m_configFilter.AddTail(pFGF);

  pFilters = pFilters->NextSiblingElement();
  
  }
}

HRESULT CFGLoader::LoadFilterRules(CStdString fileType , CComPtr<IBaseFilter> fileSource ,IBaseFilter **pBFSplitter)
{
  HRESULT hr;
  CheckPointer(pBFSplitter,E_POINTER);
  IBaseFilter *ppSBF = NULL;
  //Load the rules from the xml
  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(m_xbmcConfigFilePath))
    return false;
  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return false;
  CInterfaceList<IUnknown, &IID_IUnknown> pUnk;
  TiXmlElement *pRules = graphConfigRoot->FirstChildElement("rules");
  pRules = pRules->FirstChildElement("rule");
  while (pRules)
  {
    if (((CStdString)pRules->Attribute("filetypes")).Equals(fileType.c_str(),false))
    {
      POSITION pos = m_configFilter.GetHeadPosition();
      while(pos)
      {
        CFGFilterFile* pFGF = m_configFilter.GetNext(pos);
        if ( ((CStdString)pRules->Attribute("splitter")).Equals(pFGF->GetXFilterName().c_str(),false) )
        {
          if(SUCCEEDED(pFGF->Create(&ppSBF, pUnk)))
          {
            m_pGraphBuilder->AddFilter(ppSBF,DShowUtil::AnsiToUTF16(pFGF->GetXFilterName().c_str()).c_str());//L"XBMC SPLITTER");
            hr = ::ConnectFilters(m_pGraphBuilder,fileSource,ppSBF);
            if ( SUCCEEDED( hr ) )
              //Now the source filter is connected to the splitter lets load the rules from the xml
              CLog::Log(LOGDEBUG,"%s Connected the source to the spillter",__FUNCTION__);
            else
            {
              CLog::Log(LOGERROR,"%s Failed to connect the source to the spliter",__FUNCTION__);
              return hr;
			}
		  }
		}
        else if ( ((CStdString)pRules->Attribute("videodec")).Equals(pFGF->GetXFilterName().c_str(),false) )
        {
        CComPtr<IBaseFilter> ppBF;
        if(SUCCEEDED(pFGF->Create(&ppBF, pUnk)))
          m_pGraphBuilder->AddFilter(ppBF,DShowUtil::AnsiToUTF16(pFGF->GetXFilterName().c_str()).c_str());
        ppBF.Release();
      }
      else if ( ((CStdString)pRules->Attribute("audiodec")).Equals(pFGF->GetXFilterName().c_str(),false) )
      {
        CComPtr<IBaseFilter> ppBF;
        if(SUCCEEDED(pFGF->Create(&ppBF, pUnk)))
        {
          m_pGraphBuilder->AddFilter(ppBF,DShowUtil::AnsiToUTF16(pFGF->GetXFilterName().c_str()).c_str());
        }
        ppBF.Release();
      }
    }
  }
    pRules = pRules->NextSiblingElement();
  }
  *pBFSplitter = ppSBF;
  (*pBFSplitter)->AddRef();
  SAFE_RELEASE(ppSBF);
  return hr;
}