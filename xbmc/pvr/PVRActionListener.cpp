/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"

#include "PVRActionListener.h"

namespace PVR
{

CPVRActionListener &CPVRActionListener::GetInstance()
{
  static CPVRActionListener instance;
  return instance;
}

bool CPVRActionListener::OnAction(const CAction &action)
{
  bool bIsJumpSMS = false;

  switch (action.GetID())
  {
    case ACTION_PVR_PLAY:
    case ACTION_PVR_PLAY_TV:
    case ACTION_PVR_PLAY_RADIO:
    {
      // see if we're already playing a PVR stream and if not or the stream type
      // doesn't match the demanded type, start playback of according type
      bool isPlayingPvr(g_PVRManager.IsPlaying() && g_application.CurrentFileItem().HasPVRChannelInfoTag());
      switch (action.GetID())
      {
        case ACTION_PVR_PLAY:
          if (!isPlayingPvr)
            CPVRGUIActions::GetInstance().SwitchToChannel(PlaybackTypeAny);
          break;
        case ACTION_PVR_PLAY_TV:
          if (!isPlayingPvr || g_application.CurrentFileItem().GetPVRChannelInfoTag()->IsRadio())
            CPVRGUIActions::GetInstance().SwitchToChannel(PlaybackTypeTV);
          break;
        case ACTION_PVR_PLAY_RADIO:
          if (!isPlayingPvr || !g_application.CurrentFileItem().GetPVRChannelInfoTag()->IsRadio())
            CPVRGUIActions::GetInstance().SwitchToChannel(PlaybackTypeRadio);
          break;
      }
      return true;
    }

    case ACTION_CONTINUE_LAST_CHANNEL:
      return CPVRGUIActions::GetInstance().ContinueLastPlayedChannel();

    case ACTION_JUMP_SMS2:
    case ACTION_JUMP_SMS3:
    case ACTION_JUMP_SMS4:
    case ACTION_JUMP_SMS5:
    case ACTION_JUMP_SMS6:
    case ACTION_JUMP_SMS7:
    case ACTION_JUMP_SMS8:
    case ACTION_JUMP_SMS9:
      bIsJumpSMS = true;
      // fallthru is intended
    case REMOTE_0:
    case REMOTE_1:
    case REMOTE_2:
    case REMOTE_3:
    case REMOTE_4:
    case REMOTE_5:
    case REMOTE_6:
    case REMOTE_7:
    case REMOTE_8:
    case REMOTE_9:
    {
      if (g_application.CurrentFileItem().IsLiveTV() &&
          (g_windowManager.IsWindowActive(WINDOW_FULLSCREEN_VIDEO) ||
           g_windowManager.IsWindowActive(WINDOW_VISUALISATION)))
      {
        // do not consume action if a python modal is the top most dialog
        // as a python modal can't return that it consumed the action.
        if (g_windowManager.IsPythonWindow(g_windowManager.GetTopMostModalDialogID()))
          return false;

        int iRemote = bIsJumpSMS ? action.GetID() - (ACTION_JUMP_SMS2 - REMOTE_2) : action.GetID();
        CPVRGUIActions::GetInstance().GetChannelNumberInputHandler().AppendChannelNumberDigit(iRemote - REMOTE_0);
      }
      return true;
    }
    break;
  }
  return false;
}

} // namespace PVR
