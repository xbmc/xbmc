//
//  MyPlexPinInfo.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-12.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#ifndef MYPLEXPININFO_H
#define MYPLEXPININFO_H

#include <string>
#include "XBDateTime.h"
#include "XBMCTinyXML.h"

class CMyPlexPinInfo
{
  public:
    CMyPlexPinInfo() {}

    bool SetFromXmlElement(TiXmlElement* element);

    std::string clientid;
    CDateTime expiresAt;
    std::string code;
    int id;
    std::string authToken;

    void operator=(const CMyPlexPinInfo& other)
    {
      clientid = other.clientid;
      expiresAt = other.expiresAt;
      code = other.code;
      id = other.id;
      authToken = other.authToken;
    }
};

#endif // MYPLEXPININFO_H
