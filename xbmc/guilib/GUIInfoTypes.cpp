/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIInfoTypes.h"
#include "GUIInfoManager.h"
#include "addons/AddonManager.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIColorManager.h"
#include "GUIListItem.h"
#include "utils/StringUtils.h"
#include "addons/Skin.h"

using namespace std;
using ADDON::CAddonMgr;

CGUIInfoBool::CGUIInfoBool(bool value)
{
  m_value = value;
}

CGUIInfoBool::~CGUIInfoBool()
{
}

void CGUIInfoBool::Parse(const std::string &expression, int context)
{
  if (expression == "true")
    m_value = true;
  else if (expression == "false")
    m_value = false;
  else
  {
    m_info = g_infoManager.Register(expression, context);
    Update();
  }
}

void CGUIInfoBool::Update(const CGUIListItem *item /*= NULL*/)
{
  if (m_info)
    m_value = m_info->Get(item);
}


CGUIInfoColor::CGUIInfoColor(uint32_t color)
{
  m_color = color;
  m_info = 0;
}

CGUIInfoColor &CGUIInfoColor::operator=(color_t color)
{
  m_color = color;
  m_info = 0;
  return *this;
}

CGUIInfoColor &CGUIInfoColor::operator=(const CGUIInfoColor &color)
{
  m_color = color.m_color;
  m_info = color.m_info;
  return *this;
}

bool CGUIInfoColor::Update()
{
  if (!m_info)
    return false; // no infolabel

  // Expand the infolabel, and then convert it to a color
  std::string infoLabel(g_infoManager.GetLabel(m_info));
  color_t color = !infoLabel.empty() ? g_colorManager.GetColor(infoLabel.c_str()) : 0;
  if (m_color != color)
  {
    m_color = color;
    return true;
  }
  else
    return false;
}

void CGUIInfoColor::Parse(const std::string &label, int context)
{
  // Check for the standard $INFO[] block layout, and strip it if present
  std::string label2 = label;
  if (label == "-")
    return;

  if (StringUtils::StartsWithNoCase(label, "$var["))
  {
    label2 = label.substr(5, label.length() - 6);
    m_info = g_infoManager.TranslateSkinVariableString(label2, context);
    if (!m_info)
      m_info = g_infoManager.RegisterSkinVariableString(g_SkinInfo->CreateSkinVariable(label2, context));
    return;
  }

  if (StringUtils::StartsWithNoCase(label, "$info["))
    label2 = label.substr(6, label.length()-7);

  m_info = g_infoManager.TranslateString(label2);
  if (!m_info)
    m_color = g_colorManager.GetColor(label);
}

CGUIInfoLabel::CGUIInfoLabel() : m_dirty(false)
{
}

CGUIInfoLabel::CGUIInfoLabel(const std::string &label, const std::string &fallback /*= ""*/, int context /*= 0*/) : m_dirty(false)
{
  SetLabel(label, fallback, context);
}

int CGUIInfoLabel::GetIntValue(int contextWindow) const
{
  std::string label = GetLabel(contextWindow);
  if (!label.empty())
    return strtol(label.c_str(), NULL, 10);

  return 0;
}

void CGUIInfoLabel::SetLabel(const std::string &label, const std::string &fallback, int context /*= 0*/)
{
  m_fallback = fallback;
  Parse(label, context);
}

const std::string &CGUIInfoLabel::GetLabel(int contextWindow, bool preferImage, std::string *fallback /*= NULL*/) const
{
  bool needsUpdate = m_dirty;
  if (!m_info.empty())
  {
    for (vector<CInfoPortion>::const_iterator portion = m_info.begin(); portion != m_info.end(); ++portion)
    {
      if (portion->m_info)
      {
        std::string infoLabel;
        if (preferImage)
          infoLabel = g_infoManager.GetImage(portion->m_info, contextWindow, fallback);
        if (infoLabel.empty())
          infoLabel = g_infoManager.GetLabel(portion->m_info, contextWindow, fallback);
        needsUpdate |= portion->NeedsUpdate(infoLabel);
      }
    }
  }
  else
    needsUpdate = !m_label.empty();

  return CacheLabel(needsUpdate);
}

