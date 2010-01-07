program HowToUse_Dll;

uses
  Forms,
  MediaInfoDLL in '..\..\..\Source\MediaInfoDLL\MediaInfoDLL.pas',
  HowToUse_Dll_ in 'HowToUse_Dll_.pas' {Form1};

{$R HowToUse_Dll.res}

begin
  Application.Initialize;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
