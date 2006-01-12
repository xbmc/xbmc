#include "include.h"
#include "GUIConsoleControl.h"
#include "GUIWindowManager.h"
#include "../xbmc/utils/CharsetConverter.h"


#define CONSOLE_LINE_SPACING 1.0f

CGUIConsoleControl::CGUIConsoleControl(DWORD dwParentID, DWORD dwControlId,
                                       int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                                       const CLabelInfo& labelInfo,
                                       D3DCOLOR dwPenColor1, D3DCOLOR dwPenColor2, D3DCOLOR dwPenColor3, D3DCOLOR dwPenColor4)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{
  m_palette.push_back(dwPenColor1);
  m_palette.push_back(dwPenColor2);
  m_palette.push_back(dwPenColor3);
  m_palette.push_back(dwPenColor4);

  FLOAT fTextW;
  if (m_label.font)
  {
    m_label.font->GetTextExtent(L"X", &fTextW, &m_fFontHeight);
  }

  m_nMaxLines = dwHeight / (DWORD)(m_fFontHeight + CONSOLE_LINE_SPACING);
  m_dwLineCounter = 0;
  m_dwFrameCounter = 0;

  Line line;
  line.text = "";
  line.colour = 0;

  for (int i = 0; i < m_nMaxLines; i++)
  {
    m_lines.push_back(line);
  }

  ControlType = GUICONTROL_CONSOLE;
}

CGUIConsoleControl::~CGUIConsoleControl(void)
{}

void CGUIConsoleControl::PreAllocResources()
{
  CGUIControl::PreAllocResources();
}

void CGUIConsoleControl::AllocResources()
{
  CGUIControl::AllocResources();
}

void CGUIConsoleControl::FreeResources()
{
  CGUIControl::FreeResources();
}

bool CGUIConsoleControl::OnAction(const CAction &action)
{
  return CGUIControl::OnAction(action);
}

void CGUIConsoleControl::Render()
{
  if (!UpdateEffectState())
  {
    return ;
  }

  m_dwFrameCounter++;

  /* Debugging
  if (m_dwFrameCounter%500==0)
  {
   CStdString strDebug = "Okay, so perhaps the name 'Magic Box' is slightly misleading - this nifty little attachment isn't much use if you're looking to produce rabbits out of thin air or saw your XBOX in half without destroying it.  What it does do, though, is provide XBOX owners with the opportunity to plugin in their favourite joypad (or, as the box puts it, 'use your familiar and loving controller') instead of a regular XBOX one.  It takes Saturn, Dreamcast and all PS2 pads and joysticks - including the rather meaty Hori one also shown on this page - which means that the excuse of not being use to the controller when you're being thrashed at Pro Evolution Soccer, Street Fighter II, Tony Hawk or any other game that's best played on a PS2 pad no longer applies.  As invaluable a peripheral as they come, we'd recommend buying four... and some spares besides.";
   Write(strDebug,0xFF00FF00);
  }*/

  FLOAT fTextX = (FLOAT) m_iPosX;
  FLOAT fTextY = (FLOAT) m_iPosY;

  CStdStringW strText;

  for (int nLine = 0; nLine < m_nMaxLines; nLine++)
  {
    INT nIndex = (m_dwLineCounter + nLine) % m_nMaxLines;

    Line& line = m_lines[nIndex];
    g_charsetConverter.stringCharsetToFontCharset(line.text, strText);

    if (m_label.font)
      m_label.font->DrawText(fTextX, fTextY, line.colour, m_label.shadowColor, (LPWSTR) strText.c_str());

    fTextY += m_fFontHeight + CONSOLE_LINE_SPACING;
  }
  CGUIControl::Render();
}

void CGUIConsoleControl::AddLine(CStdString& aLine, DWORD aColour)
{
  Line line;
  line.text = aLine;
  line.colour = aColour;

  // determine which line appears at the bottom of the console
  INT nIndex = m_dwLineCounter % m_nMaxLines;

  m_lines[nIndex] = line;

  m_dwLineCounter++;
}

void CGUIConsoleControl::Clear()
{
  for (int nIndex = 0; nIndex < m_nMaxLines; nIndex++)
  {
    m_lines[nIndex].text.clear();
  }
}

void CGUIConsoleControl::Write(CStdString& aString, INT nPaletteIndex)
{
  WriteString(aString, GetPenColor(nPaletteIndex) );
}

void CGUIConsoleControl::WriteString(CStdString& aString, DWORD aColour)
{
  if (!m_label.font) return ;

  CStdString strLine;
  CStdStringW strLineW;

  int nLastSpace = -1;
  int nStartOfLine = 0;
  int nPosition = 0;

  aString.Replace('`', '\'');
  aString.Replace("\r\n", "\n");

  while ( nPosition < (int)aString.length() )
  {
    // Get the current letter in the string
    char letter = aString[nPosition];

    // Handle the newline character
    if (letter == '\n')
    {
      // Add as much of the text as we have accumulated, irrespective
      // of whether we have reached the end of the line.
      CStdString lineOfText = strLine;
      AddLine(lineOfText, aColour);

      // Reset state
      nLastSpace = -1;
      nStartOfLine = nPosition + 1;
      strLine.clear();
    }
    else
    {
      if (letter == ' ')
      {
        // Note the position of this space, we may need to refer to it.
        nLastSpace = nPosition;
      }

      // Add the current letter to our string.
      strLine += letter;

      // Calculate the accumulated text dimensions.
      g_charsetConverter.stringCharsetToFontCharset(strLine, strLineW);
      FLOAT fWidth, fHeight;
      m_label.font->GetTextExtent(strLineW.c_str(), &fWidth, &fHeight);

      // If we have exceeded the allowable line width
      if (fWidth > m_dwWidth)
      {
        // If we have detected a space separating a previous word
        if (nLastSpace > 0)
        {
          strLine = aString.Mid(nStartOfLine, nLastSpace - nStartOfLine);
          nPosition = nLastSpace;
        }

        AddLine(strLine, aColour);

        // Reset state
        nLastSpace = -1;
        nStartOfLine = nPosition + 1;
        strLine.clear();
      }
    }

    nPosition++;
  }

  if (strLine.length() > 0)
  {
    AddLine(strLine, aColour);
  }
}

