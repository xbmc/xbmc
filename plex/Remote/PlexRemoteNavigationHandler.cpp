#include "PlexRemoteNavigationHandler.h"
#include "PlexApplication.h"
#include "ApplicationMessenger.h"
#include "Application.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIWindowManager.h"
#include <vector>

#define LEGACY 1

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemoteNavigationHandler::handle(const CStdString &url, const ArgMap &arguments)
{
  int action = ACTION_NONE;
  int activeWindow = g_windowManager.GetFocusedWindow();

  CStdString navigation = url.Mid(19, url.length() - 19);

  if (navigation.Equals("moveRight"))
    action = ACTION_MOVE_RIGHT;
  else if (navigation.Equals("moveLeft"))
    action = ACTION_MOVE_LEFT;
  else if (navigation.Equals("moveDown"))
    action = ACTION_MOVE_DOWN;
  else if (navigation.Equals("moveUp"))
    action = ACTION_MOVE_UP;
  else if (navigation.Equals("select"))
    action = ACTION_SELECT_ITEM;
  else if (navigation.Equals("music"))
  {
    if (g_application.IsPlayingAudio() && activeWindow != WINDOW_VISUALISATION)
      action = ACTION_SHOW_GUI;
  }
  else if (navigation.Equals("home"))
  {
    std::vector<CStdString> args;
    g_application.WakeUpScreenSaverAndDPMS();
    CApplicationMessenger::Get().ActivateWindow(WINDOW_HOME, args, false);
    return CPlexRemoteResponse();
  }
  else if (navigation.Equals("back"))
  {
    if (g_application.IsPlayingFullScreenVideo() &&
        (activeWindow != WINDOW_DIALOG_AUDIO_OSD_SETTINGS &&
         activeWindow != WINDOW_DIALOG_VIDEO_OSD_SETTINGS &&
         activeWindow != WINDOW_DIALOG_PLEX_AUDIO_PICKER &&
         activeWindow != WINDOW_DIALOG_PLEX_SUBTITLE_PICKER))
      action = ACTION_STOP;
    else
      action = ACTION_NAV_BACK;
  }

#ifdef LEGACY
  else if (navigation.Equals("contextMenu"))
    action = ACTION_CONTEXT_MENU;
  else if (navigation.Equals("toggleOSD"))
    action = ACTION_SHOW_OSD;
  else if (navigation.Equals("pageUp"))
    action = ACTION_PAGE_UP;
  else if (navigation.Equals("pageDown"))
    action = ACTION_PAGE_DOWN;
  else if (navigation.Equals("nextLetter"))
    action = ACTION_NEXT_LETTER;
  else if (navigation.Equals("previousLetter"))
    action = ACTION_PREV_LETTER;
#endif

  if (action != ACTION_NONE)
  {
    CAction actionId(action);

    g_application.WakeUpScreenSaverAndDPMS();

    g_application.ResetSystemIdleTimer();

    if (!g_application.IsPlaying())
      g_audioManager.PlayActionSound(actionId);

    CApplicationMessenger::Get().SendAction(actionId, WINDOW_INVALID, false);
  }

  return CPlexRemoteResponse();
}
