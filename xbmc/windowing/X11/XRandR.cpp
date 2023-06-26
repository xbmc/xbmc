/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XRandR.h"

#include "CompileInfo.h"
#include "threads/SystemClock.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <string.h>

#include <sys/wait.h>

#include "PlatformDefs.h"

#if defined(TARGET_FREEBSD)
#include <sys/types.h>
#include <sys/wait.h>
#endif

using namespace std::chrono_literals;

CXRandR::CXRandR(bool query)
{
  m_bInit = false;
  m_numScreens = 1;
  if (query)
    Query();
}

bool CXRandR::Query(bool force, bool ignoreoff)
{
  if (!force)
    if (m_bInit)
      return m_outputs.size() > 0;

  m_bInit = true;

  if (getenv("KODI_BIN_HOME") == NULL)
    return false;

  m_outputs.clear();
  // query all screens
  // we are happy if at least one screen returns results
  bool success = false;
  for(unsigned int screennum=0; screennum<m_numScreens; ++screennum)
  {
    if(Query(force, screennum, ignoreoff))
      success = true;
  }
  return success;
}

bool CXRandR::Query(bool force, int screennum, bool ignoreoff)
{
  std::string cmd;
  std::string appname = CCompileInfo::GetAppName();
  StringUtils::ToLower(appname);
  if (getenv("KODI_BIN_HOME"))
  {
    cmd  = getenv("KODI_BIN_HOME");
    cmd += "/" + appname + "-xrandr";
    cmd = StringUtils::Format("{} -q --screen {}", cmd, screennum);
  }

  FILE* file = popen(cmd.c_str(),"r");
  if (!file)
  {
    CLog::Log(LOGERROR, "CXRandR::Query - unable to execute xrandr tool");
    return false;
  }

  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(file))
  {
    CLog::Log(LOGERROR, "CXRandR::Query - unable to open xrandr xml");
    pclose(file);
    return false;
  }
  pclose(file);

  auto* rootElement = xmlDoc.RootElement();
  if (atoi(rootElement->Attribute("id")) != screennum)
  {
    //! @todo ERROR
    return false;
  }

  for (auto* output = rootElement->FirstChildElement("output"); output;
       output = output->NextSiblingElement("output"))
  {
    XOutput xoutput;
    xoutput.name = output->Attribute("name");
    StringUtils::Trim(xoutput.name);
    xoutput.isConnected = (StringUtils::CompareNoCase(output->Attribute("connected"), "true") == 0);
    xoutput.screen = screennum;
    xoutput.w = (output->Attribute("w") != NULL ? atoi(output->Attribute("w")) : 0);
    xoutput.h = (output->Attribute("h") != NULL ? atoi(output->Attribute("h")) : 0);
    xoutput.x = (output->Attribute("x") != NULL ? atoi(output->Attribute("x")) : 0);
    xoutput.y = (output->Attribute("y") != NULL ? atoi(output->Attribute("y")) : 0);
    xoutput.crtc = (output->Attribute("crtc") != NULL ? atoi(output->Attribute("crtc")) : 0);
    xoutput.wmm = (output->Attribute("wmm") != NULL ? atoi(output->Attribute("wmm")) : 0);
    xoutput.hmm = (output->Attribute("hmm") != NULL ? atoi(output->Attribute("hmm")) : 0);
    if (output->Attribute("rotation") != NULL &&
        (StringUtils::CompareNoCase(output->Attribute("rotation"), "left") == 0 ||
         StringUtils::CompareNoCase(output->Attribute("rotation"), "right") == 0))
    {
      xoutput.isRotated = true;
    }
    else
      xoutput.isRotated = false;

    if (!xoutput.isConnected)
       continue;

    bool hascurrent = false;
    for (auto* mode = output->FirstChildElement("mode"); mode;
         mode = mode->NextSiblingElement("mode"))
    {
      XMode xmode;
      xmode.id = mode->Attribute("id");
      xmode.name = mode->Attribute("name");
      xmode.hz = atof(mode->Attribute("hz"));
      xmode.w = atoi(mode->Attribute("w"));
      xmode.h = atoi(mode->Attribute("h"));
      xmode.isPreferred = (StringUtils::CompareNoCase(mode->Attribute("preferred"), "true") == 0);
      xmode.isCurrent = (StringUtils::CompareNoCase(mode->Attribute("current"), "true") == 0);
      xoutput.modes.push_back(xmode);
      if (xmode.isCurrent)
        hascurrent = true;
    }
    if (hascurrent || !ignoreoff)
      m_outputs.push_back(xoutput);
    else
      CLog::Log(LOGWARNING, "CXRandR::Query - output {} has no current mode, assuming disconnected",
                xoutput.name);
  }
  return m_outputs.size() > 0;
}

