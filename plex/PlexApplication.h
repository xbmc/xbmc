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
#include "Client/PlexNetworkServiceBrowser.h"

#include "guilib/IMsgTargetCallback.h"
#include "AutoUpdate/PlexAutoUpdate.h"
#include "threads/Thread.h"
#include "GlobalsHandling.h"

#ifdef TARGET_DARWIN_OSX
#include "Helper/PlexHTHelper.h"
#endif

class CPlexServiceListener;
typedef boost::shared_ptr<CPlexServiceListener> CPlexServiceListenerPtr;

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

  CPlexServiceListenerPtr GetServiceListener() const { return m_serviceListener; }
      
private:
  /// Members
  CPlexServiceListenerPtr m_serviceListener;
  CPlexAutoUpdate* m_autoUpdater;
};

XBMC_GLOBAL_REF(PlexApplication, g_plexApplication);
#define g_plexApplication XBMC_GLOBAL_USE(PlexApplication)
