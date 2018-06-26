/*
 *      Copyright (C) 2016 Team Kodi
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
  bool m_bCanInterrupt = false; //!< Whether or not scanner is currently interruptable
  bool m_bClean = false; //!< Whether or not to perform cleaning during scanning
};
