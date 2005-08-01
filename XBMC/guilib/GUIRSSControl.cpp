#include "include.h"
#include "GUIRSSControl.h"
#include "GUIWindowManager.h"
#include "GUIFontManager.h"
#include "..\xbmc\Application.h"
#include "..\xbmc\settings.h"


extern CApplication g_application;

CGUIRSSControl::CGUIRSSControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFontName, D3DCOLOR dwChannelColor, D3DCOLOR dwHeadlineColor, D3DCOLOR dwNormalColor, CStdString& strRSSTags)
: CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{
  m_dwChannelColor = dwChannelColor;
  m_dwHeadlineColor = dwHeadlineColor;
  m_dwTextColor = dwNormalColor;
  m_pFont = g_fontManager.GetFont(strFontName);

  WCHAR wTmp[2];
  wTmp[0] = L' ';
  wTmp[1] = 0;
  float fWidth = 15, fHeight;
  if (m_pFont)
    m_pFont->GetTextExtent(wTmp, &fWidth, &fHeight);
  m_iLeadingSpaces = (int) (dwWidth / fWidth);
  m_strRSSTags = strRSSTags;

  m_pReader = NULL;
  m_pwzText = NULL;
  m_pbColors = NULL;
  ControlType = GUICONTROL_RSS;
}

CGUIRSSControl::~CGUIRSSControl(void)
{
  if (m_pwzText)
    delete[] m_pwzText;
  if (m_pbColors) //Deallocate here since there isn't any better place
    delete[] m_pbColors;

  if (m_pReader)
    m_pReader->SetObserver(NULL);

  m_pReader = NULL;
  m_pwzText = NULL;
  m_pbColors = NULL;
}

void CGUIRSSControl::SetUrls(const vector<wstring> &vecUrl)
{
  m_vecUrls = vecUrl; 
};

void CGUIRSSControl::SetIntervals(const vector<int>& vecIntervals)
{
  m_vecIntervals = vecIntervals;
}

void CGUIRSSControl::Render()
{
  if ( (!UpdateFadeState()) || !g_guiSettings.GetBool("Network.EnableRSSFeeds") || !g_guiSettings.GetBool("Network.EnableInternet"))
  {
    return ;
  }

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
    m_pReader->Create(this, m_vecUrls, m_vecIntervals, m_iLeadingSpaces);
  }

  if (m_pFont && m_pwzText)
  {
    RenderText();
  }

  if (m_pReader)
    m_pReader->CheckForUpdates();
}

void CGUIRSSControl::OnFeedUpdate(CStdStringW& aFeed, LPBYTE aColorArray)
{
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
  if (!m_pFont)
    return ;

  DWORD dwPalette[3];
  dwPalette[0]=m_dwTextColor;
  dwPalette[1]=m_dwHeadlineColor;
  dwPalette[2]=m_dwChannelColor;

  m_pFont->DrawScrollingText((float)m_iPosX, (float)m_iPosY, dwPalette, 3, m_pwzText, (float)m_dwWidth, m_scrollInfo, m_pbColors);
}

