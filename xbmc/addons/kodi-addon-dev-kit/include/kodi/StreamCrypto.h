#pragma once
/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

typedef struct CRYPTO_INFO
{
  enum CRYPTO_KEY_SYSTEM :uint16_t
  {
    CRYPTO_KEY_SYSTEM_NONE = 0,
    CRYPTO_KEY_SYSTEM_WIDEVINE,
    CRYPTO_KEY_SYSTEM_PLAYREADY,
    CRYPTO_KEY_SYSTEM_COUNT
  } m_CryptoKeySystem;                 /*!< @brief keysystem for encrypted media, KEY_SYSTEM_NONE for unencrypted media */
  char *m_CryptoSessionId;             /*!< @brief The crypto session key id */
  uint16_t m_CryptoSessionIdSize;      /*!< @brief The size of the crypto session key id */
} CRYPTO_INFO;
