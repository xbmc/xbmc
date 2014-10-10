/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "FileItem.h"
#include "SlingboxFile.h"
#include "filesystem/File.h"
#include "lib/SlingboxLib/SlingboxLib.h"
#include "profiles/ProfilesManager.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"
#include "utils/StringUtils.h"
#include "URL.h"

using namespace XFILE;
using namespace std;

CSlingboxFile::CSlingboxFile()
{
  // Create the Slingbox object
  m_pSlingbox = new CSlingbox();
}

CSlingboxFile::~CSlingboxFile()
{
  // Destroy the Slingbox object
  delete m_pSlingbox;
}

bool CSlingboxFile::Open(const CURL& url)
{
  // Setup the IP/hostname and port (setup default port if none specified)
  unsigned int uiPort;
  if (url.HasPort())
    uiPort = (unsigned int)url.GetPort();
  else
    uiPort = 5001;
  m_pSlingbox->SetAddress(url.GetHostName().c_str(), uiPort);

  // Prepare to connect to the Slingbox
  bool bAdmin;
  if (StringUtils::EqualsNoCase(url.GetUserName(), "administrator"))
    bAdmin = true;
  else if (StringUtils::EqualsNoCase(url.GetUserName(), "viewer"))
    bAdmin = false;
  else
  {
    CLog::Log(LOGERROR, "%s - Invalid or no username specified for Slingbox: %s",
      __FUNCTION__, url.GetHostName().c_str());
    return false;
  }

  // Connect to the Slingbox
  if (m_pSlingbox->Connect(bAdmin, url.GetPassWord().c_str()))
  {
    CLog::Log(LOGDEBUG, "%s - Successfully connected to Slingbox: %s",
      __FUNCTION__, url.GetHostName().c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Error connecting to Slingbox: %s",
      __FUNCTION__, url.GetHostName().c_str());
    return false;
  }

  // Initialize the stream
  if (m_pSlingbox->InitializeStream())
  {
    CLog::Log(LOGDEBUG, "%s - Successfully initialized stream on Slingbox: %s",
      __FUNCTION__, url.GetHostName().c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Error initializing stream on Slingbox: %s",
      __FUNCTION__, url.GetHostName().c_str());
    return false;
  }

  // Set correct input
  if (url.GetFileNameWithoutPath() != "")
  {
    if (m_pSlingbox->SetInput(atoi(url.GetFileNameWithoutPath().c_str())))
      CLog::Log(LOGDEBUG, "%s - Successfully requested change to input %i on Slingbox: %s",
        __FUNCTION__, atoi(url.GetFileNameWithoutPath().c_str()), url.GetHostName().c_str());
    else
      CLog::Log(LOGERROR, "%s - Error requesting change to input %i on Slingbox: %s",
        __FUNCTION__, atoi(url.GetFileNameWithoutPath().c_str()), url.GetHostName().c_str());
  }

  // Load the video settings
  LoadSettings(url.GetHostName());

  // Setup video options  
  if (m_pSlingbox->StreamSettings((CSlingbox::Resolution)m_sSlingboxSettings.iVideoResolution,
    m_sSlingboxSettings.iVideoBitrate, m_sSlingboxSettings.iVideoFramerate,
    m_sSlingboxSettings.iVideoSmoothing, m_sSlingboxSettings.iAudioBitrate,
    m_sSlingboxSettings.iIFrameInterval))
  {
    CLog::Log(LOGDEBUG, "%s - Successfully set stream options (resolution: %ix%i; "
      "video bitrate: %i kbit/s; fps: %i; smoothing: %i%%; audio bitrate %i kbit/s; "
      "I frame interval: %i) on Slingbox: %s", __FUNCTION__,
      m_sSlingboxSettings.iVideoWidth, m_sSlingboxSettings.iVideoHeight,
      m_sSlingboxSettings.iVideoBitrate, m_sSlingboxSettings.iVideoFramerate,
      m_sSlingboxSettings.iVideoSmoothing, m_sSlingboxSettings.iAudioBitrate,
      m_sSlingboxSettings.iIFrameInterval, url.GetHostName().c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Error setting stream options on Slingbox: %s",
      __FUNCTION__, url.GetHostName().c_str());
  }

  // Start the stream
  if (m_pSlingbox->StartStream())
  {
    CLog::Log(LOGDEBUG, "%s - Successfully started stream on Slingbox: %s",
      __FUNCTION__, url.GetHostName().c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Error starting stream on Slingbox: %s",
      __FUNCTION__, url.GetHostName().c_str());
    return false;
  }

  // Check for correct input
  if (url.GetFileNameWithoutPath() != "")
  {
    int input = atoi(url.GetFileNameWithoutPath().c_str());
    if (m_pSlingbox->GetInput() == -1)
      CLog::Log(LOGDEBUG, "%s - Unable to confirm change to input %i on Slingbox: %s",
        __FUNCTION__, input, url.GetHostName().c_str());
    else if (m_pSlingbox->GetInput() == input)
      CLog::Log(LOGDEBUG, "%s - Comfirmed change to input %i on Slingbox: %s",
        __FUNCTION__, input, url.GetHostName().c_str());
    else
      CLog::Log(LOGERROR, "%s - Error changing to input %i on Slingbox: %s",
        __FUNCTION__, input, url.GetHostName().c_str());
  }

  return true;
}