bool CXRandR::TurnOffOutput(const std::string& name)
{
  XOutput *output = GetOutput(name);
  if (!output)
    return false;

  std::string cmd;
  std::string appname = CCompileInfo::GetAppName();
  StringUtils::ToLower(appname);

  if (getenv("KODI_BIN_HOME"))
  {
    cmd  = getenv("KODI_BIN_HOME");
    cmd += "/" + appname + "-xrandr";
    cmd = StringUtils::Format("{} --screen {} --output {} --off", cmd, output->screen, name);
  }

  int status = system(cmd.c_str());
  if (status == -1)
    return false;

  if (WEXITSTATUS(status) != 0)
    return false;

  return true;
}

bool CXRandR::TurnOnOutput(const std::string& name)
{
  XOutput *output = GetOutput(name);
  if (!output)
    return false;

  XMode mode = GetCurrentMode(output->name);
  if (mode.isCurrent)
    return true;

  // get preferred mode
  for (unsigned int j = 0; j < m_outputs.size(); j++)
  {
    if (m_outputs[j].name == output->name)
    {
      for (unsigned int i = 0; i < m_outputs[j].modes.size(); i++)
      {
        if (m_outputs[j].modes[i].isPreferred)
        {
          mode = m_outputs[j].modes[i];
          break;
        }
      }
    }
  }

  if (!mode.isPreferred)
    return false;

  if (!SetMode(*output, mode))
    return false;

  XbmcThreads::EndTime<> timeout(5s);
  while (!timeout.IsTimePast())
  {
    if (!Query(true))
      return false;

    output = GetOutput(name);
    if (output && output->h > 0)
      return true;

    KODI::TIME::Sleep(200ms);
  }

  return false;
}

std::vector<XOutput> CXRandR::GetModes(void)
{
  Query();
  return m_outputs;
}

void CXRandR::SaveState()
{
  Query(true);
}

bool CXRandR::SetMode(const XOutput& output, const XMode& mode)
{
  if ((output.name == "" && mode.id == ""))
    return true;

  Query();

  // Make sure the output exists, if not -- complain and exit
  bool isOutputFound = false;
  XOutput outputFound;
  for (size_t i = 0; i < m_outputs.size(); i++)
  {
    if (m_outputs[i].name == output.name)
    {
      isOutputFound = true;
      outputFound = m_outputs[i];
    }
  }

  if (!isOutputFound)
  {
    CLog::Log(LOGERROR,
              "CXRandR::SetMode: asked to change resolution for non existing output: {} mode: {}",
              output.name, mode.id);
    return false;
  }

  // try to find the same exact mode (same id, resolution, hz)
  bool isModeFound = false;
  XMode modeFound;
  for (size_t i = 0; i < outputFound.modes.size(); i++)
  {
    if (outputFound.modes[i].id == mode.id)
    {
      if (outputFound.modes[i].w == mode.w &&
          outputFound.modes[i].h == mode.h &&
          outputFound.modes[i].hz == mode.hz)
      {
        isModeFound = true;
        modeFound = outputFound.modes[i];
      }
      else
      {
        CLog::Log(LOGERROR,
                  "CXRandR::SetMode: asked to change resolution for mode that exists but with "
                  "different w/h/hz: {} mode: {}. Searching for similar modes...",
                  output.name, mode.id);
        break;
      }
    }
  }

  if (!isModeFound)
  {
    for (size_t i = 0; i < outputFound.modes.size(); i++)
    {
      if (outputFound.modes[i].w == mode.w &&
          outputFound.modes[i].h == mode.h &&
          outputFound.modes[i].hz == mode.hz)
      {
        isModeFound = true;
        modeFound = outputFound.modes[i];
        CLog::Log(LOGWARNING, "CXRandR::SetMode: found alternative mode (same hz): {} mode: {}.",
                  output.name, outputFound.modes[i].id);
      }
    }
  }

  if (!isModeFound)
  {
    for (size_t i = 0; i < outputFound.modes.size(); i++)
    {
      if (outputFound.modes[i].w == mode.w &&
          outputFound.modes[i].h == mode.h)
      {
        isModeFound = true;
        modeFound = outputFound.modes[i];
        CLog::Log(LOGWARNING,
                  "CXRandR::SetMode: found alternative mode (different hz): {} mode: {}.",
                  output.name, outputFound.modes[i].id);
      }
    }
  }

  // Let's try finding a mode that is the same
  if (!isModeFound)
  {
    CLog::Log(LOGERROR,
              "CXRandR::SetMode: asked to change resolution for non existing mode: {} mode: {}",
              output.name, mode.id);
    return false;
  }

  m_currentOutput = outputFound.name;
  m_currentMode = modeFound.id;
  std::string appname = CCompileInfo::GetAppName();
  StringUtils::ToLower(appname);
  char cmd[255];

  if (getenv("KODI_BIN_HOME"))
    snprintf(cmd, sizeof(cmd), "%s/%s-xrandr --screen %d --output %s --mode %s",
               getenv("KODI_BIN_HOME"),appname.c_str(),
               outputFound.screen, outputFound.name.c_str(), modeFound.id.c_str());
  else
    return false;
  CLog::Log(LOGINFO, "XRANDR: {}", cmd);
  int status = system(cmd);
  if (status == -1)
    return false;

  if (WEXITSTATUS(status) != 0)
    return false;

  return true;
}

