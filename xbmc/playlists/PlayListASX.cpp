/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListASX.h"

#include "FileItem.h"
#include "PlayListFactory.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <iostream>
#include <string>

using namespace XFILE;

namespace KODI::PLAYLIST
{

bool CPlayListASX::LoadAsxIniInfo(std::istream& stream)
{
  CLog::Log(LOGINFO, "Parsing INI style ASX");

  std::string name, value;

  while (stream.good())
  {
    // consume blank rows, and blanks
    while ((stream.peek() == '\r' || stream.peek() == '\n' || stream.peek() == ' ') &&
           stream.good())
      stream.get();

    if (stream.peek() == '[')
    {
      // this is an [section] part, just ignore it
      while (stream.good() && stream.peek() != '\r' && stream.peek() != '\n')
        stream.get();
      continue;
    }
    name = "";
    value = "";
    // consume name
    while (stream.peek() != '\r' && stream.peek() != '\n' && stream.peek() != '=' && stream.good())
      name += stream.get();

    // consume =
    if (stream.get() != '=')
      continue;

    // consume value
    while (stream.peek() != '\r' && stream.peek() != '\n' && stream.good())
      value += stream.get();

    CLog::Log(LOGINFO, "Adding element {}={}", name, value);
    CFileItemPtr newItem(new CFileItem(value));
    newItem->SetPath(value);
    if (VIDEO::IsVideo(*newItem) &&
        !newItem->HasVideoInfoTag()) // File is a video and needs a VideoInfoTag
      newItem->GetVideoInfoTag()->Reset(); // Force VideoInfoTag creation
    Add(newItem);
  }

  return true;
}

bool CPlayListASX::LoadData(std::istream& stream)
{
  CLog::Log(LOGINFO, "Parsing ASX");

  if (stream.peek() == '[')
  {
    return LoadAsxIniInfo(stream);
  }
  else
  {
    std::string asxStream(std::istreambuf_iterator<char>(stream), {});
    CXBMCTinyXML xmlDoc;
    xmlDoc.Parse(asxStream, TIXML_DEFAULT_ENCODING);

    if (xmlDoc.Error())
    {
      CLog::Log(LOGERROR, "Unable to parse ASX info Error: {}", xmlDoc.ErrorDesc());
      return false;
    }

    TiXmlElement* pRootElement = xmlDoc.RootElement();

    if (!pRootElement)
      return false;

    // lowercase every element
    TiXmlNode* pNode = pRootElement;
    TiXmlNode* pChild = NULL;
    std::string value;
    value = pNode->Value();
    StringUtils::ToLower(value);
    pNode->SetValue(value);
    while (pNode)
    {
      pChild = pNode->IterateChildren(pChild);
      if (pChild)
      {
        if (pChild->Type() == TiXmlNode::TINYXML_ELEMENT)
        {
          value = pChild->Value();
          StringUtils::ToLower(value);
          pChild->SetValue(value);

          TiXmlAttribute* pAttr = pChild->ToElement()->FirstAttribute();
          while (pAttr)
          {
            value = pAttr->Name();
            StringUtils::ToLower(value);
            pAttr->SetName(value);
            pAttr = pAttr->Next();
          }
        }

        pNode = pChild;
        pChild = NULL;
        continue;
      }

      pChild = pNode;
      pNode = pNode->Parent();
    }
    std::string roottitle;
    TiXmlElement* pElement = pRootElement->FirstChildElement();
    while (pElement)
    {
      value = pElement->Value();
      if (value == "title" && !pElement->NoChildren())
      {
        roottitle = pElement->FirstChild()->ValueStr();
      }
      else if (value == "entry")
      {
        std::string title(roottitle);

        TiXmlElement* pRef = pElement->FirstChildElement("ref");
        TiXmlElement* pTitle = pElement->FirstChildElement("title");

        if (pTitle && !pTitle->NoChildren())
          title = pTitle->FirstChild()->ValueStr();

        while (pRef)
        { // multiple references may appear for one entry
          // duration may exist on this level too
          value = XMLUtils::GetAttribute(pRef, "href");
          if (!value.empty())
          {
            if (title.empty())
              title = value;

            CLog::Log(LOGINFO, "Adding element {}, {}", title, value);
            CFileItemPtr newItem(new CFileItem(title));
            newItem->SetPath(value);
            Add(newItem);
          }
          pRef = pRef->NextSiblingElement("ref");
        }
      }
      else if (value == "entryref")
      {
        value = XMLUtils::GetAttribute(pElement, "href");
        if (!value.empty())
        { // found an entryref, let's try loading that url
          std::unique_ptr<CPlayList> playlist(CPlayListFactory::Create(value));
          if (nullptr != playlist)
            if (playlist->Load(value))
              Add(*playlist);
        }
      }
      pElement = pElement->NextSiblingElement();
    }
  }

  return true;
}
} // namespace KODI::PLAYLIST
