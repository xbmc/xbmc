/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/GUIInfoLabel.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "games/GameServices.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIListItem.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

using namespace KODI::GUILIB::GUIINFO;

CGUIInfoLabel::CGUIInfoLabel(const std::string &label, const std::string &fallback /*= ""*/, int context /*= 0*/)
{
  SetLabel(label, fallback, context);
}

int CGUIInfoLabel::GetIntValue(int contextWindow) const
{
  const std::string label = GetLabel(contextWindow);
  if (!label.empty())
    return strtol(label.c_str(), NULL, 10);

  return 0;
}

void CGUIInfoLabel::SetLabel(const std::string &label, const std::string &fallback, int context /*= 0*/)
{
  Parse(label, m_infoLabel, context);
  Parse(fallback, m_infoFallback, context);
  m_fallback = fallback;
}

bool CGUIInfoLabel::LabelNeedsUpdate(int context,
                                     bool preferImages,
                                     std::string* fallback,
                                     const std::vector<CInfoPortion>& infoPortion) const
{
  bool needsUpdate{false};
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  for (const auto& portion : infoPortion)
  {
    if (portion.m_info)
    {
      std::string infoLabel;
      if (preferImages)
        infoLabel = infoMgr.GetImage(portion.m_info, context, fallback);
      if (infoLabel.empty())
        infoLabel = infoMgr.GetLabel(portion.m_info, context, fallback);
      needsUpdate |= portion.NeedsUpdate(infoLabel);
    }
  }
  return needsUpdate;
}

const std::string& CGUIInfoLabel::GetLabel(int contextWindow,
                                           bool preferImage,
                                           std::string* fallback /*= NULL*/) const
{
  bool needsUpdate = m_dirty;
  if (!m_infoLabel.empty())
  {
    needsUpdate |= LabelNeedsUpdate(contextWindow, preferImage, fallback, m_infoLabel);
  }
  else
  {
    needsUpdate = !m_label.empty();
  }

  if (!m_infoFallback.empty())
  {
    needsUpdate |=
        LabelNeedsUpdate(contextWindow, preferImage, fallback, m_infoFallback) && m_label.empty();
  }

  return CacheLabel(needsUpdate);
}

bool CGUIInfoLabel::ItemLabelNeedsUpdate(const CGUIListItem* item,
                                         bool preferImages,
                                         std::string* fallback,
                                         const std::vector<CInfoPortion>& infoPortion) const
{
  bool needsUpdate{false};
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  for (const auto& portion : infoPortion)
  {
    if (portion.m_info)
    {
      std::string infoLabel;
      if (preferImages)
        infoLabel = infoMgr.GetItemImage(item, 0, portion.m_info, fallback);
      else
        infoLabel =
            infoMgr.GetItemLabel(static_cast<const CFileItem*>(item), 0, portion.m_info, fallback);
      needsUpdate |= portion.NeedsUpdate(infoLabel);
    }
  }
  return needsUpdate;
}

const std::string& CGUIInfoLabel::GetItemLabel(const CGUIListItem* item,
                                               bool preferImages,
                                               std::string* fallback /*= nullptr */) const
{
  bool needsUpdate = m_dirty;
  if (item->IsFileItem())
  {
    if (!m_infoLabel.empty())
      needsUpdate |= ItemLabelNeedsUpdate(item, preferImages, fallback, m_infoLabel);

    if (!m_infoFallback.empty())
      needsUpdate |= ItemLabelNeedsUpdate(item, preferImages, fallback, m_infoFallback);
  }
  else
  {
    needsUpdate = !m_label.empty();
  }

  return CacheLabel(needsUpdate);
}

void CGUIInfoLabel::RebuildLabel(std::string& label,
                                 const std::vector<CInfoPortion>& infoPortion) const
{
  label.clear();
  for (const auto& portion : infoPortion)
  {
    label += portion.Get();
  }
}

const std::string &CGUIInfoLabel::CacheLabel(bool rebuild) const
{
  if (rebuild)
  {
    RebuildLabel(m_label, m_infoLabel);
    RebuildLabel(m_fallback, m_infoFallback);
    m_dirty = false;
  }
  if (m_label.empty())  // empty label, use the fallback
  {
    return m_fallback;
  }

  return m_label;
}

bool CGUIInfoLabel::IsEmpty() const
{
  return m_infoLabel.empty();
}

bool CGUIInfoLabel::IsConstant() const
{
  return m_infoLabel.empty() || (m_infoLabel.size() == 1 && m_infoLabel[0].m_info == 0);
}

bool CGUIInfoLabel::ReplaceSpecialKeywordReferences(const std::string &strInput, const std::string &strKeyword, const StringReplacerFunc &func, std::string &strOutput)
{
  // replace all $strKeyword[value] with resolved strings
  const std::string dollarStrPrefix = "$" + strKeyword + "[";

  size_t index = 0;
  size_t startPos;

  while ((startPos = strInput.find(dollarStrPrefix, index)) != std::string::npos)
  {
    size_t valuePos = startPos + dollarStrPrefix.size();
    size_t endPos = StringUtils::FindEndBracket(strInput, '[', ']', valuePos);
    if (endPos != std::string::npos)
    {
      if (index == 0)  // first occurrence?
        strOutput.clear();
      strOutput.append(strInput, index, startPos - index); // append part from the left side
      strOutput += func(strInput.substr(valuePos, endPos - valuePos));  // resolve and append value part
      index = endPos + 1;
    }
    else
    {
      // if closing bracket is missing, report error and leave incomplete reference in
      CLog::Log(LOGERROR, "Error parsing value - missing ']' in \"{}\"", strInput);
      break;
    }
  }

  if (index)  // if we replaced anything
  {
    strOutput.append(strInput, index, std::string::npos); // append leftover from the right side
    return true;
  }

  return false;
}