const std::string &CGUIInfoLabel::GetItemLabel(const CGUIListItem *item, bool preferImages, std::string *fallback /*= NULL*/) const
{
  bool needsUpdate = m_dirty;
  if (item->IsFileItem() && !m_info.empty())
  {
    for (vector<CInfoPortion>::const_iterator portion = m_info.begin(); portion != m_info.end(); ++portion)
    {
      if (portion->m_info)
      {
        std::string infoLabel;
        if (preferImages)
          infoLabel = g_infoManager.GetItemImage((const CFileItem *)item, portion->m_info, fallback);
        else
          infoLabel = g_infoManager.GetItemLabel((const CFileItem *)item, portion->m_info, fallback);
        needsUpdate |= portion->NeedsUpdate(infoLabel);
      }
    }
  }
  else
    needsUpdate = !m_label.empty();

  return CacheLabel(needsUpdate);
}

const std::string &CGUIInfoLabel::CacheLabel(bool rebuild) const
{
  if (rebuild)
  {
    m_label.clear();
    for (vector<CInfoPortion>::const_iterator portion = m_info.begin(); portion != m_info.end(); ++portion)
      m_label += portion->Get();
    m_dirty = false;
  }
  if (m_label.empty())  // empty label, use the fallback
    return m_fallback;
  return m_label;
}

bool CGUIInfoLabel::IsEmpty() const
{
  return m_info.size() == 0;
}

bool CGUIInfoLabel::IsConstant() const
{
  return m_info.size() == 0 || (m_info.size() == 1 && m_info[0].m_info == 0);
}

typedef std::string (*StringReplacerFunc) (const std::string &str);

void ReplaceString(std::string &work, const std::string &str, StringReplacerFunc func)
{
  // Replace all $str[number] with the real string
  size_t pos1 = work.find("$" + str + "[");
  while (pos1 != std::string::npos)
  {
    size_t pos2 = pos1 + str.length() + 2;
    size_t pos3 = StringUtils::FindEndBracket(work, '[', ']', pos2);
    if (pos3 != std::string::npos)
    {
      std::string left = work.substr(0, pos1);
      std::string right = work.substr(pos3 + 1);
      std::string replace = func(work.substr(pos2, pos3 - pos2));
      work = left + replace + right;
    }
    else
    {
      CLog::Log(LOGERROR, "Error parsing label - missing ']' in \"%s\"", work.c_str());
      return;
    }
    pos1 = work.find("$" + str + "[", pos1);
  }
}

std::string LocalizeReplacer(const std::string &str)
{
  std::string replace = g_localizeStringsTemp.Get(atoi(str.c_str()));
  if (replace.empty())
    replace = g_localizeStrings.Get(atoi(str.c_str()));
  return replace;
}

std::string AddonReplacer(const std::string &str)
{
  // assumes "addon.id #####"
  size_t length = str.find(" ");
  std::string id = str.substr(0, length);
  int stringid = atoi(str.substr(length + 1).c_str());
  return CAddonMgr::Get().GetString(id, stringid);
}

std::string NumberReplacer(const std::string &str)
{
  return str;
}

std::string CGUIInfoLabel::ReplaceLocalize(const std::string &label)
{
  std::string work(label);
  ReplaceString(work, "LOCALIZE", LocalizeReplacer);
  ReplaceString(work, "NUMBER", NumberReplacer);
  return work;
}

std::string CGUIInfoLabel::ReplaceAddonStrings(const std::string &label)
{
  std::string work(label);
  ReplaceString(work, "ADDON", AddonReplacer);
  return work;
}

enum EINFOFORMAT { NONE = 0, FORMATINFO, FORMATESCINFO, FORMATVAR };

typedef struct
{
  const char *str;
  EINFOFORMAT  val;
} infoformat;

const static infoformat infoformatmap[] = {{ "$INFO[",    FORMATINFO },
                                           { "$ESCINFO[", FORMATESCINFO},
                                           { "$VAR[",     FORMATVAR}};

