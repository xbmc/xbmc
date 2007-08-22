/*****************************************************************
|
|   Platinum - Metadata Handler
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltMetadataHandler.h"

NPT_SET_LOCAL_LOGGER("platinum.core.metadata.handler")

/*----------------------------------------------------------------------
|   PLT_MetadataHandler::Load
+---------------------------------------------------------------------*/
NPT_Result
PLT_MetadataHandler::LoadFile(const char* filename)
{
    NPT_File file(filename);
    NPT_Result res = file.Open(NPT_FILE_OPEN_MODE_READ);
    if (res != NPT_SUCCESS) {
        NPT_LOG_SEVERE_2("error %d opening : %s\n", res, filename);
    } else {
        NPT_InputStreamReference stream;
        res = file.GetInputStream(stream);
        if (NPT_FAILED(res)) {
            NPT_LOG_SEVERE_2("error %d getting input stream: %s\n", res, filename);
        }
        res = Load(*stream);
        if (NPT_FAILED(res)) {
            NPT_LOG_SEVERE_2("error %d reading header: %s\n", res, filename);
        }
    }

    return res;
}

/*----------------------------------------------------------------------
|   PLT_MetadataHandler::Save
+---------------------------------------------------------------------*/
NPT_Result
PLT_MetadataHandler::SaveFile(const char* filename)
{
    NPT_File file(filename);
    NPT_Result res = file.Open(NPT_FILE_OPEN_MODE_WRITE);
    if (res != NPT_SUCCESS) {
        NPT_LOG_SEVERE_2("error %d opening : %s\n", res, filename);
    } else {
        NPT_OutputStreamReference stream;
        res = file.GetOutputStream(stream);
        if (NPT_FAILED(res)) {
            NPT_LOG_SEVERE_2("error %d getting output stream: %s\n", res, filename);
        }
        res = Save(*stream);
        if (NPT_FAILED(res)) {
            NPT_LOG_SEVERE_2("error %d writing header: %s\n", res, filename);
        }
    }

    return res;
}
