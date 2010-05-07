' lame.vbs WindowsScript wrapper v0.5, 06/15/2001
' $id$
'
' *Purpose*
' Use this WindowsScript to encode WAVs using drag&drop:
' 0.  make sure you have windows script host v5.1 on your system
'     (enter 'cscript' in a DOS-Box and compare version number)
' 1.  adjust the path settings below to fit your needs
' 2a. put this file somewhere on the desktop
' 3a. drag one or more wav-files on the icon and watch them being lamed.
'
' 2b. start->execute, enter "sendto", drag the script or a link to it in
'     sendto window (adjust names and icon as you like)
' 3b. select wave-file(s) and send it via the send-to menu to LAME!
'
' You may wish to create copies of this file with different options set.
'
' If you would like a GUI: try to enable the HTML UI (see below)
'
' Ralf Kempkens, ralf.kempkens@epost.de
'
'
' *History*
' V0.5 * lame.vbs will automatically decode if the file has a .mp3 extension
'      * now explicitly refuses to accept folders
' V0.4 * creates single .mp3 extensions, now ID3 options in HTML interface
' V0.3 * fixed bug that prevented lame.exe to be located in a path that 
'        contained a space
'      * experimental HTML UI support (disabled by default)
' V0.2 added multiple file support
' V0.1 initial release

' *** change path to your needs ***
    path = "D:\Audio\Lame\Lame386\"   '!!! must end with a backslash !!!
    lame = "lame.exe"

' *** change default options to your needs ***
    opts = "--preset hifi"

' *** HTML GUI (experimental) ***
    useGUI = False
' it set to True, opens file lameGUI.html residing in the same path as lame.exe
' to choose options. Please look at the example HTML-file for further information.

' no changes needed below this line
' ##########################################################################
Dim wsh, args, infile, fs
title="LAME Script"

' get input files
Set wsh = WScript.CreateObject("WScript.Shell")
Set args = WScript.Arguments
If args.Count = 0 Then
  MsgBox "LAME mp3 encoder/decoder frontend script." & vbCR & _ 
	"Please use drag & drop to specify input files.", vbInformation, title
  WScript.Quit
End If

' check path
Set fso = CreateObject("Scripting.FileSystemObject")
If Not fso.FileExists(path & lame) Then
  MsgBox "Could not find LAME!" & vbCR & "(looked for '" & path & lame & "')", vbCritical, title
  WScript.Quit
End If

' start GUI
if useGUI Then
  set ie=WScript.CreateObject("InternetExplorer.Application", "ie_")
  ie.navigate(path & "lameGUI.html")
  do 
    WScript.Sleep 100 
  loop until ie.ReadyState=4 'wait for GUI

  ie.Width=640
  ie.Height=600
  ie.Toolbar=false
  ie.Statusbar=false
  ie.visible=true

  'link to GUI
  set document=ie.document
  document.forms.lameform.okbutton.onClick=GetRef("okbutton")

  'wait for user pressing ok...
  do 
    WScript.Sleep 300 
  loop until process
end if

'process files
For i = 0 To args.Count-1
  infile = args(i)
  ' check input file
  If fso.FolderExists(infile) Then
    MsgBox "'" & infile & "' is a folder!" & vbCR & _
	title & " only handles proper files.", vbInformation, title
  Else
   If Not fso.FileExists(infile) Then
      MsgBox "Error opening input-file" & vbCR & "'" & infile & "'", vbCritical , title
    Else
    ' run lame
    If(LCase(getExtension(infile))="mp3") Then 'decode
      ret = wsh.Run(Chr(34) & path & lame & CHR(34) & " --decode " & _
          Chr(34) & infile & Chr(34) & Chr(32) & Chr(34) & _
          getBasename(infile) & ".wav" & Chr(34), 1, True)
    Else ' encode
      ret = wsh.Run(Chr(34) & path & lame & CHR(34) & Chr(32) & opts & Chr(32) & _
          Chr(34) & infile & Chr(34) & Chr(32) & Chr(34) & _
          getBasename(infile) & ".mp3" & Chr(34), 1, True)
    End If
    ' diagnostics
    Select Case ret
    Case (0) 'okeydokey
    Case (-1)
      MsgBox "LAME aborted by user!", vbExclamation, title
    Case (1)
      MsgBox "Error returned by LAME!" & vbCR & "(Check LAME options and input file formats.)" & vbCR & "Used Options: " & opts, vbCritical, title
    Case Else
      MsgBox "Received unknown LAME return-code: " & ret, vbCritical, title
    End Select
   End If
  End If
Next

WScript.Quit
' *******************************************************************
' utility functions

Function getBasename(filespec)
  Dim fso
  Set fso = CreateObject("Scripting.FileSystemObject")
  Set f = fso.GetFile(filespec)
  
  getBasename = f.ParentFolder & "\" & fso.GetBaseName(filespec)
End Function

Function getExtension(filespec)
  Dim fso
  Set fso = CreateObject("Scripting.FileSystemObject")
  Set f = fso.GetFile(filespec)
  
  getExtension = fso.GetExtensionName(filespec) 
End Function

' *******************************************************************
' manage link to IE HTML-interface

sub okbutton
  'process inputs
  opts=document.all.lameoptions.Value
  ie.Quit
  MsgBox "LAME options:" & vbCR & opts, vbInformation, title
end sub

sub ie_onQuit
  process=True
end sub
'eof
