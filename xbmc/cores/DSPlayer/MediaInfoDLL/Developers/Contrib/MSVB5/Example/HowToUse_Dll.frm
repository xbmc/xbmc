VERSION 5.00
Object = "{3B7C8863-D78F-101B-B9B5-04021C009402}#1.1#0"; "RICHTX32.OCX"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   9600
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8670
   LinkTopic       =   "Form1"
   ScaleHeight     =   9600
   ScaleWidth      =   8670
   StartUpPosition =   3  'Windows-Standard
   Begin RichTextLib.RichTextBox RichTextBox1 
      Height          =   9375
      Left            =   0
      TabIndex        =   0
      Top             =   120
      Width           =   8655
      _ExtentX        =   15266
      _ExtentY        =   16536
      _Version        =   327681
      ScrollBars      =   3
      RightMargin     =   10000
      TextRTF         =   $"HowToUse_Dll.frx":0000
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "Courier New"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

' In order to run this example, put MediaInfo.dll into your system directory
' and Example.ogg into the root directory of drive C: (i.e. C:\).
'
' Use at own risk, under the same license as MediaInfo itself.
'
' Ingo Brückl, May 2006

Private Sub Form_Load()

  Dim display As String
  Dim Handle As Long

  ' information about MediaInfo

  display = bstr(MediaInfo_Option(0, StrPtr("Info_Version"), StrPtr("")))

  display = display + vbCrLf + vbCrLf + "Info_Parameters" + vbCrLf
  display = display + bstr(MediaInfo_Option(0, StrPtr("Info_Parameters"), StrPtr("")))

  display = display + vbCrLf + vbCrLf + "Info_Capacities" + vbCrLf
  display = display + bstr(MediaInfo_Option(0, StrPtr("Info_Capacities"), StrPtr("")))

  display = display + vbCrLf + vbCrLf + "Info_Codecs" + vbCrLf
  display = display + bstr(MediaInfo_Option(0, StrPtr("Info_Codecs"), StrPtr("")))

  ' an example of how to use the library

  display = display + vbCrLf + vbCrLf + "Open" + vbCrLf
  Handle = MediaInfo_New()
  Call MediaInfo_Open(Handle, StrPtr("C:\Example.ogg"))

  display = display + vbCrLf + vbCrLf + "Inform with Complete=false" + vbCrLf
  Call MediaInfo_Option(Handle, StrPtr("Complete"), StrPtr(""))
  display = display + bstr(MediaInfo_Inform(Handle, 0))

  display = display + vbCrLf + vbCrLf + "Inform with Complete=true" + vbCrLf
  Call MediaInfo_Option(Handle, StrPtr("Complete"), StrPtr("1"))
  display = display + bstr(MediaInfo_Inform(Handle, 0))

  display = display + vbCrLf + vbCrLf + "Custom Inform" + vbCrLf
  Call MediaInfo_Option(Handle, StrPtr("Inform"), StrPtr("General;File size is %FileSize% bytes"))
  display = display + bstr(MediaInfo_Inform(Handle, 0))

  display = display + vbCrLf + vbCrLf + "Get with StreamKind=General and Parameter=""FileSize""" + vbCrLf
  display = display + bstr(MediaInfo_Get(Handle, MediaInfo_Stream_General, 0, StrPtr("FileSize"), MediaInfo_Info_Text, MediaInfo_Info_Name))

  display = display + vbCrLf + vbCrLf + "GetI with StreamKind=General and Parameter=13" + vbCrLf
  display = display + bstr(MediaInfo_GetI(Handle, MediaInfo_Stream_General, 0, 13, MediaInfo_Info_Text))

  display = display + vbCrLf + vbCrLf + "Count_Get with StreamKind=Audio" + vbCrLf
  display = display & MediaInfo_Count_Get(Handle, MediaInfo_Stream_Audio, -1)

  display = display + vbCrLf + vbCrLf + "Get with StreamKind=General and Parameter=""AudioCount""" + vbCrLf
  display = display + bstr(MediaInfo_Get(Handle, MediaInfo_Stream_General, 0, StrPtr("AudioCount"), MediaInfo_Info_Text, MediaInfo_Info_Name))

  display = display + vbCrLf + vbCrLf + "Get with StreamKind=Audio and Parameter=""StreamCount""" + vbCrLf
  display = display + bstr(MediaInfo_Get(Handle, MediaInfo_Stream_Audio, 0, StrPtr("StreamCount"), MediaInfo_Info_Text, MediaInfo_Info_Name))

  display = display + vbCrLf + vbCrLf + "Close" + vbCrLf
  Call MediaInfo_Close(Handle)
  Call MediaInfo_Delete(Handle)

  ' displaying the text

  RichTextBox1.TextRTF = display
  
End Sub
