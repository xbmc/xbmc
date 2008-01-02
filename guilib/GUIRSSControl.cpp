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
  m_colors.push_back(m_label.textColor);
  m_colors.push_back(dwHeadlineColor);
  m_colors.push_back(dwChannelColor);

  m_strRSSTags = strRSSTags;

  m_pReader = NULL;
  ControlType = GUICONTROL_RSS;
}

CGUIRSSControl::~CGUIRSSControl(void)
{
  CSingleLock lock(m_criticalSection);
  if (m_pReader)
    m_pReader->SetObserver(NULL);
  m_pReader = NULL;
}

void CGUIRSSControl::SetUrls(const vector<string> &vecUrl)
{
  m_vecUrls = vecUrl; 
}

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

        StringUtils::SplitString(m_strRSSTags, ",", vecSplitTags);

        for (unsigned int i = 0;i < vecSplitTags.size();i++)
          m_pReader->AddTag(vecSplitTags[i]);
      }
      // use half the width of the control as spacing between feeds, and double this between feed sets
      float spaceWidth = (m_label.font) ? m_label.font->GetCharWidth(L' ') : 15;
      m_pReader->Create(this, m_vecUrls, m_vecIntervals, (int)(0.5f*GetWidth() / spaceWidth) + 1);
    }

    if (m_label.font)
      m_label.font->DrawScrollingText(m_posX, m_posY, m_colors, m_label.shadowColor, m_feed, 0, m_width, m_scrollInfo);

    if (m_pReader)
      m_pReader->CheckForUpdates();
  }
  CGUIControl::Render();
}

void CGUIRSSControl::OnFeedUpdate(const vector<DWORD> &feed)
{
  CSingleLock lock(m_criticalSection);
  m_feed = feed;
}



