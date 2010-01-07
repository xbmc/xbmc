//Two versions :
//MediaInfo_* : Unicode
//MediaInfoA_* : Ansi
//If you prefer Ansi version (PChar instead of PWideChar), replace MediaInfo_ by MediaInfoA_

unit HowToUse_Dll_;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, MediaInfoDll;

type
  TForm1 = class(TForm)
    Memo1: TMemo;
    procedure FormCreate(Sender: TObject);
  private
    { Déclarations privées }
  public
    { Déclarations publiques }
  end;

var
  Form1: TForm1;

implementation

{$R *.dfm}

procedure TForm1.FormCreate(Sender: TObject);
var
  Handle: Cardinal;
  To_Display: WideString;
  CR: WideString;
begin
  CR:=Chr(13) + Chr(10);

  if (MediaInfoDLL_Load('MediaInfo.dll')=false) then
  begin
      Memo1.Text := 'Error while loading MediaInfo.dll';
      exit;
  end;


  To_Display := MediaInfo_Option (0, 'Info_Version', '');

  To_Display := To_Display + CR + CR + 'Info_Parameters' + CR;
  To_Display := To_Display + MediaInfo_Option (0, 'Info_Parameters', '');

  To_Display := To_Display + CR + CR + 'Info_Capacities' + CR;
  To_Display := To_Display + MediaInfo_Option (0, 'Info_Capacities', '');

  To_Display := To_Display + CR + CR + 'Info_Codecs' + CR;
  To_Display := To_Display + MediaInfo_Option (0, 'Info_Codecs', '');

  Handle := MediaInfo_New();

  To_Display := To_Display + CR + CR + 'Open' + CR;
  To_Display := To_Display + format('%d', [MediaInfo_Open(Handle, 'Example.ogg')]);

  To_Display := To_Display + CR + CR + 'Inform with Complete=false' + CR;
  MediaInfo_Option (0, 'Complete', '');
  To_Display := To_Display + MediaInfo_Inform(Handle, 0);

  To_Display := To_Display + CR + CR + 'Inform with Complete=true' + CR;
  MediaInfo_Option (0, 'Complete', '1');
  To_Display := To_Display + MediaInfo_Inform(Handle, 0);

  To_Display := To_Display + CR + CR + 'Custom Inform' + CR;
  MediaInfo_Option (0, 'Inform', 'General;Example : FileSize=%FileSize%');
  To_Display := To_Display + MediaInfo_Inform(Handle, 0);

  To_Display := To_Display + CR + CR + 'GetI with Stream=General and Parameter:=17' + CR;
  To_Display := To_Display + MediaInfo_GetI(Handle, Stream_General, 0, 17, Info_Text);

  To_Display := To_Display + CR + CR + 'Count_Get with StreamKind=Stream_Audio' + CR;
  To_Display := To_Display + format('%d', [MediaInfo_Count_Get(Handle, Stream_Audio, -1)]);

  To_Display := To_Display + CR + CR + 'Get with Stream:=General and Parameter=^AudioCount^' + CR;
  To_Display := To_Display + MediaInfo_Get(Handle, Stream_General, 0, 'AudioCount', Info_Text, Info_Name);

  To_Display := To_Display + CR + CR + 'Get with Stream:=Audio and Parameter=^StreamCount^' + CR;
  To_Display := To_Display + MediaInfo_Get(Handle, Stream_Audio, 0, 'StreamCount', Info_Text, Info_Name);

  To_Display := To_Display + CR + CR + 'Get with Stream:=General and Parameter=^FileSize^' + CR;
  To_Display := To_Display + MediaInfo_Get(Handle, Stream_General, 0, 'FileSize', Info_Text, Info_Name);

  To_Display := To_Display + CR + CR + 'Close' + CR;
  MediaInfo_Close(Handle);

  Memo1.Text := To_Display;


end;

end.
