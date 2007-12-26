#ifdef HAS_XRANDR

#include <string.h>
#include "XRandR.h"
#include "tinyXML/tinyxml.h"
#include "Util.h"
#include "../xbmc/utils/log.h"

CXRandR::CXRandR(bool query)
{
  m_bInit = false;
  if (query)
    Query();
}

void CXRandR::Query(bool force)
{
  if (!force)
    if (m_bInit)
      return;

  m_outputs.clear();
  m_current.clear();

  char cmd[255];
  unlink("/tmp/xbmc_xranr");
  if (getenv("XBMC_HOME"))
    snprintf(cmd, sizeof(cmd), "%s/xbmc-xrandr > /tmp/xbmc_xrandr", getenv("XBMC_HOME"));
  else
    return;
  system(cmd);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile("/tmp/xbmc_xrandr"))
  {
    // TODO ERROR
    return;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcasecmp(pRootElement->Value(), "screen") != 0)
  {
    // TODO ERROR
    return;
  }

  m_bInit = true;

  for (TiXmlElement* output = pRootElement->FirstChildElement("output"); output; output = output->NextSiblingElement("output"))
  {
    XOutput xoutput;
    xoutput.name = output->Attribute("name");
    xoutput.isConnected = (strcasecmp(output->Attribute("connected"), "true") == 0);
    xoutput.w = (output->Attribute("w") != NULL ? atoi(output->Attribute("w")) : 0);
    xoutput.h = (output->Attribute("h") != NULL ? atoi(output->Attribute("h")) : 0);
    xoutput.x = (output->Attribute("x") != NULL ? atoi(output->Attribute("x")) : 0);
    xoutput.y = (output->Attribute("y") != NULL ? atoi(output->Attribute("y")) : 0);
    xoutput.wmm = (output->Attribute("wmm") != NULL ? atoi(output->Attribute("wmm")) : 0);
    xoutput.hmm = (output->Attribute("hmm") != NULL ? atoi(output->Attribute("hmm")) : 0);

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
        m_current.push_back(xoutput);
      }
    }
    m_outputs.push_back(xoutput);
  }
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

void CXRandR::RestoreState()
{
  vector<XOutput>::iterator outiter;
  for (outiter=m_current.begin() ; outiter!=m_current.end() ; outiter++)
  {
    vector<XMode> modes = (*outiter).modes;
    vector<XMode>::iterator modeiter;
    for (modeiter=modes.begin() ; modeiter!=modes.end() ; modeiter++)
    {
      XMode mode = *modeiter;
      if (mode.isCurrent)
      {
        SetMode(*outiter, mode);
        return;
      }
    }
  }
}

bool CXRandR::SetMode(XOutput output, XMode mode)
{
  if (output.name==m_currentOutput && mode.id==m_currentMode)
    return true;

  Query();
  if (strlen(mode.id.c_str())==0)
    return false;
  if (strlen(output.name.c_str())==0)
    return false;
  m_currentOutput = output.name;
  m_currentMode = mode.id;
  char cmd[255];
  if (getenv("XBMC_HOME"))
    snprintf(cmd, sizeof(cmd), "%s/xbmc-xrandr --output %s --mode %s", getenv("XBMC_HOME"), output.name.c_str(), mode.id.c_str());
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
    if (m_outputs[j].name == outputName)
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
  TiXmlDocument xmlDoc;

  if (!xmlDoc.LoadFile(_P("Q:/UserData/ModeLines.xml")))
    //if (!xmlDoc.LoadFile("UserData/ModeLines.xml"))
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

  for (TiXmlElement* modeline = pRootElement->FirstChildElement("modeline"); modeline; modeline = modeline->NextSiblingElement("modeline"))
  {
    name = modeline->Attribute("label");

    if (getenv("XBMC_HOME"))
    {
      snprintf(cmd, sizeof(cmd), "%s/xbmc-xrandr --newmode \"%s\" %s > /dev/null 2>&1", getenv("XBMC_HOME"),
               name.c_str(), modeline->FirstChild()->Value());
      system(cmd);
    }

    for (unsigned int i = 0; i < m_outputs.size(); i++)
    {
      if (getenv("XBMC_HOME"))
      {
        snprintf(cmd, sizeof(cmd), "%s/xbmc-xrandr --addmode %s \"%s\"  > /dev/null 2>&1", getenv("XBMC_HOME"),
                 m_outputs[i].name.c_str(), name.c_str());
        system(cmd);
      }
    }
  }
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
