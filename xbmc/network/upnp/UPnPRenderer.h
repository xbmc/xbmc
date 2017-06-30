/*
 *      Copyright (C) 2012-2013 Team XBMC
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
#pragma once
#include <Platinum/Source/Devices/MediaRenderer/PltMediaRenderer.h>

#include "interfaces/IAnnouncer.h"

class CVariant;

namespace UPNP
{

class CRendererReferenceHolder
{
public:
    PLT_DeviceHostReference m_Device;
};

class CUPnPRenderer : public PLT_MediaRenderer
                    , public ANNOUNCEMENT::IAnnouncer
{
public:
    CUPnPRenderer(const char*  friendly_name,
                  bool         show_ip = false,
                  const char*  uuid = NULL,
                  unsigned int port = 0);

    ~CUPnPRenderer() override;

    void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) override;
    void UpdateState();

    // Http server handler
    NPT_Result ProcessHttpGetRequest(NPT_HttpRequest&              request,
                                             const NPT_HttpRequestContext& context,
                                             NPT_HttpResponse&             response) override;

    // AVTransport methods
    NPT_Result OnNext(PLT_ActionReference& action) override;
    NPT_Result OnPause(PLT_ActionReference& action) override;
    NPT_Result OnPlay(PLT_ActionReference& action) override;
    NPT_Result OnPrevious(PLT_ActionReference& action) override;
    NPT_Result OnStop(PLT_ActionReference& action) override;
    NPT_Result OnSeek(PLT_ActionReference& action) override;
    NPT_Result OnSetAVTransportURI(PLT_ActionReference& action) override;
    NPT_Result OnSetNextAVTransportURI(PLT_ActionReference& action) override;

    // RenderingControl methods
    NPT_Result OnSetVolume(PLT_ActionReference& action) override;
    NPT_Result OnSetMute(PLT_ActionReference& action) override;

private:
    NPT_Result SetupServices() override;
    NPT_Result SetupIcons() override;
    NPT_Result GetMetadata(NPT_String& meta);
    NPT_Result PlayMedia(const NPT_String& uri,
                         const NPT_String& meta,
                         PLT_Action* action = NULL);
    NPT_Mutex m_state;
};

} /* namespace UPNP */

