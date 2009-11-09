/**
 * HowToUse - Example for MediaInfoLib (JNA) (commandline version)
 *
 * Copyright (C) 2009-2009 Jerome Martinez, Zen@MediaArea.net
 *
 * This library is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 **/

class HowToUse_Dll
{

public static void main(String... args) throws Exception
{
    String FileName = "Example.ogg";
    if (args.length > 0)
        FileName = args[0];

    String To_Display = "";

    //Info about the library

    To_Display += MediaInfo.Option_Static("Info_Version");

    To_Display += "\r\n\r\nInfo_Parameters\r\n";
    To_Display += MediaInfo.Option_Static("Info_Parameters");

    To_Display += "\r\n\r\nInfo_Capacities\r\n";
    To_Display += MediaInfo.Option_Static("Info_Capacities");

    To_Display += "\r\n\r\nInfo_Codecs\r\n";
    To_Display += MediaInfo.Option_Static("Info_Codecs");

    //An example of how to use the library

    MediaInfo MI = new MediaInfo();

    To_Display += "\r\n\r\nOpen\r\n";
    if (MI.Open(FileName)>0)
        To_Display+="is OK\r\n";
    else
        To_Display+="has a problem\r\n";

    To_Display += "\r\n\r\nInform with Complete=false\r\n";
    MI.Option("Complete", "");
    To_Display += MI.Inform();

    To_Display += "\r\n\r\nInform with Complete=true\r\n";
    MI.Option("Complete", "1");
    To_Display += MI.Inform();

    To_Display += "\r\n\r\nCustom Inform\r\n";
    MI.Option("Inform", "General;Example : FileSize=%FileSize%");
    To_Display += MI.Inform();

    To_Display += "\r\n\r\nGetI with Stream=General and Parameter=2\r\n";
    To_Display += MI.Get(MediaInfo.StreamKind.General, 0, 2, MediaInfo.InfoKind.Text);

    To_Display += "\r\n\r\nCount_Get with StreamKind=Stream_Audio\r\n";
    To_Display += MI.Count_Get(MediaInfo.StreamKind.Audio, -1);

    To_Display += "\r\n\r\nGet with Stream=General and Parameter=\"AudioCount\"\r\n";
    To_Display += MI.Get(MediaInfo.StreamKind.General, 0, "AudioCount", MediaInfo.InfoKind.Text, MediaInfo.InfoKind.Name);

    To_Display += "\r\n\r\nGet with Stream=Audio and Parameter=\"StreamCount\"\r\n";
    To_Display += MI.Get(MediaInfo.StreamKind.Audio, 0, "StreamCount", MediaInfo.InfoKind.Text, MediaInfo.InfoKind.Name);

    To_Display += "\r\n\r\nGet with Stream=General and Parameter=\"FileSize\"\r\n";
    To_Display += MI.Get(MediaInfo.StreamKind.General, 0, "FileSize", MediaInfo.InfoKind.Text, MediaInfo.InfoKind.Name);

    To_Display += "\r\n\r\nClose\r\n";
    MI.Close();

    System.out.println(To_Display);
}

}
