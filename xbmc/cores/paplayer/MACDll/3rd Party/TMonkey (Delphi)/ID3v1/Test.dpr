program Test;

uses
  Forms,
  Main in 'Main.pas' {MainForm},
  ID3v1 in 'ID3v1.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.Title := 'ID3v1 Test';
  Application.CreateForm(TMainForm, MainForm);
  Application.Run;
end.
