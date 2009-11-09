// MediaInfoDLL - All info about media files
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Microsoft Visual C# example
//
// To make this example working, you must put MediaInfo.Dll and Example.ogg
// in the "./Bin/__ConfigurationName__" folder
// and add MediaInfoDll.cs to your project
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

using System;
using MediaInfoLib;

namespace MediaInfoLib_MSCS
{
	public class CLI
	{
        [STAThread]
        static void Main(string[] Args)
        {
            String ToDisplay;
            MediaInfo MI = new MediaInfo();

            ToDisplay = MI.Option("Info_Version", "0.7.0.0;MediaInfoDLL_Example_CS;0.7.0.0");
            if (ToDisplay.Length == 0)
            {
                Console.Out.WriteLine("MediaInfo.Dll: this version of the DLL is not compatible");
                return;
            }

            //Information about MediaInfo
			ToDisplay += "\r\n\r\nInfo_Parameters\r\n";
			ToDisplay += MI.Option("Info_Parameters");

			ToDisplay += "\r\n\r\nInfo_Capacities\r\n";
            ToDisplay += MI.Option("Info_Capacities");

			ToDisplay += "\r\n\r\nInfo_Codecs\r\n";
            ToDisplay += MI.Option("Info_Codecs");

			//An example of how to use the library
			ToDisplay += "\r\n\r\nOpen\r\n";
            String File_Name;
            if (Args.Length == 0)
                File_Name = "Example.ogg";
            else
                File_Name = Args[0];
            MI.Open(File_Name);

			ToDisplay += "\r\n\r\nInform with Complete=false\r\n";
			MI.Option("Complete");
			ToDisplay += MI.Inform();

            ToDisplay += "\r\n\r\nInform with Complete=true\r\n";
			MI.Option("Complete", "1");
			ToDisplay += MI.Inform();

            ToDisplay += "\r\n\r\nCustom Inform\r\n";
            MI.Option("Inform", "General;File size is %FileSize% bytes");
            ToDisplay += MI.Inform();

            ToDisplay += "\r\n\r\nGet with Stream=General and Parameter='FileSize'\r\n";
            ToDisplay += MI.Get(0, 0, "FileSize");

            ToDisplay += "\r\n\r\nGet with Stream=General and Parameter=46\r\n";
            ToDisplay += MI.Get(0, 0, 46);

            ToDisplay += "\r\n\r\nCount_Get with StreamKind=Stream_Audio\r\n";
            ToDisplay += MI.Count_Get(StreamKind.Audio);

            ToDisplay += "\r\n\r\nGet with Stream=General and Parameter='AudioCount'\r\n";
            ToDisplay += MI.Get(StreamKind.General, 0, "AudioCount");

            ToDisplay += "\r\n\r\nGet with Stream=Audio and Parameter='StreamCount'\r\n";
            ToDisplay += MI.Get(StreamKind.Audio, 0, "StreamCount");

            ToDisplay += "\r\n\r\nClose\r\n";
            MI.Close();

			//Displaying the text
			Console.Out.WriteLine(ToDisplay);
		}
	}
}