ssize_t CSlingboxFile::Read(void * pBuffer, size_t iSize)
{
  if (iSize > SSIZE_MAX)
    iSize = SSIZE_MAX;

  // Read the data and check for any errors
  ssize_t iRead = m_pSlingbox->ReadStream(pBuffer, (unsigned int)iSize);
  if (iRead < 0)
    CLog::Log(LOGERROR, "%s - Error reading stream from Slingbox: %s", __FUNCTION__,
      m_sSlingboxSettings.strHostname.c_str());

  return iRead;
}

void CSlingboxFile::Close()
{
  // Stop the stream
  if (m_pSlingbox->StopStream())
    CLog::Log(LOGDEBUG, "%s - Successfully stopped stream on Slingbox: %s", __FUNCTION__,
    m_sSlingboxSettings.strHostname.c_str());
  else
    CLog::Log(LOGERROR, "%s - Error stopping stream on Slingbox: %s", __FUNCTION__,
    m_sSlingboxSettings.strHostname.c_str());

  // Disconnect from the Slingbox
  if (m_pSlingbox->Disconnect())
    CLog::Log(LOGDEBUG, "%s - Successfully disconnected from Slingbox: %s", __FUNCTION__,
    m_sSlingboxSettings.strHostname.c_str());
  else
    CLog::Log(LOGERROR, "%s - Error disconnecting from Slingbox: %s", __FUNCTION__,
    m_sSlingboxSettings.strHostname.c_str());
}

bool CSlingboxFile::SkipNext()
{
  return m_pSlingbox->IsConnected();
}

