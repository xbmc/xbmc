/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

#include <Neptune/Source/Core/NptReferences.h>
#include <Neptune/Source/Core/NptStrings.h>
#include <Neptune/Source/Core/NptTypes.h>

class CUPnPServer;
class CFileItem;
class CThumbLoader;
class PLT_DeviceData;
class PLT_HttpRequestContext;
class PLT_MediaItemResource;
class PLT_MediaObject;
class NPT_String;
namespace MUSIC_INFO {
  class CMusicInfoTag;
}
class CVideoInfoTag;

namespace UPNP
{
  enum UPnPService {
    UPnPServiceNone = 0,
    UPnPClient,
    UPnPContentDirectory,
    UPnPPlayer,
    UPnPRenderer
  };

  class CResourceFinder {
  public:
    CResourceFinder(const char* protocol, const char* content = NULL);
    bool operator()(const PLT_MediaItemResource& resource) const;
  private:
    NPT_String m_Protocol;
    NPT_String m_Content;
  };

  enum EClientQuirks
  {
    ECLIENTQUIRKS_NONE = 0x0

    /* Client requires folder's to be marked as storageFolders as vendor type (360)*/
  , ECLIENTQUIRKS_ONLYSTORAGEFOLDER = 0x01

    /* Client can't handle subtypes for videoItems (360) */
  , ECLIENTQUIRKS_BASICVIDEOCLASS = 0x02

    /* Client requires album to be set to [Unknown Series] to show title (WMP) */
  , ECLIENTQUIRKS_UNKNOWNSERIES = 0x04
  };

  EClientQuirks GetClientQuirks(const PLT_HttpRequestContext* context);

  enum EMediaControllerQuirks
  {
    EMEDIACONTROLLERQUIRKS_NONE   = 0x00

    /* Media Controller expects MIME type video/x-mkv instead of video/x-matroska (Samsung) */
  , EMEDIACONTROLLERQUIRKS_X_MKV  = 0x01
  };

  EMediaControllerQuirks GetMediaControllerQuirks(const PLT_DeviceData *device);

  const char* GetMimeTypeFromExtension(const char* extension, const PLT_HttpRequestContext* context = NULL);
  NPT_String  GetMimeType(const CFileItem& item, const PLT_HttpRequestContext* context = NULL);
  NPT_String  GetMimeType(const char* filename, const PLT_HttpRequestContext* context = NULL);
  const NPT_String GetProtocolInfo(const CFileItem& item, const char* protocol, const PLT_HttpRequestContext* context = NULL);


  const std::string& CorrectAllItemsSortHack(const std::string &item);

  NPT_Result PopulateTagFromObject(MUSIC_INFO::CMusicInfoTag& tag,
                                   PLT_MediaObject&           object,
                                   PLT_MediaItemResource*     resource = NULL,
                                   UPnPService                service = UPnPServiceNone);

  NPT_Result PopulateTagFromObject(CVideoInfoTag&             tag,
                                   PLT_MediaObject&           object,
                                   PLT_MediaItemResource*     resource = NULL,
                                   UPnPService                service = UPnPServiceNone);

  NPT_Result PopulateObjectFromTag(MUSIC_INFO::CMusicInfoTag& tag,
                                   PLT_MediaObject&           object,
                                   NPT_String*                file_path,
                                   PLT_MediaItemResource*     resource,
                                   EClientQuirks              quirks,
                                   UPnPService                service = UPnPServiceNone);

  NPT_Result PopulateObjectFromTag(CVideoInfoTag&             tag,
                                   PLT_MediaObject&           object,
                                   NPT_String*                file_path,
                                   PLT_MediaItemResource*     resource,
                                   EClientQuirks              quirks,
                                   UPnPService                service = UPnPServiceNone);

  PLT_MediaObject* BuildObject(CFileItem&                     item,
                               NPT_String&                    file_path,
                               bool                           with_count,
                               NPT_Reference<CThumbLoader>&   thumb_loader,
                               const PLT_HttpRequestContext*  context = NULL,
                               CUPnPServer*                   upnp_server = NULL,
                               UPnPService                    upnp_service = UPnPServiceNone);

  std::shared_ptr<CFileItem> BuildObject(PLT_MediaObject* entry,
                                         UPnPService upnp_service = UPnPServiceNone);

  bool             GetResource(const PLT_MediaObject* entry, CFileItem& item);
  std::shared_ptr<CFileItem> GetFileItem(const NPT_String& uri, const NPT_String& meta);

  /*!
   * @brief Provided a given object id, encode it into a safe format to provide to UPnP clients
   * @Note base64 is currently used as the safe format
   * @param id the object it to encode
   * @return the encoded object id
  */
  NPT_String EncodeObjectId(const std::string& id);

  /*!
   * @brief Provided a given encoded object id, decode it into a format known by the application
   * @Note base64 is currently used as the expected input format
   * @param id the object it to decode
   * @return the decoded object id
  */
  NPT_String DecodeObjectId(const std::string& id);
}

