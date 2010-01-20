unit Main;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, FileCtrl, ExtCtrls, Monkey;

type
  TMainForm = class(TForm)
    DriveList: TDriveComboBox;
    FolderList: TDirectoryListBox;
    FileList: TFileListBox;
    CloseButton: TButton;
    InfoBevel: TBevel;
    IconImage: TImage;
    ValidHeaderLabel: TLabel;
    FileLengthLabel: TLabel;
    ValidHeaderValue: TEdit;
    ChannelModeValue: TEdit;
    ChannelModeLabel: TLabel;
    SampleRateLabel: TLabel;
    BitsPerSampleLabel: TLabel;
    DurationLabel: TLabel;
    SampleRateValue: TEdit;
    BitsPerSampleValue: TEdit;
    DurationValue: TEdit;
    FileLengthValue: TEdit;
    CompressionLabel: TLabel;
    CompressionValue: TEdit;
    PeakLevelLabel: TLabel;
    PeakLevelValue: TEdit;
    FramesLabel: TLabel;
    FramesValue: TEdit;
    FlagsLabel: TLabel;
    FlagsValue: TEdit;
    SeekElementsLabel: TLabel;
    SeekElementsValue: TEdit;
    TotalSamplesLabel: TLabel;
    TotalSamplesValue: TEdit;
    procedure CloseButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FileListChange(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
  private
    { Private declarations }
    Monkey: TMonkey;
    procedure ClearAll;
  end;

var
  MainForm: TMainForm;

implementation

{$R *.dfm}

procedure TMainForm.ClearAll;
begin
  { Clear all captions }
  ValidHeaderValue.Text := '';
  FileLengthValue.Text := '';
  ChannelModeValue.Text := '';
  SampleRateValue.Text := '';
  BitsPerSampleValue.Text := '';
  DurationValue.Text := '';
  FlagsValue.Text := '';
  FramesValue.Text := '';
  TotalSamplesValue.Text := '';
  PeakLevelValue.Text := '';
  SeekElementsValue.Text := '';
  CompressionValue.Text := '';
end;

procedure TMainForm.CloseButtonClick(Sender: TObject);
begin
  { Exit }
  Close;
end;

procedure TMainForm.FormCreate(Sender: TObject);
begin
  { Create object and reset captions }
  Monkey := TMonkey.Create;
  ClearAll;
end;

procedure TMainForm.FileListChange(Sender: TObject);
begin
  { Clear captions }
  ClearAll;
  if FileList.FileName = '' then exit;
  if FileExists(FileList.FileName) then
    { Load Monkey's Audio data }
    if Monkey.ReadFromFile(FileList.FileName) then
      if Monkey.Valid then
      begin
        { Fill captions }
        ValidHeaderValue.Text := 'Yes, version ' + Monkey.Version;
        FileLengthValue.Text := IntToStr(Monkey.FileLength) + ' bytes';
        ChannelModeValue.Text := Monkey.ChannelMode;
        SampleRateValue.Text := IntToStr(Monkey.Header.SampleRate) + ' hz';
        BitsPerSampleValue.Text := IntToStr(Monkey.Bits) + ' bit';
        DurationValue.Text := FormatFloat('.000', Monkey.Duration) + ' sec.';
        FlagsValue.Text := IntToStr(Monkey.Header.Flags);
        FramesValue.Text := IntToStr(Monkey.Header.Frames);
        TotalSamplesValue.Text := IntToStr(Monkey.Samples);
        PeakLevelValue.Text := FormatFloat('00.00', Monkey.Peak) + '% - ' +
          IntToStr(Monkey.Header.PeakLevel);
        SeekElementsValue.Text := IntToStr(Monkey.Header.SeekElements);
        CompressionValue.Text := FormatFloat('00.00', Monkey.Ratio) + '% - ' +
          Monkey.Compression;
      end
      else
        { Header not found }
        ValidHeaderValue.Text := 'No'
    else
      { Read error }
      ShowMessage('Can not read header in the file: ' + FileList.FileName)
  else
    { File does not exist }
    ShowMessage('The file does not exist: ' + FileList.FileName);
end;

procedure TMainForm.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  { Free memory }
  Monkey.Free;
end;

end.
