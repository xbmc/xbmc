//
//  GUIWindowPlexStartupHelper.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-09-25.
//
//

#include "GUIWindowPlexStartupHelper.h"
#include "FileItem.h"
#include "PlexTypes.h"
#include "StdString.h"
#include "Variant.h"
#include "ApplicationMessenger.h"
#include "GUIWindowManager.h"
#include "PlexApplication.h"
#include "Client/MyPlex/MyPlexManager.h"
#include "cores/AudioEngine/AEFactory.h"
#include "utils/log.h"
#include "guilib/GUIRadioButtonControl.h"
#include "settings/GUISettings.h"
#include "cores/AudioEngine/Utils/AEChannelInfo.h"

#ifdef TARGET_DARWIN_OSX
#include "cores/AudioEngine/Engines/CoreAudio/CoreAudioDevice.h"
#include "cores/AudioEngine/Engines/CoreAudio/CoreAudioHardware.h"
#endif

#include <boost/foreach.hpp>

#define RADIO_BUTTON_ANALOG 211
#define RADIO_BUTTON_OPTICAL 212
#define RADIO_BUTTON_HDMI 213

CGUIWindowPlexStartupHelper::CGUIWindowPlexStartupHelper() :
  CGUIWindow(WINDOW_PLEX_STARTUP_HELPER, "PlexStartupHelper.xml")
{
  m_loadType = LOAD_EVERY_TIME;
  m_item = CFileItemPtr(new CFileItem);
  SetPage(WIZARD_PAGE_WELCOME);
}

bool CGUIWindowPlexStartupHelper::OnMessage(CGUIMessage &message)
{
#ifdef TARGET_RASPBERRY_PI
  g_guiSettings.SetInt("audiooutput.mode", AUDIO_HDMI);
  g_guiSettings.SetInt("audiooutput.channels", GetNumberOfHDMIChannels());
  g_guiSettings.SetBool("audiooutput.ac3passthrough", false);
  g_guiSettings.SetBool("audiooutput.dtspassthrough", false);
  if (!g_plexApplication.myPlexManager->IsSignedIn())
  {
    std::vector<CStdString> param;
    param.push_back("gohome");
    g_windowManager.ActivateWindow(WINDOW_MYPLEX_LOGIN, param, true);
  }
  else
  {
    g_windowManager.ActivateWindow(WINDOW_HOME, std::vector<CStdString>(), true);
  }

#else
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetSenderId() == 121)
    {
      switch (m_page) {
        case WIZARD_PAGE_WELCOME:
          SetPage(WIZARD_PAGE_AUDIO);
          break;
        case WIZARD_PAGE_AUDIO:
        {
          if (!g_plexApplication.myPlexManager->IsSignedIn())
          {
            std::vector<CStdString> param;
            param.push_back("gohome");
            g_windowManager.ActivateWindow(WINDOW_MYPLEX_LOGIN, param, true);
          }
          else
          {
            g_windowManager.ActivateWindow(WINDOW_HOME, std::vector<CStdString>(), true);
          }
          break;
        }
        default:
          break;
      }
    }
    else if (message.GetSenderId() == RADIO_BUTTON_ANALOG ||
             message.GetSenderId() == RADIO_BUTTON_OPTICAL ||
             message.GetSenderId() == RADIO_BUTTON_HDMI)
    {
      AudioControlSelected(message.GetSenderId());
      return true;
    }
  }
