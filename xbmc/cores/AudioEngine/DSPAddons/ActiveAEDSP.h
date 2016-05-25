#pragma once
/*
 *      Copyright (C) 2010-2014 Team KODI
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

#include "ServiceBroker.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Observer.h"
#include "messaging/IMessageTarget.h"

#include "ActiveAEDSPAddon.h"
#include "ActiveAEDSPDatabase.h"
#include "ActiveAEDSPMode.h"

#define ACTIVE_AE_DSP_STATE_OFF       0
#define ACTIVE_AE_DSP_STATE_ON        1

#define ACTIVE_AE_DSP_SYNC_ACTIVATE   0
#define ACTIVE_AE_DSP_ASYNC_ACTIVATE  1

extern "C" {
#include "libavcodec/avcodec.h"
}

class CAction;

namespace ActiveAE
{
  class CActiveAEDSPProcess;
  class CActiveAEDSPAddon;

  typedef std::shared_ptr<ActiveAE::CActiveAEDSPProcess>  CActiveAEDSPProcessPtr;
  typedef std::map< int, AE_DSP_ADDON >                   AE_DSP_ADDONMAP;
  typedef std::map< int, AE_DSP_ADDON >::iterator         AE_DSP_ADDONMAP_ITR;
  typedef std::map< int, AE_DSP_ADDON >::const_iterator   AE_DSP_ADDONMAP_CITR;

  //@{
  /*!
   * Static dsp handling class
   */
  class CActiveAEDSP : public ADDON::IAddonMgrCallback,
                       public ISettingCallback
  {
  /*! @name Master audio dsp control class */
  //@{
  public:
    /*!
     * @brief Create a new CActiveAEDSP instance, which handles all audio DSP related operations in KODI.
     */
    CActiveAEDSP(void);

    /*!
     * @brief Stop the ActiveAEDSP and destroy all objects it created.
     */
    virtual ~CActiveAEDSP();

    void Init(void);
  //@}

  /*! @name initialization and configuration methods */
  //@{
    /*!
     * @brief Activate the addon dsp processing.
     */
    void Activate(void);

    /*!
     * @brief Stops dsp processing and the backend info update thread.
     */
    void Deactivate(void);

    /*!
     * @brief Delete all objects and processing classes.
     */
    void Cleanup(void);

    /*!
     * @brief Reset the audio dsp database to it's initial state and delete all the data inside.
     */
    void ResetDatabase(void);

    /*!
     * @brief Check whether an add-on can be upgraded or installed without restarting the audio dsp, when the add-on is in use
     * @param strAddonId The add-on to check.
     * @return True when the add-on can be installed, false otherwise.
     */
    bool InstallAddonAllowed(const std::string& strAddonId) const;

    /*!
     * @brief Get the audio dsp database pointer.
     * @return The audio dsp database.
     */
    CActiveAEDSPDatabase *GetADSPDatabase(void) { return &m_databaseDSP; }
  //@}

  /*! @name Settings and action callback methods */
  //@{
    virtual void OnSettingAction(const CSetting *setting) override;
  //@}

  /*! @name Backend methods */
  //@{
    /*!
     * @return True when processing is possible
     */
    bool IsActivated(void) const;

    /*!
     * @return True when processing is active
     */
    bool IsProcessing(void) const;
  //@}

  /*! @name addon installation callback methods */
  //@{
    /*!
     * @brief Restart a single audio dsp addon add-on.
     * @param addon The add-on to restart.
     * @param bDataChanged True if the addon's data changed, false otherwise (unused).
     * @return True if the audio dsp addon was found and restarted, false otherwise.
     */
    virtual bool RequestRestart(ADDON::AddonPtr addon, bool bDataChanged) override;

    /*!
     * @brief Remove a single audio dsp add-on.
     * @param addon The add-on to remove.
     * @return True if it was found and removed, false otherwise.
     */
    virtual bool RequestRemoval(ADDON::AddonPtr addon) override;

    /*!
     * @brief Checks whether an add-on is loaded
     * @param strAddonId The add-on id to check
     * @return True when in use, false otherwise
     */
    bool IsInUse(const std::string& strAddonId) const;

    /*!
     * @brief Stop a audio dsp addon.
     * @param addon The dsp addon to stop.
     * @param bRestart If true, restart the addon.
     * @return True if the it was found, false otherwise.
     */
    bool StopAudioDSPAddon(ADDON::AddonPtr addon, bool bRestart);

    /*!
     * @return The amount of enabled audio dsp addons.
     */
    int EnabledAudioDSPAddonAmount(void) const;

    /*!
     * @return True when at least one audio dsp addon is known and enabled, false otherwise.
     */
    bool HasEnabledAudioDSPAddons(void) const;

    /*!
     * @brief Get all enabled audio dsp addons.
     * @param addons Store the enabled addons in this map.
     * @return The amount of enabled audio addons.
     */
    int GetEnabledAudioDSPAddons(AE_DSP_ADDONMAP &addons) const;

    /*!
     * @return The amount of ready audio dsp addons on current stream.
     */
    int ReadyAudioDSPAddonAmount(void) const;

    /*!
     * @brief Check whether there are any ready audio dsp addons.
     * @return True if at least one audio dsp addon is ready.
     */
    bool HasReadyAudioDSPAddons(void) const;

    /*!
     * @brief Check whether a audio dsp addon ID points to a valid and ready add-on.
     * @param iAddonId The addon ID.
     * @return True when the addon ID is valid and ready, false otherwise.
     */
    bool IsReadyAudioDSPAddon(int iAddonId) const;

    /*!
     * @brief Check whether a audio dsp addon pointer points to a valid and ready add-on.
     * @param addon The addon addon pointer.
     * @return True when the addon pointer is valid and ready, false otherwise.
     */
    bool IsReadyAudioDSPAddon(const ADDON::AddonPtr& addon);

    /*!
     * @brief Get the instance of the audio dsp addon.
     * @param strId The string id of the addon to get.
     * @param addon The audio dsp addon.
     * @return True if the addon was found, false otherwise.
     */
    bool GetAudioDSPAddon(const std::string &strId, ADDON::AddonPtr &addon) const;

    /*!
     * @brief Get the instance of the audio dsp addon.
     * @param iAddonId The id of the addon to get.
     * @param addon The audio dsp addon.
     * @return True if the addon was found, false otherwise.
     */
    bool GetAudioDSPAddon(int iAddonId, AE_DSP_ADDON &addon) const;

    /*!
     * @brief Get the friendly name for the audio dsp addon with the given id.
     * @param iAddonId The id of the addon.
     * @param strName The friendly name of the audio dsp addon or an empty string when it wasn't found.
     * @return True if it was found, false otherwise.
     */
    bool GetAudioDSPAddonName(int iAddonId, std::string &strName) const;

    /*!
     * @brief Update add-ons from the AddonManager
     */
    void UpdateAddons(void);

    int GetAddonId(const std::string& strId) const;
  //@}

  /*! @name GUIInfoManager calls */
  //@{
    /*!
     * @brief Get a GUIInfoManager boolean.
     * @param dwInfo The boolean to get.
     * @return The requested boolean or false if it wasn't found.
     */
    bool TranslateBoolInfo(DWORD dwInfo) const;

    /*!
     * @brief Get a GUIInfoManager character string.
     * @param dwInfo The string to get.
     * @return The requested string or an empty one if it wasn't found.
     */
    bool TranslateCharInfo(DWORD dwInfo, std::string &strValue) const;
  //@}

  /*! @name Current processing streams control function methods */
  //@{
    /*!>
     * Get the channel position defination for given channel layout
     * @param stdLayout The layout identifier
     * @return the from given identifier set channel information class
     */
    CAEChannelInfo GetInternalChannelLayout(AEStdChLayout stdLayout);

    /*!>
     * Create the dsp processing with check of all addons about the used input and output audio format.
     * @param streamId The id of this stream
     * @param inputFormat The used audio stream input format
     * @param outputFormat Audio output format which is needed to send to the sinks
     * @param quality The requested quality from settings
     * @param wasActive if it is true a recreation of present stream control becomes performed (process class becomes not deleted)
     * @return True if the dsp processing becomes available
     */
    bool CreateDSPs(unsigned int &streamId, CActiveAEDSPProcessPtr &process, const AEAudioFormat &inputFormat, const AEAudioFormat &outputFormat,
                    bool upmix, AEQuality quality, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type,
                    int profile, bool wasActive = false);

    /*!>
     * Destroy all allocated dsp addons for this stream id and stops the processing.
     * @param streamId The id of this stream
     */
    void DestroyDSPs(unsigned int streamId);

    /*!>
     * Get the dsp processing class of given stream id
     * @param streamId The id of this stream
     */
    CActiveAEDSPProcessPtr GetDSPProcess(unsigned int streamId);

    /*!>
     * Get the amount of used dsp process stream handlers
     * @return Returns amount of processes
     */
    unsigned int GetProcessingStreamsAmount(void);

    /*!>
     * Get the currently active processing stream id
     * @return Stream id, or max unsigned int value (-1) if not active
     */
    unsigned int GetActiveStreamId(void);

    /*!
     * @brief Check for available modes present from add-ons
     *
     * @return true if one or more modes are available
     */
    bool HasAvailableModes(void) const;

    /*!>
     * Used to get all available modes on currently enabled add-ons
     * It is used from CActiveAEDSPProcess to get a sorted modes list for a processing
     * over the add-ons, several call to the same addon is possible with different mode id's.
     * @param modeType The type to get
     * @return modes Pointer to a buffer array where all available modes of type written in
     */
    const AE_DSP_MODELIST &GetAvailableModes(AE_DSP_MODE_TYPE modeType);

    /*!
     * @brief Load the settings for the current audio from the database.
     * @return If it was present inside settings it return the type of this settings
     */
    AE_DSP_STREAMTYPE LoadCurrentAudioSettings(void);

    /*!
     * @brief Perfoms a update of all processing calls over the add-ons
     * @param bAsync if true the update becomes performed on background
     */
    void TriggerModeUpdate(bool bAsync = true);
  //@}

  /*! @name Menu hook methods */
  //@{
    /*!
     * @brief Check whether a audio dsp addon has any audio DSP specific menu entries.
     * @param cat The category to know
     * @param iAddonId The ID of the addon to get the menu entries for. Get the menu for the active channel if iAddonId < 0.
     * @return True if the dsp addon has any menu hooks, false otherwise.
     * @note The main usage for this method is to have bigger modifiable addon setting dialogs which make the usage of
     * standard addon settings dialog as option to it
     * see kodi_adsp_types.h for available types
     */
    bool HaveMenuHooks(AE_DSP_MENUHOOK_CAT cat, int iAddonId = -1);

    /*!
     * @brief Get the menu hooks for a dsp addon.
     * @param iDSPAddonID The dsp addon to get the hooks for.
     * @param cat The requested menu category
     * @param hooks The container to add the hooks to.
     * @return True if the hooks were added successfully (if any), false otherwise.
     * @note The main usage for this method is to have bigger modifiable addon setting dialogs, the basic addon settings dialog
     * can't be opened with it (is only in the menu list from ProcessMenuHooks)
     * see kodi_adsp_types.h for available types
     */
    bool GetMenuHooks(int iDSPAddonID, AE_DSP_MENUHOOK_CAT cat, AE_DSP_MENUHOOKS &hooks);
  //@}

  /*! @name General helper functions */
  //@{
    /*!
     * @brief Translate audio dsp channel flag to KODI channel flag
     */
    static enum AEChannel GetKODIChannel(AE_DSP_CHANNEL channel);

    /*!
     * @brief Translate KODI channel flag to audio dsp channel flag
     */
    static AE_DSP_CHANNEL GetDSPChannel(enum AEChannel channel);

    /*!
     * @brief Get name label id to given stream type id
     */
    static int GetStreamTypeName(unsigned int streamType);
  //@}

  protected:
    /*!
     * @brief Check whether a dsp addon is registered.
     * @param addon The dsp addon to check.
     * @return True if this addon is registered, false otherwise.
     */
    bool IsKnownAudioDSPAddon(const ADDON::AddonPtr& addon) const;

    /*!
     * @brief Get the instance of the dsp addon, if it's ready.
     * @param iAddonId The id of the dsp addon to get.
     * @param addon The addon data pointer.
     * @return True if the addon is ready, false otherwise.
     */
    bool GetReadyAudioDSPAddon(int iAddonId, AE_DSP_ADDON &addon) const;

    /*!
     * @brief Get the dsp related Id for selected addon
     * @param addon The addon class pointer.
     * @return the id of the asked addon, -1 if not available
     */
    int GetAudioDSPAddonId(const ADDON::AddonPtr& addon) const;


    static const int        m_StreamTypeNameTable[];                    /*!< Table for stream type strings related to type id */
    bool                    m_isActive;                                 /*!< set to true if all available dsp addons are loaded */
    AE_DSP_ADDONMAP         m_addonMap;                                 /*!< a map of all known audio dsp addons */
    CActiveAEDSPDatabase    m_databaseDSP;                              /*!< the database for all audio DSP related data */
    CCriticalSection        m_critSection;                              /*!< Critical lock for control functions */
    CCriticalSection        m_critUpdateSection;                        /*!< Critical lock for update thread related functions */
    unsigned int            m_usedProcessesCnt;                         /*!< the amount of used addon processes */
    CActiveAEDSPProcessPtr  m_usedProcesses[AE_DSP_STREAM_MAX_STREAMS]; /*!< Pointer to active process performing classes */
    unsigned int            m_activeProcessId;                          /*!< The currently active audio stream id of a playing file source */
    bool                    m_isValidAudioDSPSettings;                  /*!< if settings load was successfull it becomes true */
    AE_DSP_MODELIST         m_modes[AE_DSP_MODE_TYPE_MAX];              /*!< list of currently used dsp processing calls */
    std::map<std::string, int> m_addonNameIds; /*!< map add-on names to IDs */
  };
  //@}
}
