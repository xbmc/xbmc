#include "stdafx.h"
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
  float fWidth, fHeight;
  if (m_pFont)
    m_pFont->GetTextExtent(wTmp, &fWidth, &fHeight);
  m_iLeadingSpaces = (int) (dwWidth / fWidth);
  m_strRSSTags = strRSSTags;
  m_iCharIndex = 0;
  m_iStartFrame = 0;
  m_iScrollX = 1;
  m_pdwPalette = new DWORD[3];
  m_pdwPalette[0] = dwNormalColor;
  m_pdwPalette[1] = dwHeadlineColor;
  m_pdwPalette[2] = dwChannelColor;

  m_pReader = NULL;
  m_pwzText = NULL;
  m_pwzBuffer = NULL;
  m_pbBuffer = NULL;
  m_pbColors = NULL;
  ControlType = GUICONTROL_RSS;
  m_fTextHeight;
  m_fTextWidth;
}

CGUIRSSControl::~CGUIRSSControl(void)
{
  //if(m_pdwPalette) //Shouldn't be deallocated as it is used by all RSSControls
  //  delete[] m_pdwPalette;
  if (m_pReader)
  {
    m_pReader->ResumeThread();
    m_pReader->StopThread();
    delete m_pReader;
  }
  if (m_pwzText)
    delete[] m_pwzText;
  if (m_pwzBuffer)
    delete[] m_pwzBuffer;
  if (m_pbBuffer)
    delete[] m_pbBuffer;
  if (m_pbColors) //Deallocate here since there isn't any better place
    delete[] m_pbColors;

  m_pdwPalette = NULL;
  m_pReader = NULL;
  m_pwzText = NULL;
  m_pwzBuffer = NULL;
  m_pbColors = NULL;
}

void CGUIRSSControl::SetUrls(const vector<wstring> &vecUrl)
{
  m_vecUrls = vecUrl; 
};

void CGUIRSSControl::Render()
{
  if ( (!IsVisible()) || g_application.IsPlayingVideo() || !g_guiSettings.GetBool("LookAndFeel.EnableRSSFeeds") || !g_guiSettings.GetBool("Network.EnableInternet"))
  {
    return ;
  }

  if (m_pReader == NULL)
  {
    // Create RSS background/worker thread
    m_pReader = new CRssReader();

    if (m_strRSSTags != "")
    {
      CStdStringArray vecSplitTags;
      int i;

      StringUtils::SplitString(m_strRSSTags, ",", vecSplitTags);

      for (i = 0;i < (int)vecSplitTags.size();i++)
        m_pReader->AddTag(vecSplitTags[i]);
    }
    GetLocalTime(&timeSnapShot);
    m_pReader->Create(this, m_vecUrls, m_iLeadingSpaces);
  }

  if (m_pFont && m_pwzText)
  {
    RenderText();
  }
  SYSTEMTIME time;
  GetLocalTime(&time);

  if (((time.wDay * 24 * 60) + (time.wHour * 60) + time.wMinute) - ((timeSnapShot.wDay * 24 * 60) + (timeSnapShot.wHour * 60) + timeSnapShot.wMinute) > 30 )
  {
    CLog::Log(LOGDEBUG, "Updating RSS");

    GetLocalTime(&timeSnapShot);
    m_pReader->ResumeThread();
  }
}

void CGUIRSSControl::OnFeedUpdate(CStdString& aFeed, LPBYTE aColorArray)
{
  int nStringLength = aFeed.GetLength() + 1;
  if (m_pwzText)
    delete[] m_pwzText;
  if (m_pwzBuffer)
    delete[] m_pwzBuffer;
  if (m_pbBuffer)
    delete[] m_pbBuffer;

  m_pwzText = NULL;
  m_pwzBuffer = NULL;
  m_pbBuffer = NULL;

  m_pwzText = new WCHAR[nStringLength];
  m_pwzBuffer = new WCHAR[nStringLength];
  swprintf(m_pwzText, L"%S", aFeed.c_str() );

  if (m_pFont) m_pFont->GetTextExtent( m_pwzText, &m_fTextWidth, &m_fTextHeight);

  m_iTextLenght = (int)wcslen(m_pwzText);

  m_pbColors = aColorArray;
  m_pbBuffer = new BYTE[nStringLength];
}