void CGUIInfoLabel::Parse(const std::string &label, int context)
{
  m_info.clear();
  m_dirty = true;
  // Step 1: Replace all $LOCALIZE[number] with the real string
  std::string work = ReplaceLocalize(label);
  // Step 2: Replace all $ADDON[id number] with the real string
  work = ReplaceAddonStrings(work);
  // Step 3: Find all $INFO[info,prefix,postfix] blocks
  EINFOFORMAT format;
  do
  {
    format = NONE;
    size_t pos1 = work.size();
    size_t pos2;
    size_t len = 0;
    for (size_t i = 0; i < sizeof(infoformatmap) / sizeof(infoformat); i++)
    {
      pos2 = work.find(infoformatmap[i].str);
      if (pos2 != string::npos && pos2 < pos1)
      {
        pos1 = pos2;
        len = strlen(infoformatmap[i].str);
        format = infoformatmap[i].val;
      }
    }

    if (format != NONE)
    {
      if (pos1 > 0)
        m_info.push_back(CInfoPortion(0, work.substr(0, pos1), ""));

      pos2 = StringUtils::FindEndBracket(work, '[', ']', pos1 + len);
      if (pos2 != std::string::npos)
      {
        // decipher the block
        std::string block = work.substr(pos1 + len, pos2 - pos1 - len);
        vector<string> params = StringUtils::Split(block, ",");
        if (!params.empty())
        {
          int info;
          if (format == FORMATVAR)
          {
            info = g_infoManager.TranslateSkinVariableString(params[0], context);
            if (info == 0)
              info = g_infoManager.RegisterSkinVariableString(g_SkinInfo->CreateSkinVariable(params[0], context));
            if (info == 0) // skinner didn't define this conditional label!
              CLog::Log(LOGWARNING, "Label Formating: $VAR[%s] is not defined", params[0].c_str());
          }
          else
            info = g_infoManager.TranslateString(params[0]);
          std::string prefix, postfix;
          if (params.size() > 1)
            prefix = params[1];
          if (params.size() > 2)
            postfix = params[2];
          m_info.push_back(CInfoPortion(info, prefix, postfix, format == FORMATESCINFO));
        }
        // and delete it from our work string
        work = work.substr(pos2 + 1);
      }
      else
      {
        CLog::Log(LOGERROR, "Error parsing label - missing ']' in \"%s\"", label.c_str());
        return;
      }
    }
  }
  while (format != NONE);

  if (!work.empty())
    m_info.push_back(CInfoPortion(0, work, ""));
}

CGUIInfoLabel::CInfoPortion::CInfoPortion(int info, const std::string &prefix, const std::string &postfix, bool escaped /*= false */)
{
  m_info = info;
  m_prefix = prefix;
  m_postfix = postfix;
  m_escaped = escaped;
  // filter our prefix and postfix for comma's
  StringUtils::Replace(m_prefix, "$COMMA", ",");
  StringUtils::Replace(m_postfix, "$COMMA", ",");
  StringUtils::Replace(m_prefix, "$LBRACKET", "["); StringUtils::Replace(m_prefix, "$RBRACKET", "]");
  StringUtils::Replace(m_postfix, "$LBRACKET", "["); StringUtils::Replace(m_postfix, "$RBRACKET", "]");
}

bool CGUIInfoLabel::CInfoPortion::NeedsUpdate(const std::string &label) const
{
  if (m_label != label)
  {
    m_label = label;
    return true;
  }
  return false;
}

std::string CGUIInfoLabel::CInfoPortion::Get() const
{
  if (!m_info)
    return m_prefix;
  else if (m_label.empty())
    return "";
  std::string label = m_prefix + m_label + m_postfix;
  if (m_escaped) // escape all quotes and backslashes, then quote
  {
    StringUtils::Replace(label, "\\", "\\\\");
    StringUtils::Replace(label, "\"", "\\\"");
    return "\"" + label + "\"";
  }
  return label;
}

std::string CGUIInfoLabel::GetLabel(const std::string &label, int contextWindow /*= 0*/, bool preferImage /*= false */)
{ // translate the label
  CGUIInfoLabel info(label, "", contextWindow);
  return info.GetLabel(contextWindow, preferImage);
}
