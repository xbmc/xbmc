/*!
\file GUIInfoColor.h
\brief 
*/

#ifndef GUILIB_GUIINFOCOLOR_H
#define GUILIB_GUIINFOCOLOR_H

#pragma once

class CGUIInfoColor
{
public:
  CGUIInfoColor(DWORD color = 0);

  const CGUIInfoColor &operator=(const CGUIInfoColor &color);
  const CGUIInfoColor &operator=(DWORD color);
  operator DWORD() const { return GetColor(); };

  void Parse(const CStdString &label);

private:
  DWORD GetColor() const;
  int m_info;
  DWORD m_color;
};

#endif
