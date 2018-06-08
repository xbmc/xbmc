/*
 *      Copyright (C) 2005-2018 Team Kodi
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

#pragma once

#include "AddonClass.h"
#include "Exception.h"
#include "commons/Buffer.h"

namespace DRM
{
  class CCryptoSession;
}

namespace XBMCAddon
{

  typedef std::vector<char> charVec;

  namespace xbmcdrm
  {

    XBMCCOMMONS_STANDARD_EXCEPTION(DRMException);

    //
    /// \defgroup python_drm CryptoSession
    /// \ingroup python_xbmcdrm
    /// @{
    /// @brief <b>Kodi DRM CryptoSession class.</b>
    ///--------------------------------------------------------------------------
    /// \python_class{ xbmcdrm.CryptoSession(UUID) }
    ///
    /// @param UUID             String  16 byte UUID of the DRM system to use
    /// @param cipherAlgorithm  String algorithm used for en- / decryption
    /// @param macAlgorithm     String algorithm used for sign / verify
    ///
    /// @throws RuntimeException if the session can not be established
    ///
    //
    class CryptoSession : public AddonClass
    {
      DRM::CCryptoSession* m_cryptoSession;
    public:

      CryptoSession(String UUID, String cipherAlgorithm, String macAlgorithm);
      ~CryptoSession() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcdrm
      /// @brief \python_func{ GetKeyRequest(init, mimeType, offlineKey, optionalParameters) }
      ///-----------------------------------------------------------------------
      /// Generate a key request which is supposed to be send to the key server.
      /// The servers response is passed to provideKeyResponse to activate the keys.
      ///
      /// @param      [byte] init Initialization bytes / depends on key system
      /// @param      String mimeType Type of media which is xchanged, e.g. application/xml, video/mp4
      /// @param      bool offlineKey Persistant (offline) or temporary (streaming) key
      /// @param      [map] optionalParameters optional parameters / depends on key system
      ///
      /// @return     opaque key request data (challenge) which is send to key server
      ///
      GetKeyRequest(...);
#else
      XbmcCommons::Buffer GetKeyRequest(const XbmcCommons::Buffer &init, const String &mimeType, bool offlineKey, const std::map<String, String> &optionalParameters);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcdrm
      /// @brief \python_func{ GetPropertyString(name) }
      ///-----------------------------------------------------------------------
      /// Request a system specific property value of the DRM system
      ///
      /// @param      String Name name of the property to query
      ///
      /// @return     Value of the requested property
      ///
      GetPropertyString(...);
#else
      String GetPropertyString(const String &name);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcdrm
      /// @brief \python_func{ ProvideKeyResponse(response) }
      ///-----------------------------------------------------------------------
      /// Provide key data returned from key server. See getKeyRequest(...)
      ///
      /// @param      [byte] response Key data returned from key server
      ///
      /// @return     String If offline keays are requested, a keySetId which can be used later
      ///                    with restoreKeys, empty for online / streaming) keys.
      ///
      ProvideKeyResponse(...);
#else
      String ProvideKeyResponse(const XbmcCommons::Buffer &response);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcdrm
      /// @brief \python_func{ RemoveKeys() }
      ///-----------------------------------------------------------------------
      /// removes all keys currently loaded in a session.
      ///
      /// @param      None
      ///
      /// @return     None
      ///
      RemoveKeys(...);
#else
      void RemoveKeys();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcdrm
      /// @brief \python_func{ RestoreKeys(keySetId) }
      ///-----------------------------------------------------------------------
      /// restores keys stored during previous provideKeyResponse call.
      ///
      /// @param      String keySetId
      ///
      /// @return     None
      ///
      RestoreKeys(...);
#else
      void RestoreKeys(String keySetId);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcdrm
      /// @brief \python_func{ SetPropertyString(name, value) }
      ///-----------------------------------------------------------------------
      /// Sets a system specific property value in the DRM system
      ///
      /// @param      String name   Name of the property to query
      /// @param      String value  Value of the property to query
      ///
      /// @return     Value of the requested property
      ///
      SetPropertyString(...);
#else
      void SetPropertyString(const String &name, const String &value);
#endif

/*******************Crypto section *****************/

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcdrm
      /// @brief \python_func{ Decrypt(cipherKeyId, input, iv) }
      ///-----------------------------------------------------------------------
      /// Sets a system specific property value in the DRM system
      ///
      /// @param      [byte] cipherKeyId
      /// @param      [byte] input
      /// @param      [byte] iv
      ///
      /// @return     Decrypted input data
      ///
      Decrypt(...);
#else
      XbmcCommons::Buffer Decrypt(const XbmcCommons::Buffer &cipherKeyId, const XbmcCommons::Buffer &input, const XbmcCommons::Buffer &iv);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcdrm
      /// @brief \python_func{ Encrypt(cipherKeyId, input, iv) }
      ///-----------------------------------------------------------------------
      /// Sets a system specific property value in the DRM system
      ///
      /// @param      [byte] cipherKeyId
      /// @param      [byte] input
      /// @param      [byte] iv
      ///
      /// @return     Encrypted input data
      ///
      Encrypt(...);
#else
      XbmcCommons::Buffer Encrypt(const XbmcCommons::Buffer &cipherKeyId, const XbmcCommons::Buffer &input, const XbmcCommons::Buffer &iv);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcdrm
      /// @brief \python_func{ Sign(macKeyId, message) }
      ///-----------------------------------------------------------------------
      /// Sets a system specific property value in the DRM system
      ///
      /// @param      [byte] macKeyId
      /// @param      [byte] message
      ///
      /// @return     [byte] Signature
      ///
      Sign(...);
#else
      XbmcCommons::Buffer Sign(const XbmcCommons::Buffer &macKeyId, const XbmcCommons::Buffer &message);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcdrm
      /// @brief \python_func{ Verify(macKeyId, message, signature) }
      ///-----------------------------------------------------------------------
      /// Sets a system specific property value in the DRM system
      ///
      /// @param      [byte] macKeyId
      /// @param      [byte] message
      /// @param      [byte] signature
      ///
      /// @return     true if message verification succeded
      ///
      Verify(...);
#else
      bool Verify(const XbmcCommons::Buffer &macKeyId, const XbmcCommons::Buffer &message, const XbmcCommons::Buffer &signature);
#endif

    };
    //@}
  }
}