XMode CXRandR::GetCurrentMode(const std::string& outputName)
{
  Query();
  XMode result;

  for (unsigned int j = 0; j < m_outputs.size(); j++)
  {
    if (m_outputs[j].name == outputName || outputName == "")
    {
      for (unsigned int i = 0; i < m_outputs[j].modes.size(); i++)
      {
        if (m_outputs[j].modes[i].isCurrent)
        {
          result = m_outputs[j].modes[i];
          break;
        }
      }
    }
  }

  return result;
}

XMode CXRandR::GetPreferredMode(const std::string& outputName)
{
  Query();
  XMode result;

  for (unsigned int j = 0; j < m_outputs.size(); j++)
  {
    if (m_outputs[j].name == outputName || outputName == "")
    {
      for (unsigned int i = 0; i < m_outputs[j].modes.size(); i++)
      {
        if (m_outputs[j].modes[i].isPreferred)
        {
          result = m_outputs[j].modes[i];
          break;
        }
      }
    }
  }

  return result;
}

void CXRandR::LoadCustomModeLinesToAllOutputs(void)
{
  Query();
  CXBMCTinyXML2 xmlDoc;

  if (!xmlDoc.LoadFile("special://xbmc/userdata/ModeLines.xml"))
  {
    return;
  }

  auto* rootElement = xmlDoc.RootElement();
  if (StringUtils::CompareNoCase(rootElement->Value(), "modelines") != 0)
  {
    //! @todo ERROR
    return;
  }

  char cmd[255];
  std::string name;
  std::string strModeLine;

  for (auto* modeline = rootElement->FirstChildElement("modeline"); modeline;
       modeline = modeline->NextSiblingElement("modeline"))
  {
    name = modeline->Attribute("label");
    StringUtils::Trim(name);
    strModeLine = modeline->FirstChild()->Value();
    StringUtils::Trim(strModeLine);
    std::string appname = CCompileInfo::GetAppName();
    StringUtils::ToLower(appname);

    if (getenv("KODI_BIN_HOME"))
    {
      snprintf(cmd, sizeof(cmd), "%s/%s-xrandr --newmode \"%s\" %s > /dev/null 2>&1", getenv("KODI_BIN_HOME"),
               appname.c_str(), name.c_str(), strModeLine.c_str());
      if (system(cmd) != 0)
        CLog::Log(LOGERROR, "Unable to create modeline \"{}\"", name);
    }

    for (unsigned int i = 0; i < m_outputs.size(); i++)
    {
      if (getenv("KODI_BIN_HOME"))
      {
        snprintf(cmd, sizeof(cmd), "%s/%s-xrandr --addmode %s \"%s\"  > /dev/null 2>&1", getenv("KODI_BIN_HOME"),
                 appname.c_str(), m_outputs[i].name.c_str(), name.c_str());
        if (system(cmd) != 0)
          CLog::Log(LOGERROR, "Unable to add modeline \"{}\"", name);
      }
    }
  }
}

void CXRandR::SetNumScreens(unsigned int num)
{
  m_numScreens = num;
  m_bInit = false;
}

bool CXRandR::IsOutputConnected(const std::string& name)
{
  bool result = false;
  Query();

  for (unsigned int i = 0; i < m_outputs.size(); ++i)
  {
    if (m_outputs[i].name == name)
    {
      result = true;
      break;
    }
  }
  return result;
}

XOutput* CXRandR::GetOutput(const std::string& outputName)
{
  XOutput *result = 0;
  Query();
  for (unsigned int i = 0; i < m_outputs.size(); ++i)
  {
    if (m_outputs[i].name == outputName)
    {
      result = &m_outputs[i];
      break;
    }
  }
  return result;
}

int CXRandR::GetCrtc(int x, int y, float &hz)
{
  int crtc = 0;
  for (unsigned int i = 0; i < m_outputs.size(); ++i)
  {
    if (!m_outputs[i].isConnected)
      continue;

    if ((m_outputs[i].x <= x && (m_outputs[i].x+m_outputs[i].w) > x) &&
        (m_outputs[i].y <= y && (m_outputs[i].y+m_outputs[i].h) > y))
    {
      crtc = m_outputs[i].crtc;
      for (const auto& mode : m_outputs[i].modes)
      {
        if (mode.isCurrent)
        {
          hz = mode.hz;
          break;
        }
      }
      break;
    }
  }
  return crtc;
}

CXRandR g_xrandr;

/*
  int main()
  {
  CXRandR r;
  r.LoadCustomModeLinesToAllOutputs();
  }
*/