bool CGUIInfoLabel::ReplaceSpecialKeywordReferences(std::string &work, const std::string &strKeyword, const StringReplacerFunc &func)
{
  std::string output;
  if (ReplaceSpecialKeywordReferences(work, strKeyword, func, output))
  {
    work = std::move(output);
    return true;
  }
  return false;
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
  size_t length = str.find(' ');
  const std::string addonid = str.substr(0, length);
  int stringid = atoi(str.substr(length + 1).c_str());
  return g_localizeStrings.GetAddonString(addonid, stringid);
}

std::string ControllerFeatureReplacer(const std::string& str)
{
  // assumes "feature name,controller ID"
  const size_t length = str.find(',');
  const std::string featureName = str.substr(0, length);
  const std::string controllerId = str.substr(length + 1);

  return CServiceBroker::GetGameServices().TranslateFeature(controllerId, featureName);
}

std::string NumberReplacer(const std::string &str)
{
  return str;
}

std::string CGUIInfoLabel::ReplaceLocalize(const std::string &label)
{
  std::string work(label);
  ReplaceSpecialKeywordReferences(work, "LOCALIZE", LocalizeReplacer);
  ReplaceSpecialKeywordReferences(work, "NUMBER", NumberReplacer);
  return work;
}

std::string CGUIInfoLabel::ReplaceAddonStrings(std::string &&label)
{
  ReplaceSpecialKeywordReferences(label, "ADDON", AddonReplacer);
  return std::move(label);
}

std::string CGUIInfoLabel::ReplaceControllerStrings(std::string&& label)
{
  ReplaceSpecialKeywordReferences(label, "FEATURE", ControllerFeatureReplacer);
  return std::move(label);
}

enum EINFOFORMAT { NONE = 0, FORMATINFO, FORMATESCINFO, FORMATVAR, FORMATESCVAR };

typedef struct
{
  const char *str;
  EINFOFORMAT  val;
} infoformat;

const static infoformat infoformatmap[] = {{ "$INFO[",    FORMATINFO},
                                           { "$ESCINFO[", FORMATESCINFO},
                                           { "$VAR[",     FORMATVAR},
                                           { "$ESCVAR[",  FORMATESCVAR}};

void CGUIInfoLabel::Parse(const std::string& label,
                          std::vector<CInfoPortion>& infoPortion,
                          int context)
{
  infoPortion.clear();
  m_dirty = true;
  // Step 1: Replace all $LOCALIZE[number] with the real string
  std::string work = ReplaceLocalize(label);
  // Step 2: Replace all $ADDON[id number] with the real string
  work = ReplaceAddonStrings(std::move(work));
  // Step 3: Replace all game controller strings with the real string
  work = ReplaceControllerStrings(std::move(work));
  // Step 4: Find all $INFO[info,prefix,postfix] blocks
  EINFOFORMAT format;
  do
  {
    format = NONE;
    size_t pos1 = work.size();
    size_t pos2;
    size_t len = 0;
    for (const infoformat& infoformat : infoformatmap)
    {
      pos2 = work.find(infoformat.str);
      if (pos2 != std::string::npos && pos2 < pos1)
      {
        pos1 = pos2;
        len = strlen(infoformat.str);
        format = infoformat.val;
      }
    }

    if (format != NONE)
    {
      if (pos1 > 0)
        infoPortion.emplace_back(0, work.substr(0, pos1), "");

      pos2 = StringUtils::FindEndBracket(work, '[', ']', pos1 + len);
      if (pos2 != std::string::npos)
      {
        // decipher the block
        std::vector<std::string> params = StringUtils::Split(work.substr(pos1 + len, pos2 - pos1 - len), ",");
        if (!params.empty())
        {
          CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

          int info;
          if (format == FORMATVAR || format == FORMATESCVAR)
          {
            info = infoMgr.TranslateSkinVariableString(params[0], context);
            if (info == 0)
              info = infoMgr.RegisterSkinVariableString(g_SkinInfo->CreateSkinVariable(params[0], context));
            if (info == 0) // skinner didn't define this conditional label!
              CLog::Log(LOGWARNING, "Label Formatting: $VAR[{}] is not defined", params[0]);
          }
          else
            info = infoMgr.TranslateString(params[0]);
          std::string prefix, postfix;
          if (params.size() > 1)
            prefix = params[1];
          if (params.size() > 2)
            postfix = params[2];
          infoPortion.emplace_back(info, prefix, postfix,
                                   format == FORMATESCINFO || format == FORMATESCVAR);
        }
        // and delete it from our work string
        work.erase(0, pos2 + 1);
      }
      else
      {
        CLog::Log(LOGERROR, "Error parsing label - missing ']' in \"{}\"", label);
        return;
      }
    }
  }
  while (format != NONE);

  if (!work.empty())
    infoPortion.emplace_back(0, work, "");
}

CGUIInfoLabel::CInfoPortion::CInfoPortion(int info, const std::string &prefix, const std::string &postfix, bool escaped /*= false */):
  m_prefix(prefix),
  m_postfix(postfix)
{
  m_info = info;
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
  const CGUIInfoLabel info(label, "", contextWindow);
  return info.GetLabel(contextWindow, preferImage);
}

std::string CGUIInfoLabel::GetItemLabel(const std::string &label, const CGUIListItem *item, bool preferImage /*= false */)
{ // translate the label
  const CGUIInfoLabel info(label);
  return info.GetItemLabel(item, preferImage);
}
