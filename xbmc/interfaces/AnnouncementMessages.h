/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//! \brief The messages announced through CAnnouncementManager. JSON-RPC publishes each as the
//! method of a notification, prefixed by its flag, e.g. "Player.OnPlay".
namespace ANNOUNCEMENT::MESSAGE
{

inline constexpr char ON_ADD[] = "OnAdd";
inline constexpr char ON_ADDED[] = "OnAdded";
inline constexpr char ON_AUDIO_FORMAT_HOLD[] = "OnAudioFormatHold";
inline constexpr char ON_AV_CHANGE[] = "OnAVChange";
inline constexpr char ON_AV_START[] = "OnAVStart";
inline constexpr char ON_BLURAY_ENCRYPTED_ERROR[] = "OnBlurayEncryptedError";
inline constexpr char ON_BLURAY_MENU_ERROR[] = "OnBlurayMenuError";
inline constexpr char ON_CHANGED[] = "OnChanged";
inline constexpr char ON_CLEAN_FINISHED[] = "OnCleanFinished";
inline constexpr char ON_CLEAN_STARTED[] = "OnCleanStarted";
inline constexpr char ON_CLEAR[] = "OnClear";
inline constexpr char ON_COMMERCIAL[] = "OnCommercial";
inline constexpr char ON_CONTENT_GEOMETRY_CHANGE[] = "OnContentGeometryChange";
inline constexpr char ON_DPMS_ACTIVATED[] = "OnDPMSActivated";
inline constexpr char ON_DPMS_DEACTIVATED[] = "OnDPMSDeactivated";
inline constexpr char ON_EXPORT[] = "OnExport";
inline constexpr char ON_INPUT_FINISHED[] = "OnInputFinished";
inline constexpr char ON_INPUT_REQUESTED[] = "OnInputRequested";
inline constexpr char ON_LEVEL_CHANGED[] = "OnLevelChanged";
inline constexpr char ON_LOW_BATTERY[] = "OnLowBattery";
inline constexpr char ON_MENU[] = "OnMenu";
inline constexpr char ON_PAUSE[] = "OnPause";
inline constexpr char ON_PLAY[] = "OnPlay";
inline constexpr char ON_PLAY_COUNT_CHANGED[] = "OnPlayCountChanged";
inline constexpr char ON_PLAYBACK_FAILED[] = "OnPlaybackFailed";
inline constexpr char ON_PROCESS_INFO[] = "OnProcessInfo";
inline constexpr char ON_PROPERTIES_CHANGED[] = "OnPropertiesChanged";
inline constexpr char ON_QUIT[] = "OnQuit";
inline constexpr char ON_REFRESH[] = "OnRefresh";
inline constexpr char ON_REMOVE[] = "OnRemove";
inline constexpr char ON_REMOVED[] = "OnRemoved";
inline constexpr char ON_RESTART[] = "OnRestart";
inline constexpr char ON_RESUME[] = "OnResume";
inline constexpr char ON_RESUME_POINT_CHANGED[] = "OnResumePointChanged";
inline constexpr char ON_SCAN_FINISHED[] = "OnScanFinished";
inline constexpr char ON_SCAN_STARTED[] = "OnScanStarted";
inline constexpr char ON_SCREENSAVER_ACTIVATED[] = "OnScreensaverActivated";
inline constexpr char ON_SCREENSAVER_DEACTIVATED[] = "OnScreensaverDeactivated";
inline constexpr char ON_SEEK[] = "OnSeek";
inline constexpr char ON_SKIN_LOAD_FAILED[] = "OnSkinLoadFailed";
inline constexpr char ON_SKIN_LOADED[] = "OnSkinLoaded";
inline constexpr char ON_SKIN_UNLOADING[] = "OnSkinUnloading";
inline constexpr char ON_SLEEP[] = "OnSleep";
inline constexpr char ON_SPEED_CHANGED[] = "OnSpeedChanged";
inline constexpr char ON_STOP[] = "OnStop";
inline constexpr char ON_TOGGLE_SKIP_COMMERCIALS[] = "OnToggleSkipCommercials";
inline constexpr char ON_UPDATE[] = "OnUpdate";
inline constexpr char ON_UPDATED[] = "OnUpdated";
inline constexpr char ON_WAKE[] = "OnWake";
inline constexpr char RDS_RADIO_RTC[] = "RDSRadioRTC";
inline constexpr char RDS_RADIO_TA[] = "RDSRadioTA";
inline constexpr char RDS_RADIO_TMC[] = "RDSRadioTMC";
inline constexpr char SOURCE_SLOW[] = "SourceSlow";
inline constexpr char WINDOW_FOCUSED[] = "WindowFocused";
inline constexpr char WINDOW_UNFOCUSED[] = "WindowUnfocused";

} // namespace ANNOUNCEMENT::MESSAGE
