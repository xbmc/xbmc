/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/IAnnouncer.h"
#include "utils/logtypes.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include <Platinum/Source/Devices/MediaConnect/PltMediaConnect.h>

class CFileItem;
class CFileItemList;
class CThumbLoader;
class CVariant;
class PLT_MediaObject;
class PLT_HttpRequestContext;

namespace UPNP
{

class CUPnPServer : public PLT_MediaConnect,
                    public PLT_FileMediaConnectDelegate,
                    public ANNOUNCEMENT::IAnnouncer
{
public:
    CUPnPServer(const char* friendly_name, const char* uuid = NULL, int port = 0);
    ~CUPnPServer() override;
    void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                  const std::string& sender,
                  const std::string& message,
                  const CVariant& data) override;

    // PLT_MediaServer methods
    NPT_Result OnBrowseMetadata(PLT_ActionReference&          action,
                                const char*                   object_id,
                                const char*                   filter,
                                NPT_UInt32                    starting_index,
                                NPT_UInt32                    requested_count,
                                const char*                   sort_criteria,
                                const PLT_HttpRequestContext& context) override;
    NPT_Result OnBrowseDirectChildren(PLT_ActionReference&          action,
                                      const char*                   object_id,
                                      const char*                   filter,
                                      NPT_UInt32                    starting_index,
                                      NPT_UInt32                    requested_count,
                                      const char*                   sort_criteria,
                                      const PLT_HttpRequestContext& context) override;
    NPT_Result OnSearchContainer(PLT_ActionReference&          action,
                                 const char*                   container_id,
                                 const char*                   search_criteria,
                                 const char*                   filter,
                                 NPT_UInt32                    starting_index,
                                 NPT_UInt32                    requested_count,
                                 const char*                   sort_criteria,
                                 const PLT_HttpRequestContext& context) override;

    NPT_Result OnUpdateObject(PLT_ActionReference&             action,
                              const char*                      object_id,
                              NPT_Map<NPT_String,NPT_String>&  current_vals,
                              NPT_Map<NPT_String,NPT_String>&  new_vals,
                              const PLT_HttpRequestContext&    context) override;

    // PLT_FileMediaServer methods
    NPT_Result ServeFile(const NPT_HttpRequest&        request,
                         const NPT_HttpRequestContext& context,
                         NPT_HttpResponse&             response,
                         const NPT_String&             file_path) override;

    // PLT_DeviceHost methods
    NPT_Result ProcessGetSCPD(PLT_Service*                  service,
                              NPT_HttpRequest&              request,
                              const NPT_HttpRequestContext& context,
                              NPT_HttpResponse&             response) override;

    NPT_Result SetupServices() override;
    NPT_Result SetupIcons() override;
    NPT_String BuildSafeResourceUri(const NPT_HttpUrl &rooturi,
                                    const char*        host,
                                    const char*        file_path);

    void AddSafeResourceUri(PLT_MediaObject* object,
                            const NPT_HttpUrl& rooturi,
                            const NPT_List<NPT_IpAddress>& ips,
                            const char* file_path,
                            const NPT_String& info)
    {
        PLT_MediaItemResource res;
        for(NPT_List<NPT_IpAddress>::Iterator ip = ips.GetFirstItem(); ip; ++ip) {
            res.m_ProtocolInfo = PLT_ProtocolInfo(info);
            res.m_Uri          = BuildSafeResourceUri(rooturi, (*ip).ToString(), file_path);
            object->m_Resources.Add(res);
        }
    }

    /* Samsung's devices get subtitles from header in response (for movie file), not from didl.
       It's a way to store subtitle uri generated when building didl, to use later in http response*/
    NPT_Result AddSubtitleUriForSecResponse(const NPT_String& movie_md5,
                                            const NPT_String& subtitle_uri);


  private:
    void OnScanCompleted(int type);
    void UpdateContainer(const std::string& id);
    void PropagateUpdates();

    PLT_MediaObject* Build(const std::shared_ptr<CFileItem>& item,
                           bool with_count,
                           const PLT_HttpRequestContext& context,
                           NPT_Reference<CThumbLoader>& thumbLoader,
                           const char* parent_id = NULL);
    NPT_Result BuildResponse(PLT_ActionReference&          action,
                             CFileItemList&                items,
                             const char*                   filter,
                             NPT_UInt32                    starting_index,
                             NPT_UInt32                    requested_count,
                             const char*                   sort_criteria,
                             const PLT_HttpRequestContext& context,
                             const char*                   parent_id /* = NULL */);

    // class methods
    static void DefaultSortItems(CFileItemList& items);
    static NPT_String GetParentFolder(const NPT_String& file_path)
    {
      int index = file_path.ReverseFind("\\");
      if (index == -1)
        return "";

      return file_path.Left(index);
    }

    static int GetRequiredVideoDbDetails(const NPT_String& filter);

    NPT_Mutex m_CacheMutex;

    NPT_Mutex m_FileMutex;
    NPT_Map<NPT_String, NPT_String> m_FileMap;

    std::map<std::string, std::pair<bool, unsigned long> > m_UpdateIDs;
    bool m_scanning;

    Logger m_logger;

  public:
    // class members
    static NPT_UInt32 m_MaxReturnedItems;
};

} /* namespace UPNP */

