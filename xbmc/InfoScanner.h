/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <set>
#include <string>
#include <vector>

class CGUIDialogProgressBarHandle;

class CInfoScanner
{
public:
  /*!
    \brief Return values from the information lookup functions.
   */
  enum INFO_RET
  {
    INFO_CANCELLED,
    INFO_ERROR,
    INFO_NOT_NEEDED,
    INFO_HAVE_ALREADY,
    INFO_NOT_FOUND,
    INFO_ADDED
  };

  /*
   \brief Type of information returned from tag readers.
  */

  enum INFO_TYPE
  {
    NO_NFO       = 0, //!< No info found
    FULL_NFO     = 1, //!< Full info specified
    URL_NFO      = 2, //!< A URL to grab info from was found
    OVERRIDE_NFO = 3, //!< Override info was found
    COMBINED_NFO = 4, //!< A URL to grab info from + override info was found
    ERROR_NFO    = 5, //!< Error processing info
    TITLE_NFO    = 6  //!< At least Title was read (and optionally the Year)
  };

  //! \brief Empty destructor.
  virtual ~CInfoScanner() = default;

  virtual bool DoScan(const std::string& strDirectory) = 0;

  /*! \brief Check if the folder is excluded from scanning process
   \param strDirectory Directory to scan
   \return true if there is a .nomedia file
   */
  bool HasNoMedia(const std::string& strDirectory) const;

  //! \brief Set whether or not to show a progress dialog.
  void ShowDialog(bool show) { m_showDialog = show; }

  //! \brief Returns whether or not a scan is in progress.
  bool IsScanning() const { return m_bRunning; }

protected:
  //! \brief Protected constructor to only allow subclass instances.
  CInfoScanner() = default;

  std::set<std::string> m_pathsToScan; //!< Set of paths to scan
  bool m_showDialog = false; //!< Whether or not to show progress bar dialog
  CGUIDialogProgressBarHandle* m_handle = nullptr; //!< Progress bar handle
  bool m_bRunning = false; //!< Whether or not scanner is running
  bool m_bCanInterrupt = false; //!< Whether or not scanner is currently interruptible
  bool m_bClean = false; //!< Whether or not to perform cleaning during scanning
};
