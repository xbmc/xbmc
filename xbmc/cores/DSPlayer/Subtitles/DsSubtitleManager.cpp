

#include "DsSubtitleManager.h"
#include "WindowingFactory.h" //for d3d device
#include "CharsetConverter.h" //g_charsetconverter

CDsSubManager g_dllMpcSubs;

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
  tmpSize.cx = 800;
  tmpSize.cy = 640;
  
  
  CStdStringW pFilePath,pSubPath;
  g_charsetConverter.subtitleCharsetToW(CStdString(fn),pFilePath);
  g_charsetConverter.subtitleCharsetToW(CStdString(paths),pSubPath);
  
  return m_dllMpcSubs.LoadSubtitles(g_Windowing.Get3DDevice(),tmpSize,pFilePath.c_str(),pGB,pSubPath.c_str());
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
void CDsSubManager::Render(int x, int y, int width, int height)
{
  int current = m_dllMpcSubs.GetCurrent();
  if (current > 0)
  {
    int count = m_dllMpcSubs.GetCount();
    
    m_dllMpcSubs.SetCurrent(count);
    m_dllMpcSubs.SetEnable(true);
  }
  m_dllMpcSubs.Render(x,y,width,height);
}


