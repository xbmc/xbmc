/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include <sys/types.h>
#include <sys/sysctl.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <string>
#include <Carbon/Carbon.h>


using namespace std;

#include "XBMCHelper.h"

#include "PlatformDefs.h"
#include "log.h"
#include "system.h"
#include "Settings.h"
#include "Util.h"
#include "XFileUtils.h"
#include "utils/SystemInfo.h"

XBMCHelper g_xbmcHelper;

#define XBMC_HELPER_PROGRAM "XBMCHelper"
#define SOFA_CONTROL_PROGRAM "Sofa Control"
#define XBMC_LAUNCH_PLIST "org.xbmc.helper.plist"

static int GetBSDProcessList(kinfo_proc **procList, size_t *procCount);

/////////////////////////////////////////////////////////////////////////////
XBMCHelper::XBMCHelper()
  : m_alwaysOn(false)
  , m_mode(APPLE_REMOTE_DISABLED)
  , m_sequenceDelay(0)
  , m_errorStarting(false)
{
  CStdString homePath;
  CUtil::GetHomePath(homePath);

  // Compute the helper filename.
  m_helperFile = homePath + "/XBMCHelper";

  // Compute the local (pristine) launch agent filename.
  m_launchAgentLocalFile = homePath + "/" XBMC_LAUNCH_PLIST;

  // Compute the install path for the launch agent.
  m_launchAgentInstallFile = getenv("HOME");
  m_launchAgentInstallFile += "/Library/LaunchAgents/" XBMC_LAUNCH_PLIST;

  // Compute the configuration file name.
  m_configFile = getenv("HOME");
  m_configFile += "/Library/Application Support/XBMC/XBMCHelper.conf";
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Start()
{
  printf("Asking helper to start.\n");
  int pid = GetProcessPid(XBMC_HELPER_PROGRAM);
  if (pid == -1)
  {
    string cmd = "\"" + m_helperFile + "\" &";
    system(cmd.c_str());
  }
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Stop()
{
  printf("Asked to stop\n");

  // Kill the process.
  int pid = GetProcessPid(XBMC_HELPER_PROGRAM);
  if (pid != -1)
    kill(pid, SIGKILL);
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Configure()
{
  int oldMode = m_mode;
  int oldDelay = m_sequenceDelay;
  int oldAlwaysOn = m_alwaysOn;

  // Read the new configuration.
  m_errorStarting = false;
  m_mode = g_guiSettings.GetInt("appleremote.mode");
  m_sequenceDelay = g_guiSettings.GetInt("appleremote.sequencetime");
  m_alwaysOn = g_guiSettings.GetBool("appleremote.alwayson");

  // Don't let it enable if sofa control or remote buddy is around.
  if (IsRemoteBuddyInstalled() || IsSofaControlRunning())
  {
    // If we were starting then remember error.
    if (oldMode == APPLE_REMOTE_DISABLED && m_mode != APPLE_REMOTE_DISABLED)
      m_errorStarting = true;

    m_mode = APPLE_REMOTE_DISABLED;
    g_guiSettings.SetInt("appleremote.mode", APPLE_REMOTE_DISABLED);
  }

  // New configuration.
  if (oldMode != m_mode || oldDelay != m_sequenceDelay)
  {
    // Build a new config string.
    std::string strConfig;
    if (m_mode == APPLE_REMOTE_UNIVERSAL)
      strConfig = "--universal ";

    char strDelay[64];
    sprintf(strDelay, "--timeout %d", m_sequenceDelay);
    strConfig += strDelay;

    // Write the new configuration.
    WriteFile(m_configFile.c_str(), strConfig);

    // If process is running, kill -HUP to have it reload settings.
    int pid = GetProcessPid(XBMC_HELPER_PROGRAM);
    if (pid != -1)
      kill(pid, SIGHUP);
  }

  // Turning off?
  if (oldMode != APPLE_REMOTE_DISABLED && m_mode == APPLE_REMOTE_DISABLED)
  {
    Stop();
    Uninstall();
  }

  // Turning on.
  if (oldMode == APPLE_REMOTE_DISABLED && m_mode != APPLE_REMOTE_DISABLED)
    Start();

  // Installation/uninstallation.
  if (oldAlwaysOn == false && m_alwaysOn == true)
    Install();
  if (oldAlwaysOn == true && m_alwaysOn == false)
    Uninstall();
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Install()
{
  // Make sure directory exists.
  string strDir = getenv("HOME");
  strDir += "/Library/LaunchAgents";
  CreateDirectory(strDir.c_str(), NULL);

  // Load template.
  string plistData = ReadFile(m_launchAgentLocalFile.c_str());

  if (plistData != "") 
  {
      // Replace it in the file.
      int start = plistData.find("${PATH}");
      plistData.replace(start, 7, m_helperFile.c_str(), m_helperFile.length());

      // Install it.
      WriteFile(m_launchAgentInstallFile.c_str(), plistData);

      // Load it.
      int pid = GetProcessPid(XBMC_HELPER_PROGRAM);
      if (pid == -1)
      {
          string cmd = "/bin/launchctl load ";
          cmd += m_launchAgentInstallFile;
          system(cmd.c_str());
      }
  }
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Uninstall()
{
  // Call the unloader.
  string cmd = "/bin/launchctl unload ";
  cmd += m_launchAgentInstallFile;
  system(cmd.c_str());

  // Remove the plist file.
  DeleteFile(m_launchAgentInstallFile.c_str());
}

/////////////////////////////////////////////////////////////////////////////
bool XBMCHelper::IsRunning()
{
  return (GetProcessPid(XBMC_HELPER_PROGRAM)!=-1);
}

/////////////////////////////////////////////////////////////////////////////
bool XBMCHelper::IsAppleTV()
{
  char        buffer[512];
  size_t      len = 512;
  std::string hw_model = "unknown";
  
  if (sysctlbyname("hw.model", &buffer, &len, NULL, 0) == 0)
    hw_model = buffer;
  
  if (hw_model.find("AppleTV") != std::string::npos)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::CaptureAllInput()
{
  // Take keyboard focus away from FrontRow and native screen saver
  if (IsAppleTV())
  {
    ProcessSerialNumber psn = {0, kCurrentProcess};
       
    SetFrontProcess(&psn);
    EnableSecureEventInput();
  }
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::ReleaseAllInput()
{
  // Give keyboard focus back to FrontRow and native screen saver
  if (IsAppleTV())
  {
    DisableSecureEventInput();
  }
}

/////////////////////////////////////////////////////////////////////////////
bool XBMCHelper::IsRemoteBuddyInstalled()
{
  return false;
  // Check for existence of kext file.
  return access("/System/Library/Extensions/RBIOKitHelper.kext", R_OK) != -1;
}

/////////////////////////////////////////////////////////////////////////////
bool XBMCHelper::IsSofaControlRunning()
{
  return false;
  // Check for a "Sofa Control" process running.
  return GetProcessPid(SOFA_CONTROL_PROGRAM) != -1;
}

/////////////////////////////////////////////////////////////////////////////
std::string XBMCHelper::ReadFile(const char* fileName)
{
  std::string ret = "";
  ifstream is;
  
  is.open(fileName);
  if( is.good() )
  {
    // Get length of file:
    is.seekg (0, ios::end);
    int length = is.tellg();
    is.seekg (0, ios::beg);

    // Allocate memory:
    char* buffer = new char [length+1];

    // Read data as a block:
    is.read(buffer,length);
    is.close();
    buffer[length] = '\0';

    ret = buffer;
    delete[] buffer;
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::WriteFile(const char* fileName, const std::string& data)
{
  ofstream out(fileName);
  if (!out)
  {
    CLog::Log(LOGERROR, "XBMCHelper: Unable to open file '%s'", fileName);
  }
  else
  {
    // Write new configuration.
    out << data << endl;
    out.flush();
    out.close();
  }
}

/////////////////////////////////////////////////////////////////////////////
int XBMCHelper::GetProcessPid(const char* strProgram)
{
  kinfo_proc* mylist = (kinfo_proc *)malloc(sizeof(kinfo_proc));
  size_t mycount = 0;
  int ret = -1;

  GetBSDProcessList(&mylist, &mycount);
  for (size_t k = 0; k < mycount && ret == -1; k++)
  {
    kinfo_proc *proc = NULL;
    proc = &mylist[k];

    if (strcmp(proc->kp_proc.p_comm, strProgram) == 0)
    {
      //if (ignorePid == 0 || ignorePid != proc->kp_proc.p_pid)
      ret = proc->kp_proc.p_pid;
    }
  }

  free (mylist);

  return ret;
}

typedef struct kinfo_proc kinfo_proc;

// Returns a list of all BSD processes on the system.  This routine
// allocates the list and puts it in *procList and a count of the
// number of entries in *procCount.  You are responsible for freeing
// this list (use "free" from System framework).
// On success, the function returns 0.
// On error, the function returns a BSD errno value.
//
static int GetBSDProcessList(kinfo_proc **procList, size_t *procCount)
{
  int err;
  kinfo_proc * result;
  bool done;
  static const int name[] =
  { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };

  // Declaring name as const requires us to cast it when passing it to
  // sysctl because the prototype doesn't include the const modifier.
  size_t length;

  assert(procList != NULL);
  assert(procCount != NULL);

  *procCount = 0;

  // We start by calling sysctl with result == NULL and length == 0.
  // That will succeed, and set length to the appropriate length.
  // We then allocate a buffer of that size and call sysctl again
  // with that buffer.  If that succeeds, we're done.  If that fails
  // with ENOMEM, we have to throw away our buffer and loop.  Note
  // that the loop causes use to call sysctl with NULL again; this
  // is necessary because the ENOMEM failure case sets length to
  // the amount of data returned, not the amount of data that
  // could have been returned.
  //
  result = NULL;
  done = false;
  do
  {
    assert(result == NULL);

    // Call sysctl with a NULL buffer.
    length = 0;
    err = sysctl((int *) name, (sizeof(name) / sizeof(*name)) - 1, NULL,
        &length, NULL, 0);
    if (err == -1)
      err = errno;

    // Allocate an appropriately sized buffer based on the results from the previous call.
    if (err == 0)
    {
      result = (kinfo_proc*) malloc(length);
      if (result == NULL)
        err = ENOMEM;
    }

    // Call sysctl again with the new buffer.  If we get an ENOMEM
    // error, toss away our buffer and start again.
    //
    if (err == 0)
    {
      err = sysctl((int *) name, (sizeof(name) / sizeof(*name)) - 1, result,
          &length, NULL, 0);

      if (err == -1)
        err = errno;
      else if (err == 0)
        done = true;
      else if (err == ENOMEM)
      {
        assert(result != NULL);
        free(result);
        result = NULL;
        err = 0;
      }
    }
  } while (err == 0 && !done);

  // Clean up and establish post conditions.
  if (err != 0 && result != NULL)
  {
    free(result);
    result = NULL;
  }

  *procList = result;
  if (err == 0)
    *procCount = length / sizeof(kinfo_proc);

  assert( (err == 0) == (*procList != NULL) );
  return err;
}
