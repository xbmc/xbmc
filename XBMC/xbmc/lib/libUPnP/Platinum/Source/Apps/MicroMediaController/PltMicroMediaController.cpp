/*****************************************************************
|
|   Platinum - Miccro Media Controller
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltMicroMediaController.h"
#include "PltLeaks.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::PLT_MicroMediaController
+---------------------------------------------------------------------*/
PLT_MicroMediaController::PLT_MicroMediaController(PLT_CtrlPointReference& ctrlPoint)
{
    // create the stack that will be the directory where the
    // user is currently browsing. 
    // push the root directory onto the directory stack.
    m_CurBrowseDirectoryStack.Push("0");

    m_MediaBrowser = new PLT_SyncMediaBrowser(ctrlPoint, true);
    m_MediaController = new PLT_MediaController(ctrlPoint, this);
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::PLT_MicroMediaController
+---------------------------------------------------------------------*/
PLT_MicroMediaController::~PLT_MicroMediaController()
{
    delete m_MediaBrowser;
    delete m_MediaController;
}

/*
*  Remove trailing white space from a string
*/
void strchomp(char* str)
{
    if (!str) return;
    char* e = str+NPT_StringLength(str)-1;

    while (e >= str && *e) {
        if ((*e != ' ')  &&
            (*e != '\t') &&
            (*e != '\r') &&
            (*e != '\n'))
        {
            *(e+1) = '\0';
            break;
        }
        --e;
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::ChooseIDFromTable
+---------------------------------------------------------------------*/
/* 
 * Presents a list to the user, allows the user to choose one item.
 *
 * Parameters:
 *		PLT_StringMap: A map that contains the set of items from
 *                        which the user should choose.  The key should be a unique ID,
 *						 and the value should be a string describing the item. 
 *       returns a NPT_String with the unique ID. 
 */
const char*
PLT_MicroMediaController::ChooseIDFromTable(PLT_StringMap& table)
{
    printf("Select one of the following:\n");

    NPT_List<PLT_StringMapEntry*> entries = table.GetEntries();
    if (entries.GetItemCount() == 0) {
        printf("None available\n"); 
    } else {
        // display the list of entries
        NPT_List<PLT_StringMapEntry*>::Iterator entry = entries.GetFirstItem();
        int count = 0;
        while (entry) {
            printf("%d)\t%s (%s)\n", ++count, (const char*)(*entry)->GetValue(), (const char*)(*entry)->GetKey());
            ++entry;
        }

        int index, watchdog = 3;
        char buffer[1024];

        // wait for input
        while (watchdog > 0) {
            fgets(buffer, 1024, stdin);
            strchomp(buffer);

            if (1 != sscanf(buffer, "%d", &index)) {
                printf("Please enter a number\n");
            } else if (index < 0 || index > count)	{
                printf("Please choose one of the above, or 0 for none\n");
                watchdog--;
                index = 0;
            } else {	
                watchdog = 0;
            }
        }

        // find the entry back
        if (index != 0) {
            entry = entries.GetFirstItem();
            while (entry && --index) {
                ++entry;
            }
            if (entry) {
                return (*entry)->GetKey();
            }
        }
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::PopDirectoryStackToRoot
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::PopDirectoryStackToRoot(void)
{
    NPT_String val;
    while (NPT_SUCCEEDED(m_CurBrowseDirectoryStack.Peek(val)) && val.Compare("0")) {
        m_CurBrowseDirectoryStack.Pop(val);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::OnMRAddedRemoved
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::OnMRAddedRemoved(PLT_DeviceDataReference& device, int added)
{
    NPT_String uuid = device->GetUUID();

    if (added) {
        NPT_AutoLock lock(m_MediaRenderers);

        // test if it's a media renderer
        PLT_Service* service;
        if (NPT_SUCCEEDED(device->FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service))) {
            m_MediaRenderers.Put(uuid, device);
        }
    } else { /* removed */
        {
            NPT_AutoLock lock(m_MediaRenderers);
            m_MediaRenderers.Erase(uuid);
        }

        {
            NPT_AutoLock lock(m_CurMediaRendererLock);

            // if it's the currently selected one, we have to get rid of it
            if (!m_CurMediaRenderer.IsNull() && m_CurMediaRenderer == device) {
                m_CurMediaRenderer = NULL;
            }
        }
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::ChooseIDGetCurMediaServerFromTable
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::GetCurMediaServer(PLT_DeviceDataReference& server)
{
    NPT_AutoLock lock(m_CurMediaServerLock);

    if (m_CurMediaServer.IsNull()) {
        printf("No server selected, select one with setms\n");
    } else {
        server = m_CurMediaServer;
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::GetCurMediaRenderer
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::GetCurMediaRenderer(PLT_DeviceDataReference& renderer)
{
    NPT_AutoLock lock(m_CurMediaRendererLock);

    if (m_CurMediaRenderer.IsNull()) {
        printf("No renderer selected, select one with setmr\n");
    } else {
        renderer = m_CurMediaRenderer;
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::DoBrowse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MicroMediaController::DoBrowse()
{
    NPT_Result res = NPT_FAILURE;
    PLT_DeviceDataReference device;
    GetCurMediaServer(device);
    if (!device.IsNull()) {
        NPT_String object_id;
        m_CurBrowseDirectoryStack.Peek(object_id);

        // send off the browse packet.  Note that this will
        // not block.  There is a call to WaitForResponse in order
        // to block until the response comes back.
        res = m_MediaBrowser->Browse(device, (const char*)object_id, m_MostRecentBrowseResults);		
    }

    return res;
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_getms
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::HandleCmd_getms()
{
    PLT_DeviceDataReference device;
    GetCurMediaServer(device);
    if (!device.IsNull()) {
        printf("Current media server: %s\n", (const char*)device->GetFriendlyName());
    } else {
        // this output is taken care of by the GetCurMediaServer call
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_getmr
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::HandleCmd_getmr()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        printf("Current media renderer: %s\n", (const char*)device->GetFriendlyName());
    } else {
        // this output is taken care of by the GetCurMediaRenderer call
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::ChooseDevice
+---------------------------------------------------------------------*/
PLT_DeviceDataReference
PLT_MicroMediaController::ChooseDevice(const NPT_Lock<PLT_DeviceMap>& deviceList)
{
    PLT_StringMap            namesTable;
    PLT_DeviceDataReference* result = NULL;
    NPT_String               chosenUUID;
    NPT_AutoLock             lock(m_MediaServers);

    // create a map with the device UDN -> device Name 
    const NPT_List<PLT_DeviceMapEntry*>& entries = deviceList.GetEntries();
    NPT_List<PLT_DeviceMapEntry*>::Iterator entry = entries.GetFirstItem();
    while (entry) {
        PLT_DeviceDataReference device = (*entry)->GetValue();
        NPT_String              name   = device->GetFriendlyName();
        namesTable.Put((*entry)->GetKey(), name);

        ++entry;
    }

    // ask user to choose
    chosenUUID = ChooseIDFromTable(namesTable);
    if (chosenUUID.GetLength()) {
        deviceList.Get(chosenUUID, result);
    }

    return result?*result:PLT_DeviceDataReference(); // return empty reference if not device was selected
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_setms
+---------------------------------------------------------------------*/
void 
PLT_MicroMediaController::HandleCmd_setms()
{
    NPT_AutoLock lock(m_CurMediaServerLock);

    PopDirectoryStackToRoot();
    m_CurMediaServer = ChooseDevice(m_MediaBrowser->GetMediaServers());
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_setmr
+---------------------------------------------------------------------*/
void 
PLT_MicroMediaController::HandleCmd_setmr()
{
    NPT_AutoLock lock(m_CurMediaRendererLock);

    m_CurMediaRenderer = ChooseDevice(m_MediaRenderers);
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_ls
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::HandleCmd_ls()
{
    DoBrowse();

    if (!m_MostRecentBrowseResults.IsNull()) {
        printf("There were %d results\n", m_MostRecentBrowseResults->GetItemCount());

        NPT_List<PLT_MediaObject*>::Iterator item = m_MostRecentBrowseResults->GetFirstItem();
        while (item) {
            if ((*item)->IsContainer()) {
                printf("Container: %s (%s)\n", (*item)->m_Title.GetChars(), (*item)->m_ObjectID.GetChars());
            } else {
                printf("Item: %s (%s)\n", (*item)->m_Title.GetChars(), (*item)->m_ObjectID.GetChars());
            }
            ++item;
        }

        m_MostRecentBrowseResults = NULL;
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_cd
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::HandleCmd_cd()
{
    NPT_String      newObjID;
    PLT_StringMap   containers;

    DoBrowse();

    if (!m_MostRecentBrowseResults.IsNull()) {
        NPT_List<PLT_MediaObject*>::Iterator item = m_MostRecentBrowseResults->GetFirstItem();
        while (item) {
            if ((*item)->IsContainer()) {
                containers.Put((*item)->m_ObjectID, (*item)->m_Title);
            }
            ++item;
        }

        newObjID = ChooseIDFromTable(containers);
        if (newObjID.GetLength()) {
            m_CurBrowseDirectoryStack.Push(newObjID);
        }

        m_MostRecentBrowseResults = NULL;
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_cdup
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::HandleCmd_cdup()
{
    // we don't want to pop the root off now....
    NPT_String val;
    m_CurBrowseDirectoryStack.Peek(val);
    if (val.Compare("0")) {
        m_CurBrowseDirectoryStack.Pop(val);
    } else {
        printf("Already at root\n");
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_pwd
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::HandleCmd_pwd()
{
    NPT_Stack<NPT_String> tempStack;
    NPT_String val;

    while (NPT_SUCCEEDED(m_CurBrowseDirectoryStack.Peek(val))) {
        m_CurBrowseDirectoryStack.Pop(val);
        tempStack.Push(val);
    }

    while (NPT_SUCCEEDED(tempStack.Peek(val))) {
        tempStack.Pop(val);
        printf("%s/", (const char*)val);
        m_CurBrowseDirectoryStack.Push(val);
    }
    printf("\n");
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_open
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::HandleCmd_open()
{
    NPT_String              objID;
    PLT_StringMap           tracks;
    PLT_DeviceDataReference device;

    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        // get the protocol info to try to see in advance if a track would play on the device

        // issue a browse
        DoBrowse();

        if (!m_MostRecentBrowseResults.IsNull()) {
            // create a map item id -> item title
            NPT_List<PLT_MediaObject*>::Iterator item = m_MostRecentBrowseResults->GetFirstItem();
            while (item) {
                if (!(*item)->IsContainer()) {
                    tracks.Put((*item)->m_ObjectID, (*item)->m_Title);
                }
                ++item;
            }

            // let the user choose which one
            objID = ChooseIDFromTable(tracks);
            if (objID.GetLength()) {
                // look back for the PLT_MediaItem in the results
                PLT_MediaObject* track = NULL;
                if (NPT_SUCCEEDED(NPT_ContainerFind(*m_MostRecentBrowseResults, PLT_MediaItemIDFinder(objID), track))) {
                    if (track->m_Resources.GetItemCount() > 0) {
                        // invoke the setUri
                        m_MediaController->SetAVTransportURI(device, 0, track->m_Resources[0].m_Uri, track->m_Didl, NULL);
                        m_MediaController->Play(device, 0, "1", NULL);                        
                    } else {
                        printf("Couldn't find the proper resource\n");
                    }

                } else {
                    printf("Couldn't find the track\n");
                }
            }

            m_MostRecentBrowseResults = NULL;
        }
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_play
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::HandleCmd_play()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        m_MediaController->Play(device, 0, "1", NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_stop
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::HandleCmd_stop()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        m_MediaController->Stop(device, 0, NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_help
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::HandleCmd_help()
{
    printf("\n\nNone of the commands take arguments.  The commands with a * \n");
    printf("signify ones that will prompt the user for more information once\n");
    printf("the command is called\n\n");
    printf("The available commands are:\n\n");
    printf(" quit    -   shutdown the Control Point\n");
    printf(" exit    -   same as quit\n");

    printf(" setms   - * select a media server to become the active media server\n");
    printf(" getms   -   print the friendly name of the active media server\n");
    printf(" ls      -   list the contents of the current directory on the active \n");
    printf("             media server\n");
    printf(" cd      - * traverse down one level in the content tree on the active\n");
    printf("             media server\n");
    printf(" cd ..   -   traverse up one level in the content tree on the active\n");
    printf("             media server\n");
    printf(" pwd     -   print the path from the root to your current position in the \n");
    printf("             content tree on the active media server\n");
    printf(" setmr   - * select a media renderer to become the active media renderer\n");
    printf(" getmr   -   print the friendly name of the active media renderer\n");
    printf(" open    -   set the uri on the active media renderer\n");
    printf(" play    -   play the active uri on the active media renderer\n");
    printf(" stop    -   stop the active uri on the active media renderer\n");

    printf(" help    -   print this help message\n\n");
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::ProcessCommandLoop
+---------------------------------------------------------------------*/
void 
PLT_MicroMediaController::ProcessCommandLoop()
{
    char command[2048];
    bool abort = false;

    command[0] = '\0';
    while (!abort) {
        printf("command> ");
        fflush(stdout);
        fgets(command, 2048, stdin);
        strchomp(command);

        if (0 == strcmp(command, "quit") || 0 == strcmp(command, "exit")) {
            abort = true;
        } else if (0 == strcmp(command, "setms")) {
            HandleCmd_setms();
        } else if (0 == strcmp(command, "getms")) {
            HandleCmd_getms();
        } else if (0 == strcmp(command, "ls")) {
            HandleCmd_ls();
        } else if (0 == strcmp(command, "cd")) {
            HandleCmd_cd();
        } else if (0 == strcmp(command, "cd ..")) {
            HandleCmd_cdup();
        } else if (0 == strcmp(command, "pwd")) {
            HandleCmd_pwd();
        } else if (0 == strcmp(command, "setmr")) {
            HandleCmd_setmr();
        } else if (0 == strcmp(command, "getmr")) {
            HandleCmd_getmr();
        } else if (0 == strcmp(command, "open")) {
            HandleCmd_open();
        } else if (0 == strcmp(command, "play")) {
            HandleCmd_play();
        } else if (0 == strcmp(command, "stop")) {
            HandleCmd_stop();
        } else if (0 == strcmp(command, "help")) {
            HandleCmd_help();
        } else if (0 == strcmp(command, "")) {
            // just prompt again
        } else {
            printf("Unrecognized command: %s\n", command);
            HandleCmd_help();
        }
    }
}

