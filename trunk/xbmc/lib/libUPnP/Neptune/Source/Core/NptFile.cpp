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
        int dot = result.ReverseFind('.');
        if (dot >= 0) {
            result.SetLength(dot);
        }
    }

    return result;
}

/*----------------------------------------------------------------------
|   NPT_FilePath::DirName
+---------------------------------------------------------------------*/
NPT_String 
NPT_FilePath::DirName(const char* path)
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
NPT_FilePath::Create(const char* directory, const char* basename)
{
    if (!directory || NPT_StringLength(directory) == 0) return basename;
    if (!basename || NPT_StringLength(basename) == 0) return directory;

    NPT_String result = directory;
    if (!result.EndsWith(Separator) && basename[0] != Separator[0]) {
        result += Separator;
    }
    result += basename;

    return result;
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
|   NPT_File::CreateDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::CreateDir(const char* path, bool recursively)
{
    NPT_String fullpath = path;

    // replace delimiters with the proper one for the platform
    fullpath.Replace((NPT_FilePath::Separator[0] == '/')?'\\':'/',
                     NPT_FilePath::Separator);
    // remove excessive delimiters
    fullpath.TrimRight(NPT_FilePath::Separator);

    if (recursively) {
        NPT_String parent_path;

        // look for a delimiter from the beginning
        int delimiter = fullpath.Find(NPT_FilePath::Separator, 1);
        while (delimiter > 0) {
            // copy the path up to the delimiter
            parent_path = fullpath.SubString(0, delimiter);

            // create the directory non recursively
            NPT_CHECK(NPT_File::CreateDir(parent_path, false));

            // look for the next delimiter
            delimiter = fullpath.Find(NPT_FilePath::Separator, delimiter + 1);
        }
    }

    // create directory
    NPT_Result res = NPT_File::CreateDir(fullpath);

    // return error only if file didn't exist
    if (NPT_FAILED(res) && res != NPT_ERROR_FILE_ALREADY_EXISTS) {
        NPT_CHECK_FATAL(res);
    }
    return NPT_SUCCESS;
}


/*----------------------------------------------------------------------
|   NPT_DirectoryRemove
+---------------------------------------------------------------------*/
NPT_Result 
NPT_File::RemoveDir(const char* path, bool recursively)
{
    NPT_String root_path = path;

    // replace delimiters with the proper one for the platform
    root_path.Replace((NPT_FilePath::Separator[0] == '/')?'\\':'/', 
                      NPT_FilePath::Separator);
    // remove excessive delimiters
    root_path.TrimRight(NPT_FilePath::Separator);

    if (recursively) {
        // enumerate all entries
        NPT_File dir(root_path);
        NPT_List<NPT_String> entries;
        NPT_CHECK(dir.ListDir(entries));
        for (NPT_List<NPT_String>::Iterator it = entries.GetFirstItem();
             it;
             ++it) {
            NPT_File::Remove(NPT_FilePath::Create(root_path, *it), true);
        }
    }

    return NPT_File::RemoveDir(root_path);
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
    NPT_CHECK_WARNING(GetOutputStream(output));

    // write to the stream
    return output->WriteFully(buffer.GetData(), buffer.GetDataSize());
}

/*----------------------------------------------------------------------
|   NPT_File::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetInfo(NPT_FileInfo& info)
{
    if (m_IsSpecial) {
        info.m_Type           = NPT_FileInfo::FILE_TYPE_SPECIAL;
        info.m_Size           = 0;
        info.m_Attributes     = 0;
        info.m_AttributesMask = 0;
        return NPT_SUCCESS;
    }
    return GetInfo(m_Path.GetChars(), &info);
}

/*----------------------------------------------------------------------
|   NPT_File::GetSize
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetSize(NPT_LargeSize& size)
{
    // default value
    size = 0;
    
    // get the size from the info (call GetInfo() in case it has not
    // yet been called)
    NPT_FileInfo info;
    GetInfo(info);
    size = info.m_Size;
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File::GetSize
+---------------------------------------------------------------------*/
NPT_Result        
NPT_File::GetSize(const char* path, NPT_LargeSize& size)
{
	NPT_File file(path);
	return file.GetSize(size);
}

/*----------------------------------------------------------------------
|   NPT_File::Remove
+---------------------------------------------------------------------*/
NPT_Result 
NPT_File::Remove(const char* path, bool recursively /* = false */)
{
    NPT_FileInfo info;

    // make sure the path exists
    NPT_CHECK(GetInfo(path, &info));

    if (info.m_Type == NPT_FileInfo::FILE_TYPE_DIRECTORY) {
        return RemoveDir(path, recursively);
    } else {
        return RemoveFile(path);
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
|   NPT_File::ListDir
+---------------------------------------------------------------------*/
NPT_Result        
NPT_File::ListDir(NPT_List<NPT_String>& entries)
{
    entries.Clear();
    return ListDir(m_Path.GetChars(), entries);
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
    NPT_CHECK(ListDir(entries));
    
    count = entries.GetItemCount();
    return NPT_SUCCESS;
}

