/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "Exception.h"
#include "commons/Buffer.h"

#include <map>
#include <vector>

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
    /// \defgroup python_xbmcdrm Library - xbmcdrm
    ///@{
    /// @brief **Kodi's %DRM class.**
    ///
    /// Offers classes and functions that allow a developer to work with
    /// DRM-protected contents like Widevine.
    ///
    /// This type of functionality is closely related to the type of %DRM
    /// used and the service to be implemented.
    ///
    /// Using the \ref xbmcdrm_CryptoSession "CryptoSession" constructor allow you
    /// to have access to a %DRM session.
    /// With a %DRM session you can read and write the %DRM properties
    /// \ref xbmcdrm_GetPropertyString "GetPropertyString",
    /// \ref xbmcdrm_SetPropertyString "SetPropertyString"
    /// and establish session keys with
    /// \ref xbmcdrm_GetKeyRequest "GetKeyRequest" and
    /// \ref xbmcdrm_ProvideKeyResponse "ProvideKeyResponse",
    /// or resume previous session keys with
    /// \ref xbmcdrm_RestoreKeys "RestoreKeys".
    ///
    /// When the session keys are established you can use these methods
    /// to perform various operations:
    /// \ref xbmcdrm_Encrypt "Encrypt" /
    /// \ref xbmcdrm_Decrypt "Decrypt" for data encryption / decryption,
    /// \ref xbmcdrm_Sign "Sign" /
    /// \ref xbmcdrm_Verify "Verify" for make or verify data-signature.
    /// Useful for example to implement encrypted communication between
    /// a client and the server.
    ///
    /// An example where such functionality is useful is the Message
    /// Security Layer (MSL) transmission protocol used in some VOD applications.
    /// This protocol (or rather framework) is used to increase the level of security 
    /// in the exchange of messages (such as licences, manifests or other data),
    /// which defines a security extension / layer on top of the HTTP protocol.
    ///
    ///--------------------------------------------------------------------------
    /// Constructor for %DRM crypto session
    ///
    /// \anchor xbmcdrm_CryptoSession
    /// \python_class{ xbmcdrm.CryptoSession(UUID, cipherAlgorithm, macAlgorithm) }
    ///
    /// @param UUID             string - 16 byte UUID of the %DRM system to use
    /// @param cipherAlgorithm  string - Algorithm used for encryption / decryption ciphers
    /// @param macAlgorithm     string - Algorithm used for sign / verify
    ///
    /// @throws RuntimeException If the session can not be established
    ///
    ///
    ///
    ///------------------------------------------------------------------------
    /// @python_v18 New class added.
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// uuid_widevine = 'edef8ba9-79d6-4ace-a3c8-27dcd51d21ed'
    /// crypto_session = xbmcdrm.CryptoSession(uuid_widevine, 'AES/CBC/NoPadding', 'HmacSHA256')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
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
      /// \anchor xbmcdrm_GetKeyRequest
      /// Generate a key request, used for request/response exchange between the app
      /// and a license server to obtain or release keys used to decrypt encrypted content.
      /// After the app has received the key request response from the license server,
      /// it should deliver to the response to the %DRM instance using
      /// the method \ref xbmcdrm_ProvideKeyResponse "ProvideKeyResponse", to activate the keys.
      ///
      /// @param      init            byte - Initialization bytes container-specific data,
      ///                             its meaning is interpreted based on the mime type provided
      ///                             in the mimeType parameter. It could contain, for example,
      ///                             the content ID, key ID or other data required in generating
      ///                             the key request.
      /// @param      mimeType        string - Type of media which is exchanged
      ///                             (e.g. "application/xml", "video/mp4")
      /// @param      offlineKey      bool - Specifies the type of the request.
      ///                             The request may be to acquire keys for Streaming or Offline content
      /// @param      optionalParameters [opt] map - Will be included in the key request message
      ///                                to allow a client application to provide additional
      ///                                message parameters to the server
      ///
      /// @return     byte - The opaque key request data (challenge) which is send to key server
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New function added.
      /// @python_v19 With python 3 the init param must be a bytearray instead of byte.
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
      /// \anchor xbmcdrm_GetPropertyString
      /// Request a system specific property value of the %DRM system.
      ///
      /// @param      Name    string - Name of the property to query
      ///
      /// @return     Value of the requested property
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New function added.
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
      /// \anchor xbmcdrm_ProvideKeyResponse
      /// When a key response is received from the license server,
      /// must be sent to the %DRM instance by using provideKeyResponse.
      /// See also \ref xbmcdrm_GetKeyRequest "GetKeyRequest".
      ///
      /// @param      response   byte - Key data returned from the license server
      ///
      /// @return     A keySetId if the response is for an offline key requests which
      ///             can be used later with restoreKeys,
      ///             else return empty for streaming key requests.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New function added.
      /// @python_v19 With python 3 the response argument must be a bytearray instead of byte.
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
      /// Removes all keys currently loaded in a session.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New function added.
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
      /// \anchor xbmcdrm_RestoreKeys
      /// Restores session keys stored during previous
      /// \ref xbmcdrm_ProvideKeyResponse "ProvideKeyResponse" call.
      ///
      /// @param     keySetId   string - Identifies the saved key set to restore.
      ///                                This value must never be null.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New function added.
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
      /// \anchor xbmcdrm_SetPropertyString
      /// Set a system specific property value in the %DRM system.
      ///
      /// @param     name   string - Name of the property. This value must never be null.
      /// @param     value  string - Value of the property to set. This value must never be null.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New function added.
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
      /// \anchor xbmcdrm_Decrypt
      /// Decrypt an encrypted data by using session keys.
      ///
      /// @param     cipherKeyId byte - Encryption key id (provided from a service handshake)
      /// @param     input       byte - Cipher text to decrypt
      /// @param     iv          byte - Initialization vector of cipher text
      ///
      /// @return    Decrypted input data
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New function added.
      /// @python_v19 With python 3 all arguments need to be of type bytearray instead of byte.
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
      /// \anchor xbmcdrm_Encrypt
      /// Encrypt data by using session keys.
      ///
      /// @param     cipherKeyId byte - Encryption key id (provided from a service handshake)
      /// @param     input       byte - Encrypted text
      /// @param     iv          byte - Initialization vector of encrypted text
      ///
      /// @return    byte - Encrypted input data
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New function added.
      /// @python_v19 With python 3 all arguments need to be of type bytearray instead of byte.
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
      /// \anchor xbmcdrm_Sign
      /// Generate a %DRM encrypted signature for a text message.
      ///
      /// @param     macKeyId  byte - HMAC key id (provided from a service handshake)
      /// @param     message   byte - Message text on which to base the signature
      ///
      /// @return    byte - Signature
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New function added.
      /// @python_v19 With python 3 all arguments need to be of type bytearray instead of byte.
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
      /// \anchor xbmcdrm_Verify
      /// Verify the validity of a %DRM signature of a text message.
      ///
      /// @param     macKeyId  byte - HMAC key id (provided from a service handshake)
      /// @param     message   byte - Message text on which the signature is based
      /// @param     signature byte - The signature to verify
      ///
      /// @return    true when the signature is valid
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New function added.
      /// @python_v19 With python 3 for all arguments is needed to pass bytearray instead of byte.
      ///
      Verify(...);
#else
      bool Verify(const XbmcCommons::Buffer &macKeyId, const XbmcCommons::Buffer &message, const XbmcCommons::Buffer &signature);
#endif

    };
    ///@}
  }
}
