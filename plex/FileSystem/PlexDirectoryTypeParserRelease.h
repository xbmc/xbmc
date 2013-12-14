#ifndef PLEXDIRECTORYTYPEPARSERRELEASE_H
#define PLEXDIRECTORYTYPEPARSERRELEASE_H

#include "PlexDirectoryTypeParser.h"

class CPlexDirectoryTypeParserRelease : public CPlexDirectoryTypeParserBase
{
  public:
    virtual void Process(CFileItem &item, CFileItem &mediaContainer, XML_ELEMENT *itemElement);
};

#endif // PLEXDIRECTORYTYPEPARSERRELEASE_H
