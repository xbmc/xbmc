/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if defined(__APPLE__) && !defined(__arm__)
#include <fstream>
#include <signal.h>
#include <sstream>
#include <mach-o/dyld.h>

#include "XBMCHelper.h"
#include "PlatformDefs.h"
#include "Util.h"

#include "utils/log.h"
#include "system.h"
#include "settings/GUISettings.h"
#include "utils/SystemInfo.h"

#include "threads/Atomics.h"

static long sg_singleton_lock_variable = 0;
XBMCHelper* XBMCHelper::smp_instance = 0;

#define XBMC_HELPER_PROGRAM "XBMCHelper"
#define SOFA_CONTROL_PROGRAM "Sofa Control"
#define XBMC_LAUNCH_PLIST "org.xbmc.helper.plist"

static int GetBSDProcessList(kinfo_proc **procList, size_t *procCount);

XBMCHelper&
XBMCHelper::GetInstance()
{
  CAtomicSpinLock lock(sg_singleton_lock_variable);
  if( ! smp_instance )
  {
    smp_instance = new XBMCHelper();
  }
  return *smp_instance;
}

/////////////////////////////////////////////////////////////////////////////
XBMCHelper::XBMCHelper()
  : m_alwaysOn(false)
  , m_mode(APPLE_REMOTE_DISABLED)
  , m_sequenceDelay(0)
  , m_port(0)
  , m_errorStarting(false)
{
  // Compute the XBMC_HOME path.
  CStdString homePath;
  CUtil::GetHomePath(homePath);
  m_homepath = homePath;

  // Compute the helper filename.
  m_helperFile = m_homepath + "/tools/darwin/runtime/";
  m_helperFile += XBMC_HELPER_PROGRAM;
  
  // Compute the local (pristine) launch agent filename.
  m_launchAgentLocalFile = m_homepath + "/tools/darwin/runtime/";
  m_launchAgentLocalFile += XBMC_LAUNCH_PLIST;

  // Compute the install path for the launch agent.
  // not to be confused with app home, this is user home
  m_launchAgentInstallFile = getenv("HOME");
  m_launchAgentInstallFile += "/Library/LaunchAgents/";
  m_launchAgentInstallFile += XBMC_LAUNCH_PLIST;

  // Compute the configuration file name.
  m_configFile = getenv("HOME");
  m_configFile += "/Library/Application Support/XBMC/XBMCHelper.conf";
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Start()
{
  int pid = GetProcessPid(XBMC_HELPER_PROGRAM);
  if (pid == -1)
  {
    //printf("Asking helper to start.\n");
    // use -x to have XBMCHelper read its configure file
    std::string cmd = "\"" + m_helperFile + "\" -x &";
    system(cmd.c_str());
  }
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Stop()
{

  // Kill the process.
  int pid = GetProcessPid(XBMC_HELPER_PROGRAM);
  if (pid != -1)
  {
    printf("Asked to stop\n");
    kill(pid, SIGKILL);
  }
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Configure()
{
  int oldMode = m_mode;
  int oldDelay = m_sequenceDelay;
  int oldAlwaysOn = m_alwaysOn;
  int oldPort = m_port;

  // Read the new configuration.
  m_errorStarting = false;
  m_mode = g_guiSettings.GetInt("input.appleremotemode");
  m_sequenceDelay = g_guiSettings.GetInt("input.appleremotesequencetime");
  m_alwaysOn = g_guiSettings.GetBool("input.appleremotealwayson");
  CStdString port_string = g_guiSettings.GetString("services.esport");
  m_port = atoi(port_string.c_str());


  // Don't let it enable if sofa control or remote buddy is around.
  if (IsRemoteBuddyInstalled() || IsSofaControlRunning())
  {
    // If we were starting then remember error.
    if (oldMode == APPLE_REMOTE_DISABLED && m_mode != APPLE_REMOTE_DISABLED)
      m_errorStarting = true;

    m_mode = APPLE_REMOTE_DISABLED;
    g_guiSettings.SetInt("input.appleremotemode", APPLE_REMOTE_DISABLED);
  }

  // New configuration.
  if (oldMode != m_mode || oldDelay != m_sequenceDelay || oldPort != m_port)
  {
    // Build a new config string.
    std::string strConfig;
    switch (m_mode) {
      case APPLE_REMOTE_UNIVERSAL:
        strConfig = "--universal ";
        break;
      case APPLE_REMOTE_MULTIREMOTE:
        strConfig = "--multiremote ";
        break;
      default:
        break;
    }
    std::stringstream strPort;
    strPort << "--port " << m_port;
    strConfig += strPort.str();

#ifdef _DEBUG
    strConfig += "--verbose ";
#endif
    char strDelay[64];
    sprintf(strDelay, "--timeout %d ", m_sequenceDelay);
    strConfig += strDelay;

    // Find out where we're running from.
    char real_path[2*MAXPATHLEN];
    char given_path[2*MAXPATHLEN];
    uint32_t path_size = 2*MAXPATHLEN;

    if (_NSGetExecutablePath(given_path, &path_size) == 0)
    {
      if (realpath(given_path, real_path) != NULL)
      {
        strConfig += "--appPath \"";
        strConfig += real_path;
        strConfig += "\" ";

        strConfig += "--appHome \"";
        strConfig += m_homepath;
        strConfig += "\" ";
      }
    }

    // Write the new configuration.
    strConfig + "\n";
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
  std::string strDir = getenv("HOME");
  strDir += "/Library/LaunchAgents";
  CreateDirectory(strDir.c_str(), NULL);

  // Load template.
  std::string plistData = ReadFile(m_launchAgentLocalFile.c_str());

  if (plistData != "") 
  {
      std::string launchd_args;

      // Replace PATH with path to app.
      int start = plistData.find("${PATH}");
      plistData.replace(start, 7, m_helperFile.c_str(), m_helperFile.length());

      // Replace ARG1 with a single argument, additional args 
      // will need ARG2, ARG3 added to plist.
      launchd_args = "-x";
      start = plistData.find("${ARG1}");
      plistData.replace(start, 7, launchd_args.c_str(), launchd_args.length());

      // Install it.
      WriteFile(m_launchAgentInstallFile.c_str(), plistData);

      // Load it if not running already.
      int pid = GetProcessPid(XBMC_HELPER_PROGRAM);
      if (pid == -1)
      {
          std::string cmd = "/bin/launchctl load ";
          cmd += m_launchAgentInstallFile;
          system(cmd.c_str());
      }
  }
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Uninstall()
{
  // Call the unloader.
  std::string cmd = "/bin/launchctl unload ";
  cmd += m_launchAgentInstallFile;
  system(cmd.c_str());
  
  //this also stops the helper, so restart it here again, if not disabled
  if(m_mode != APPLE_REMOTE_DISABLED)
    Start();

  // Remove the plist file.
  DeleteFile(m_launchAgentInstallFile.c_str());
}

/////////////////////////////////////////////////////////////////////////////
bool XBMCHelper::IsRunning()
{
  return (GetProcessPid(XBMC_HELPER_PROGRAM)!=-1);
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
  std::ifstream is;
  
  is.open(fileName);
  if( is.good() )
  {
    // Get length of file:
    is.seekg(0, std::ios::end);
    int length = is.tellg();
    is.seekg(0, std::ios::beg);

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
  std::ofstream out(fileName);
  if (!out)
  {
    CLog::Log(LOGERROR, "XBMCHelper: Unable to open file '%s'", fileName);
  }
  else
  {
    // Write new configuration.
    out << data << std::endl;
    out.flush();
    out.close();
  }
}

/////////////////////////////////////////////////////////////////////////////
int XBMCHelper::GetProcessPid(const char* strProgram)
{
  kinfo_proc* mylist = 0;
  size_t mycount = 0;
  int ret = -1;

  GetBSDProcessList(&mylist, &mycount);
  for (size_t k = 0; k < mycount && ret == -1; k++)
  {
    kinfo_proc *proc = NULL;
    proc = &mylist[k];

    // Process names are at most sixteen characters long.
    if (strncmp(proc->kp_proc.p_comm, strProgram, 16) == 0)
    {
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
  // example from http://developer.apple.com/qa/qa2001/qa1123.html
  int err;
  kinfo_proc * result;
  bool done;
  static const int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };

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
        
      if (err == 0)
      {
        done = true;
      }
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
#endif
