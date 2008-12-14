;
; install.iss -- Inno Setup 4 install configuration file for Mbedthis Appweb
;
; Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
;

[Setup]
AppName=!!BLD_NAME!!
AppVerName=!!BLD_NAME!! !!BLD_VERSION!!-!!BLD_NUMBER!!
DefaultDirName={sd}!!BLD_PREFIX!!
DefaultGroupName=!!BLD_NAME!!
UninstallDisplayIcon={app}/!!BLD_PRODUCT!!.exe
LicenseFile=!!BLD_PREFIX!!/LICENSE.TXT

[Icons]
Name: "{group}\!!BLD_NAME!!"; Filename: "{app}/bin/!!BLD_PRODUCT!!.exe"; Parameters: "-c -f appweb.conf"
Name: "{group}\ReadMe"; Filename: "{app}/README.TXT"

[Registry]
;Root: HKLM; Subkey: "System\Current Control Set\Services\EventLog\Application\!!BLD_PRODUCT!!"; Flags: uninsdeletekeyifempty
;Root: HKCU; Subkey: "Software\!!BLD_COMPANY!!"; Flags: uninsdeletekeyifempty
;Root: HKCU; Subkey: "Software\!!BLD_COMPANY!!\Sample"; Flags: uninsdeletekey
;Root: HKLM; Subkey: "Software\!!BLD_COMPANY!!"; Flags: uninsdeletekeyifempty
;Root: HKLM; Subkey: "Software\!!BLD_COMPANY!!\Sample"; Flags: uninsdeletekey
;Root: HKLM; Subkey: "Software\!!BLD_COMPANY!!\Sample\Settings"; ValueType: string; ValueName: "Path"; ValueData: "{app}"

[Types]
Name: "full"; Description: "Complete Installation with Documentation and Samples"; 
Name: "binary"; Description: "Binary Installation"; 
Name: "documentation"; Description: "Documentation"; 
Name: "development"; Description: "Development Headers and Samples"; 
; Name: "source"; Description: "Program Source Code"; 

[Components]
Name: "bin"; Description: "Binary Files"; Types: binary full;
Name: "doc"; Description: "Documentation Files"; Types: documentation full;
Name: "dev"; Description: "Development Headers"; Types: development full;
; Name: "src"; Description: "Program Source Code"; Types: source full;

[Dirs]
Name: "{app}/logs"
Name: "{app}/bin"

[UninstallDelete]
Type: files; Name: "{app}/appweb.conf";
Type: files; Name: "{app}/logs/access.log";
Type: files; Name: "{app}/logs/access.log.old";
Type: files; Name: "{app}/logs/error.log";
Type: files; Name: "{app}/logs/error.log.old";
Type: filesandordirs; Name: "{app}/*.obj";

[Code]
function IsPresent(const file: String): Boolean;
begin
  file := ExpandConstant(file);
  if FileExists(file) then begin
    Result := True;
  end else begin
    Result := False;
  end
end;

[Run]
Filename: "{app}/bin/!!BLD_PRODUCT!!.exe"; Parameters: "-i default"; WorkingDir: "{app}"; StatusMsg: "Installing Appweb as a Windows Service"; Flags: waituntilidle; Check: IsPresent('{app}/bin/!!BLD_PRODUCT!!.exe')
Filename: "{app}/bin/!!BLD_PRODUCT!!.exe"; Parameters: "-g"; WorkingDir: "{app}"; StatusMsg: "Starting the Appweb Server"; Flags: waituntilidle; Check: IsPresent('{app}/bin/!!BLD_PRODUCT!!.exe')
Filename: "http://127.0.0.1:7777/"; Description: "View the Documentation"; Flags: skipifsilent waituntilidle shellexec postinstall; Check: IsPresent('{app}/doc/product/index.html'); Components: bin

[UninstallRun]
Filename: "{app}/bin/!!BLD_PRODUCT!!.exe"; Parameters: "-c -u"; WorkingDir: "{app}"; Check: IsPresent('{app}/bin/!!BLD_PRODUCT!!.exe'); Components: bin
Filename: "{app}/bin/removeFiles.exe"; Parameters: "-r -s 5"; WorkingDir: "{app}"; Flags:

[Files]
