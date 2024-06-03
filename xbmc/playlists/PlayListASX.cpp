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
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <iostream>
#include <string>

#include <tinyxml2.h>

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

    CXBMCTinyXML2 xmlDoc;
    xmlDoc.Parse(asxStream);

    if (xmlDoc.Error())
    {
      CLog::Log(LOGERROR, "Unable to parse ASX info Error: {}", xmlDoc.ErrorStr());
      return false;
    }

    auto* srcRootElement = xmlDoc.RootElement();

    if (!srcRootElement)
      return false;

    // lowercase every element - copy to second  temp doc
    tinyxml2::XMLDocument targetDoc;
    std::string value = srcRootElement->Value();

    StringUtils::ToLower(value);
    auto targetRootElement = targetDoc.NewElement(value.c_str());

    auto* rootAttrib = srcRootElement->ToElement()->FirstAttribute();
    while (rootAttrib)
    {
      std::string attribName = rootAttrib->Name();
      auto attribValue = rootAttrib->Value();
      StringUtils::ToLower(attribName);
      targetRootElement->SetAttribute(attribName.c_str(), attribValue);
      rootAttrib = rootAttrib->Next();
    }

    auto* sourceNode = srcRootElement->FirstChild();
    while (sourceNode)
    {
      // Function to check all child elements and lowercase the elem/attrib names
      recurseLowercaseNames(*targetRootElement, sourceNode);

      sourceNode = sourceNode->NextSiblingElement();
    }

    targetDoc.InsertFirstChild(targetRootElement);

    // now data is lowercased, we can parse contents
    std::string roottitle;
    auto* element = targetDoc.RootElement()->FirstChildElement();
    while (element)
    {
      value = element->Value();
      if (value == "title" && !element->NoChildren())
      {
        roottitle = element->FirstChild()->Value();
      }
      else if (value == "entry")
      {
        std::string title(roottitle);

        auto* refElement = element->FirstChildElement("ref");
        auto* titleElement = element->FirstChildElement("title");

        if (titleElement && !titleElement->NoChildren())
          title = titleElement->FirstChild()->Value();

        while (refElement)
        { // multiple references may appear for one entry
          // duration may exist on this level too
          value = XMLUtils::GetAttribute(refElement, "href");
          if (!value.empty())
          {
            if (title.empty())
              title = value;

            CLog::Log(LOGINFO, "Adding element {}, {}", title, value);
            CFileItemPtr newItem(new CFileItem(title));
            newItem->SetPath(value);
            Add(newItem);
          }
          refElement = refElement->NextSiblingElement("ref");
        }
      }
      else if (value == "entryref")
      {
        value = XMLUtils::GetAttribute(element, "href");
        if (!value.empty())
        { // found an entryref, let's try loading that url
          std::unique_ptr<CPlayList> playlist(CPlayListFactory::Create(value));
          if (nullptr != playlist)
            if (playlist->Load(value))
              Add(*playlist);
        }
      }
      element = element->NextSiblingElement();
    }
  }

  return true;
}

void CPlayListASX::recurseLowercaseNames(tinyxml2::XMLNode& targetNode,
                                         tinyxml2::XMLNode* sourceNode)
{
  if (sourceNode->ToElement())
  {
    std::string strNodeValue = sourceNode->Value();
    StringUtils::ToLower(strNodeValue);
    auto* targetElement = targetNode.GetDocument()->NewElement(strNodeValue.c_str());

    auto* attrib = sourceNode->ToElement()->FirstAttribute();
    while (attrib)
    {
      std::string attribName = attrib->Name();
      auto attribValue = attrib->Value();
      StringUtils::ToLower(attribName);
      targetElement->SetAttribute(attribName.c_str(), attribValue);
      attrib = attrib->Next();
    }

    if (!sourceNode->NoChildren())
    {
      for (auto* child = sourceNode->FirstChild(); child != nullptr; child = child->NextSibling())
      {
        recurseLowercaseNames(*targetElement, child);
      }
    }

    targetNode.InsertEndChild(targetElement);
    return;
  }
  else if (sourceNode->ToText())
  {
    auto* sourceTextElement = sourceNode->ToText();
    auto* sourceText = sourceTextElement->Value();
    auto* targetText = targetNode.GetDocument()->NewText(sourceText);

    if (!sourceNode->NoChildren())
    {
      for (auto* child = sourceNode->FirstChildElement(); child != nullptr;
           child = child->NextSiblingElement())
      {
        recurseLowercaseNames(*targetText, child);
      }
    }

    targetNode.InsertEndChild(targetText);
    return;
  }
}

} // namespace KODI::PLAYLIST
