#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/Variant.h"

#define KEYRING_DEFAULT ""
#define KEYRING_SYSTEM  "xbmc-system"

class IKeyringManager
{
public:
  virtual ~IKeyringManager() { }

  /*!
   \brief Find a stored secret

   \param keyring to look for secret within
   \param key of secret to look for
   \param secret will be filled with secret if found
   \return true if found, false if not
   */
  virtual bool FindSecret(const std::string &keyring, const std::string &key, CVariant &secret) = 0;

  /*!
   \brief Erase a stored secret

   \param keyring to look for secret within
   \param key of secret to look for
   \return true if secret was erased i.e. not existant in keyring, false if failed to do so
   */
  virtual bool EraseSecret(const std::string &keyring, const std::string &key) = 0;

  /*!
   \brief Store a secret

   \param keyring to store secret within
   \param key of secret to store
   \param secret to be stored
   \return true if stored, false if not
   */
  virtual bool StoreSecret(const std::string &keyring, const std::string &key, const CVariant &secret) = 0;
};
