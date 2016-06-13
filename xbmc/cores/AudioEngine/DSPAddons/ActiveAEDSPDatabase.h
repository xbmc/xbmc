#pragma once
/*
 *      Copyright (C) 2012-2014 Team KODI
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

#include <string>
#include <vector>

#include "ActiveAEDSPMode.h"

class CAudioSettings;

namespace ActiveAE
{
  class CActiveAEDSPAddon;
  class CActiveAEDSPProcess;

  /** The audio DSP database */

  class CActiveAEDSPDatabase : public CDatabase
  {
  public:
    /*!
     * @brief Create a new instance of the audio DSP database.
     */
    CActiveAEDSPDatabase(void) {};
    virtual ~CActiveAEDSPDatabase(void) {};

    /*!
     * @brief Open the database.
     * @return True if it was opened successfully, false otherwise.
     */
    virtual bool Open();

    /*!
     * @brief Get the minimal database version that is required to operate correctly.
     * @return The minimal database version.
     */
    virtual int GetMinVersion() const { return 1; };

    /*!
     * @brief Get the default sqlite database filename.
     * @return The default filename.
     */
    const char *GetBaseDBName() const { return "ADSP"; };

    /*! @name mode methods */
    //@{
    /*!
     * @brief Used to check for present mode types
     * @param modeType The mode type identfier
     * @return True if modes present of given type
     */
    bool ContainsModes(int modeType);

    /*!
     * @brief Remove all modes from the database.
     * @return True if all modes were removed, false otherwise.
     */
    bool DeleteModes(void);

    /*!
     * @brief Remove all modes from the database of a type.
     * @param modeType The mode type identfier of functions to delete.
     * @return True if the modes were deleted, false otherwise.
     */
    bool DeleteModes(int modeType);

    /*!
     * @brief Remove all modes from a add-on from the database.
     * @param addonId The add-on identifier to delete the modes for.
     * @return True if the modes were deleted, false otherwise.
     */
    bool DeleteAddonModes(int addonId);

    /*!
     * @brief Remove a mode entry from the database
     * @param mode The mode to remove.
     * @return True if the mode was removed, false otherwise.
     */
    bool DeleteMode(const CActiveAEDSPMode &mode);

    /*!
     * @brief Add or update mode entries in the database
     * @param modes The modes to persist.
     * @param modeType If true, don't write immediately
     * @return True when persisted or queued, false otherwise.
     */
    bool PersistModes(std::vector<CActiveAEDSPModePtr> &modes, int modeType);

    /*!
     * @brief Update user selectable mode settings inside database
     * @param modeType the mode type to get
     * @param active true if the mode is enabled
     * @param addonId  the addon id of this mode
     * @param addonModeNumber the from addon set mode number
     * @param listNumber the list number on processing chain
     * @return True if the modes were updated, false otherwise.
     */
    bool UpdateMode(int modeType, bool active, int addonId, int addonModeNumber, int listNumber);

    /*!
     * @brief Add or if present update mode inside database
     * @param addon The add-on to check the modes for.
     * @return True if the modes were updated or added, false otherwise.
     */
    bool AddUpdateMode(CActiveAEDSPMode &mode);

    /*!
     * @brief Get id of mode inside database
     * @param mode The mode to check for inside the database
     * @return The id or -1 if not found
     */
    int GetModeId(const CActiveAEDSPMode &mode);

    /*!
     * @brief Get the list of modes from type on database
     * @param results The mode group to store the results in.
     * @param modeType the mode type to get
     * @return The amount of modes that were added.
     */
    int GetModes(AE_DSP_MODELIST &results, int modeType);
    //@}

    /*! @name Add-on methods */
    //@{
    /*!
     * @brief Remove all add-on information from the database.
     * @return True if all add-on's were removed successfully.
     */
    bool DeleteAddons();

    /*!
     * @brief Remove a add-on from the database
     * @param strAddonUid The unique ID of the add-on.
     * @return True if the add-on was removed successfully, false otherwise.
     */
    bool Delete(const std::string &strAddonUid);

    /*!
     * @brief Get the database ID of a add-on.
     * @param strAddonUid The unique ID of the add-on.
     * @return The database ID of the add-on or -1 if it wasn't found.
     */
    int GetAudioDSPAddonId(const std::string &strAddonUid);

    /*!
     * @brief Add a add-on to the database if it's not already in there.
     * @param addon The pointer to the addon class
     * @return The database ID of the client.
     */
    int Persist(const ADDON::AddonPtr &addon);
    //@}

    /*! @name Settings methods */
    //@{

    /*!
     * @brief Remove all active dsp settings from the database.
     * @return True if all dsp data were removed successfully, false if not.
     */
    bool DeleteActiveDSPSettings();

    /*!
     * @brief Remove active dsp settings from the database for file.
     * @return True if dsp data were removed successfully, false if not.
     */
    bool DeleteActiveDSPSettings(const CFileItem &item);

    /*!
     * @brief GetVideoSettings() obtains any saved video settings for the current file.
     * @return Returns true if the settings exist, false otherwise.
     */
    bool GetActiveDSPSettings(const CFileItem &item, CAudioSettings &settings);

    /*!
     * @brief Sets the settings for a particular used file
     */
    void SetActiveDSPSettings(const CFileItem &item, const CAudioSettings &settings);

    /*!
     * @brief EraseActiveDSPSettings() Erases the dsp Settings table and reconstructs it
     */
    void EraseActiveDSPSettings();
    //@}

  private:
    /*!
     * @brief Create the audio DSP database tables.
     */
    virtual void CreateTables();
    virtual void CreateAnalytics();

    /*!
     * @brief Update an old version of the database.
     * @param version The version to update the database from.
     */
    virtual void UpdateTables(int version);
    virtual int GetSchemaVersion() const { return 0; }

    void SplitPath(const std::string& strFileNameAndPath, std::string& strPath, std::string& strFileName);
  };
}
