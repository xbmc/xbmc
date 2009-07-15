/*****************************************************************
|
|      Platinum - Frame Streamer
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
|       includes
+---------------------------------------------------------------------*/
#include "Platinum.h"
#include "PltFrameBuffer.h"
#include "PltFrameStream.h"
#include "PltFrameServer.h"

#include <stdlib.h>

NPT_SET_LOCAL_LOGGER("platinum.core.framestreamer")

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
struct Options {
    const char* path;
} Options;

/*----------------------------------------------------------------------
|   FrameWriter::FrameWriter
+---------------------------------------------------------------------*/
class FrameWriter : public NPT_Thread
{
public:
    FrameWriter(PLT_FrameBuffer& frame_buffer,
                const char*      frame_folder,
                NPT_TimeInterval frame_delay = NPT_TimeInterval(1.f)) : 
        m_FrameBuffer(frame_buffer),
        m_Aborted(false),
        m_Folder(frame_folder),
        m_Delay(frame_delay)
        {}

    const char* GetNextEntry(NPT_List<NPT_String>::Iterator& entry) {
        if (!entry) return NULL;

        if (!entry->EndsWith(".jpg", true)) {
            return GetNextEntry(++entry);
        }

        return *entry;
    }

    void Run() {
        NPT_List<NPT_String> entries;
        NPT_File::ListDir(m_Folder, entries);
        NPT_List<NPT_String>::Iterator entry = entries.GetFirstItem();
        
        const char* frame_path = GetNextEntry(entry);
        if (!frame_path) {
            NPT_LOG_SEVERE("No images found!");
            return;
        }

        while (!m_Aborted) {
            // load frame
            NPT_DataBuffer frame;
            NPT_Result res = NPT_File::Load(NPT_FilePath::Create(m_Folder, frame_path), frame);
            if (NPT_FAILED(res)) {
                NPT_LOG_SEVERE_1("Failed to load %s", frame_path);
                return;
            }

            m_FrameBuffer.SetNextFrame(frame.GetData(), frame.GetDataSize());
            if (NPT_FAILED(res)) {
                NPT_LOG_SEVERE_1("Failed to set next frame %s", frame_path);
                return;
            }

            // Wait before loading next frame
            NPT_System::Sleep(m_Delay);

            // look for next entry
            frame_path = GetNextEntry(++entry);

            // loop back if none left
            if (!frame_path) {
                entry = entries.GetFirstItem();
                frame_path = GetNextEntry(entry);
            }
        }

        // one more time to unblock any pending readers
        m_FrameBuffer.SetNextFrame(NULL, 0);
    }

    PLT_FrameBuffer& m_FrameBuffer;
    bool             m_Aborted;
    NPT_String       m_Folder;
    NPT_TimeInterval m_Delay;
};

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit(char** args)
{
    fprintf(stderr, "usage: %s <images path>\n", args[0]);
    fprintf(stderr, "<path> : local path to serve images from\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   ParseCommandLine
+---------------------------------------------------------------------*/
static void
ParseCommandLine(char** args)
{
    char** _args = args++;
    const char* arg;

    /* default values */
    Options.path = NULL;
    
    while ((arg = *args++)) {
        if (Options.path == NULL) {
            Options.path = arg;
        } else {
            fprintf(stderr, "ERROR: too many arguments\n");
            PrintUsageAndExit(_args);
        }
    }

    /* check args */
    if (Options.path == NULL) {
        fprintf(stderr, "ERROR: path missing\n");
        PrintUsageAndExit(_args);
    }
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    NPT_COMPILER_UNUSED(argc);
    
    /* parse command line */
    ParseCommandLine(argv);
    
    PLT_FrameBuffer frame_buffer;
    FrameWriter writer(frame_buffer, Options.path, NPT_TimeInterval(.2f));
    writer.Start();

    PLT_UPnP upnp;

    PLT_DeviceHostReference device(new PLT_FrameServer(frame_buffer, 
                                                       "/Users/sylvain/dev/veodia_frontend/bin-debug",
                                                       "Platinum: FrameServer: ",
                                                       false,
                                                       NULL,
                                                       8099));
    upnp.AddDevice(device);

    if (NPT_FAILED(upnp.Start()))
        return 1;

    char buf[256];
    while (gets(buf))
    {
        if (*buf == 'q')
        {
            break;
        }
    }

    writer.m_Aborted = true;
    upnp.Stop();

    return 0;
}
