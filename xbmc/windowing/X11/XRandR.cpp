/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "XRandR.h"

#ifdef HAS_XRANDR

#include <string.h>
#include <sys/wait.h>
#include "system.h"
#include "PlatformInclude.h"
#include "utils/XBMCTinyXML.h"
#include "../xbmc/utils/log.h"

#if defined(TARGET_FREEBSD)
#include <sys/types.h>
#include <sys/wait.h>
#endif

using namespace std;

CXRandR::CXRandR(bool query)
{
  m_bInit = false;
  m_numScreens = 1;
  if (query)
    Query();
}

bool CXRandR::Query(bool force)
{
  if (!force)
    if (m_bInit)
      return m_outputs.size() > 0;

  m_bInit = true;

  if (getenv("XBMC_BIN_HOME") == NULL)
    return false;

  m_outputs.clear();
  // query all screens
  // we are happy if at least one screen returns results
  bool success = false;
  for(unsigned int screennum=0; screennum<m_numScreens; ++screennum)
  {
    if(Query(force, screennum))
      success = true;
  }
  return success;
}

bool CXRandR::Query(bool force, int screennum)
{
  CStdString cmd;
  cmd  = getenv("XBMC_BIN_HOME");
  cmd += "/xbmc-xrandr";
  cmd.AppendFormat(" -q --screen %d", screennum);

  FILE* file = popen(cmd.c_str(),"r");
  if (!file)
  {
    CLog::Log(LOGERROR, "CXRandR::Query - unable to execute xrandr tool");
    return false;
  }


  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file, TIXML_DEFAULT_ENCODING))
  {
    CLog::Log(LOGERROR, "CXRandR::Query - unable to open xrandr xml");
    pclose(file);
    return false;
  }
  pclose(file);

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (atoi(pRootElement->Attribute("id")) != screennum)
  {
    // TODO ERROR
    return false;
  }

  for (TiXmlElement* output = pRootElement->FirstChildElement("output"); output; output = output->NextSiblingElement("output"))
  {
    XOutput xoutput;
    xoutput.name = output->Attribute("name");
    xoutput.name.TrimLeft(" \n\r\t");
    xoutput.name.TrimRight(" \n\r\t");
    xoutput.isConnected = (strcasecmp(output->Attribute("connected"), "true") == 0);
    xoutput.screen = screennum;
    xoutput.w = (output->Attribute("w") != NULL ? atoi(output->Attribute("w")) : 0);
    xoutput.h = (output->Attribute("h") != NULL ? atoi(output->Attribute("h")) : 0);
    xoutput.x = (output->Attribute("x") != NULL ? atoi(output->Attribute("x")) : 0);
    xoutput.y = (output->Attribute("y") != NULL ? atoi(output->Attribute("y")) : 0);
    xoutput.wmm = (output->Attribute("wmm") != NULL ? atoi(output->Attribute("wmm")) : 0);
    xoutput.hmm = (output->Attribute("hmm") != NULL ? atoi(output->Attribute("hmm")) : 0);
    if (output->Attribute("rotation") != NULL
        && (strcasecmp(output->Attribute("rotation"), "left") == 0 || strcasecmp(output->Attribute("rotation"), "right") == 0))
    {
      xoutput.isRotated = true;
    }
    else
      xoutput.isRotated = false;

    if (!xoutput.isConnected)
       continue;

    bool hascurrent = false;
    for (TiXmlElement* mode = output->FirstChildElement("mode"); mode; mode = mode->NextSiblingElement("mode"))
    {
      XMode xmode;
      xmode.id = mode->Attribute("id");
      xmode.name = mode->Attribute("name");
      xmode.hz = atof(mode->Attribute("hz"));
      xmode.w = atoi(mode->Attribute("w"));
      xmode.h = atoi(mode->Attribute("h"));
      xmode.isPreferred = (strcasecmp(mode->Attribute("preferred"), "true") == 0);
      xmode.isCurrent = (strcasecmp(mode->Attribute("current"), "true") == 0);
      xoutput.modes.push_back(xmode);
      if (xmode.isCurrent)
      {
        hascurrent = true;
      }
    }
    if (hascurrent)
      m_outputs.push_back(xoutput);
    else
      CLog::Log(LOGWARNING, "CXRandR::Query - output %s has no current mode, assuming disconnected", xoutput.name.c_str());
  }
  return m_outputs.size() > 0;
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

