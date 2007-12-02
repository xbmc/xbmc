#include "include.h"
#include "GUIConsoleControl.h"
#include "GUIWindowManager.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "GUITextLayout.h"


#define CONSOLE_LINE_SPACING 1.0f

CGUIConsoleControl::CGUIConsoleControl(DWORD dwParentID, DWORD dwControlId,
                                       float posX, float posY, float width, float height,
                                       const CLabelInfo& labelInfo,
                                       D3DCOLOR dwPenColor1, D3DCOLOR dwPenColor2, D3DCOLOR dwPenColor3, D3DCOLOR dwPenColor4)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_label = labelInfo;
  m_palette.push_back(dwPenColor1);
  m_palette.push_back(dwPenColor2);
  m_palette.push_back(dwPenColor3);
  m_palette.push_back(dwPenColor4);

  if (m_label.font)
    m_fFontHeight = m_label.font->GetLineHeight();

  m_nMaxLines = (DWORD)(height / (m_fFontHeight + CONSOLE_LINE_SPACING));
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

void CGUIConsoleControl::Render()
{
  m_dwFrameCounter++;

  /* Debugging
  if (m_dwFrameCounter%500==0)
  {
   CStdString strDebug = "Okay, so perhaps the name 'Magic Box' is slightly misleading - this nifty little attachment isn't much use if you're looking to produce rabbits out of thin air or saw your XBOX in half without destroying it.  What it does do, though, is provide XBOX owners with the opportunity to plugin in their favourite joypad (or, as the box puts it, 'use your familiar and loving controller') instead of a regular XBOX one.  It takes Saturn, Dreamcast and all PS2 pads and joysticks - including the rather meaty Hori one also shown on this page - which means that the excuse of not being use to the controller when you're being thrashed at Pro Evolution Soccer, Street Fighter II, Tony Hawk or any other game that's best played on a PS2 pad no longer applies.  As invaluable a peripheral as they come, we'd recommend buying four... and some spares besides.";
   Write(strDebug,0xFF00FF00);
  }*/

  FLOAT fTextX = (FLOAT) m_posX;
  FLOAT fTextY = (FLOAT) m_posY;

  // queue up our new lines
  for (unsigned int line = 0; line < m_queuedLines.size(); line++)
    AddLine(m_queuedLines[line].text, m_queuedLines[line].colour);
  m_queuedLines.clear();

  for (int nLine = 0; nLine < m_nMaxLines; nLine++)
  {
    INT nIndex = (m_dwLineCounter + nLine) % m_nMaxLines;

    Line& line = m_lines[nIndex];

    CGUITextLayout::DrawText(m_label.font, fTextX, fTextY, line.colour, 0, line.text, 0);

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
  m_queuedLines.clear();
}

void CGUIConsoleControl::Write(CStdString& aString, INT nPaletteIndex)
{
  Line line;
  line.text = aString;
  line.colour = GetPenColor(nPaletteIndex);
  m_queuedLines.push_back(line);
}

