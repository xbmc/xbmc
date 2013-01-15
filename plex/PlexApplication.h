/*
 *  PlexApplication.h
 *  XBMC
 *
 *  Created by Jamie Kirkpatrick on 20/01/2011.
 *  Copyright 2011 Plex Inc. All rights reserved.
 *
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include "Network/PlexNetworkServices.h"

#include "guilib/IMsgTargetCallback.h"
#include "AutoUpdate/PlexAutoUpdate.h"
#include "threads/Thread.h"
#include "GlobalsHandling.h"

#ifdef TARGET_DARWIN_OSX
#include "Helper/PlexHelper.h"
#endif

class PlexServiceListener;
typedef boost::shared_ptr<PlexServiceListener> PlexServiceListenerPtr;

///
/// The hub of all Plex goodness.
///
class PlexApplication : public IMsgTargetCallback
{
public:
  PlexApplication() {}
  void Start();

  /// Destructor
  virtual ~PlexApplication();
  
  /// Handle internal messages.
  virtual bool OnMessage(CGUIMessage& message);

  void OnWakeUp();

  void ForceVersionCheck()
  {
    m_autoUpdater->ForceCheckInBackground();
  }

  PlexServiceListenerPtr GetServiceListener() const { return m_serviceListener; }
      
private:
  /// Members
  PlexServiceListenerPtr m_serviceListener;
  CPlexAutoUpdate* m_autoUpdater;
};

XBMC_GLOBAL_REF(PlexApplication, g_plexApplication);
#define g_plexApplication XBMC_GLOBAL_USE(PlexApplication)
