#include "include.h"
#include "GUIInfoColor.h"
#include "utils/CharsetConverter.h"
#include "utils/GUIInfoManager.h"
#include "LocalizeStrings.h"
#include "GUIColorManager.h"

using namespace std;

CGUIInfoColor::CGUIInfoColor(DWORD color)
{
  m_color = color;
  m_info = 0;
}

const CGUIInfoColor &CGUIInfoColor::operator=(DWORD color)
{
  m_color = color;
  m_info = 0;
  return *this;
}

const CGUIInfoColor &CGUIInfoColor::operator=(const CGUIInfoColor &color)
{
  m_color = color.m_color;
  m_info = color.m_info;
  return *this;
}

DWORD CGUIInfoColor::GetColor() const
{
  if (!m_info)
    return m_color; // no infolabel so just return the stored color

  // Expand the infolabel, and then convert it to a color
  CStdString infoLabel(g_infoManager.GetLabel(m_info));
  if (infoLabel.IsEmpty())
    return 0;

  // We now have an expanded label that we can convert into a color
  int color;
  sscanf(infoLabel.c_str(), "%x", &color);
  return color;
}

void CGUIInfoColor::Parse(const CStdString &label)
{
  // Check for the standard $INFO[] block layout, and strip it if present
  CStdString label2 = label;
  if (label.Equals("-", false))
    return;

  if (label.Left(5).Equals("$INFO", false))
    label2 = label.Mid(6, label.length()-7);

  m_info = g_infoManager.TranslateString(label2);
  if (!m_info)
    m_color = g_colorManager.GetColor(label);
}

