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

class CGUIDialogProgressBarHandle;

class CInfoScanner
{
public:
  /*!
    \brief Return values from the information lookup functions.
   */
  enum class InfoRet
  {
    CANCELLED,
    INFO_ERROR, // ERROR clashes with windows macro
    NOT_NEEDED,
    HAVE_ALREADY,
    NOT_FOUND,
    ADDED
  };

  /*
   \brief Type of information returned from tag readers.
  */

  enum class InfoType
  {
    NONE = 0, //!< No info found
    FULL = 1, //!< Full info specified
    URL = 2, //!< A URL to grab info from was found
    OVERRIDE = 3, //!< Override info was found
    COMBINED = 4, //!< A URL to grab info from + override info was found
    ERROR_NFO = 5, //!< Error processing info
    TITLE = 6, //!< At least Title was read (and optionally the Year)
  };

  //! \brief Empty destructor.
  virtual ~CInfoScanner() = default;

  virtual bool DoScan(const std::string& strDirectory) = 0;

  /*! \brief Check if the folder is excluded from scanning process
   \param strDirectory Directory to scan
   \return true if there is a .nomedia file
   */
  static bool HasNoMedia(const std::string& strDirectory);

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
