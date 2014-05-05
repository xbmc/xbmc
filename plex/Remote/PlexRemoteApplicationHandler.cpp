#include "PlexRemoteApplicationHandler.h"
#include "PlexApplication.h"
#include "guilib/GUIWindowManager.h"
#include "ApplicationMessenger.h"
#include "Client/PlexTimelineManager.h"
#include "Application.h"
#include <boost/lexical_cast.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemoteApplicationHandler::handle(const CStdString &url, const ArgMap &arguments)
{
  if (url.Equals("/player/application/sendString") ||
      url.Equals("/player/application/setText"))
    return sendString(arguments);
  else if (url.Equals("/player/application/sendVirtualKey") ||
           url.Equals("/player/application/sendKey"))
    return sendVKey(arguments);

  return CPlexRemoteResponse(500, "not implemented");
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemoteApplicationHandler::sendString(const ArgMap &arguments)
{
  std::string newString;

  /* old school */
  if (arguments.find("text") != arguments.end())
  {
    newString = arguments.find("text")->second;
  }
  else if (g_plexApplication.timelineManager->IsTextFieldFocused())
  {
    if (arguments.find(g_plexApplication.timelineManager->GetCurrentFocusedTextField()) != arguments.end())
      newString = arguments.find(g_plexApplication.timelineManager->GetCurrentFocusedTextField())->second;
  }

  g_application.WakeUpScreenSaverAndDPMS();

  int currentWindow = g_windowManager.GetActiveWindow();
  CGUIWindow* win = g_windowManager.GetWindow(currentWindow);
  CGUIControl* ctrl = NULL;

  if (currentWindow == WINDOW_PLEX_SEARCH)
    ctrl = (CGUIControl*)win->GetControl(310);
  else
    ctrl = win->GetFocusedControl();

  if (!ctrl)
    return CPlexRemoteResponse(500, "Internal error");

  if (ctrl->GetControlType() != CGUIControl::GUICONTROL_EDIT)
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::sendString focused control %d is not a edit control", ctrl->GetID());
    return CPlexRemoteResponse(500, "Current focused control doesn't accept text");
  }

  /* instead of calling keyb->SetText() we want to send this as a message
   * to avoid any thread locking and contention */
  CGUIMessage msg(GUI_MSG_SET_TEXT, 0, ctrl->GetID());
  msg.SetLabel(newString);
  msg.SetParam1(0);

  CApplicationMessenger::Get().SendGUIMessage(msg, g_windowManager.GetFocusedWindow());

  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemoteApplicationHandler::sendVKey(const ArgMap &arguments)
{
  if (arguments.find("code") == arguments.end())
    return CPlexRemoteResponse(500, "Missing code argument");

  int code = boost::lexical_cast<int>(arguments.find("code")->second);
  CLog::Log(LOGDEBUG, "CPlexHTTPRemoteHandler::sendVKey got code %d", code);
  int action;

  switch (code)
  {
    case 8:
      action = ACTION_BACKSPACE;
      break;
    case 13:
      action = ACTION_SELECT_ITEM;
      break;
    default:
      action = ACTION_NONE;
  }

  if (action != ACTION_NONE)
  {
    g_application.WakeUpScreenSaverAndDPMS();
    CApplicationMessenger::Get().SendAction(CAction(action), g_windowManager.GetFocusedWindow(), false);
  }

  return CPlexRemoteResponse();
}
