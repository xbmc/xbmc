unit MediaInfoDll;

{
  MediaInfoLib (MediaInfo.dll v0.7.7.6) Interface for Delphi
    (c)2008 by Norbert Mereg (Icebob)

    http://mediainfo.sourceforge.net
                                                                }


interface
uses
{$IFDEF WIN32}
  Windows;
{$ELSE}
  Wintypes, WinProcs;
{$ENDIF}

type TMIStreamKind =
(
    Stream_General,
    Stream_Video,
    Stream_Audio,
    Stream_Text,
    Stream_Chapters,
    Stream_Image,
    Stream_Menu,
    Stream_Max
);

type TMIInfo = 
(
    Info_Name,
    Info_Text,
    Info_Measure,
    Info_Options,
    Info_Name_Text,
    Info_Measure_Text,
    Info_Info,
    Info_HowTo,
    Info_Max
);

type TMIInfoOption =
(
    InfoOption_ShowInInform,
    InfoOption_Reserved,
    InfoOption_ShowInSupported,
    InfoOption_TypeOfValue,
    InfoOption_Max
);


{$IFDEF STATIC}
  // Unicode methods
  function  MediaInfo_New(): Cardinal cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  procedure MediaInfo_Delete(Handle: Cardinal) cdecl  {$IFDEF WIN32} stdcall {$ENDIF}; external 'MediaInfo.Dll';
  function  MediaInfo_Open(Handle: Cardinal; File__: PWideChar): Cardinal cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  procedure MediaInfo_Close(Handle: Cardinal) cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  function  MediaInfo_Inform(Handle: Cardinal; Reserved: Integer): PWideChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  function  MediaInfo_GetI(Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer; Parameter: Integer; KindOfInfo: TMIInfo): PWideChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll'; //Default: KindOfInfo=Info_Text
  function  MediaInfo_Get(Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer; Parameter: PWideChar; KindOfInfo: TMIInfo; KindOfSearch: TMIInfo): PWideChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll'; //Default: KindOfInfo=Info_Text, KindOfSearch=Info_Name
  function  MediaInfo_Option(Handle: Cardinal; Option: PWideChar; Value: PWideChar): PWideChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  function  MediaInfo_State_Get(Handle: Cardinal): Integer cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  function  MediaInfo_Count_Get(Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer): Integer cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';

  // Ansi methods
  function  MediaInfoA_New(): Cardinal cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  procedure MediaInfoA_Delete(Handle: Cardinal) cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  function  MediaInfoA_Open(Handle: Cardinal; File__: PChar): Cardinal cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  procedure MediaInfoA_Close(Handle: Cardinal) cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  function  MediaInfoA_Inform(Handle: Cardinal; Reserved: Integer): PChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  function  MediaInfoA_GetI(Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer; Parameter: Integer; KindOfInfo: TMIInfo): PChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll'; //Default: KindOfInfo=Info_Text
  function  MediaInfoA_Get(Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer; Parameter: PChar; KindOfInfo: TMIInfo; KindOfSearch: TMIInfo): PChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll'; //Default: KindOfInfo=Info_Text, KindOfSearch=Info_Name
  function  MediaInfoA_Option(Handle: Cardinal; Option: PChar; Value: PChar): PChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  function  MediaInfoA_State_Get(Handle: Cardinal): Integer cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
  function  MediaInfoA_Count_Get(Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer): Integer cdecl  {$IFDEF WIN32} stdcall {$ENDIF};external 'MediaInfo.Dll';
{$ELSE}

