'' MediaInfoDLL - All info about media files, for DLL
'' Copyright (C) 2002-2006 Jerome Martinez, Zen@MediaArea.net
''
'' This library is free software; you can redistribute it and/or
'' modify it under the terms of the GNU Lesser General Public
'' License as published by the Free Software Foundation; either
'' version 2.1 of the License, or (at your option) any later version.
''
'' This library is distributed in the hope that it will be useful,
'' but WITHOUT ANY WARRANTY; without even the implied warranty of
'' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
'' Lesser General Public License for more details.
''
'' You should have received a copy of the GNU Lesser General Public
'' License along with this library; if not, write to the Free Software
'' Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
''
''+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
''+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
''
'' Microsoft Visual Basic wrapper for MediaInfo Library
'' See MediaInfo.h for help
''
'' To make it working, you must put MediaInfo.Dll
'' in the executable folder
''
''+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Imports System.Runtime.InteropServices

Public Enum StreamKind As UInteger
    General
    Visual
    Audio
    Text
    Chapters
    Image
    Menu
    Max
End Enum

Public Enum InfoKind As UInteger
    Name
    Text
    Measure
    Options
    NameText
    MeasureText
    Info
    HowTo
    Max
End Enum

Public Enum InfoOptions As UInteger
    ShowInInform
    Reserved
    ShowInSupported
    TypeOfValue
    Max
End Enum

Public Class MediaInfo
    Private Declare Unicode Function MediaInfo_New Lib "MediaInfo.DLL" () As IntPtr
    Private Declare Unicode Sub MediaInfo_Delete Lib "MediaInfo.DLL" (ByVal Handle As IntPtr)
    Private Declare Unicode Function MediaInfo_Open Lib "MediaInfo.DLL" (ByVal Handle As IntPtr, ByVal FileName As String) As UIntPtr
    Private Declare Unicode Sub MediaInfo_Close Lib "MediaInfo.DLL" (ByVal Handle As IntPtr)
    Private Declare Unicode Function MediaInfo_Inform Lib "MediaInfo.DLL" (ByVal Handle As IntPtr, ByVal Reserved As UIntPtr) As IntPtr
    Private Declare Unicode Function MediaInfo_GetI Lib "MediaInfo.DLL" (ByVal Handle As IntPtr, ByVal StreamKind As UIntPtr, ByVal StreamNumber As UIntPtr, ByVal Parameter As UIntPtr, ByVal KindOfInfo As UIntPtr) As IntPtr 'See MediaInfoDLL.h for enumeration values
    Private Declare Unicode Function MediaInfo_Get Lib "MediaInfo.DLL" (ByVal Handle As IntPtr, ByVal StreamKind As UIntPtr, ByVal StreamNumber As UIntPtr, ByVal Parameter As String, ByVal KindOfInfo As UIntPtr, ByVal KindOfSearch As UIntPtr) As IntPtr
    Private Declare Unicode Function MediaInfo_Option Lib "MediaInfo.DLL" (ByVal Handle As IntPtr, ByVal Option_ As String, ByVal Value As String) As IntPtr
    Private Declare Unicode Function MediaInfo_State_Get Lib "MediaInfo.DLL" (ByVal Handle As IntPtr) As UIntPtr 'see MediaInfo.h for details
    Private Declare Unicode Function MediaInfo_Count_Get Lib "MediaInfo.DLL" (ByVal Handle As IntPtr, ByVal StreamKind As UIntPtr, ByVal StreamNumber As IntPtr) As UIntPtr 'see MediaInfoDLL.h for enumeration values

    Dim Handle As IntPtr

    Sub New()
        Handle = MediaInfo_New()
    End Sub
    Protected Overrides Sub Finalize()
        MediaInfo_Delete(Handle)
    End Sub
    Function Open(ByVal FileName As String) As Integer
        Return MediaInfo_Open(Handle, FileName)
    End Function
    Sub Close()
        MediaInfo_Close(Handle)
    End Sub
    Function Inform() As String
        Return Marshal.PtrToStringUni(MediaInfo_Inform(Handle, 0))
    End Function
    Function Get_(ByVal StreamKind As StreamKind, ByVal StreamNumber As Integer, ByVal Parameter As Integer, Optional ByVal KindOfInfo As InfoKind = InfoKind.Text) As String
        Return Marshal.PtrToStringUni(MediaInfo_GetI(Handle, StreamKind, StreamNumber, Parameter, KindOfInfo))
    End Function
    Function Get_(ByVal StreamKind As StreamKind, ByVal StreamNumber As Integer, ByVal Parameter As String, Optional ByVal KindOfInfo As InfoKind = InfoKind.Text, Optional ByVal KindOfSearch As InfoKind = InfoKind.Name) As String
        Return Marshal.PtrToStringUni(MediaInfo_Get(Handle, StreamKind, StreamNumber, Parameter, KindOfInfo, KindOfSearch))
    End Function
    Function Option_(ByVal Option__ As String, Optional ByVal Value As String = "") As String
        Return Marshal.PtrToStringUni(MediaInfo_Option(Handle, Option__, Value))
    End Function
    Function State_Get() As Integer
        Return MediaInfo_State_Get(Handle)
    End Function
    Function Count_Get(ByVal StreamKind As StreamKind, Optional ByVal StreamNumber As UInteger = UInteger.MaxValue) As Integer
        If StreamNumber = UInteger.MaxValue Then
            Dim A As Long
            A = 0
            A = A - 1 'If you know how to have (IntPtr)(-1) easier, I am interested ;-)
            Return MediaInfo_Count_Get(Handle, StreamKind, A)
        Else
            Return MediaInfo_Count_Get(Handle, StreamKind, StreamNumber)
        End If
    End Function
End Class