#endif 
  return CGUIWindow::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexStartupHelper::AudioControlSelected(int id)
{
  CGUIRadioButtonControl *analog = (CGUIRadioButtonControl*)(GetControl(RADIO_BUTTON_ANALOG));
  CGUIRadioButtonControl *optical = (CGUIRadioButtonControl*)(GetControl(RADIO_BUTTON_OPTICAL));
  CGUIRadioButtonControl *hdmi = (CGUIRadioButtonControl*)(GetControl(RADIO_BUTTON_HDMI));

  if (!analog || !optical || !hdmi)
    return;

  analog->SetSelected(false);
  optical->SetSelected(false);
  hdmi->SetSelected(false);

  CStdString outputDevice = "default";
  if (id == RADIO_BUTTON_ANALOG)
  {
    analog->SetSelected(true);
    g_guiSettings.SetInt("audiooutput.mode", AUDIO_ANALOG);
    g_guiSettings.SetInt("audiooutput.channels", AE_CH_LAYOUT_2_0);
    g_guiSettings.SetBool("audiooutput.ac3passthrough", false);
    g_guiSettings.SetBool("audiooutput.dtspassthrough", false);

#ifdef TARGET_DARWIN_OSX
    outputDevice = "CoreAudio:Built-In Output";
#endif
  }
  else if (id == RADIO_BUTTON_OPTICAL)
  {
    optical->SetSelected(true);
    g_guiSettings.SetInt("audiooutput.mode", AUDIO_IEC958);
    g_guiSettings.SetInt("audiooutput.channels", AE_CH_LAYOUT_5_1);
    g_guiSettings.SetBool("audiooutput.ac3passthrough", true);
    g_guiSettings.SetBool("audiooutput.dtspassthrough", true);

#ifdef TARGET_DARWIN_OSX
    outputDevice = "CoreAudio:Built-In Output";
#endif
  }
  else if (id == RADIO_BUTTON_HDMI)
  {
    hdmi->SetSelected(true);
    g_guiSettings.SetInt("audiooutput.mode", AUDIO_HDMI);
    g_guiSettings.SetInt("audiooutput.channels", GetNumberOfHDMIChannels());
    g_guiSettings.SetBool("audiooutput.ac3passthrough", true);
    g_guiSettings.SetBool("audiooutput.dtspassthrough", true);

#ifdef TARGET_DARWIN_OSX
    outputDevice = "CoreAudio:HDMI";
#endif
  }

  CAEFactory::VerifyOutputDevice(outputDevice, false);
  g_guiSettings.SetString("audiooutput.audiodevice", outputDevice);
  CAEFactory::OnSettingsChange("audiooutput.mode");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexStartupHelper::SetupAudioStuff()
{
  AEDeviceList devices;

  CAEFactory::EnumerateOutputDevices(devices, true);
  std::pair<CStdString, CStdString> dev;

  bool hasPassthrough = false;
#ifdef TARGET_DARWIN_OSX
  BOOST_FOREACH(dev, devices)
  {
    CLog::Log(LOGDEBUG, "CGUIWindowPlexStartupHelper::SetupAudioStuff %s | %s", dev.first.c_str(), dev.second.c_str());
    if (dev.first == "HDMI") // on OSX the output device is always called HDMI
      hasPassthrough = true;
  }
#else
  hasPassthrough = devices.size() > 0;
#endif

  m_item->SetProperty("HDMI", hasPassthrough ? "yes" : "");

  if (!hasPassthrough)
    AudioControlSelected(RADIO_BUTTON_ANALOG);
  else
    AudioControlSelected(RADIO_BUTTON_HDMI);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CGUIWindowPlexStartupHelper::GetNumberOfHDMIChannels()
{
#ifdef TARGET_DARWIN_OSX
  AudioDeviceID id = CCoreAudioHardware::FindAudioDevice("HDMI");
  CCoreAudioDevice device(id);

  int channels = device.GetTotalOutputChannels();
  if (channels < 3) return AE_CH_LAYOUT_2_0;
  else if (channels >= 3 && channels <= 6) return AE_CH_LAYOUT_5_1;
  else return AE_CH_LAYOUT_7_1;
#endif

  return AE_CH_LAYOUT_2_0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexStartupHelper::SetPage(WizardPage page)
{
  m_page = page;
  m_item->SetProperty("WizardWelcome", CVariant());
  m_item->SetProperty("WizardMyPlex", CVariant());
  m_item->SetProperty("WizardAudio", CVariant());
  
  if (page == WIZARD_PAGE_WELCOME)
    m_item->SetProperty("WizardWelcome", 1);
  else if (page == WIZARD_PAGE_MYPLEX)
    m_item->SetProperty("WizardMyPlex", 1);
  else if (page == WIZARD_PAGE_AUDIO)
  {
    m_item->SetProperty("WizardAudio", 1);
    SetupAudioStuff();
  }
}
