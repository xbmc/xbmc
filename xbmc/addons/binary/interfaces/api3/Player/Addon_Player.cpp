/*
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon_Player.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/Player/Addon_Player.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace Player
{
extern "C"
{

void CAddOnPlayer::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->AddonPlayer.GetSupportedMedia            = V2::KodiAPI::Player::CAddOnPlayer::GetSupportedMedia;
  interfaces->AddonPlayer.New                          = V2::KodiAPI::Player::CAddOnPlayer::New;
  interfaces->AddonPlayer.Delete                       = V2::KodiAPI::Player::CAddOnPlayer::Delete;
  interfaces->AddonPlayer.PlayFile                     = V2::KodiAPI::Player::CAddOnPlayer::PlayFile;
  interfaces->AddonPlayer.PlayFileItem                 = V2::KodiAPI::Player::CAddOnPlayer::PlayFileItem;
  interfaces->AddonPlayer.Stop                         = V2::KodiAPI::Player::CAddOnPlayer::Stop;
  interfaces->AddonPlayer.Pause                        = V2::KodiAPI::Player::CAddOnPlayer::Pause;
  interfaces->AddonPlayer.PlayNext                     = V2::KodiAPI::Player::CAddOnPlayer::PlayNext;
  interfaces->AddonPlayer.PlayPrevious                 = V2::KodiAPI::Player::CAddOnPlayer::PlayPrevious;
  interfaces->AddonPlayer.PlaySelected                 = V2::KodiAPI::Player::CAddOnPlayer::PlaySelected;
  interfaces->AddonPlayer.IsPlaying                    = V2::KodiAPI::Player::CAddOnPlayer::IsPlaying;
  interfaces->AddonPlayer.IsPlayingAudio               = V2::KodiAPI::Player::CAddOnPlayer::IsPlayingAudio;
  interfaces->AddonPlayer.IsPlayingVideo               = V2::KodiAPI::Player::CAddOnPlayer::IsPlayingVideo;
  interfaces->AddonPlayer.IsPlayingRDS                 = V2::KodiAPI::Player::CAddOnPlayer::IsPlayingRDS;
  interfaces->AddonPlayer.GetPlayingFile               = V2::KodiAPI::Player::CAddOnPlayer::GetPlayingFile;
  interfaces->AddonPlayer.GetTotalTime                 = V2::KodiAPI::Player::CAddOnPlayer::GetTotalTime;
  interfaces->AddonPlayer.GetTime                      = V2::KodiAPI::Player::CAddOnPlayer::GetTime;
  interfaces->AddonPlayer.SeekTime                     = V2::KodiAPI::Player::CAddOnPlayer::SeekTime;
  interfaces->AddonPlayer.GetAvailableVideoStreams     = V2::KodiAPI::Player::CAddOnPlayer::GetAvailableVideoStreams;
  interfaces->AddonPlayer.SetVideoStream               = V2::KodiAPI::Player::CAddOnPlayer::SetVideoStream;
  interfaces->AddonPlayer.GetAvailableAudioStreams     = V2::KodiAPI::Player::CAddOnPlayer::GetAvailableAudioStreams;
  interfaces->AddonPlayer.SetAudioStream               = V2::KodiAPI::Player::CAddOnPlayer::SetAudioStream;
  interfaces->AddonPlayer.GetAvailableSubtitleStreams  = V2::KodiAPI::Player::CAddOnPlayer::GetAvailableSubtitleStreams;
  interfaces->AddonPlayer.SetSubtitleStream            = V2::KodiAPI::Player::CAddOnPlayer::SetSubtitleStream;
  interfaces->AddonPlayer.ShowSubtitles                = V2::KodiAPI::Player::CAddOnPlayer::ShowSubtitles;
  interfaces->AddonPlayer.GetCurrentSubtitleName       = V2::KodiAPI::Player::CAddOnPlayer::GetCurrentSubtitleName;
  interfaces->AddonPlayer.AddSubtitle                  = V2::KodiAPI::Player::CAddOnPlayer::AddSubtitle;
  interfaces->AddonPlayer.ClearList                    = V2::KodiAPI::Player::CAddOnPlayer::ClearList;
}

} /* extern "C" */
} /* namespace Player */

} /* namespace KodiAPI */
} /* namespace V3 */
