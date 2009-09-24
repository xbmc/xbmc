/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIInfoTypes.h"
#include "utils/CharsetConverter.h"
#include "utils/GUIInfoManager.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIColorManager.h"
#include "GUIListItem.h"
#include "StringUtils.h"

using namespace std;

CGUIInfoBool::CGUIInfoBool(bool value)
{
  m_info = 0;
  m_value = value;
}

void CGUIInfoBool::Parse(const CStdString &info)
{
  m_info = g_infoManager.TranslateString(info);
  if (m_info == SYSTEM_ALWAYS_TRUE)
  {
    m_value = true;
    m_info = 0;
  }
  else if (m_info == SYSTEM_ALWAYS_FALSE)
  {
    m_value = false;
    m_info = 0;
  }
  else
    m_info = g_infoManager.GetBool(m_info);
}

void CGUIInfoBool::Update(int parentID, const CGUIListItem *item)
{
  if (m_info)
    m_value = g_infoManager.GetBool(m_info, parentID, item);
}


CGUIInfoColor::CGUIInfoColor(uint32_t color)
{
  m_color = color;
  m_info = 0;
}

const CGUIInfoColor &CGUIInfoColor::operator=(color_t color)
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

void CGUIInfoColor::Update()
{
  if (!m_info)
    return; // no infolabel

  // Expand the infolabel, and then convert it to a color
  CStdString infoLabel(g_infoManager.GetLabel(m_info));
  if (!infoLabel.IsEmpty())
    m_color = g_colorManager.GetColor(infoLabel.c_str());
  else
    m_color = 0;
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

CGUIInfoLabel::CGUIInfoLabel()
{
}

CGUIInfoLabel::CGUIInfoLabel(const CStdString &label, const CStdString &fallback)
{
  SetLabel(label, fallback);
}

void CGUIInfoLabel::SetLabel(const CStdString &label, const CStdString &fallback)
{
  m_fallback = fallback;
  Parse(label);
}

CStdString CGUIInfoLabel::GetLabel(int contextWindow, bool preferImage) const
{
  CStdString label;
  for (unsigned int i = 0; i < m_info.size(); i++)
  {
    const CInfoPortion &portion = m_info[i];
    if (portion.m_info)
    {
      CStdString infoLabel;
      if (preferImage)
        infoLabel = g_infoManager.GetImage(portion.m_info, contextWindow);
      if (infoLabel.IsEmpty())
        infoLabel = g_infoManager.GetLabel(portion.m_info, contextWindow);
      if (!infoLabel.IsEmpty())
      {
        label += portion.m_prefix;
        label += infoLabel;
        label += portion.m_postfix;
      }
    }
    else
    { // no info, so just append the prefix
      label += portion.m_prefix;
    }
  }
  if (label.IsEmpty())  // empty label, use the fallback
    return m_fallback;
  return label;
}

CStdString CGUIInfoLabel::GetItemLabel(const CGUIListItem *item, bool preferImages) const
{
  if (!item->IsFileItem()) return "";
  CStdString label;
  for (unsigned int i = 0; i < m_info.size(); i++)
  {
    const CInfoPortion &portion = m_info[i];
    if (portion.m_info)
    {
      CStdString infoLabel;
      if (preferImages)
        infoLabel = g_infoManager.GetItemImage((const CFileItem *)item, portion.m_info);
      else
        infoLabel = g_infoManager.GetItemLabel((const CFileItem *)item, portion.m_info);
      if (!infoLabel.IsEmpty())
      {
        label += portion.m_prefix;
        label += infoLabel;
        label += portion.m_postfix;
      }
    }
    else
    { // no info, so just append the prefix
      label += portion.m_prefix;
    }
  }
  if (label.IsEmpty())
    return m_fallback;
  return label;
}

bool CGUIInfoLabel::IsEmpty() const
{
  return m_info.size() == 0;
}

bool CGUIInfoLabel::IsConstant() const
{
  return m_info.size() == 0 || (m_info.size() == 1 && m_info[0].m_info == 0);
}

void CGUIInfoLabel::Parse(const CStdString &label)
{
  m_info.clear();
  CStdString work(label);
  // Step 1: Replace all $LOCALIZE[number] with the real string
  int pos1 = work.Find("$LOCALIZE[");
  while (pos1 >= 0)
  {
    int pos2 = StringUtils::FindEndBracket(work, '[', ']', pos1 + 10);
    if (pos2 > pos1)
    {
      CStdString left = work.Left(pos1);
      CStdString right = work.Mid(pos2 + 1);
      CStdString replace = g_localizeStringsTemp.Get(atoi(work.Mid(pos1 + 10).c_str()));
      if (replace == "")
         replace = g_localizeStrings.Get(atoi(work.Mid(pos1 + 10).c_str()));
      work = left + replace + right;
    }
    else
    {
      CLog::Log(LOGERROR, "Error parsing label - missing ']'");
      return;
    }
    pos1 = work.Find("$LOCALIZE[", pos1);
  }
  // Step 2: Find all $INFO[info,prefix,postfix] blocks
  pos1 = work.Find("$INFO[");
  while (pos1 >= 0)
  {
    // output the first block (contents before first $INFO)
    if (pos1 > 0)
      m_info.push_back(CInfoPortion(0, work.Left(pos1), ""));

    // ok, now decipher the $INFO block
    int pos2 = StringUtils::FindEndBracket(work, '[', ']', pos1 + 6);
    if (pos2 > pos1)
    {
      // decipher the block
      CStdString block = work.Mid(pos1 + 6, pos2 - pos1 - 6);
      CStdStringArray params;
      StringUtils::SplitString(block, ",", params);
      int info = g_infoManager.TranslateString(params[0]);
      CStdString prefix, postfix;
      if (params.size() > 1)
        prefix = params[1];
      if (params.size() > 2)
        postfix = params[2];
      m_info.push_back(CInfoPortion(info, prefix, postfix));
      // and delete it from our work string
      work = work.Mid(pos2 + 1);
    }
    else
    {
      CLog::Log(LOGERROR, "Error parsing label - missing ']'");
      return;
    }
    pos1 = work.Find("$INFO[");
  }
  // add any last block
  if (!work.IsEmpty())
    m_info.push_back(CInfoPortion(0, work, ""));
}

CGUIInfoLabel::CInfoPortion::CInfoPortion(int info, const CStdString &prefix, const CStdString &postfix)
{
  m_info = info;
  m_prefix = prefix;
  m_postfix = postfix;
  // filter our prefix and postfix for comma's
  m_prefix.Replace("$COMMA", ",");
  m_postfix.Replace("$COMMA", ",");
  m_prefix.Replace("$LBRACKET", "["); m_prefix.Replace("$RBRACKET", "]");
  m_postfix.Replace("$LBRACKET", "["); m_postfix.Replace("$RBRACKET", "]");
}

CStdString CGUIInfoLabel::GetLabel(const CStdString &label, bool preferImage)
{ // translate the label
  CGUIInfoLabel info(label, "");
  return info.GetLabel(0, preferImage);
}
