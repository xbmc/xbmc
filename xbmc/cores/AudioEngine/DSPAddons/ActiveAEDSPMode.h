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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ActiveAEDSPAddon.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

namespace ActiveAE
{
  class CActiveAEDSPMode;
  class CActiveAEDSPDatabase;

  typedef std::shared_ptr<CActiveAEDSPMode>                 CActiveAEDSPModePtr;
  typedef std::pair <CActiveAEDSPModePtr, AE_DSP_ADDON>     AE_DSP_MODEPAIR;
  typedef std::vector<AE_DSP_MODEPAIR >                     AE_DSP_MODELIST;

  #define AE_DSP_MASTER_MODE_ID_INTERNAL_TYPES              0xFF00
  #define AE_DSP_MASTER_MODE_ID_INTERNAL_STEREO_UPMIX       (AE_DSP_MASTER_MODE_ID_INTERNAL_TYPES)  /*!< Used to for internal stereo upmix over ffmpeg */
  #define AE_DSP_MASTER_MODE_ID_PASSOVER                    0  /*!< Used to ignore master processing */
  #define AE_DSP_MASTER_MODE_ID_INVALID                     -1

  /*!
   * DSP Mode information class
   */
  //@{
  class CActiveAEDSPMode : public Observable
  {
  public:
    /*! @brief Create a new mode */
    CActiveAEDSPMode();

    /*!
     * @brief Create a KODI internal processing mode infomation class
     * @param modeId internal processing mode identifier
     * @param baseType the used base of this internal mode
     * @note this creation is only used to get a internal bypass mode (no addon call process mode)
     */
    CActiveAEDSPMode(int modeId, const AE_DSP_BASETYPE baseType);

    /*!
     * @brief Create the class about from addon given values
     * @param mode the from addon set identification structure
     * @param iAddonId the addon identification of the given data
     */
    CActiveAEDSPMode(const AE_DSP_MODES::AE_DSP_MODE &mode, int iAddonId);

    /*!
     * @brief Create a new class about given class
     * @param mode the parent mode to copy data from
     */
    CActiveAEDSPMode(const CActiveAEDSPMode &mode);

    bool operator ==(const CActiveAEDSPMode &right) const;
    bool operator !=(const CActiveAEDSPMode &right) const;
    CActiveAEDSPMode &operator=(const CActiveAEDSPMode &mode);

    /*! @name General mode related functions
     *  @note changes are not written inside database and must be performed with AddUpdate call
     */
    //@{
    /*!
     * @brief Check this mode as known or new one
     * @return true if this mode is new and not stored inside database
     */
    bool IsNew(void) const;

    /*!
     * @brief Check this mode about data changes
     * @return true if anything becomes changed on the mode data
     */
    bool IsChanged(void) const;

    /*!
     * @brief Check this mode about source
     * @return true if internal KODI mode
     */
    bool IsInternal(void) const;

    /*!
     * @brief Check this mode is enabled for usage
     * @return true if enabled
     */
    bool IsEnabled(void) const;

    /*!
     * @brief Enable or disable the usage of this mode
     * @param bIsEnabled true to enable
     * @return true if set was successful
     */
    bool SetEnabled(bool bIsEnabled);

    /*!
     * @brief Get the mode process chain position inside his mode type
     * @return the mode process position or -1 not set
     */
    int ModePosition(void) const;

    /*!
     * @brief Set the mode process chain position inside his mode type
     * @param iModePosition the process chain position
     * @return true if the position becomes set and a database update becomes required
     */
    bool SetModePosition(int iModePosition);

    /*!
     * @brief Ask about stream type to given flags
     * @param streamType the type to ask
     * @param flags the stream types flags to check in accordance with AE_DSP_ASTREAM_PRESENT
     * @return true if the mode is set as enabled under the flags
     */
    static bool SupportStreamType(AE_DSP_STREAMTYPE streamType, unsigned int flags);

    /*!
     * @brief Ask this mode about stream type
     * @param streamType the type to ask
     * @return true if the mode is set as enabled of this mode
     */
    bool SupportStreamType(AE_DSP_STREAMTYPE streamType) const;
    //@}

    /*! @name Mode user interface related data functions
     */
    //@{
    /*!
     * @brief Get the mode name string identification code
     * @return the identifier code on addon strings or -1 if unset
     */
    int ModeName(void) const;

    /*!
     * @brief Get the mode name string identification code used on setup entries
     * @return the identifier code on addon strings or -1 if unset
     */
    int ModeSetupName(void) const;

    /*!
     * @brief Get the mode help string identification code used as help text on dsp manager helper dialog
     * @return the identifier code on addon strings or -1 if unset
     */
    int ModeHelp(void) const;

    /*!
     * @brief Get the mode description string identification code used as small help text on dsp manager dialog
     * @return the identifier code on addon strings or -1 if unset
     */
    int ModeDescription(void) const;

    /*!
     * @brief Get the path to a from addon set mode identification image
     * @return the path to the image or empty if not present
     */
    const std::string &IconOwnModePath(void) const;

