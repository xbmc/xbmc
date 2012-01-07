/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2010, 2011 Alexander Pipelka
 *
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <getopt.h>
#include <vdr/plugin.h>
#include "vnsi.h"

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
    return "  -t n, --timeout=n      stream data timeout in seconds (default: 10)\n";
}

bool cPluginVNSIServer::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  static struct option long_options[] = {
       { "timeout",  required_argument, NULL, 't' },
       { NULL,       no_argument,       NULL,  0  }
     };

  int c;

  while ((c = getopt_long(argc, argv, "t:", long_options, NULL)) != -1) {
        switch (c) {
          case 't': if(optarg != NULL) VNSIServerConfig.stream_timeout = atoi(optarg);
                    break;
          default:  return false;
          }
        }
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
  Server = new cVNSIServer(VNSIServerConfig.listen_port);

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
