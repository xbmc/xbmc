#include "include.h"
#include "GUIRSSControl.h"
#include "GUIWindowManager.h"
#include "../xbmc/Settings.h"
#include "../xbmc/utils/CriticalSection.h"
#include "../xbmc/utils/SingleLock.h"


CGUIRSSControl::CGUIRSSControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, D3DCOLOR dwChannelColor, D3DCOLOR dwHeadlineColor, CStdString& strRSSTags)
: CGUIControl(dwParentID, dwControlId, posX, posY, width, height),
  m_scrollInfo(0,0,1.0f,"")
{
  m_label = labelInfo;
  m_dwChannelColor = dwChannelColor;
  m_dwHeadlineColor = dwHeadlineColor;

  m_strRSSTags = strRSSTags;

  m_pReader = NULL;
  m_pwzText = NULL;
  m_pbColors = NULL;
  ControlType = GUICONTROL_RSS;
}

CGUIRSSControl::~CGUIRSSControl(void)
{
  CSingleLock lock(m_criticalSection);
  if (m_pReader)
    m_pReader->SetObserver(NULL);
  m_pReader = NULL;

  if (m_pwzText)
    delete[] m_pwzText;
  m_pwzText = NULL;

  if (m_pbColors)
    delete[] m_pbColors;
  m_pbColors = NULL;
}

void CGUIRSSControl::SetUrls(const vector<string> &vecUrl)
{
  m_vecUrls = vecUrl; 
};

void CGUIRSSControl::SetIntervals(const vector<int>& vecIntervals)
{
  m_vecIntervals = vecIntervals;
}

void CGUIRSSControl::Render()
{
  // only render the control if they are enabled
  if (g_guiSettings.GetBool("lookandfeel.enablerssfeeds"))
  {
    CSingleLock lock(m_criticalSection);
    // Create RSS background/worker thread if needed
    if (m_pReader == NULL && !g_rssManager.GetReader(GetID(), GetParentID(), this, m_pReader))
    {
      if (m_strRSSTags != "")
      {
        CStdStringArray vecSplitTags;
        int i;

        StringUtils::SplitString(m_strRSSTags, ",", vecSplitTags);

        for (i = 0;i < (int)vecSplitTags.size();i++)
          m_pReader->AddTag(vecSplitTags[i]);
      }
      // use half the width of the control as spacing between feeds, and double this between feed sets
      WCHAR wTmp[2];
      wTmp[0] = L' ';
      wTmp[1] = 0;
      float spaceWidth = 15;
      if (m_label.font)
        spaceWidth = m_label.font->GetTextWidth(wTmp);
      m_pReader->Create(this, m_vecUrls, m_vecIntervals, (int)(0.5f*GetWidth() / spaceWidth) + 1);
    }

    if (m_label.font && m_pwzText)
    {
      RenderText();
    }

    if (m_pReader)
      m_pReader->CheckForUpdates();
  }
  CGUIControl::Render();
}

void CGUIRSSControl::OnFeedUpdate(CStdStringW& aFeed, LPBYTE aColorArray)
{
  CSingleLock lock(m_criticalSection);
  int nStringLength = aFeed.GetLength() + 1;
  if (m_pwzText)
    delete[] m_pwzText;

  m_pwzText = NULL;

  m_pwzText = new WCHAR[nStringLength+1];
  swprintf(m_pwzText, L"%s", aFeed.c_str() );

  m_pbColors = aColorArray;
}

void CGUIRSSControl::RenderText()
{
  if (!m_label.font)
    return ;

  DWORD dwPalette[3];
  dwPalette[0]=m_label.textColor;
  dwPalette[1]=m_dwHeadlineColor;
  dwPalette[2]=m_dwChannelColor;
  
  m_label.font->DrawScrollingText(m_posX, m_posY, dwPalette, 3, m_label.shadowColor, m_pwzText, m_width, m_scrollInfo, m_pbColors);
}

