#include <string.h>
#include "XRandR.h"
#include "tinyXML/tinyxml.h"
#include "Util.h"

CXRandR::CXRandR()
{
   Query();
}

void CXRandR::Query()
{
   m_outputs.clear();
   
   char cmd[255];
   unlink("/tmp/xbmc_xranr");
   sprintf(cmd, "%s/xbmc-xrandr > /tmp/xbmc_xrandr", getenv("XBMC_HOME"));
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
     }
     
     m_outputs.push_back(xoutput);
  }
}

std::vector<XOutput> CXRandR::GetModes(void)
{
   return m_outputs;
}

bool CXRandR::SetMode(XOutput output, XMode mode)
{
   char cmd[255];
	sprintf(cmd, "%s/xbmc-xrandr --output %s --mode %s", getenv("XBMC_HOME"), output.name.c_str(), mode.id.c_str());
   int status = system(cmd);
   if (status == -1)
      return false;
      
   if (WEXITSTATUS(status) != 0)
      return false;
   
   Query();
   
   return true;
}

XMode CXRandR::GetCurrentMode(CStdString outputName)
{
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
   TiXmlDocument xmlDoc;
   //if (!xmlDoc.LoadFile(_P("Q:/UserData/ModeLines.xml")))
   if (!xmlDoc.LoadFile("/home/yuvalt/linuxport/XBMC/userdata/ModeLines.xml"))
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

      sprintf(cmd, "%s/xbmc-xrandr --newmode \"%s\" %s > /dev/null 2>&1", getenv("XBMC_HOME"), 
        name.c_str(), modeline->FirstChild()->Value());
      system(cmd);
 
      for (unsigned int i = 0; i < m_outputs.size(); i++)
      {
         sprintf(cmd, "%s/xbmc-xrandr --addmode %s \"%s\"  > /dev/null 2>&1", getenv("XBMC_HOME"), 
           m_outputs[i].name.c_str(), name.c_str());
         system(cmd);
      }
   }
}

/*
int main()
{
   CXRandR r;
   r.LoadCustomModeLinesToAllOutputs();
}
*/
