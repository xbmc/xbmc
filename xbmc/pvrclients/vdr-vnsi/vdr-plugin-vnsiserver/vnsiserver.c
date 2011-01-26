/*
 * vnsiserver.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "server.h"

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "VDR-Network-Streaming-Interface (VNSI) Server";

class cPluginVNSIServer : public cPlugin {
private:
  cServer *Server;

public:
  cPluginVNSIServer(void);
  virtual ~cPluginVNSIServer();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
  virtual const char *MainMenuEntry(void) { return NULL; }
  virtual cOsdObject *MainMenuAction(void) { return NULL; }
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginVNSIServer::cPluginVNSIServer(void)
{
  Server = NULL;
}

cPluginVNSIServer::~cPluginVNSIServer()
{
  // Clean up after yourself!
}

const char *cPluginVNSIServer::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginVNSIServer::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginVNSIServer::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  VNSIServerConfig.ConfigDirectory = ConfigDirectory(PLUGIN_NAME_I18N);
  return true;
}

bool cPluginVNSIServer::Start(void)
{
  Server = new cServer(VNSIServerConfig.listen_port);

  return true;
}

void cPluginVNSIServer::Stop(void)
{
  delete Server;
  Server = NULL;
}

void cPluginVNSIServer::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginVNSIServer::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginVNSIServer::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

time_t cPluginVNSIServer::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

cMenuSetupPage *cPluginVNSIServer::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginVNSIServer::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginVNSIServer::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginVNSIServer::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginVNSIServer::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginVNSIServer); // Don't touch this!
