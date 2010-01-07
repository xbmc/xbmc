VERSION 5.00
Object = "{3B7C8863-D78F-101B-B9B5-04021C009402}#1.2#0"; "RICHTX32.OCX"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   6240
   ClientLeft      =   60
   ClientTop       =   450
   ClientWidth     =   10500
   LinkTopic       =   "Form1"
   ScaleHeight     =   6240
   ScaleWidth      =   10500
   StartUpPosition =   3  'Windows Default
   Begin RichTextLib.RichTextBox RichTextBox1 
      Height          =   6015
      Left            =   240
      TabIndex        =   0
      Top             =   120
      Width           =   10095
      _ExtentX        =   17806
      _ExtentY        =   10610
      _Version        =   393217
      Enabled         =   -1  'True
      ScrollBars      =   2
      TextRTF         =   $"HowToUse_Dll.frx":0000
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'YOU MUST DO : Replace _.OGM by a valid filename
'WARNING : Use GetString Method for converting char* to String
'TODO : how to say to the richEditBox a carriage return???
Private Declare Function lstrlenA Lib "kernel32" (ByVal Ptr As Any) As Long
Private Declare Function lstrcpyA Lib "kernel32" (ByVal RetVal As String, ByVal Ptr As Long) As Long
Private Declare Function MediaInfoA_New Lib "MediaInfo.DLL" () As Long
Private Declare Function MediaInfoA_Delete Lib "MediaInfo.DLL" (ByVal Handle As Long) As Long 'Should be void
Private Declare Function MediaInfoA_Open Lib "MediaInfo.DLL" (ByVal Handle As Long, ByVal File As String) As Long
Private Declare Function MediaInfoA_Close Lib "MediaInfo.DLL" (ByVal Handle As Long) As Long
Private Declare Function MediaInfoA_Inform Lib "MediaInfo.DLL" (ByVal Handle As Long) As Long 'WARNING : This is a char* value
Private Declare Function MediaInfoA_GetI Lib "MediaInfo.DLL" (ByVal Handle As Long, ByVal StreamKind As Long, ByVal StreamNumber As Long, ByVal Parameter As Long, ByVal KindOfInfo As Long) As Long 'WARNING : This is a char* value, see MediaInfoDLL.h for enumeration values
Private Declare Function MediaInfoA_Get Lib "MediaInfo.DLL" (ByVal Handle As Long, ByVal StreamKind As Long, ByVal StreamNumber As Long, ByVal Parameter As String, ByVal KindOfInfo As Long, ByVal KindOfSearch As String) As Long 'WARNING : This is a char* value, see MediaInfoDLL.h for enumeration values
Private Declare Function MediaInfoA_Count_Get Lib "MediaInfo.DLL" (ByVal Handle As Long, ByVal StreamKind As Long, ByVal StreamNumber As Long) As Long 'see MediaInfoDLL.h for enumeration values
Private Declare Function MediaInfoA_Complete_Get Lib "MediaInfo.DLL" () As Long
Private Declare Function MediaInfoA_Complete_Set Lib "MediaInfo.DLL" (ByVal Complete As Long) As Long
Private Declare Function MediaInfoA_Inform_Set Lib "MediaInfo.DLL" (ByVal Handle As Long, ByVal Text As String) As Long

Public Function GetString(ByVal lpszA As Long) As String

    GetString = String$(lstrlenA(ByVal lpszA), 0)
    Call lstrcpyA(ByVal GetString, ByVal lpszA)
  
End Function
Private Sub Form_Load()
    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nNew\r\n"
    Handle = MediaInfoA_New()

    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nOpen\r\n"
    MediaInfoA_Open(Handle, "D:\\_.ogm")

    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nInform with Complete=false\r\n"
    MediaInfoA_Complete_Set (False)
    AAfficher = GetString(MediaInfoA_Inform(Handle))
    RichTextBox1.Text = RichTextBox1.Text + AAfficher

    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nInform with Complete=true\r\n"
    MediaInfoA_Complete_Set (True)
    AAfficher = GetString(MediaInfoA_Inform(Handle))
    RichTextBox1.Text = RichTextBox1.Text + AAfficher

    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nCustom Inform\r\n"
    MediaInfoA_Inform_Set Handle, "General;%FileSize%"
    AAfficher = GetString(MediaInfoA_Inform(Handle))
    RichTextBox1.Text = RichTextBox1.Text + AAfficher

    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nGetI with Stream=General and Parameter=13\r\n"
    AAfficher = Format(MediaInfoA_GetI(Handle, 0, 0, 13, Info_Text), "##########")
    RichTextBox1.Text = RichTextBox1.Text + AAfficher

    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nCount_Get with StreamKind=Stream_Audio\r\n"
    AAfficher = Format(MediaInfoA_Count_Get(Handle, 2, -1), "##########")
    RichTextBox1.Text = RichTextBox1.Text + AAfficher

    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nGet with Stream=General and Parameter='AudioCount'\r\n"
    AAfficher = GetString(MediaInfoA_Get(Handle, 0, 0, "AudioCount", 1, 0))
    RichTextBox1.Text = RichTextBox1.Text + AAfficher

    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nGet with Stream=Audio and Parameter='StreamCount'\r\n"
    AAfficher = GetString(MediaInfoA_Get(Handle, 2, 0, "StreamCount", 1, 0))
    RichTextBox1.Text = RichTextBox1.Text + AAfficher

    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nGet with Stream=General and Parameter='FileSize'\r\n"
    AAfficher = GetString(MediaInfoA_Get(Handle, 0, 0, "FileSize", 1, 0))
    RichTextBox1.Text = RichTextBox1.Text + AAfficher

    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nClose\r\n"
    MediaInfoA_Close Handle
    
    RichTextBox1.Text = RichTextBox1.Text + "\r\n\r\nDelete\r\n"
    Handle = MediaInfoA_Delte()
End Sub
