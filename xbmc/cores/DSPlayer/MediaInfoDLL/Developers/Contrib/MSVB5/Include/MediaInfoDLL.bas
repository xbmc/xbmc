Attribute VB_Name = "MediaInfoDLL"
Option Explicit

' Include this module in your Visual Basic project for access to MediaInfo.dll and
' keep two things in mind concerning strings for Unicode ability:
'
' 1. All strings (constants and variables) passed to MediaInfo must be passed with StrPtr().
' 2. All strings returned by MediaInfo must be converted with bstr() to a Visual Basic String.
'
' This module is tested and will work with Visual Basic 5 and 6 on Windows 98SE and 2000
' (at least).
'
' Use at own risk, under the same license as MediaInfo itself.
'
' Ingo Brückl, May 2006

#If MEDIAINFO_NO_ENUMS Then
#Else

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

#End If

Public Declare Sub MediaInfo_Close Lib "MediaInfo.dll" (ByVal Handle As Long)
Public Declare Sub MediaInfo_Delete Lib "MediaInfo.dll" (ByVal Handle As Long)
Public Declare Function MediaInfo_Count_Get Lib "MediaInfo.dll" (ByVal Handle As Long, ByVal StreamKind As MediaInfo_stream_C, ByVal StreamNumber As Long) As Long
Public Declare Function MediaInfo_Get Lib "MediaInfo.dll" (ByVal Handle As Long, ByVal StreamKind As MediaInfo_stream_C, ByVal StreamNumber As Long, ByVal Parameter As Long, ByVal InfoKind As MediaInfo_info_C, ByVal SearchKind As MediaInfo_info_C) As Long
Public Declare Function MediaInfo_GetI Lib "MediaInfo.dll" (ByVal Handle As Long, ByVal StreamKind As MediaInfo_stream_C, ByVal StreamNumber As Long, ByVal Parameter As Long, ByVal InfoKind As MediaInfo_info_C) As Long
Public Declare Function MediaInfo_Inform Lib "MediaInfo.dll" (ByVal Handle As Long, ByVal Reserved As Long) As Long
Public Declare Function MediaInfo_New Lib "MediaInfo.dll" () As Long
Public Declare Function MediaInfo_New_Quick Lib "MediaInfo.dll" (ByVal File As Long, ByVal Options As Long) As Long
Public Declare Function MediaInfo_Open Lib "MediaInfo.dll" (ByVal Handle As Long, ByVal File As Long) As Long
Public Declare Function MediaInfo_Open_Buffer Lib "MediaInfo.dll" (ByVal Handle As Long, Begin As Any, ByVal Begin_Size As Long, End_ As Any, ByVal End_Size As Long) As Long
Public Declare Function MediaInfo_Option Lib "MediaInfo.dll" (ByVal Handle As Long, ByVal Option_ As Long, ByVal Value As Long) As Long
Public Declare Function MediaInfo_Save Lib "MediaInfo.dll" (ByVal Handle As Long) As Long
Public Declare Function MediaInfo_Set Lib "MediaInfo.dll" (ByVal Handle As Long, ByVal ToSet As Long, ByVal StreamKind As MediaInfo_stream_C, ByVal StreamNumber As Long, ByVal Parameter As Long, ByVal OldParameter As Long) As Long
Public Declare Function MediaInfo_SetI Lib "MediaInfo.dll" (ByVal Handle As Long, ByVal ToSet As Long, ByVal StreamKind As MediaInfo_stream_C, ByVal StreamNumber As Long, ByVal Parameter As Long, ByVal OldParameter As Long) As Long
Public Declare Function MediaInfo_State_Get Lib "MediaInfo.dll" (ByVal Handle As Long) As Long

Private Declare Function lstrlenW Lib "kernel32" (ByVal pStr As Long) As Long
Private Declare Sub RtlMoveMemory Lib "kernel32" (pDst As Any, pSrc As Any, ByVal bLen As Long)

Public Function bstr(ptr As Long) As String
' convert a C wchar* to a Visual Basic string

  Dim l As Long

  l = lstrlenW(ptr)
  bstr = String$(l, vbNullChar)

  RtlMoveMemory ByVal StrPtr(bstr), ByVal ptr, l * 2

End Function
