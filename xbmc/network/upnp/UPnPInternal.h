#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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
#include "system.h"
#include "utils/StdString.h"
#include "NptTypes.h"
#include "NptReferences.h"

class CUPnPServer;
class CFileItem;
class CThumbLoader;
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

  enum EClientQuirks
  {
    ECLIENTQUIRKS_NONE = 0x0

    /* Client requires folder's to be marked as storageFolers as verndor type (360)*/
  , ECLIENTQUIRKS_ONLYSTORAGEFOLDER = 0x01

    /* Client can't handle subtypes for videoItems (360) */
  , ECLIENTQUIRKS_BASICVIDEOCLASS = 0x02

    /* Client requires album to be set to [Unknown Series] to show title (WMP) */
  , ECLIENTQUIRKS_UNKNOWNSERIES = 0x04
  };

  EClientQuirks GetClientQuirks(const PLT_HttpRequestContext* context);

  const char* GetMimeTypeFromExtension(const char* extension, const PLT_HttpRequestContext* context = NULL);
  NPT_String  GetMimeType(const CFileItem& item, const PLT_HttpRequestContext* context = NULL);
  NPT_String  GetMimeType(const char* filename, const PLT_HttpRequestContext* context = NULL);
  const NPT_String GetProtocolInfo(const CFileItem& item, const char* protocol, const PLT_HttpRequestContext* context = NULL);


  const CStdString& CorrectAllItemsSortHack(const CStdString &item);

  NPT_Result PopulateTagFromObject(MUSIC_INFO::CMusicInfoTag& tag,
                                   PLT_MediaObject&           object,
                                   PLT_MediaItemResource*     resource = NULL);
  NPT_Result PopulateTagFromObject(CVideoInfoTag&             tag,
                                   PLT_MediaObject&           object,
                                   PLT_MediaItemResource*     resource = NULL);

  NPT_Result PopulateObjectFromTag(MUSIC_INFO::CMusicInfoTag&         tag,
                                          PLT_MediaObject&       object,
                                          NPT_String*            file_path,
                                          PLT_MediaItemResource* resource,
                                          EClientQuirks          quirks);
  NPT_Result PopulateObjectFromTag(CVideoInfoTag&         tag,
                                          PLT_MediaObject&       object,
                                          NPT_String*            file_path,
                                          PLT_MediaItemResource* resource,
                                          EClientQuirks          quirks);

  PLT_MediaObject* BuildObject(CFileItem&              item,
                                      NPT_String&                   file_path,
                                      bool                          with_count,
                                      NPT_Reference<CThumbLoader>&  thumb_loader,
                                      const PLT_HttpRequestContext* context = NULL,
                                      CUPnPServer*                  upnp_server = NULL);

}

