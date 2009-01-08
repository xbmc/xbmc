/*****************************************************************
|
|   Neptune - Files
|
| Copyright (c) 2002-2008, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include "NptLogging.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune.file")

/*----------------------------------------------------------------------
|   NPT_FilePath::BaseName
+---------------------------------------------------------------------*/
NPT_String 
NPT_FilePath::BaseName(const char* path, bool with_extension /* = true */)
{
    NPT_String result = path;
    int separator = result.ReverseFind(Separator);
    if (separator >= 0) {
        result = path+separator+NPT_StringLength(Separator);
    } 

    if (!with_extension) {
        int separator = result.ReverseFind('.');
        if (separator >= 0) {
            result.SetLength(separator);
        }
    }

    return result;
}

/*----------------------------------------------------------------------
|   NPT_FilePath::DirectoryName
+---------------------------------------------------------------------*/
NPT_String 
NPT_FilePath::DirectoryName(const char* path)
{
    NPT_String result = path;
    int separator = result.ReverseFind(Separator);
    if (separator >= 0) {
        if (separator == 0) {
            result.SetLength(NPT_StringLength(Separator));
        } else {
            result.SetLength(separator);
        }
    } else {
        result.SetLength(0);
    } 

    return result;
}

/*----------------------------------------------------------------------
|   NPT_FilePath::FileExtension
+---------------------------------------------------------------------*/
NPT_String 
NPT_FilePath::FileExtension(const char* path)
{
    NPT_String result = path;
    int separator = result.ReverseFind('.');
    if (separator >= 0) {
        result = path+separator;
    } else {
        result.SetLength(0);
    }

    return result;
}

/*----------------------------------------------------------------------
|   NPT_FilePath::Create
+---------------------------------------------------------------------*/
NPT_String 
NPT_FilePath::Create(const char* directory, const char* base)
{
    if (!directory || NPT_StringLength(directory) == 0) return base;

    NPT_String result = directory;
    result.TrimRight(Separator);
    NPT_String _base = base;
    _base.TrimLeft(Separator);
    
    return result + Separator + _base;
}

/*----------------------------------------------------------------------
|   NPT_File::GetCount
+---------------------------------------------------------------------*/
NPT_Result        
NPT_File::GetCount(const char* path, NPT_Cardinal& count)
{
    NPT_File file(path);
    return file.GetCount(count);
}

/*----------------------------------------------------------------------
|   NPT_File::Load
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Load(const char* path, NPT_DataBuffer& buffer, NPT_FileInterface::OpenMode mode)
{
    // create and open the file
    NPT_File file(path);
    NPT_Result result = file.Open(mode);
    if (NPT_FAILED(result)) return result;
    
    // load the file
    result = file.Load(buffer);

    // close the file
    file.Close();

    return result;
}

/*----------------------------------------------------------------------
|   NPT_File::Load
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Load(const char* path, NPT_String& data, NPT_FileInterface::OpenMode mode)
{
    NPT_DataBuffer buffer;

    // reset ouput params
    data = "";

    // create and open the file
    NPT_File file(path);
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
|   NPT_File::Save
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Save(const char* filename, NPT_String& data)
{
    NPT_DataBuffer buffer(data.GetChars(), data.GetLength());
    return NPT_File::Save(filename, buffer);
}

/*----------------------------------------------------------------------
|   NPT_File::Save
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Save(const char* filename, const NPT_DataBuffer& buffer)
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
    NPT_CHECK_WARNING(GetInputStream(input));

    // read the stream
    return input->Load(buffer);
}

/*----------------------------------------------------------------------
|   NPT_File::Save
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Save(const NPT_DataBuffer& buffer)
{
    NPT_OutputStreamReference output;

    // get the output stream for the file
    NPT_CHECK_FATAL(GetOutputStream(output));

    // write to the stream
    return output->WriteFully(buffer.GetData(), buffer.GetDataSize());
}

/*----------------------------------------------------------------------
|   NPT_File::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetInfo(NPT_FileInfo& info)
{
    NPT_Result result = NPT_SUCCESS;
    
    // get the file info if we don't already have it
    if (m_Info.m_Type == NPT_FileInfo::FILE_TYPE_NONE) {
        result = GetInfo(m_Path.GetChars(), &m_Info);
    }
    
    info = m_Info;
    
    return result;
}

/*----------------------------------------------------------------------
|   NPT_File::GetSize
+---------------------------------------------------------------------*/
/*NPT_Result
NPT_File::GetSize(NPT_LargeSize& size)
{
    // default value
    size = 0;
    
    // get the size from the info (call GetInfo() in case it has not
    // yet been called)
    NPT_FileInfo info;
    NPT_CHECK_FATAL(GetInfo(info));
    size = info.m_Size;
    
    return NPT_SUCCESS;
}*/

/*----------------------------------------------------------------------
|   NPT_File::Delete
+---------------------------------------------------------------------*/
NPT_Result 
NPT_File::Delete(const char* path)
{
    NPT_FileInfo info;

    // make sure the path exists
    NPT_CHECK(GetInfo(path, &info));

    if (info.m_Type == NPT_FileInfo::FILE_TYPE_DIRECTORY) {
        return DeleteDirectory(path);
    } else {
        return DeleteFile(path);
    }
}

/*----------------------------------------------------------------------
|   NPT_File::Rename
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Rename(const char* path)
{
    NPT_Result result = Rename(m_Path.GetChars(), path);
    if (NPT_SUCCEEDED(result)) {
        m_Path = path;
    }
    return result;
}

/*----------------------------------------------------------------------
|   NPT_File::ListDirectory
+---------------------------------------------------------------------*/
NPT_Result        
NPT_File::ListDirectory(NPT_List<NPT_String>& entries)
{
    entries.Clear();
    return ListDirectory(m_Path.GetChars(), entries);
}

/*----------------------------------------------------------------------
|   NPT_File::GetCount
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetCount(NPT_Cardinal& count)
{
    // reset output params
    count = 0;

    NPT_List<NPT_String> entries;
    NPT_CHECK(ListDirectory(entries));
    
    count = entries.GetItemCount();
    return NPT_SUCCESS;
}

