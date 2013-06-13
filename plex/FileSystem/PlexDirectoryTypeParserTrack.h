#pragma once

#include "PlexDirectoryTypeParserVideo.h"
#include "XBMCTinyXML.h"

class CPlexDirectoryTypeParserTrack : public CPlexDirectoryTypeParserVideo
{
public:
  CPlexDirectoryTypeParserTrack() {}
  virtual void Process(CFileItem& item, CFileItem& mediaContainer, TiXmlElement* itemElement);
};