    /*!
     * @brief Get the path to a from addon set mode identification image to overirde the from KODI used one, e.g. Dolby Digital with Dolby Digital EX
     * @return the path to the image or empty if not present
     */
    const std::string &IconOverrideModePath(void) const;
    //@}

    /*! @name Master mode type related functions
     */
    //@{
    /*!
     * @brief Get the used base type of this mode
     * @return the base type
     */
    AE_DSP_BASETYPE BaseType(void) const;

    /*!
     * @brief Set the used base type of this mode
     * @return baseType the base type to set
     * @return true if the position becomes set and a database update becomes required
     */
    bool SetBaseType(AE_DSP_BASETYPE baseType);
    //@}

    /*! @name Audio DSP database related functions
     */
    //@{
    /*!
     * @brief Get the identifier of this mode used on database
     * @return the mode identifier or -1 if unknown and not safed to database
     */
    int ModeID(void) const;

    /*!
     * @brief Add or update this mode to the audio DSP database
     * @param force if it is false it write only to the database on uknown id or if a change was inside the mode
     * @return the database identifier of this mode, or -1 if a error was occurred
     */
    int AddUpdate(bool force = false);

    /*!
     * @brief Delete this mode from the audio dsp database
     * @return true if deletion was successful
     */
    bool Delete(void);

    /*!
     * @brief Ask database about this mode that it is alread known
     * @return true if present inside database
     */
    bool IsKnown(void) const;
    //@}

    /*! @name Dynamic processing related functions
     */
    //@{
    /*!
     * @brief Get the cpu usage of this mode
     * @return percent The percent value (0.0 - 100.0)
     * @note only be usable if mode is active in process chain
     */
    float CPUUsage(void) const;

    /*!
     * @brief Set the cpu usage of this mode if active and in process list
     * @param percent The percent value (0.0 - 100.0)
     */
    void SetCPUUsage(float percent);
    //@}

    /*! @name Fixed audio dsp add-on related mode functions
     */
    //@{
    /*!
     * @brief Get the addon identifier
     * @return returns the inside addon database used identifier of this mode based addon
     */
    int AddonID(void) const;

    /*!
     * @brief Get the addon processing mode identifier
     * @return returns the from addon itself set identifier of this mode
     */
    unsigned int AddonModeNumber(void) const;

    /*!
     * @brief The processing mode type identifier of this mode
     * @return returns the mode type, it should be never AE_DSP_MODE_TYPE_UNDEFINED
     */
    AE_DSP_MODE_TYPE ModeType(void) const;

    /*!
     * @brief Get the addon mode name
     * @return returns the from addon set name of this mode, used for log messages
     */
    const std::string &AddonModeName(void) const;

    /*!
     * @brief Have this mode settings dialogs
     * @return returns true if one or more dialogs are available to this mode
     * @note if it is true the addon menu hook database can be checked with the addon mode identifier
     */
    bool HasSettingsDialog(void) const;

    /*!
     * @brief Get the from addon mode supported stream type flags
     * @return returns the flags in accordance with AE_DSP_ASTREAM_PRESENT
     */
    unsigned int StreamTypeFlags(void) const;
    //@}

  private:
    friend class CActiveAEDSPDatabase;

    /*! @name KODI related mode data
     */
    //@{
    AE_DSP_MODE_TYPE  m_iModeType;               /*!< the processing mode type */
    int               m_iModePosition;           /*!< the processing mode position */
    int               m_iModeId;                 /*!< the identifier given to this mode by the DSP database */
    AE_DSP_BASETYPE   m_iBaseType;               /*!< The stream source coding format */
    bool              m_bIsEnabled;              /*!< true if this mode is enabled, false if not */
    std::string       m_strOwnIconPath;          /*!< the path to the icon for this mode */
    std::string       m_strOverrideIconPath;     /*!< the path to the icon for this mode */
    int               m_iModeName;               /*!< the name id for this mode used by KODI */
    int               m_iModeSetupName;          /*!< the name id for this mode inside settings used by KODI */
    int               m_iModeDescription;        /*!< the description id for this mode used by KODI */
    int               m_iModeHelp;               /*!< the help id for this mode used by KODI */
    bool              m_bChanged;                /*!< true if anything in this entry was changed that needs to be persisted */
    bool              m_bIsInternal;             /*!< true if this mode is an internal KODI mode */
    std::string       m_strModeName;             /*!< the log name of this mode on the Addon or inside KODI */
    //@}

    /*! @name Dynamic processing related data
     */
    //@{
    float             m_fCPUUsage;               /*!< if mode is active the used cpu force in percent is set here */
    //@}

    /*! @name Audio dsp add-on related mode data
     */
    //@{
    int               m_iAddonId;                /*!< the identifier of the Addon that serves this mode */
    unsigned int      m_iAddonModeNumber;        /*!< the mode number on the Addon */
    bool              m_bHasSettingsDialog;      /*!< the mode have a own settings dialog */
    unsigned int      m_iStreamTypeFlags;        /*!< The stream content type flags in accordance with AE_DSP_ASTREAM_PRESENT */
    //@}

    CCriticalSection  m_critSection;
  };
  //@}
}
