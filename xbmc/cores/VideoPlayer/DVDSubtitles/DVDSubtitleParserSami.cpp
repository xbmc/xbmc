/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleParserSami.h"

#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDStreamInfo.h"
#include "DVDSubtitleTagSami.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

CDVDSubtitleParserSami::CDVDSubtitleParserSami(std::unique_ptr<CDVDSubtitleStream> && pStream, const std::string& filename)
    : CDVDSubtitleParserText(std::move(pStream), filename)
{

}

CDVDSubtitleParserSami::~CDVDSubtitleParserSami()
{
  Dispose();
}

bool CDVDSubtitleParserSami::Open(CDVDStreamInfo &hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  char line[1024];

  CRegExp reg(true);
  if (!reg.RegComp("<SYNC START=\"?([0-9]+)\"?>"))
    return false;

  std::string strFileName;
  std::string strClassID;
  strFileName = URIUtils::GetFileName(m_filename);

  CDVDSubtitleTagSami TagConv;
  if (!TagConv.Init())
    return false;
  TagConv.LoadHead(m_pStream.get());
  if (TagConv.m_Langclass.size() >= 2)
  {
    for (unsigned int i = 0; i < TagConv.m_Langclass.size(); i++)
    {
      if (strFileName.find(TagConv.m_Langclass[i].Name, 9) == 9)
      {
        strClassID = TagConv.m_Langclass[i].ID;
        StringUtils::ToLower(strClassID);
        break;
      }
    }
  }
  const char *lang = NULL;
  if (!strClassID.empty())
    lang = strClassID.c_str();

  CDVDOverlayText* pOverlay = NULL;
  while (m_pStream->ReadLine(line, sizeof(line)))
  {
    if ((strlen(line) > 0) && (line[strlen(line) - 1] == '\r'))
      line[strlen(line) - 1] = 0;

    int pos = reg.RegFind(line);
    const char* text = line;
    if (pos > -1)
    {
      std::string start = reg.GetMatch(1);
      if(pOverlay)
      {
        TagConv.ConvertLine(pOverlay, text, pos, lang);
        pOverlay->iPTSStopTime  = (double)atoi(start.c_str()) * DVD_TIME_BASE / 1000;
        pOverlay->Release();
        TagConv.CloseTag(pOverlay);
      }

      pOverlay = new CDVDOverlayText();
      pOverlay->Acquire(); // increase ref count with one so that we can hold a handle to this overlay

      pOverlay->iPTSStartTime = (double)atoi(start.c_str()) * DVD_TIME_BASE / 1000;
      pOverlay->iPTSStopTime  = DVD_NOPTS_VALUE;
      m_collection.Add(pOverlay);
      text += pos + reg.GetFindLen();
    }
    if(pOverlay)
      TagConv.ConvertLine(pOverlay, text, strlen(text), lang);
  }
  m_collection.Sort();
  return true;
}

