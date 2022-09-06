/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace PVR
{

class CPVRClientCapabilities
{
public:
  CPVRClientCapabilities() = default;
  virtual ~CPVRClientCapabilities() = default;

  CPVRClientCapabilities(const CPVRClientCapabilities& other);
  const CPVRClientCapabilities& operator=(const CPVRClientCapabilities& other);

  const CPVRClientCapabilities& operator=(const PVR_ADDON_CAPABILITIES& addonCapabilities);

  void clear();

  /////////////////////////////////////////////////////////////////////////////////
  //
  // Channels
  //
  /////////////////////////////////////////////////////////////////////////////////

  /*!
   * @brief Check whether this add-on supports TV channels.
   * @return True if supported, false otherwise.
   */
  bool SupportsTV() const { return m_addonCapabilities && m_addonCapabilities->bSupportsTV; }

  /*!
   * @brief Check whether this add-on supports radio channels.
   * @return True if supported, false otherwise.
   */
  bool SupportsRadio() const { return m_addonCapabilities && m_addonCapabilities->bSupportsRadio; }

  /*!
   * @brief Check whether this add-on supports providers.
   * @return True if supported, false otherwise.
   */
  bool SupportsProviders() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsProviders;
  }

  /*!
   * @brief Check whether this add-on supports channel groups.
   * @return True if supported, false otherwise.
   */
  bool SupportsChannelGroups() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsChannelGroups;
  }

  /*!
   * @brief Check whether this add-on supports scanning for new channels on the backend.
   * @return True if supported, false otherwise.
   */
  bool SupportsChannelScan() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsChannelScan;
  }

  /*!
   * @brief Check whether this add-on supports the following functions:
   * DeleteChannel, RenameChannel, DialogChannelSettings and DialogAddChannel.
   *
   * @return True if supported, false otherwise.
   */
  bool SupportsChannelSettings() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsChannelSettings;
  }

  /*!
   * @brief Check whether this add-on supports descramble information for playing channels.
   * @return True if supported, false otherwise.
   */
  bool SupportsDescrambleInfo() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsDescrambleInfo;
  }

  /////////////////////////////////////////////////////////////////////////////////
  //
  // EPG
  //
  /////////////////////////////////////////////////////////////////////////////////

  /*!
   * @brief Check whether this add-on provides EPG information.
   * @return True if supported, false otherwise.
   */
  bool SupportsEPG() const { return m_addonCapabilities && m_addonCapabilities->bSupportsEPG; }

  /*!
   * @brief Check whether this add-on supports asynchronous transfer of epg events.
   * @return True if supported, false otherwise.
   */
  bool SupportsAsyncEPGTransfer() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsAsyncEPGTransfer;
  }

  /////////////////////////////////////////////////////////////////////////////////
  //
  // Timers
  //
  /////////////////////////////////////////////////////////////////////////////////

  /*!
   * @brief Check whether this add-on supports the creation and editing of timers.
   * @return True if supported, false otherwise.
   */
  bool SupportsTimers() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsTimers;
  }

  /////////////////////////////////////////////////////////////////////////////////
  //
  // Recordings
  //
  /////////////////////////////////////////////////////////////////////////////////

  /*!
   * @brief Check whether this add-on supports recordings.
   * @return True if supported, false otherwise.
   */
  bool SupportsRecordings() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings;
  }

  /*!
   * @brief Check whether this add-on supports undelete of deleted recordings.
   * @return True if supported, false otherwise.
   */
  bool SupportsRecordingsUndelete() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings &&
           m_addonCapabilities->bSupportsRecordingsUndelete;
  }

  /*!
   * @brief Check whether this add-on supports play count for recordings.
   * @return True if supported, false otherwise.
   */
  bool SupportsRecordingsPlayCount() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings &&
           m_addonCapabilities->bSupportsRecordingPlayCount;
  }

  /*!
   * @brief Check whether this add-on supports store/retrieve of last played position for recordings..
   * @return True if supported, false otherwise.
   */
  bool SupportsRecordingsLastPlayedPosition() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings &&
           m_addonCapabilities->bSupportsLastPlayedPosition;
  }

  /*!
   * @brief Check whether this add-on supports retrieving an edit decision list for recordings.
   * @return True if supported, false otherwise.
   */
  bool SupportsRecordingsEdl() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings &&
           m_addonCapabilities->bSupportsRecordingEdl;
  }

  /*!
   * @brief Check whether this add-on supports retrieving an edit decision list for epg tags.
   * @return True if supported, false otherwise.
   */
  bool SupportsEpgTagEdl() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsEPG &&
           m_addonCapabilities->bSupportsEPGEdl;
  }

  /*!
   * @brief Check whether this add-on supports renaming recordings..
   * @return True if supported, false otherwise.
   */
  bool SupportsRecordingsRename() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings &&
           m_addonCapabilities->bSupportsRecordingsRename;
  }

  /*!
   * @brief Check whether this add-on supports changing lifetime of recording.
   * @return True if supported, false otherwise.
   */
  bool SupportsRecordingsLifetimeChange() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings &&
           m_addonCapabilities->bSupportsRecordingsLifetimeChange;
  }

  /*!
   * @brief Obtain a list with all possible values for recordings lifetime.
   * @param list out, the list with the values or an empty list, if lifetime is not supported.
   */
  void GetRecordingsLifetimeValues(std::vector<std::pair<std::string, int>>& list) const;

  /*!
   * @brief Check whether this add-on supports retrieving the size recordings..
   * @return True if supported, false otherwise.
   */
  bool SupportsRecordingsSize() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings &&
           m_addonCapabilities->bSupportsRecordingSize;
  }

  /*!
   * @brief Check whether this add-on supports deleting recordings.
   * @return True if supported, false otherwise.
   */
  bool SupportsRecordingsDelete() const
  {
    return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings &&
           m_addonCapabilities->bSupportsRecordingsDelete;
  }

  /////////////////////////////////////////////////////////////////////////////////
  //
  // Streams
  //
  /////////////////////////////////////////////////////////////////////////////////

  /*!
   * @brief Check whether this add-on provides an input stream. false if Kodi handles the stream.
   * @return True if supported, false otherwise.
   */
  bool HandlesInputStream() const
  {
    return m_addonCapabilities && m_addonCapabilities->bHandlesInputStream;
  }

  /*!
   * @brief Check whether this add-on demultiplexes packets.
   * @return True if supported, false otherwise.
   */
  bool HandlesDemuxing() const
  {
    return m_addonCapabilities && m_addonCapabilities->bHandlesDemuxing;
  }

private:
  void InitRecordingsLifetimeValues();

  std::unique_ptr<PVR_ADDON_CAPABILITIES> m_addonCapabilities;
  std::vector<std::pair<std::string, int>> m_recordingsLifetimeValues;
};

} // namespace PVR
