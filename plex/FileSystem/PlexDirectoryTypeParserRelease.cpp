#include "PlexDirectoryTypeParserRelease.h"
#include "PlexDirectory.h"
#include "PlexAttributeParser.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectoryTypeParserRelease::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  for (TiXmlElement* package = itemElement->FirstChildElement(); package ; package = package->NextSiblingElement())
  {
    CFileItemPtr pItem = XFILE::CPlexDirectory::NewPlexElement(package, item, item.GetPath());
    if(!pItem)
      continue;

    CPlexAttributeParserKey key;

    key.Process(mediaContainer.GetPath(), "filePath", pItem->GetProperty("file").asString(), pItem.get());
    key.Process(mediaContainer.GetPath(), "manifestPath", pItem->GetProperty("manifest").asString(), pItem.get());

    item.m_mediaItems.push_back(pItem);
  }
}