void CGUIRSSControl::RenderText()
{
  if (!m_pFont)
    return ;

  float fPosX = (float) m_iPosX;
  float fPosY = (float) m_iPosY;
  float fMaxWidth = (float)m_dwWidth;

  float fPosCX = fPosX;
  float fPosCY = fPosY;
  g_graphicsContext.Correct(fPosCX, fPosCY);

  if (fPosCX < 0)
  {
    fPosCX = 0.0f;
  }

  if (fPosCY < 0)
  {
    fPosCY = 0.0f;
  }

  if (fPosCY > g_graphicsContext.GetHeight())
  {
    fPosCY = (float)g_graphicsContext.GetHeight();
  }

  float fHeight = 60.0f;

  if (fHeight + fPosCY >= g_graphicsContext.GetHeight())
  {
    fHeight = g_graphicsContext.GetHeight() - fPosCY - 1;
  }

  if (fHeight <= 0)
  {
    return ;
  }

  float fwidth = fMaxWidth - 5.0f;

  D3DVIEWPORT8 newviewport, oldviewport;
  g_graphicsContext.Get3DDevice()->GetViewport(&oldviewport);
  newviewport.X = (DWORD)fPosCX;
  newviewport.Y = (DWORD)fPosCY;
  newviewport.Width = (DWORD)(fwidth);
  newviewport.Height = (DWORD)(fHeight);
  newviewport.MinZ = 0.0f;
  newviewport.MaxZ = 1.0f;
  g_graphicsContext.Get3DDevice()->SetViewport(&newviewport);

  // if size of text is bigger than can be drawn onscreen
  if (m_fTextWidth > fMaxWidth)
  {
    fMaxWidth += 50.0f;

    // if start frame is greater than 25? (why?)
    if (m_iStartFrame > 25)
    {
      WCHAR wTmp[3];

      // if scroll char index is bigger or equal to the text length
      if (m_iCharIndex >= m_iTextLenght )
      {
        // use a space character
        wTmp[0] = L' ';
      }
      else
      {
        // use the next character
        wTmp[0] = m_pwzText[m_iCharIndex];
      }

      // determine the width of the character
      wTmp[1] = 0;
      float fWidth, fHeight;
      m_pFont->GetTextExtent(wTmp, &fWidth, &fHeight);
      // if the scroll offset is the same or as big as the next character
      if ( m_iScrollX >= fWidth)
      {
        // increase the character offset
        ++m_iCharIndex;

        // if we've reached the end of the text
        if (m_iCharIndex > m_iTextLenght )
        {
          // start from the begning
          m_iCharIndex = 0;
        }

        // reset the scroll offset
        m_iScrollX = 1;
      }
      else
      {
        // increase the scroll offset
        m_iScrollX++;
      }

      int ipos = 0;
      // truncate the scroll text starting from the character index and wrap
      // the text.
      for (int i = 0; i < m_iTextLenght; i++)
      {
        if (i + m_iCharIndex < m_iTextLenght)
        {
          m_pwzBuffer[i] = m_pwzText[i + m_iCharIndex];
          m_pbBuffer[i] = m_pbColors[i + m_iCharIndex];
        }
        else
        {
          if (ipos == 0)
          {
            m_pwzBuffer[i] = L' ';
            m_pbBuffer[i] = 0;
          }
          else
          {
            m_pwzBuffer[i] = m_pwzText[ipos - 1];
            m_pbBuffer[i] = m_pbColors[ipos - 1];
          }
          ipos++;
        }
        m_pwzBuffer[i + 1] = 0;
      }

      if (fPosY >= 0.0)
      {
        m_pFont->DrawColourTextWidth(fPosX - m_iScrollX, fPosY, m_pdwPalette, m_pwzBuffer, m_pbBuffer, fMaxWidth);
      }
    }
    else
    {
      m_iStartFrame++;
      if (fPosY >= 0.0)
      {
        m_pFont->DrawColourTextWidth(fPosX, fPosY, m_pdwPalette, m_pwzText, m_pbColors, fMaxWidth);
      }
    }
  }
  g_graphicsContext.Get3DDevice()->SetViewport(&oldviewport);
}

