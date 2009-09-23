program Test;

uses
  Forms,
  Main in 'Main.pas' {MainForm},
  APEtag in 'APEtag.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.Title := 'APEtag Test';
  Application.CreateForm(TMainForm, MainForm);
  Application.Run;
end.
