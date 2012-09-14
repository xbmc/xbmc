//
//  LaunchHost.h
//  Plex
//
//  Created by Max Feingold on 10/27/2011.
//  Copyright 2011 Plex Inc. All rights reserved.
//

#pragma once

// A very simple notification interface
class LaunchHost
{
public:
  virtual ~LaunchHost() {}
  
  virtual void OnStartup() = 0;
  virtual void OnShutdown() = 0;
};

LaunchHost* DetectLaunchHost();