//
//  MediaCenterLaunchHost.h
//  Plex
//
//  Created by Max Feingold on 10/27/2011.
//  Copyright 2011 Plex Inc. All rights reserved.
//

#pragma once

#include "LaunchHost.h"

class MediaCenterLaunchHost : public LaunchHost
{
public:
  virtual void OnStartup();
  virtual void OnShutdown();
};

