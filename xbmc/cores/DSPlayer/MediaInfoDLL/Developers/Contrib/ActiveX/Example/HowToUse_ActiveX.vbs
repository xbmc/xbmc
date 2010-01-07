' In order to run this example, put MediaInfo.dll and MediaInfoActiveX.dll
' into your system directory and Example.ogg into the root directory of drive
' C: (i.e. C:\). Use regsvr32.exe which is provided with Windows to register
' MediaInfoActiveX.dll.
'
' Use at own risk, under the same license as MediaInfo itself.
'
' Ingo Brückl, May 2006

Const MediaInfo_Stream_General  	= 0
Const MediaInfo_Stream_Video    	= 1
Const MediaInfo_Stream_Audio    	= 2
Const MediaInfo_Stream_Text     	= 3 
Const MediaInfo_Stream_Chapters 	= 4
Const MediaInfo_Stream_Image    	= 5
Const MediaInfo_Stream_Menu     	= 6

Const MediaInfo_Info_Name 	 		= 0 
Const MediaInfo_Info_Text 			= 1
Const MediaInfo_Info_Measure 		= 2
Const MediaInfo_Info_Options 		= 3
Const MediaInfo_Info_Name_Text 		= 4
Const MediaInfo_Info_Measure_Text 	= 5
Const MediaInfo_Info_Info 			= 6
Const MediaInfo_Info_HowTo 			= 7
Const MediaInfo_Info_Domain 		= 8

set obj = createObject("MediaInfo.ActiveX")

' information about MediaInfo

msgbox obj.MediaInfo_Option(0, "Info_Version", ""), ,"MediaInfo ActiveX Example"
msgbox obj.MediaInfo_Option(0, "Info_Parameters", ""), ,"Info_Parameters"
msgbox obj.MediaInfo_Option(0, "Info_Capacities", ""), ,"Info_Capacities"
msgbox obj.MediaInfo_Option(0, "Info_Codecs", ""), ,"Info_Codecs"

' an example of how to use the library

msgbox "Open", , "MediaInfo ActiveX Example"
handle = obj.MediaInfo_New()
obj.MediaInfo_Open handle, "C:\Example.ogg"

obj.MediaInfo_Option handle, "Complete", ""
msgbox obj.MediaInfo_Inform(handle, 0), , "Inform with Complete=false"

obj.MediaInfo_Option handle, "Complete", "1"
msgbox obj.MediaInfo_Inform(handle, 0), , "Inform with Complete=true"

obj.MediaInfo_Option handle, "Inform", "General;File size is %FileSize% bytes"
msgbox obj.MediaInfo_Inform(handle, 0), , "Custom Inform"

msgbox obj.MediaInfo_Get(handle, MediaInfo_Stream_General, 0, "FileSize", MediaInfo_Info_Text, MediaInfo_Info_Name), , "Get with StreamKind=General and Parameter=""FileSize"""

msgbox obj.MediaInfo_GetI(handle, MediaInfo_Stream_General, 0, 13, MediaInfo_Info_Text), , "GetI with StreamKind=General and Parameter=13"

msgbox obj.MediaInfo_Count_Get(handle, MediaInfo_Stream_Audio, -1), , "Count_Get with StreamKind=Audio"

msgbox obj.MediaInfo_Get(handle, MediaInfo_Stream_General, 0, "AudioCount", MediaInfo_Info_Text, MediaInfo_Info_Name), , "Get with StreamKind=General and Parameter=""AudioCount"""

msgbox obj.MediaInfo_Get(handle, MediaInfo_Stream_Audio, 0, "StreamCount", MediaInfo_Info_Text, MediaInfo_Info_Name), , "Get with StreamKind=Audio and Parameter=""StreamCount"""

msgbox "Close", , "MediaInfo ActiveX Example"
obj.MediaInfo_Close handle
obj.MediaInfo_Delete handle
