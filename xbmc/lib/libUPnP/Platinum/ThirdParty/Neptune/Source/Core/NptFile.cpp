/*****************************************************************
|
|   Neptune - Files
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptFile.h"
#include "NptUtils.h"
#include "NptConstants.h"
#include "NptStreams.h"
#include "NptDataBuffer.h"

/*----------------------------------------------------------------------
|   NPT_File::Load
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Load(const char* filename, NPT_String& data, NPT_FileInterface::OpenMode mode)
{
    NPT_DataBuffer buffer;

    // reset ouput params
    data = "";

    // create and open the file
    NPT_File file(filename);
    NPT_Result result = file.Open(mode);
    if (NPT_FAILED(result)) return result;

    // load the file
    result = file.Load(buffer);

    if (NPT_SUCCEEDED(result) && buffer.GetDataSize() > 0) {
        data.Assign((const char*)buffer.GetData(), buffer.GetDataSize());
        data.SetLength(buffer.GetDataSize());
    }

    // close the file
    file.Close();

    return result;
}

/*----------------------------------------------------------------------
|   NPT_File::Load
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Load(const char* filename, NPT_DataBuffer& buffer, NPT_FileInterface::OpenMode mode)
{
    // create and open the file
    NPT_File file(filename);
    NPT_Result result = file.Open(mode);
    if (NPT_FAILED(result)) return result;
    
    // load the file
    result = file.Load(buffer);

    // close the file
    file.Close();

    return result;
}

/*----------------------------------------------------------------------
|   NPT_File::Save
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Save(const char* filename, NPT_DataBuffer& buffer)
{
    // create and open the file
    NPT_File file(filename);
    NPT_Result result = file.Open(NPT_FILE_OPEN_MODE_WRITE | NPT_FILE_OPEN_MODE_CREATE | NPT_FILE_OPEN_MODE_TRUNCATE);
    if (NPT_FAILED(result)) return result;

    // load the file
    result = file.Save(buffer);

    // close the file
    file.Close();

    return result;
}

/*----------------------------------------------------------------------
|   NPT_File::Load
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Load(NPT_DataBuffer& buffer)
{
    NPT_InputStreamReference input;

    // get the input stream for the file
    NPT_CHECK(GetInputStream(input));

    // read the stream
    return input->Load(buffer);
}

/*----------------------------------------------------------------------
|   NPT_File::Save
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Save(NPT_DataBuffer& buffer)
{
    NPT_OutputStreamReference output;

    // get the output stream for the file
    NPT_CHECK(GetOutputStream(output));

    // write to the stream
    return output->WriteFully(buffer.GetData(), buffer.GetDataSize());
}
