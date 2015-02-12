#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <map>
#include <string>
#include <stdint.h>
#include "threads/CriticalSection.h"

class CURL;

/*!
 \ingroup filesystem
 \brief Password Manager class for saving authentication details
 
 Handles access to previously saved passwords for paths, translating normal URLs
 into authenticated URLs if the user has details about the username and password
 for a path previously saved. Should be accessed via CPasswordManager::GetInstance()
 */
class CPasswordManager
{
public:
 /*!
   \brief The only way through which the global instance of the CPasswordManager should be accessed.
   \return the global instance.
   */
  static CPasswordManager &GetInstance();

  /*!
   \brief Authenticate a URL by looking the URL up in the temporary and permanent caches
   First looks up based on host and share name.  If that fails, it will try a match purely
   on the host name (eg different shares on the same host with the same credentials)
   \param url a CURL to authenticate
   \return true if we have details in the cache, false otherwise.
   \sa CURL
   */
  bool AuthenticateURL(CURL &url);

  /*!
   \brief Prompt for a username and password for the particular URL.
   
   This routine pops up a dialog, requesting the user enter a username and password
   to access the given URL.  The user may optionally save these details.  If saved
   we write the details into the users profile.  If not saved, the details are temporarily
   stored so that further access no longer requires prompting for authentication.
   
   \param url the URL to authenticate.
   \return true if the user entered details, false if the user cancelled the dialog.
   \sa CURL, SaveAuthenticatedURL
   */
  bool PromptToAuthenticateURL(CURL &url);

  /*!
   \brief Save an authenticated URL.

   This routine stores an authenticated URL in the temporary cache, and optionally
   saves these details into the users profile.

   \param url the URL to authenticate.
   \param saveToProfile whether to save in the users profile, defaults to true.
   \sa CURL, PromptToAuthenticateURL
   */
  void SaveAuthenticatedURL(const CURL &url, bool saveToProfile = true);

  /*!
   \brief Clear any previously cached passwords
   */
  void Clear();

private:
  // private construction, and no assignements; use the provided singleton methods
  CPasswordManager();
  CPasswordManager(const CPasswordManager&);
  CPasswordManager const & operator=(CPasswordManager const&);
  ~CPasswordManager() {};

  void Load();
  void Save() const;
  std::string GetLookupPath(const CURL &url) const;
  std::string GetServerLookup(const std::string &path) const;

  std::map<std::string, std::string>  m_temporaryCache;
  std::map<std::string, std::string>  m_permanentCache;
  bool m_loaded;

  CCriticalSection m_critSection;
};