bool CXRandR::SetMode(XOutput output, XMode mode)
{
  if ((output.name == m_currentOutput && mode.id == m_currentMode) || (output.name == "" && mode.id == ""))
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
    CLog::Log(LOGERROR, "CXRandR::SetMode: asked to change resolution for non existing output: %s mode: %s", output.name.c_str(), mode.id.c_str());
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
        CLog::Log(LOGERROR, "CXRandR::SetMode: asked to change resolution for mode that exists but with different w/h/hz: %s mode: %s. Searching for similar modes...", output.name.c_str(), mode.id.c_str());
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
        CLog::Log(LOGWARNING, "CXRandR::SetMode: found alternative mode (same hz): %s mode: %s.", output.name.c_str(), outputFound.modes[i].id.c_str());
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
        CLog::Log(LOGWARNING, "CXRandR::SetMode: found alternative mode (different hz): %s mode: %s.", output.name.c_str(), outputFound.modes[i].id.c_str());
      }
    }
  }

  // Let's try finding a mode that is the same
  if (!isModeFound)
  {
    CLog::Log(LOGERROR, "CXRandR::SetMode: asked to change resolution for non existing mode: %s mode: %s", output.name.c_str(), mode.id.c_str());
    return false;
  }

  m_currentOutput = outputFound.name;
  m_currentMode = modeFound.id;
  char cmd[255];
  if (getenv("XBMC_BIN_HOME"))
    snprintf(cmd, sizeof(cmd), "%s/xbmc-xrandr --output %s --mode %s", getenv("XBMC_BIN_HOME"), outputFound.name.c_str(), modeFound.id.c_str());
  else
    return false;
  CLog::Log(LOGINFO, "XRANDR: %s", cmd);
  int status = system(cmd);
  if (status == -1)
    return false;

  if (WEXITSTATUS(status) != 0)
    return false;

  return true;
}

XMode CXRandR::GetCurrentMode(CStdString outputName)
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

void CXRandR::LoadCustomModeLinesToAllOutputs(void)
{
  Query();
  CXBMCTinyXML xmlDoc;

  if (!xmlDoc.LoadFile("special://xbmc/userdata/ModeLines.xml"))
  {
    return;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcasecmp(pRootElement->Value(), "modelines") != 0)
  {
    // TODO ERROR
    return;
  }

  char cmd[255];
  CStdString name;
  CStdString strModeLine;

  for (TiXmlElement* modeline = pRootElement->FirstChildElement("modeline"); modeline; modeline = modeline->NextSiblingElement("modeline"))
  {
    name = modeline->Attribute("label");
    name.TrimLeft(" \n\t\r");
    name.TrimRight(" \n\t\r");
    strModeLine = modeline->FirstChild()->Value();
    strModeLine.TrimLeft(" \n\t\r");
    strModeLine.TrimRight(" \n\t\r");
    if (getenv("XBMC_BIN_HOME"))
    {
      snprintf(cmd, sizeof(cmd), "%s/xbmc-xrandr --newmode \"%s\" %s > /dev/null 2>&1", getenv("XBMC_BIN_HOME"),
               name.c_str(), strModeLine.c_str());
      if (system(cmd) != 0)
        CLog::Log(LOGERROR, "Unable to create modeline \"%s\"", name.c_str());
    }

    for (unsigned int i = 0; i < m_outputs.size(); i++)
    {
      if (getenv("XBMC_BIN_HOME"))
      {
        snprintf(cmd, sizeof(cmd), "%s/xbmc-xrandr --addmode %s \"%s\"  > /dev/null 2>&1", getenv("XBMC_BIN_HOME"),
                 m_outputs[i].name.c_str(), name.c_str());
        if (system(cmd) != 0)
          CLog::Log(LOGERROR, "Unable to add modeline \"%s\"", name.c_str());
      }
    }
  }
}

void CXRandR::SetNumScreens(unsigned int num)
{
  m_numScreens = num;
  m_bInit = false;
}

bool CXRandR::IsOutputConnected(CStdString name)
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

XOutput* CXRandR::GetOutput(CStdString outputName)
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

CXRandR g_xrandr;

#endif // HAS_XRANDR

/*
  int main()
  {
  CXRandR r;
  r.LoadCustomModeLinesToAllOutputs();
  }
*/