var
  LibHandle: THandle = 0;

  // Unicode methods
  MediaInfo_New:        function  (): Cardinal cdecl stdcall;
  MediaInfo_Delete:     procedure (Handle: Cardinal) cdecl stdcall;
  MediaInfo_Open:       function  (Handle: Cardinal; File__: PWideChar): Cardinal cdecl stdcall;
  MediaInfo_Close:      procedure (Handle: Cardinal) cdecl stdcall;
  MediaInfo_Inform:     function  (Handle: Cardinal; Reserved: Integer): PWideChar cdecl stdcall;
  MediaInfo_GetI:       function  (Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer; Parameter: Integer;   KindOfInfo: TMIInfo): PWideChar cdecl stdcall; //Default: KindOfInfo=Info_Text,
  MediaInfo_Get:        function  (Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer; Parameter: PWideChar; KindOfInfo: TMIInfo; KindOfSearch: TMIInfo): PWideChar cdecl stdcall; //Default: KindOfInfo=Info_Text, KindOfSearch=Info_Name
  MediaInfo_Option:     function  (Handle: Cardinal; Option: PWideChar; Value: PWideChar): PWideChar cdecl stdcall;
  MediaInfo_State_Get:  function  (Handle: Cardinal): Integer cdecl stdcall;
  MediaInfo_Count_Get:  function  (Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer): Integer cdecl stdcall;

  // Ansi methods
  MediaInfoA_New:       function  (): Cardinal cdecl stdcall;
  MediaInfoA_Delete:    procedure (Handle: Cardinal) cdecl stdcall;
  MediaInfoA_Open:      function  (Handle: Cardinal; File__: PChar): Cardinal cdecl stdcall;
  MediaInfoA_Close:     procedure (Handle: Cardinal) cdecl stdcall;
  MediaInfoA_Inform:    function  (Handle: Cardinal; Reserved: Integer): PChar cdecl stdcall;
  MediaInfoA_GetI:      function  (Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer; Parameter: Integer; KindOfInfo: TMIInfo): PChar cdecl stdcall; //Default: KindOfInfo=Info_Text
  MediaInfoA_Get:       function  (Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer; Parameter: PChar;   KindOfInfo: TMIInfo; KindOfSearch: TMIInfo): PChar cdecl stdcall; //Default: KindOfInfo=Info_Text, KindOfSearch=Info_Name
  MediaInfoA_Option:    function  (Handle: Cardinal; Option: PChar; Value: PChar): PChar cdecl stdcall;
  MediaInfoA_State_Get: function  (Handle: Cardinal): Integer cdecl stdcall;
  MediaInfoA_Count_Get: function  (Handle: Cardinal; StreamKind: TMIStreamKind; StreamNumber: Integer): Integer cdecl stdcall;

function MediaInfoDLL_Load(LibPath: string): boolean;

{$ENDIF}

implementation

{$IFNDEF STATIC}
function MI_GetProcAddress(Name: PChar; var Addr: Pointer): boolean;
begin
  Addr := GetProcAddress(LibHandle, Name);
  Result := Addr <> nil;
end;

function MediaInfoDLL_Load(LibPath: string): boolean;
begin
  Result := False;

  if LibHandle = 0 then
    LibHandle := LoadLibrary(PChar(LibPath));

  if LibHandle <> 0 then
  begin
    MI_GetProcAddress('MediaInfo_New',        @MediaInfo_New);
    MI_GetProcAddress('MediaInfo_Delete',     @MediaInfo_Delete);
    MI_GetProcAddress('MediaInfo_Open',       @MediaInfo_Open);
    MI_GetProcAddress('MediaInfo_Close',      @MediaInfo_Close);
    MI_GetProcAddress('MediaInfo_Inform',     @MediaInfo_Inform);
    MI_GetProcAddress('MediaInfo_GetI',       @MediaInfo_GetI);
    MI_GetProcAddress('MediaInfo_Get',        @MediaInfo_Get);
    MI_GetProcAddress('MediaInfo_Option',     @MediaInfo_Option);
    MI_GetProcAddress('MediaInfo_State_Get',  @MediaInfo_State_Get);
    MI_GetProcAddress('MediaInfo_Count_Get',  @MediaInfo_Count_Get);

    MI_GetProcAddress('MediaInfoA_New',       @MediaInfoA_New);
    MI_GetProcAddress('MediaInfoA_Delete',    @MediaInfoA_Delete);
    MI_GetProcAddress('MediaInfoA_Open',      @MediaInfoA_Open);
    MI_GetProcAddress('MediaInfoA_Close',     @MediaInfoA_Close);
    MI_GetProcAddress('MediaInfoA_Inform',    @MediaInfoA_Inform);
    MI_GetProcAddress('MediaInfoA_GetI',      @MediaInfoA_GetI);
    MI_GetProcAddress('MediaInfoA_Get',       @MediaInfoA_Get);
    MI_GetProcAddress('MediaInfoA_Option',    @MediaInfoA_Option);
    MI_GetProcAddress('MediaInfoA_State_Get', @MediaInfoA_State_Get);
    MI_GetProcAddress('MediaInfoA_Count_Get', @MediaInfoA_Count_Get);

    Result := True;
  end;
end;

{$ENDIF}

end.
