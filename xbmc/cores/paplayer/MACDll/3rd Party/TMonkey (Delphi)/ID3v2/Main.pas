unit Main;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, FileCtrl, ExtCtrls, ID3v2;

type
  TMainForm = class(TForm)
    DriveList: TDriveComboBox;
    FolderList: TDirectoryListBox;
    FileList: TFileListBox;
    SaveButton: TButton;
    RemoveButton: TButton;
    CloseButton: TButton;
    InfoBevel: TBevel;
    IconImage: TImage;
    TagExistsLabel: TLabel;
    TagExistsValue: TEdit;
    VersionLabel: TLabel;
    VersionValue: TEdit;
    SizeLabel: TLabel;
    SizeValue: TEdit;
    TitleLabel: TLabel;
    TitleEdit: TEdit;
    ArtistLabel: TLabel;
    ArtistEdit: TEdit;
    AlbumLabel: TLabel;
    AlbumEdit: TEdit;
    TrackLabel: TLabel;
    TrackEdit: TEdit;
    YearLabel: TLabel;
    YearEdit: TEdit;
    GenreLabel: TLabel;
    GenreEdit: TEdit;
    CommentLabel: TLabel;
    CommentEdit: TEdit;
    ComposerLabel: TLabel;
    ComposerEdit: TEdit;
    EncoderLabel: TLabel;
    EncoderEdit: TEdit;
    CopyrightLabel: TLabel;
    CopyrightEdit: TEdit;
    LanguageLabel: TLabel;
    LanguageEdit: TEdit;
    LinkLabel: TLabel;
    LinkEdit: TEdit;
    procedure FormCreate(Sender: TObject);
    procedure FileListChange(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure SaveButtonClick(Sender: TObject);
    procedure RemoveButtonClick(Sender: TObject);
    procedure CloseButtonClick(Sender: TObject);
  private
    { Private declarations }
    FileTag: TID3v2;
    procedure ClearAll;
  end;

var
  MainForm: TMainForm;

implementation

{$R *.dfm}

procedure TMainForm.ClearAll;
begin
  { Clear all captions }
  TagExistsValue.Text := '';
  VersionValue.Text := '';
  SizeValue.Text := '';
  TitleEdit.Text := '';
  ArtistEdit.Text := '';
  AlbumEdit.Text := '';
  TrackEdit.Text := '';
  YearEdit.Text := '';
  GenreEdit.Text := '';
  CommentEdit.Text := '';
  ComposerEdit.Text := '';
  EncoderEdit.Text := '';
  CopyrightEdit.Text := '';
  LanguageEdit.Text := '';
  LinkEdit.Text := '';
end;

procedure TMainForm.FormCreate(Sender: TObject);
begin
  { Create object and clear captions }
  FileTag := TID3v2.Create;
  ClearAll;
end;

procedure TMainForm.FileListChange(Sender: TObject);
begin
  { Clear captions }
  ClearAll;
  if FileList.FileName = '' then exit;
  if FileExists(FileList.FileName) then
    { Load tag data }
    if FileTag.ReadFromFile(FileList.FileName) then
      if FileTag.Exists then
      begin
        { Fill captions }
        TagExistsValue.Text := 'Yes';
        VersionValue.Text := '2.' + IntToStr(FileTag.VersionID);
        SizeValue.Text := IntToStr(FileTag.Size) + ' bytes';
        TitleEdit.Text := FileTag.Title;
        ArtistEdit.Text := FileTag.Artist;
        AlbumEdit.Text := FileTag.Album;
        if FileTag.Track > 0 then TrackEdit.Text := IntToStr(FileTag.Track);
        YearEdit.Text := FileTag.Year;
        GenreEdit.Text := FileTag.Genre;
        CommentEdit.Text := FileTag.Comment;
        ComposerEdit.Text := FileTag.Composer;
        EncoderEdit.Text := FileTag.Encoder;
        CopyrightEdit.Text := FileTag.Copyright;
        LanguageEdit.Text := FileTag.Language;
        LinkEdit.Text := FileTag.Link;
      end
      else
        { Tag not found }
        TagExistsValue.Text := 'No'
    else
      { Read error }
      ShowMessage('Can not read tag from the file: ' + FileList.FileName)
  else
    { File does not exist }
    ShowMessage('The file does not exist: ' + FileList.FileName);
end;

procedure TMainForm.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  { Free memory }
  FileTag.Free;
end;

procedure TMainForm.SaveButtonClick(Sender: TObject);
var
  Value, Code: Integer;
begin
  { Prepare tag data }
  FileTag.Title := TitleEdit.Text;
  FileTag.Artist := ArtistEdit.Text;
  FileTag.Album := AlbumEdit.Text;
  Val(TrackEdit.Text, Value, Code);
  if (Code = 0) and (Value > 0) then FileTag.Track := Value
  else FileTag.Track := 0;
  FileTag.Year := YearEdit.Text;
  FileTag.Genre := GenreEdit.Text;
  FileTag.Comment := CommentEdit.Text;
  FileTag.Composer := ComposerEdit.Text;
  FileTag.Encoder := EncoderEdit.Text;
  FileTag.Copyright := CopyrightEdit.Text;
  FileTag.Language := LanguageEdit.Text;
  FileTag.Link := LinkEdit.Text;
  { Save tag data }
  if (not FileExists(FileList.FileName)) or
    (not FileTag.SaveToFile(FileList.FileName)) then
    ShowMessage('Can not save tag to the file: ' + FileList.FileName);
  FileListChange(Self);
end;

procedure TMainForm.RemoveButtonClick(Sender: TObject);
begin
  { Delete tag data }
  if (FileExists(FileList.FileName)) and
    (FileTag.RemoveFromFile(FileList.FileName)) then ClearAll
  else ShowMessage('Can not remove tag from the file: ' + FileList.FileName);
end;

procedure TMainForm.CloseButtonClick(Sender: TObject);
begin
  { Exit }
  Close;
end;

end.
