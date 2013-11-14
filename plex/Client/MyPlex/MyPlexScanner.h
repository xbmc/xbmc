//
//  MyPlexScanner.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-12.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#ifndef MYPLEXSCANNER_H
#define MYPLEXSCANNER_H

#include "Client/MyPlex/MyPlexManager.h"

class CMyPlexScanner
{
  public:
    CMyPlexScanner() {} ;
    static CMyPlexManager::EMyPlexError DoScan();
};

#endif // MYPLEXSCANNER_H
