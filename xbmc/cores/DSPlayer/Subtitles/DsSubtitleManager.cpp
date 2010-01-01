

#include "DsSubtitleManager.h"
#include "WindowingFactory.h" //for d3d device
#include "CharsetConverter.h" //g_charsetconverter
#include "GraphicContext.h" //g_graphicsContext
#include <list>

CDsSubManager g_dllMpcSubs;

using namespace std;

CDsSubManager::CDsSubManager()
{
  m_bCurrentlyEnabled=false;
}
bool CDsSubManager::Load()
{
  bool loaded=m_dllMpcSubs.IsLoaded();
  if (!loaded)
    loaded=m_dllMpcSubs.Load();
  return loaded;
}

bool CDsSubManager::LoadSubtitles(const char* fn, IGraphBuilder* pGB, const char* paths)
{
  SIZE tmpSize;
  tmpSize.cx = g_graphicsContext.GetWidth();
  tmpSize.cy = g_graphicsContext.GetHeight();
  InitDefaultStyle();
  
  CStdStringW pFilePath,pSubPath;
  g_charsetConverter.subtitleCharsetToW(CStdString(fn),pFilePath);
  g_charsetConverter.subtitleCharsetToW(CStdString(paths),pSubPath);
  
  return m_dllMpcSubs.LoadSubtitles(g_Windowing.Get3DDevice(),tmpSize,pFilePath.c_str(),pGB,L".\\,.\\Subtitles\\");//pSubPath.c_str());
}

void CDsSubManager::EnableSubtitle(bool enable)
{
  m_bCurrentlyEnabled = false;
  if (m_dllMpcSubs.IsLoaded())
  {
    m_dllMpcSubs.SetEnable(enable);
    m_bCurrentlyEnabled = enable;
  }
  
}

void CDsSubManager::InitDefaultStyle()
{
  //m_pCurrentStyle = NULL;
  m_pCurrentStyle.borderWidth = 2;
  m_pCurrentStyle.fontCharset = 1;
  m_pCurrentStyle.fontColor = 16777215;
  m_pCurrentStyle.fontIsBold = true;
  m_pCurrentStyle.fontName = L"Arial";
  m_pCurrentStyle.fontSize = 18;
  m_pCurrentStyle.isBorderOutline = true;
  m_pCurrentStyle.shadow = 3;
  m_dllMpcSubs.SetDefaultStyle(&m_pCurrentStyle,false);
  //m_dllMpcSubs
}

void CDsSubManager::GetSubtitlesList()
{
  
  int count = m_dllMpcSubs.GetCount();
  
  CStdString strLangA;
  
  for (int xx =1 ; xx < count ; xx++)
  {
    g_charsetConverter.wToUTF8(CStdStringW(m_dllMpcSubs.GetLanguage(xx)),strLangA);
    CLog::Log(LOGNOTICE,"%s",strLangA.c_str()); 
  }
  
}

void CDsSubManager::Render(int x, int y, int width, int height)
{
  m_dllMpcSubs.Render(x,y,width,height);
}

