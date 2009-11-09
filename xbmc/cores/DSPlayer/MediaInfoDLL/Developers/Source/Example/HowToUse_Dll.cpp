// HowToUse - Example for MediaInfoLib (commandline version)
// Copyright (C) 2004-2008 Jerome Martinez, Zen@MediaArea.net
//
// This library is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Example for MediaInfoLib
// Command line version
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef MEDIAINFO_LIBRARY
    #include "MediaInfo/MediaInfo.h" //Staticly-loaded library (.lib or .a or .so)
    #define MediaInfoNameSpace MediaInfoLib;
#else //MEDIAINFO_LIBRARY
    #include "MediaInfoDLL/MediaInfoDLL.h" //Dynamicly-loaded library (.dll or .so)
    #define MediaInfoNameSpace MediaInfoDLL;
#endif //MEDIAINFO_LIBRARY
#include <iostream>
#include <iomanip>
using namespace MediaInfoNameSpace;

#ifdef __MINGW32__
    #ifdef _UNICODE
        #define _itot _itow
    #else //_UNICODE
        #define _itot itoa
    #endif //_UNICODE
#endif //__MINGW32

int main (int argc, Char *argv[])
{
    //Information about MediaInfo
    MediaInfo MI;
    String To_Display=MI.Option(_T("Info_Version"), _T("0.7.13;MediaInfoDLL_Example_MSVC;0.7.13")).c_str();

    To_Display += _T("\r\n\r\nInfo_Parameters\r\n");
    To_Display += MI.Option(_T("Info_Parameters")).c_str();

    To_Display += _T("\r\n\r\nInfo_Codecs\r\n");
    To_Display += MI.Option(_T("Info_Codecs")).c_str();

    //An example of how to use the library
    To_Display += _T("\r\n\r\nOpen\r\n");
    MI.Open(_T("Example.ogg"));

    To_Display += _T("\r\n\r\nInform with Complete=false\r\n");
    MI.Option(_T("Complete"));
    To_Display += MI.Inform().c_str();

    To_Display += _T("\r\n\r\nInform with Complete=true\r\n");
    MI.Option(_T("Complete"), _T("1"));
    To_Display += MI.Inform().c_str();

    To_Display += _T("\r\n\r\nCustom Inform\r\n");
    MI.Option(_T("Inform"), _T("General;Example : FileSize=%FileSize%"));
    To_Display += MI.Inform().c_str();

    To_Display += _T("\r\n\r\nGet with Stream=General and Parameter=\"FileSize\"\r\n");
    To_Display += MI.Get(Stream_General, 0, _T("FileSize"), Info_Text, Info_Name).c_str();

    To_Display += _T("\r\n\r\nGetI with Stream=General and Parameter=46\r\n");
    To_Display += MI.Get(Stream_General, 0, 46, Info_Text).c_str();

    To_Display += _T("\r\n\r\nCount_Get with StreamKind=Stream_Audio\r\n");
    #ifdef __MINGW32__
        Char* C1=new Char[33];
        _itot (MI.Count_Get(Stream_Audio), C1, 10);
        To_Display +=C1;
        delete[] C1;
    #else
        toStringStream SS;
        SS << std::setbase(10) << MI.Count_Get(Stream_Audio);
        To_Display += SS.str();
    #endif

    To_Display += _T("\r\n\r\nGet with Stream=General and Parameter=\"AudioCount\"\r\n");
    To_Display += MI.Get(Stream_General, 0, _T("AudioCount"), Info_Text, Info_Name).c_str();

    To_Display += _T("\r\n\r\nGet with Stream=Audio and Parameter=\"StreamCount\"\r\n");
    To_Display += MI.Get(Stream_Audio, 0, _T("StreamCount"), Info_Text, Info_Name).c_str();

    To_Display += _T("\r\n\r\nClose\r\n");
    MI.Close();

    #ifdef _UNICODE
        std::wcout << To_Display;
    #else
        std::cout  << To_Display;
    #endif

    return 1;
}

//---------------------------------------------------------------------------
/*
//Note: you can replace ZenLib::File by your own file management class
int ExampleWithBuffers (int argc, Char *argv[])
{
    //Initilaizing MediaInfo
    MediaInfo MI;

    //From: preparing an example file for reading
    ZenLib::File From; From.Open(FileName, ZenLib::File::Access_Read); //You can use something else than a file

    //From: preparing a memory buffer for reading
    ZenLib::int8u* From_Buffer=new ZenLib::int8u[7*188]; //Note: you can do your own buffer
    size_t From_Buffer_Size; //The size of the read file buffer

    //Preparing to fill MediaInfo with a buffer
    MI.Open_Buffer_Init(From.Size_Get(), 0);

    //The parsing loop
    do
    {
        //Reading data somewhere, do what you want for this.
        From_Buffer_Size=From.Read(From_Buffer, 7*188);

        //Sending the buffer to MediaInfo
        if (MI.Open_Buffer_Continue(From_Buffer, From_Buffer_Size)==0)
            break;

        //Testing if MediaInfo request to go elsewhere
        if (MI.Open_Buffer_Continue_GoTo_Get()!=(ZenLib::int64u)-1)
        {
            From.GoTo(MI.Open_Buffer_Continue_GoTo_Get());              //Position the file
            MI.Open_Buffer_Init(From.Size_Get(), From.Position_Get());  //Informaing MediaInfo we have seek
        }
    }
    while (From_Buffer_Size>0);

    //Finalizing
    MI.Open_Buffer_Finalize(); //This is the end of the stream, MediaInfo must finnish some work

    //Get() example
    MediaInfoNameSpace::String Text=MI.Get(Stream_General, 0, _T("Format"));
}
*/