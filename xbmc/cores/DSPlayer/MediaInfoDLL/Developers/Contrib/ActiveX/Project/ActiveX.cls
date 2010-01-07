VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "ActiveX"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Attribute VB_Description = "MediaInfo ActiveX Wrapper"
Option Explicit

' A simple ActiveX wrapper for the MediaInfo DLL.
'
' Use at own risk, under the same license as MediaInfo itself.
'
' Ingo Brückl, May 2006

' The enums are only declared here in order to appear in the type library information.

Public Enum MediaInfo_stream_C
  MediaInfo_Stream_General
  MediaInfo_Stream_Video
  MediaInfo_Stream_Audio
  MediaInfo_Stream_Text
  MediaInfo_Stream_Chapters
  MediaInfo_Stream_Image
  MediaInfo_Stream_Menu
  MediaInfo_Stream_Max
End Enum

Public Enum MediaInfo_info_C
  MediaInfo_Info_Name
  MediaInfo_Info_Text
  MediaInfo_Info_Measure
  MediaInfo_Info_Options
  MediaInfo_Info_Name_Text
  MediaInfo_Info_Measure_Text
  MediaInfo_Info_Info
  MediaInfo_Info_HowTo
  MediaInfo_Info_Max
End Enum

Public Enum MediaInfo_infooptions_C
  MediaInfo_InfoOption_ShowInInform
  MediaInfo_InfoOption_Support
  MediaInfo_InfoOption_ShowInSupported
  MediaInfo_InfoOption_TypeOfValue
  MediaInfo_InfoOption_Max
End Enum

Public Sub MediaInfo_Close(ByVal Handle As Long)
Attribute MediaInfo_Close.VB_Description = "Close a file opened before with Open() (without saving)"

  Call MediaInfoDLL.MediaInfo_Close(Handle)

End Sub

Public Sub MediaInfo_Delete(ByVal Handle As Long)
Attribute MediaInfo_Delete.VB_Description = "Delete a MediaInfo interface"

  Call MediaInfoDLL.MediaInfo_Delete(Handle)

End Sub

Public Function MediaInfo_Count_Get(ByVal Handle As Long, ByVal StreamKind As MediaInfo_stream_C, ByVal StreamNumber As Long) As Long
Attribute MediaInfo_Count_Get.VB_Description = "Count of streams of a stream kind (StreamNumber not filled), or count of piece of information in this stream"

  MediaInfo_Count_Get = MediaInfoDLL.MediaInfo_Count_Get(Handle, StreamKind, StreamNumber)

End Function

Public Function MediaInfo_Get(ByVal Handle As Long, ByVal StreamKind As MediaInfo_stream_C, ByVal StreamNumber As Long, Parameter As String, ByVal InfoKind As MediaInfo_info_C, ByVal SearchKind As MediaInfo_info_C) As String
Attribute MediaInfo_Get.VB_Description = "Get a piece of information about a file (parameter is a string)"

  MediaInfo_Get = bstr(MediaInfoDLL.MediaInfo_Get(Handle, StreamKind, StreamNumber, StrPtr(Parameter), InfoKind, SearchKind))

End Function

Public Function MediaInfo_GetI(ByVal Handle As Long, ByVal StreamKind As MediaInfo_stream_C, ByVal StreamNumber As Long, ByVal Parameter As Long, ByVal InfoKind As MediaInfo_info_C) As String
Attribute MediaInfo_GetI.VB_Description = "Get a piece of information about a file (parameter is an integer)"

  MediaInfo_GetI = bstr(MediaInfoDLL.MediaInfo_GetI(Handle, StreamKind, StreamNumber, Parameter, InfoKind))

End Function

Public Function MediaInfo_Inform(ByVal Handle As Long, ByVal Reserved As Long) As String
Attribute MediaInfo_Inform.VB_Description = "Get all details about a file in one string"

  MediaInfo_Inform = bstr(MediaInfoDLL.MediaInfo_Inform(Handle, Reserved))

End Function

Public Function MediaInfo_New() As Long
Attribute MediaInfo_New.VB_Description = "Create a new MediaInfo interface and return a handle to it"

  MediaInfo_New = MediaInfoDLL.MediaInfo_New()

End Function

Public Function MediaInfo_New_Quick(File As String, Options As String) As Long
Attribute MediaInfo_New_Quick.VB_Description = "Create a new MediaInfo interface (with a quick init of useful options) and return a handle to it"

  MediaInfo_New_Quick = MediaInfoDLL.MediaInfo_New_Quick(StrPtr(File), StrPtr(Options))

End Function

Public Function MediaInfo_Open(ByVal Handle As Long, File As String) As Long
Attribute MediaInfo_Open.VB_Description = "Open a file and collect information about it (technical information and tags)"

  MediaInfo_Open = MediaInfoDLL.MediaInfo_Open(Handle, StrPtr(File))

End Function

Public Function MediaInfo_Open_Buffer(ByVal Handle As Long, Begin As Long, ByVal Begin_Size As Long, End_ As Long, ByVal End_Size As Long) As Long
Attribute MediaInfo_Open_Buffer.VB_Description = "Open a stream (begin and end of the stream) and collect information about it (technical information and tags)"

  MediaInfo_Open_Buffer = MediaInfoDLL.MediaInfo_Open_Buffer(Handle, ByVal Begin, Begin_Size, ByVal End_, End_Size)

End Function

Public Function MediaInfo_Option(ByVal Handle As Long, Option_ As String, Value As String) As String
Attribute MediaInfo_Option.VB_Description = "Configure or get information about MediaInfoLib"

  MediaInfo_Option = bstr(MediaInfoDLL.MediaInfo_Option(Handle, StrPtr(Option_), StrPtr(Value)))

End Function

Public Function MediaInfo_Save(ByVal Handle As Long) As Long
Attribute MediaInfo_Save.VB_Description = "Save the file opened before with Open() (modifications of tags)"

  MediaInfo_Save = MediaInfoDLL.MediaInfo_Save(Handle)

End Function

Public Function MediaInfo_Set(ByVal Handle As Long, ToSet As String, ByVal StreamKind As MediaInfo_stream_C, ByVal StreamNumber As Long, Parameter As String, OldParameter As String) As Long
Attribute MediaInfo_Set.VB_Description = "Set a piece of information about a file (parameter is a string)"

  MediaInfo_Set = MediaInfoDLL.MediaInfo_Set(Handle, StrPtr(ToSet), StreamKind, StreamNumber, StrPtr(Parameter), StrPtr(OldParameter))

End Function

Public Function MediaInfo_SetI(ByVal Handle As Long, ToSet As String, ByVal StreamKind As MediaInfo_stream_C, ByVal StreamNumber As Long, ByVal Parameter As Long, OldParameter As String) As Long
Attribute MediaInfo_SetI.VB_Description = "Set a piece of information about a file (parameter is an integer)"

  MediaInfo_SetI = MediaInfoDLL.MediaInfo_SetI(Handle, StrPtr(ToSet), StreamKind, StreamNumber, Parameter, StrPtr(OldParameter))

End Function

Public Function MediaInfo_State_Get(ByVal Handle As Long) As Long
Attribute MediaInfo_State_Get.VB_Description = "Get the state of the library"

  MediaInfo_State_Get = MediaInfoDLL.MediaInfo_State_Get(Handle)

End Function
