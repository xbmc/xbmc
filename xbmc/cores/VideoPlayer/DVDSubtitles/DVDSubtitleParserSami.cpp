/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleParserSami.h"

#include "DVDStreamInfo.h"
#include "DVDSubtitleTagSami.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

CDVDSubtitleParserSami::CDVDSubtitleParserSami(std::unique_ptr<CDVDSubtitleStream>&& pStream,
                                               const std::string& filename)
  : CDVDSubtitleParserText(std::move(pStream), filename, "SAMI Subtitle Parser")
{
}

bool CDVDSubtitleParserSami::Open(CDVDStreamInfo& hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  if (!Initialize())
    return false;

  CRegExp regLine(true);
  if (!regLine.RegComp("<SYNC START=\"?([0-9]+)\"?>(.+)?"))
    return false;
  CRegExp regClassID(true);
  if (!regClassID.RegComp("<P Class=\"?([\\w\\d]+)\"?>"))
    return false;

  std::string strFileName;
  std::string strClassID;
  strFileName = StringUtils::ToLower(URIUtils::GetFileName(m_filename));

  CDVDSubtitleTagSami TagConv;
  if (!TagConv.Init())
    return false;

  TagConv.LoadHead(m_pStream.get());

  // If there are more languages contained in a file,
  // try getting the language class ID that matches the language name
  // specified in the filename
  if (TagConv.m_Langclass.size() >= 2)
  {
    for (unsigned int i = 0; i < TagConv.m_Langclass.size(); i++)
    {
      std::string langName = TagConv.m_Langclass[i].Name;
      StringUtils::ToLower(langName);
      if (strFileName.find(langName) != std::string::npos)
      {
        strClassID = TagConv.m_Langclass[i].ID;
        break;
      }
    }
    // No language specified or found, try to select the first class ID
    if (strClassID.empty() && !(TagConv.m_Langclass.empty()))
    {
      strClassID = TagConv.m_Langclass[0].ID;
    }
  }

  const char* langClassID{nullptr};
  if (!strClassID.empty())
  {
    StringUtils::ToLower(strClassID);
    langClassID = strClassID.c_str();
  }

  int prevSubId = NO_SUBTITLE_ID;
  double lastPTSStartTime = 0;
  std::string lastLangClassID;
  // SAMI synchronization provides only the start time value,
  // for the stop time it takes in consideration the start time of the next line,
  // that, could, be an empty string with a "&nbsp;" tag.
  // Last line could not have the stop time then we set as default 4 secs.
  int defaultDuration = 4 * DVD_TIME_BASE;
  std::string line;

  while (m_pStream->ReadLine(line))
  {
    // Find the language Class ID in current line (if exist)
    if (regClassID.RegFind(line) > -1)
    {
      lastLangClassID = regClassID.GetMatch(1);
      StringUtils::ToLower(lastLangClassID);
    }

    int pos = regLine.RegFind(line);
    if (pos > -1) // Sync tag found
    {
      double currStartTime = static_cast<double>(std::atoi(regLine.GetMatch(1).c_str()));
      double currPTSStartTime = currStartTime * DVD_TIME_BASE / 1000;

      // We set the duration for the previous line (Event) by using the current start time
      ChangeSubtitleStopTime(prevSubId, currPTSStartTime);

      // We try to get text after Sync tag (if exists)
      std::string text = regLine.GetMatch(2);
      if (text.empty())
      {
        prevSubId = NO_SUBTITLE_ID;
      }
      else
      {
        TagConv.ConvertLine(text, langClassID);
        TagConv.CloseTag(text);
        prevSubId = AddSubtitle(text, currPTSStartTime, currPTSStartTime + defaultDuration);
      }

      lastPTSStartTime = currPTSStartTime;
    }
    else
    {
      // Lines without Sync tag e.g. for multiple styles or lines,
      // need to be appended to last line added with sync tag
      // but they have to match the current language Class ID (if set)
      if (!strClassID.empty() && strClassID != lastLangClassID)
        continue;

      std::string text(line);
      TagConv.ConvertLine(text, langClassID);
      TagConv.CloseTag(text);
      if (prevSubId != NO_SUBTITLE_ID)
      {
        text.insert(0, "\n");
        AppendToSubtitle(prevSubId, text.c_str());
      }
      else
      {
        prevSubId = AddSubtitle(text, lastPTSStartTime, lastPTSStartTime + defaultDuration);
      }
    }
  }

  m_collection.Add(CreateOverlay());

  return true;
}
