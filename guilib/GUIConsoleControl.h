/*!
\file GUIConsoleControl.h
\brief 
*/

#ifndef GUILIB_GUIConsoleControl_H
#define GUILIB_GUIConsoleControl_H

#pragma once

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIConsoleControl : public CGUIControl
{
public:

  CGUIConsoleControl(DWORD dwParentID, DWORD dwControlId,
                     float posX, float posY, float width, float height,
                     const CLabelInfo &labelInfo,
                     const CGUIInfoColor &penColor1, const CGUIInfoColor &penColor2,
                     const CGUIInfoColor &penColor3, const CGUIInfoColor &penColor4);

  virtual ~CGUIConsoleControl(void);

  virtual void Render();

  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual bool CanFocus() const { return false; };

  void Clear();
  void Write(CStdString& aString, INT nPaletteIndex = 0);

protected:

  void AddLine(CStdString& aString, DWORD aColour);

  DWORD GetPenColor(INT nPaletteIndex)
  {
    return nPaletteIndex < (INT)m_palette.size() ? m_palette[nPaletteIndex] : 0xFF808080;
  };

protected:

  struct Line
  {
    CStdString text;
    DWORD colour;
  };

  typedef std::vector<Line> LINEVECTOR;
  LINEVECTOR m_lines;
  LINEVECTOR m_queuedLines;

  typedef std::vector<D3DCOLOR> PALETTE;
  PALETTE m_palette;

  INT m_nMaxLines;
  DWORD m_dwLineCounter;
  DWORD m_dwFrameCounter;

  FLOAT m_fFontHeight;
  CLabelInfo m_label;
};
#endif
