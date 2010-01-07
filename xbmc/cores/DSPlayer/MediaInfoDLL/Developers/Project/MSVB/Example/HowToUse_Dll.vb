'' MediaInfoDLL - All info about media files
'' This software is provided 'as-is', without any express or implied
'' warranty.  In no event will the authors be held liable for any damages
'' arising from the use of this software.
''
'' Permission is granted to anyone to use this software for any purpose,
'' including commercial applications, and to alter it and redistribute it
'' freely, subject to the following restrictions:
''
'' 1. The origin of this software must not be misrepresented; you must not
''    claim that you wrote the original software. If you use this software
''    in a product, an acknowledgment in the product documentation would be
''    appreciated but is not required.
'' 2. Altered source versions must be plainly marked as such, and must not be
''    misrepresented as being the original software.
'' 3. This notice may not be removed or altered from any source distribution.
''
''+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
''+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
''
'' Microsoft Visual Basic example
''
'' To make this example working, you must put MediaInfo.Dll and Example.ogg
'' in the "./Bin/__ConfigurationName__" folder
'' and add MediaInfo_Dll.vb to your project
''
''+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Public Class Form1

    Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Dim To_Display As String
        Dim Temp As String
        Dim MI As MediaInfo

        MI = New MediaInfo
        To_Display = MI.Option_("Info_Version", "0.7.0.0;MediaInfoDLL_Example_MSVB;0.7.0.0")

        If (To_Display.Length() = 0) Then
            RichTextBox1.Text = "MediaInfo.Dll: this version of the DLL is not compatible"
            Return
        End If

        'Information about MediaInfo
        To_Display += vbCrLf + vbCrLf + "Info_Parameters" + vbCrLf
        To_Display += MI.Option_("Info_Parameters")

        To_Display += vbCrLf + vbCrLf + "Info_Capacities" + vbCrLf
        To_Display += MI.Option_("Info_Capacities")

        To_Display += vbCrLf + vbCrLf + "Info_Codecs" + vbCrLf
        To_Display += MI.Option_("Info_Codecs")

        'An example of how to use the library
        To_Display += vbCrLf + vbCrLf + "Open" + vbCrLf
        MI.Open("Example.ogg")

        To_Display += vbCrLf + vbCrLf + "Inform with Complete=false" + vbCrLf
        MI.Option_("Complete")
        To_Display += MI.Inform()

        To_Display += vbCrLf + vbCrLf + "Inform with Complete=true" + vbCrLf
        MI.Option_("Complete", "1")
        To_Display += MI.Inform()

        To_Display += vbCrLf + vbCrLf + "Custom Inform" + vbCrLf
        MI.Option_("Inform", "General;Example : FileSize=%FileSize%")
        To_Display += MI.Inform()

        To_Display += vbCrLf + vbCrLf + "Get with Stream=General and Parameter='FileSize'" + vbCrLf
        To_Display += MI.Get_(StreamKind.General, 0, "FileSize")

        To_Display += vbCrLf + vbCrLf + "GetI with Stream=General and Parameter=46" + vbCrLf
        To_Display += MI.Get_(StreamKind.General, 0, 46)

        To_Display += vbCrLf + vbCrLf + "Count_Get with StreamKind=Stream_Audio" + vbCrLf
        Temp = MI.Count_Get(StreamKind.Audio)
        To_Display += Temp

        To_Display += vbCrLf + vbCrLf + "Get with Stream=General and Parameter='AudioCount'" + vbCrLf
        To_Display += MI.Get_(StreamKind.General, 0, "AudioCount")

        To_Display += vbCrLf + vbCrLf + "Get with Stream=Audio and Parameter='StreamCount'" + vbCrLf
        To_Display += MI.Get_(StreamKind.Audio, 0, "StreamCount")

        To_Display += RichTextBox1.Text + vbCrLf + vbCrLf + "Close" + vbCrLf
        MI.Close()

        'Displaying the text
        RichTextBox1.Text = To_Display
    End Sub
End Class