bool CSlingboxFile::NextChannel(bool bPreview /* = false */)
{
  // Prepare variables
  bool bSuccess = true;
  int iPrevChannel = m_pSlingbox->GetChannel();

  // Stop the stream
  if (m_pSlingbox->StopStream())
  {
    CLog::Log(LOGDEBUG, "%s - Successfully stopped stream before channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Error stopping stream before channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
    bSuccess = false;
  }

  // Figure out which method to use
  if (m_sSlingboxSettings.uiCodeChannelUp == 0)
  {
    // Change the channel
    if (m_pSlingbox->ChannelUp())
    {
      CLog::Log(LOGDEBUG, "%s - Successfully requested channel change on Slingbox: %s",
        __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());

      if (m_pSlingbox->GetChannel() == -1)
      {
        CLog::Log(LOGDEBUG, "%s - Unable to confirm channel change on Slingbox: %s",
          __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
      }
      else if (m_pSlingbox->GetChannel() != iPrevChannel)
      {
        CLog::Log(LOGDEBUG, "%s - Confirmed change to channel %i on Slingbox: %s",
          __FUNCTION__, m_pSlingbox->GetChannel(), m_sSlingboxSettings.strHostname.c_str());
      }
      else
      {
        CLog::Log(LOGERROR, "%s - Error changing channel on Slingbox: %s",
          __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
        bSuccess = false;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "%s - Error requesting channel change on Slingbox: %s",
        __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
      bSuccess = false;
    }
  }
  else
  {
    // Change the channel using IR command
    if (m_pSlingbox->SendIRCommand(m_sSlingboxSettings.uiCodeChannelUp))
    {
      CLog::Log(LOGDEBUG, "%s - Successfully sent IR command (code: 0x%.2X) from "
        "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.uiCodeChannelUp,
        m_sSlingboxSettings.strHostname.c_str());
    }
    else
    {
      CLog::Log(LOGERROR, "%s - Error sending IR command (code: 0x%.2X) from "
        "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.uiCodeChannelUp,
        m_sSlingboxSettings.strHostname.c_str());
      bSuccess = false;
    }
  }

  // Start the stream again
  if (m_pSlingbox->StartStream())
  {
    CLog::Log(LOGDEBUG, "%s - Successfully started stream after channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Error starting stream after channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
    bSuccess = false;
  }

  return bSuccess;
}

bool CSlingboxFile::PrevChannel(bool bPreview /* = false */)
{
  // Prepare variables
  bool bSuccess = true;
  int iPrevChannel = m_pSlingbox->GetChannel();

  // Stop the stream
  if (m_pSlingbox->StopStream())
  {
    CLog::Log(LOGDEBUG, "%s - Successfully stopped stream before channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Error stopping stream before channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
    bSuccess = false;
  }

  // Figure out which method to use
  if (m_sSlingboxSettings.uiCodeChannelDown == 0)
  {
    // Change the channel
    if (m_pSlingbox->ChannelDown())
    {
      CLog::Log(LOGDEBUG, "%s - Successfully requested channel change on Slingbox: %s",
        __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());

      if (m_pSlingbox->GetChannel() == -1)
      {
        CLog::Log(LOGDEBUG, "%s - Unable to confirm channel change on Slingbox: %s",
          __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
      }
      else if (m_pSlingbox->GetChannel() != iPrevChannel)
      {
        CLog::Log(LOGDEBUG, "%s - Confirmed change to channel %i on Slingbox: %s",
          __FUNCTION__, m_pSlingbox->GetChannel(), m_sSlingboxSettings.strHostname.c_str());
      }
      else
      {
        CLog::Log(LOGERROR, "%s - Error changing channel on Slingbox: %s",
          __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
        bSuccess = false;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "%s - Error requesting channel change on Slingbox: %s",
        __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
      bSuccess = false;
    }
  }
  else
  {
    // Change the channel using IR command
    if (m_pSlingbox->SendIRCommand(m_sSlingboxSettings.uiCodeChannelDown))
    {
      CLog::Log(LOGDEBUG, "%s - Successfully sent IR command (code: 0x%.2X) from "
        "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.uiCodeChannelDown,
        m_sSlingboxSettings.strHostname.c_str());
    }
    else
    {
      CLog::Log(LOGERROR, "%s - Error sending IR command (code: 0x%.2X) from "
        "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.uiCodeChannelDown,
        m_sSlingboxSettings.strHostname.c_str());
      bSuccess = false;
    }
  }    

  // Start the stream again
  if (m_pSlingbox->StartStream())
  {
    CLog::Log(LOGDEBUG, "%s - Successfully started stream after channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Error starting Slingbox stream after channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
    bSuccess = false;
  }

  return bSuccess;
}

bool CSlingboxFile::SelectChannel(unsigned int uiChannel)
{
  // Check if a channel change is required
  if (m_pSlingbox->GetChannel() == (int)uiChannel)
    return false;

  // Prepare variables
  bool bSuccess = true;

  // Stop the stream
  if (m_pSlingbox->StopStream())
  {
    CLog::Log(LOGDEBUG, "%s - Successfully stopped stream before channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Error stopping stream before channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
    bSuccess = false;
  }

  // Figure out which method to use
  unsigned int uiButtonsWithCode = 0;
  for (unsigned int i = 0; i < 10; i++)
  {
    if (m_sSlingboxSettings.uiCodeNumber[i] != 0)
      uiButtonsWithCode++;
  }
  if (uiButtonsWithCode == 0)
  {
    // Change the channel
    if (m_pSlingbox->SetChannel(uiChannel))
    {
      CLog::Log(LOGDEBUG, "%s - Successfully requested change to channel %i on Slingbox: %s",
        __FUNCTION__, uiChannel, m_sSlingboxSettings.strHostname.c_str());

      if (m_pSlingbox->GetChannel() == -1)
      {
        CLog::Log(LOGDEBUG, "%s - Unable to confirm change to channel %i on Slingbox: %s",
          __FUNCTION__, uiChannel, m_sSlingboxSettings.strHostname.c_str());
      }
      else if (m_pSlingbox->GetChannel() == (int)uiChannel)
      {
        CLog::Log(LOGDEBUG, "%s - Confirmed change to channel %i on Slingbox: %s",
          __FUNCTION__, uiChannel, m_sSlingboxSettings.strHostname.c_str());
      }
      else
      {
        CLog::Log(LOGERROR, "%s - Error changing to channel %i on Slingbox: %s",
          __FUNCTION__, uiChannel, m_sSlingboxSettings.strHostname.c_str());
        bSuccess = false;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "%s - Error requesting change to channel %i on Slingbox: %s",
        __FUNCTION__, uiChannel, m_sSlingboxSettings.strHostname.c_str());
      bSuccess = false;
    }
  }
  else if (uiButtonsWithCode == 10)
  {
    // Prepare variables
    std::string strDigits = StringUtils::Format("%u", uiChannel);
    size_t uiNumberOfDigits = strDigits.size();

    // Change the channel using IR commands
    for (size_t i = 0; i < uiNumberOfDigits; i++)
    {
      if (m_pSlingbox->SendIRCommand(m_sSlingboxSettings.uiCodeNumber[strDigits[i] - '0']))
      {
        CLog::Log(LOGDEBUG, "%s - Successfully sent IR command (code: 0x%.2X) from "
          "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.uiCodeNumber[strDigits[i] - '0'],
          m_sSlingboxSettings.strHostname.c_str());
      }
      else
      {
        CLog::Log(LOGDEBUG, "%s - Error sending IR command (code: 0x%.2X) from "
          "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.uiCodeNumber[strDigits[i] - '0'],
          m_sSlingboxSettings.strHostname.c_str());
        bSuccess = false;
      }
    }
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Error requesting change to channel %i on Slingbox due to one or more "
      "missing button codes from advancedsettings.xml for Slingbox: %s", __FUNCTION__, uiChannel,
      m_sSlingboxSettings.strHostname.c_str());
    bSuccess = false;
  }

  // Start the stream again
  if (m_pSlingbox->StartStream())
  {
    CLog::Log(LOGDEBUG, "%s - Successfully started stream after channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Error starting stream after channel change request on "
      "Slingbox: %s", __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
    bSuccess = false;
  }

  return bSuccess;
}

void CSlingboxFile::LoadSettings(const std::string& strHostname)
{
  // Load default settings
  m_sSlingboxSettings.strHostname = strHostname;
  m_sSlingboxSettings.iVideoWidth = 320;
  m_sSlingboxSettings.iVideoHeight = 240;
  m_sSlingboxSettings.iVideoResolution = (int)CSlingbox::RESOLUTION320X240;
  m_sSlingboxSettings.iVideoBitrate = 704;
  m_sSlingboxSettings.iVideoFramerate = 30;
  m_sSlingboxSettings.iVideoSmoothing = 50;
  m_sSlingboxSettings.iAudioBitrate = 64;
  m_sSlingboxSettings.iIFrameInterval = 10;
  m_sSlingboxSettings.uiCodeChannelUp = 0;
  m_sSlingboxSettings.uiCodeChannelDown = 0;
  for (unsigned int i = 0; i < 10; i++)
    m_sSlingboxSettings.uiCodeNumber[i] = 0;

  // Check if a SlingboxSettings.xml file exists
  std::string slingboxXMLFile = CProfilesManager::Get().GetUserDataItem("SlingboxSettings.xml");
  if (!CFile::Exists(slingboxXMLFile))
  {
    CLog::Log(LOGNOTICE, "No SlingboxSettings.xml file (%s) found - using default settings",
      slingboxXMLFile.c_str());
    return;
  }

  // Load the XML file
  CXBMCTinyXML slingboxXML;
  if (!slingboxXML.LoadFile(slingboxXMLFile))
  {
    CLog::Log(LOGERROR, "%s - Error loading %s - line %d\n%s", __FUNCTION__, 
      slingboxXMLFile.c_str(), slingboxXML.ErrorRow(), slingboxXML.ErrorDesc());
    return;
  }

  // Check to make sure layout is correct
  TiXmlElement * pRootElement = slingboxXML.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(), "slingboxsettings") != 0)
  {
    CLog::Log(LOGERROR, "%s - Error loading %s - no <slingboxsettings> node found",
      __FUNCTION__, slingboxXMLFile.c_str());
    return;
  }

  // Success so far
  CLog::Log(LOGNOTICE, "Loaded SlingboxSettings.xml from %s", slingboxXMLFile.c_str());

  // Search for the first settings that specify no hostname or match our hostname
  TiXmlElement *pElement;
  for (pElement = pRootElement->FirstChildElement("slingbox"); pElement;
    pElement = pElement->NextSiblingElement("slingbox"))
  {
    const char *hostname = pElement->Attribute("hostname");
    if (!hostname || StringUtils::EqualsNoCase(m_sSlingboxSettings.strHostname, hostname))
    {
      // Load setting values
      XMLUtils::GetInt(pElement, "width", m_sSlingboxSettings.iVideoWidth, 0, 640);
      XMLUtils::GetInt(pElement, "height", m_sSlingboxSettings.iVideoHeight, 0, 480);
      XMLUtils::GetInt(pElement, "videobitrate", m_sSlingboxSettings.iVideoBitrate, 50, 8000);
      XMLUtils::GetInt(pElement, "framerate", m_sSlingboxSettings.iVideoFramerate, 1, 30);
      XMLUtils::GetInt(pElement, "smoothing", m_sSlingboxSettings.iVideoSmoothing, 0, 100);
      XMLUtils::GetInt(pElement, "audiobitrate", m_sSlingboxSettings.iAudioBitrate, 16, 96);
      XMLUtils::GetInt(pElement, "iframeinterval", m_sSlingboxSettings.iIFrameInterval, 1, 30);

      // Load any button code values
      TiXmlElement * pCodes = pElement->FirstChildElement("buttons");
      if (pCodes)
      {
        XMLUtils::GetHex(pCodes, "channelup", m_sSlingboxSettings.uiCodeChannelUp);
        XMLUtils::GetHex(pCodes, "channeldown", m_sSlingboxSettings.uiCodeChannelDown);
        XMLUtils::GetHex(pCodes, "zero", m_sSlingboxSettings.uiCodeNumber[0]);
        XMLUtils::GetHex(pCodes, "one", m_sSlingboxSettings.uiCodeNumber[1]);
        XMLUtils::GetHex(pCodes, "two", m_sSlingboxSettings.uiCodeNumber[2]);
        XMLUtils::GetHex(pCodes, "three", m_sSlingboxSettings.uiCodeNumber[3]);
        XMLUtils::GetHex(pCodes, "four", m_sSlingboxSettings.uiCodeNumber[4]);
        XMLUtils::GetHex(pCodes, "five", m_sSlingboxSettings.uiCodeNumber[5]);
        XMLUtils::GetHex(pCodes, "six", m_sSlingboxSettings.uiCodeNumber[6]);
        XMLUtils::GetHex(pCodes, "seven", m_sSlingboxSettings.uiCodeNumber[7]);
        XMLUtils::GetHex(pCodes, "eight", m_sSlingboxSettings.uiCodeNumber[8]);
        XMLUtils::GetHex(pCodes, "nine", m_sSlingboxSettings.uiCodeNumber[9]);
      }

      break;
    }
  }

  // Prepare our resolution enum mapping array
  const struct
  {
    unsigned int uiWidth;
    unsigned int uiHeight;
    CSlingbox::Resolution eEnum;
  } m_resolutionMap[11] = {
    {0, 0, CSlingbox::NOVIDEO},
    {128, 96, CSlingbox::RESOLUTION128X96},
    {160, 120, CSlingbox::RESOLUTION160X120},
    {176, 120, CSlingbox::RESOLUTION176X120},
    {224, 176, CSlingbox::RESOLUTION224X176},
    {256, 192, CSlingbox::RESOLUTION256X192},
    {320, 240, CSlingbox::RESOLUTION320X240},
    {352, 240, CSlingbox::RESOLUTION352X240},
    {320, 480, CSlingbox::RESOLUTION320X480},
    {640, 240, CSlingbox::RESOLUTION640X240},
    {640, 480, CSlingbox::RESOLUTION640X480}
  };

  // See if the specified resolution matches something in our mapping array and
  // setup things accordingly
  for (unsigned int i = 0; i < 11; i++)
  {
    if (m_sSlingboxSettings.iVideoWidth == (int)m_resolutionMap[i].uiWidth &&
      m_sSlingboxSettings.iVideoHeight == (int)m_resolutionMap[i].uiHeight)
    {
      m_sSlingboxSettings.iVideoResolution = (int)m_resolutionMap[i].eEnum;
      return;
    }
  }

  // If it didn't match anything setup safe defaults
  CLog::Log(LOGERROR, "%s - Defaulting to 320x240 resolution due to invalid "
    "resolution specified in SlingboxSettings.xml for Slingbox: %s",
    __FUNCTION__, m_sSlingboxSettings.strHostname.c_str());
  m_sSlingboxSettings.iVideoWidth = 320;
  m_sSlingboxSettings.iVideoHeight = 240;
  m_sSlingboxSettings.iVideoResolution = (int)CSlingbox::RESOLUTION320X240;
}
