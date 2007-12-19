#include <string.h>
#include "XRandR.h"
#include "tinyXML/tinyxml.h"

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
      xoutput.w = atoi(output->Attribute("w"));
      xoutput.h = atoi(output->Attribute("h"));
      xoutput.x = atoi(output->Attribute("x"));
      xoutput.y = atoi(output->Attribute("y"));
      xoutput.wmm = atoi(output->Attribute("wmm"));
      xoutput.hmm = atoi(output->Attribute("hmm"));      
         
      for (TiXmlElement* mode = output->FirstChildElement("mode"); mode; mode = mode->NextSiblingElement("mode"))
      {
         XMode xmode;
         xmode.id = mode->Attribute("id");
         xmode.name = mode->Attribute("name");
         xmode.hz = atof(mode->Attribute("hz"));
         xmode.isPreferred = (strcasecmp(mode->Attribute("preferred"), "true") == 0); 
         xmode.isCurrent = (strcasecmp(mode->Attribute("current"), "true") == 0);              
         xoutput.modes.push_back(xmode);
     }
     
     m_outputs.push_back(xoutput);
  }
}

std::vector<XOutput> CXRandR::GetModes()
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

/*
int main()
{
   CXRandR r;
   std::vector<XOutput>  output = r.GetModes();
   r.SetMode(output[0], output[0].modes[2]);
   XMode mode = r.GetCurrentMode("default");
   printf("%s\n", mode.name.c_str());
}
*/