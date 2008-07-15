unit Main;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, FileCtrl, ExtCtrls, ID3v1;

type
  TMainForm = class(TForm)
    DriveList: TDriveComboBox;
    FolderList: TDirectoryListBox;
    FileList: TFileListBox;
    CloseButton: TButton;
    RemoveButton: TButton;
    SaveButton: TButton;
    InfoBevel: TBevel;
    IconImage: TImage;
    TagExistsLabel: TLabel;
    TagVersionLabel: TLabel;
    TitleLabel: TLabel;
    ArtistLabel: TLabel;
    AlbumLabel: TLabel;
    YearLabel: TLabel;
    CommentLabel: TLabel;
    TrackLabel: TLabel;
    GenreLabel: TLabel;
    TitleEdit: TEdit;
    ArtistEdit: TEdit;
    AlbumEdit: TEdit;
    TrackEdit: TEdit;
    YearEdit: TEdit;
    CommentEdit: TEdit;
    GenreComboBox: TComboBox;
    TagExistsValue: TEdit;
    TagVersionValue: TEdit;
    procedure CloseButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FileListChange(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure SaveButtonClick(Sender: TObject);
    procedure RemoveButtonClick(Sender: TObject);
  private
    { Private declarations }
    FileTag: TID3v1;
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
  TagVersionValue.Text := '';
  TitleEdit.Text := '';
  ArtistEdit.Text := '';
  AlbumEdit.Text := '';
  TrackEdit.Text := '';
  YearEdit.Text := '';
  GenreComboBox.ItemIndex := 0;
  CommentEdit.Text := '';
end;

procedure TMainForm.CloseButtonClick(Sender: TObject);
begin
  { Exit }
  Close;
end;

procedure TMainForm.FormCreate(Sender: TObject);
var
  Iterator: Integer;
begin
  { Create object }
  FileTag := TID3v1.Create;
  { Fill and initialize genres }
  GenreComboBox.Items.Add('');
  for Iterator := 0 to MAX_MUSIC_GENRES - 1 do
    GenreComboBox.Items.Add(MusicGenre[Iterator]);
  { Reset }
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
        if FileTag.VersionID = TAG_VERSION_1_0 then
          TagVersionValue.Text := '1.0'
        else
          TagVersionValue.Text := '1.1';
        TitleEdit.Text := FileTag.Title;
        ArtistEdit.Text := FileTag.Artist;
        AlbumEdit.Text := FileTag.Album;
        TrackEdit.Text := IntToStr(FileTag.Track);
        YearEdit.Text := FileTag.Year;
        if FileTag.GenreID < MAX_MUSIC_GENRES then
          GenreComboBox.ItemIndex := FileTag.GenreID + 1;
        CommentEdit.Text := FileTag.Comment;
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
  FileTag.Year := YearEdit.Text;
  Val(TrackEdit.Text, Value, Code);
  if (Code = 0) and (Value > 0) then FileTag.Track := Value
  else FileTag.Track := 0;
  if GenreComboBox.ItemIndex = 0 then FileTag.GenreID := DEFAULT_GENRE
  else FileTag.GenreID := GenreComboBox.ItemIndex - 1;
  FileTag.Comment := CommentEdit.Text;
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

end.
