{ *************************************************************************** }
{                                                                             }
{ Audio Tools Library (Freeware)                                              }
{ Class TAPEtag - for manipulating with APE tags                              }
{                                                                             }
{ Copyright (c) 2001,2002 by Jurgen Faul                                      }
{ E-mail: jfaul@gmx.de                                                        }
{ http://jfaul.de/atl                                                         }
{                                                                             }
{ Version 1.0 (21 April 2002)                                                 }
{   - Reading & writing support for APE 1.0 tags                              }
{   - Reading support for APE 2.0 tags (UTF-8 decoding)                       }
{   - Tag info: title, artist, album, track, year, genre, comment, copyright  }
{                                                                             }
{ *************************************************************************** }

unit APEtag;

interface

uses
  Classes, SysUtils;

type
  { Class TAPEtag }
  TAPEtag = class(TObject)
    private
      { Private declarations }
      FExists: Boolean;
      FVersion: Integer;
      FSize: Integer;
      FTitle: string;
      FArtist: string;
      FAlbum: string;
      FTrack: Byte;
      FYear: string;
      FGenre: string;
      FComment: string;
      FCopyright: string;
      procedure FSetTitle(const NewTitle: string);
      procedure FSetArtist(const NewArtist: string);
      procedure FSetAlbum(const NewAlbum: string);
      procedure FSetTrack(const NewTrack: Byte);
      procedure FSetYear(const NewYear: string);
      procedure FSetGenre(const NewGenre: string);
      procedure FSetComment(const NewComment: string);
      procedure FSetCopyright(const NewCopyright: string);
    public
      { Public declarations }
      constructor Create;                                     { Create object }
      procedure ResetData;                                   { Reset all data }
      function ReadFromFile(const FileName: string): Boolean;      { Load tag }
      function RemoveFromFile(const FileName: string): Boolean;  { Delete tag }
      function SaveToFile(const FileName: string): Boolean;        { Save tag }
      property Exists: Boolean read FExists;              { True if tag found }
      property Version: Integer read FVersion;                  { Tag version }
      property Size: Integer read FSize;                     { Total tag size }
      property Title: string read FTitle write FSetTitle;        { Song title }
      property Artist: string read FArtist write FSetArtist;    { Artist name }
      property Album: string read FAlbum write FSetAlbum;       { Album title }
      property Track: Byte read FTrack write FSetTrack;        { Track number }
      property Year: string read FYear write FSetYear;         { Release year }
      property Genre: string read FGenre write FSetGenre;        { Genre name }
      property Comment: string read FComment write FSetComment;     { Comment }
      property Copyright: string read FCopyright write FSetCopyright;   { (c) }
  end;

implementation

const
  { Tag ID }
  ID3V1_ID = 'TAG';                                                   { ID3v1 }
  APE_ID = 'APETAGEX';                                                  { APE }

  { Size constants }
  ID3V1_TAG_SIZE = 128;                                           { ID3v1 tag }
  APE_TAG_FOOTER_SIZE = 32;                                  { APE tag footer }
  APE_TAG_HEADER_SIZE = 32;                                  { APE tag header }

  { First version of APE tag }
  APE_VERSION_1_0 = 1000;

  { Max. number of supported tag fields }
  APE_FIELD_COUNT = 8;

  { Names of supported tag fields }
  APE_FIELD: array [1..APE_FIELD_COUNT] of string =
    ('Title', 'Artist', 'Album', 'Track', 'Year', 'Genre',
     'Comment', 'Copyright');

type
  { APE tag data - for internal use }
  TagInfo = record
    { Real structure of APE footer }
    ID: array [1..8] of Char;                             { Always "APETAGEX" }
    Version: Integer;                                           { Tag version }
    Size: Integer;                                { Tag size including footer }
    Fields: Integer;                                       { Number of fields }
    Flags: Integer;                                               { Tag flags }
    Reserved: array [1..8] of Char;                  { Reserved for later use }
    { Extended data }
    DataShift: Byte;                                { Used if ID3v1 tag found }
    FileSize: Integer;                                    { File size (bytes) }
    Field: array [1..APE_FIELD_COUNT] of string;    { Information from fields }
  end;

{ ********************* Auxiliary functions & procedures ******************** }

function ReadFooter(const FileName: string; var Tag: TagInfo): Boolean;
var
  SourceFile: file;
  TagID: array [1..3] of Char;
  Transferred: Integer;
begin
  { Load footer from file to variable }
  try
    Result := true;
    { Set read-access and open file }
    AssignFile(SourceFile, FileName);
    FileMode := 0;
    Reset(SourceFile, 1);
    Tag.FileSize := FileSize(SourceFile);
    { Check for existing ID3v1 tag }
    Seek(SourceFile, Tag.FileSize - ID3V1_TAG_SIZE);
    BlockRead(SourceFile, TagID, SizeOf(TagID));
    if TagID = ID3V1_ID then Tag.DataShift := ID3V1_TAG_SIZE;
    { Read footer data }
    Seek(SourceFile, Tag.FileSize - Tag.DataShift - APE_TAG_FOOTER_SIZE);
    BlockRead(SourceFile, Tag, APE_TAG_FOOTER_SIZE, Transferred);
    CloseFile(SourceFile);
    { if transfer is not complete }
    if Transferred < APE_TAG_FOOTER_SIZE then Result := false;
  except
    { Error }
    Result := false;
  end;
end;

{ --------------------------------------------------------------------------- }

function ConvertFromUTF8(const Source: string): string;
var
  Iterator, SourceLength, FChar, NChar: Integer;
begin
  { Convert UTF-8 string to ANSI string }
  Result := '';
  Iterator := 0;
  SourceLength := Length(Source);
  while Iterator < SourceLength do
  begin
    Inc(Iterator);
    FChar := Ord(Source[Iterator]);
    if FChar >= $80 then
    begin
      Inc(Iterator);
      if Iterator > SourceLength then break;
      FChar := FChar and $3F;
      if (FChar and $20) <> 0 then
      begin
        FChar := FChar and $1F;
        NChar := Ord(Source[Iterator]);
        if (NChar and $C0) <> $80 then  break;
        FChar := (FChar shl 6) or (NChar and $3F);
        Inc(Iterator);
        if Iterator > SourceLength then break;
      end;
      NChar := Ord(Source[Iterator]);
      if (NChar and $C0) <> $80 then break;
      Result := Result + WideChar((FChar shl 6) or (NChar and $3F));
    end
    else
      Result := Result + WideChar(FChar);
  end;
end;

{ --------------------------------------------------------------------------- }

procedure SetTagItem(const FieldName, FieldValue: string; var Tag: TagInfo);
var
  Iterator: Byte;
begin
  { Set tag item if supported field found }
  for Iterator := 1 to APE_FIELD_COUNT do
    if UpperCase(FieldName) = UpperCase(APE_FIELD[Iterator]) then
      if Tag.Version > APE_VERSION_1_0 then
        Tag.Field[Iterator] := ConvertFromUTF8(FieldValue)
      else
        Tag.Field[Iterator] := FieldValue;
end;

{ --------------------------------------------------------------------------- }

procedure ReadFields(const FileName: string; var Tag: TagInfo);
var
  SourceFile: file;
  FieldName: string;
  FieldValue: array [1..250] of Char;
  NextChar: Char;
  Iterator, ValueSize, ValuePosition, FieldFlags: Integer;
begin
  try
    { Set read-access, open file }
    AssignFile(SourceFile, FileName);
    FileMode := 0;
    Reset(SourceFile, 1);
    Seek(SourceFile, Tag.FileSize - Tag.DataShift - Tag.Size);
    { Read all stored fields }
    for Iterator := 1 to Tag.Fields do
    begin
      FillChar(FieldValue, SizeOf(FieldValue), 0);
      BlockRead(SourceFile, ValueSize, SizeOf(ValueSize));
      BlockRead(SourceFile, FieldFlags, SizeOf(FieldFlags));
      FieldName := '';
      repeat
        BlockRead(SourceFile, NextChar, SizeOf(NextChar));
        FieldName := FieldName + NextChar;
      until Ord(NextChar) = 0;
      ValuePosition := FilePos(SourceFile);
      BlockRead(SourceFile, FieldValue, ValueSize mod SizeOf(FieldValue));
      SetTagItem(Trim(FieldName), Trim(FieldValue), Tag);
      Seek(SourceFile, ValuePosition + ValueSize);
    end;
    CloseFile(SourceFile);
  except
  end;
end;

{ --------------------------------------------------------------------------- }

function GetTrack(const TrackString: string): Byte;
var
  Index, Value, Code: Integer;
begin
  { Get track from string }
  Index := Pos('/', TrackString);
  if Index = 0 then Val(TrackString, Value, Code)
  else Val(Copy(TrackString, 1, Index - 1), Value, Code);
  if Code = 0 then Result := Value
  else Result := 0;
end;

{ --------------------------------------------------------------------------- }

function TruncateFile(const FileName: string; TagSize: Integer): Boolean;
var
  SourceFile: file;
begin
  try
    Result := true;
    { Allow write-access and open file }
    FileSetAttr(FileName, 0);
    AssignFile(SourceFile, FileName);
    FileMode := 2;
    Reset(SourceFile, 1);
    { Delete tag }
    Seek(SourceFile, FileSize(SourceFile) - TagSize);
    Truncate(SourceFile);
    CloseFile(SourceFile);
  except
    { Error }
    Result := false;
  end;
end;

{ --------------------------------------------------------------------------- }

procedure BuildFooter(var Tag: TagInfo);
var
  Iterator: Integer;
begin
  { Build tag footer }
  Tag.ID := APE_ID;
  Tag.Version := APE_VERSION_1_0;
  Tag.Size := APE_TAG_FOOTER_SIZE;
  for Iterator := 1 to APE_FIELD_COUNT do
    if Tag.Field[Iterator] <> '' then
    begin
      Inc(Tag.Size, Length(APE_FIELD[Iterator] + Tag.Field[Iterator]) + 10);
      Inc(Tag.Fields);
    end;
end;

{ --------------------------------------------------------------------------- }

function AddToFile(const FileName: string; TagData: TStream): Boolean;
var
  FileData: TFileStream;
begin
  try
    { Add tag data to file }
    FileData := TFileStream.Create(FileName, fmOpenWrite or fmShareExclusive);
    FileData.Seek(0, soFromEnd);
    TagData.Seek(0, soFromBeginning);
    FileData.CopyFrom(TagData, TagData.Size);
    FileData.Free;
    Result := true;
  except
    { Error }
    Result := false;
  end;
end;

{ --------------------------------------------------------------------------- }

function SaveTag(const FileName: string; Tag: TagInfo): Boolean;
var
  TagData: TStringStream;
  Iterator, ValueSize, Flags: Integer;
begin
  { Build and write tag fields and footer to stream }
  TagData := TStringStream.Create('');
  for Iterator := 1 to APE_FIELD_COUNT do
    if Tag.Field[Iterator] <> '' then
    begin
      ValueSize := Length(Tag.Field[Iterator]) + 1;
      Flags := 0;
      TagData.Write(ValueSize, SizeOf(ValueSize));
      TagData.Write(Flags, SizeOf(Flags));
      TagData.WriteString(APE_FIELD[Iterator] + #0);
      TagData.WriteString(Tag.Field[Iterator] + #0);
    end;
  BuildFooter(Tag);
  TagData.Write(Tag, APE_TAG_FOOTER_SIZE);
  { Add created tag to file }
  Result := AddToFile(FileName, TagData);
  TagData.Free;
end;

{ ********************** Private functions & procedures ********************* }

procedure TAPEtag.FSetTitle(const NewTitle: string);
begin
  { Set song title }
  FTitle := Trim(NewTitle);
end;

{ --------------------------------------------------------------------------- }

procedure TAPEtag.FSetArtist(const NewArtist: string);
begin
  { Set artist name }
  FArtist := Trim(NewArtist);
end;

{ --------------------------------------------------------------------------- }

procedure TAPEtag.FSetAlbum(const NewAlbum: string);
begin
  { Set album title }
  FAlbum := Trim(NewAlbum);
end;

{ --------------------------------------------------------------------------- }

procedure TAPEtag.FSetTrack(const NewTrack: Byte);
begin
  { Set track number }
  FTrack := NewTrack;
end;

{ --------------------------------------------------------------------------- }

procedure TAPEtag.FSetYear(const NewYear: string);
begin
  { Set release year }
  FYear := Trim(NewYear);
end;

{ --------------------------------------------------------------------------- }

procedure TAPEtag.FSetGenre(const NewGenre: string);
begin
  { Set genre name }
  FGenre := Trim(NewGenre);
end;

{ --------------------------------------------------------------------------- }

procedure TAPEtag.FSetComment(const NewComment: string);
begin
  { Set comment }
  FComment := Trim(NewComment);
end;

{ --------------------------------------------------------------------------- }

procedure TAPEtag.FSetCopyright(const NewCopyright: string);
begin
  { Set copyright information }
  FCopyright := Trim(NewCopyright);
end;

{ ********************** Public functions & procedures ********************** }

constructor TAPEtag.Create;
begin
  { Create object }
  inherited;
  ResetData;
end;

{ --------------------------------------------------------------------------- }

procedure TAPEtag.ResetData;
begin
  { Reset all variables }
  FExists := false;
  FVersion := 0;
  FSize := 0;
  FTitle := '';
  FArtist := '';
  FAlbum := '';
  FTrack := 0;
  FYear := '';
  FGenre := '';
  FComment := '';
  FCopyright := '';
end;

{ --------------------------------------------------------------------------- }

function TAPEtag.ReadFromFile(const FileName: string): Boolean;
var
  Tag: TagInfo;
begin
  { Reset data and load footer from file to variable }
  ResetData;
  FillChar(Tag, SizeOf(Tag), 0);
  Result := ReadFooter(FileName, Tag);
  { Process data if loaded and footer valid }
  if (Result) and (Tag.ID = APE_ID) then
  begin
    FExists := true;
    { Fill properties with footer data }
    FVersion := Tag.Version;
    FSize := Tag.Size;
    { Get information from fields }
    ReadFields(FileName, Tag);
    FTitle := Tag.Field[1];
    FArtist := Tag.Field[2];
    FAlbum := Tag.Field[3];
    FTrack := GetTrack(Tag.Field[4]);
    FYear := Tag.Field[5];
    FGenre := Tag.Field[6];
    FComment := Tag.Field[7];
    FCopyright := Tag.Field[8];
  end;
end;

{ --------------------------------------------------------------------------- }

function TAPEtag.RemoveFromFile(const FileName: string): Boolean;
var
  Tag: TagInfo;
begin
  { Remove tag from file if found }
  FillChar(Tag, SizeOf(Tag), 0);
  if ReadFooter(FileName, Tag) then
  begin
    if Tag.ID <> APE_ID then Tag.Size := 0;
    if (Tag.Flags shr 31) > 0 then Inc(Tag.Size, APE_TAG_HEADER_SIZE);
    Result := TruncateFile(FileName, Tag.DataShift + Tag.Size)
  end
  else
    Result := false;
end;

{ --------------------------------------------------------------------------- }

function TAPEtag.SaveToFile(const FileName: string): Boolean;
var
  Tag: TagInfo;
begin
  { Prepare tag data and save to file }
  FillChar(Tag, SizeOf(Tag), 0);
  Tag.Field[1] := FTitle;
  Tag.Field[2] := FArtist;
  Tag.Field[3] := FAlbum;
  if FTrack > 0 then Tag.Field[4] := IntToStr(FTrack);
  Tag.Field[5] := FYear;
  Tag.Field[6] := FGenre;
  Tag.Field[7] := FComment;
  Tag.Field[8] := FCopyright;
  { Delete old tag if exists and write new tag }
  Result := (RemoveFromFile(FileName)) and (SaveTag(FileName, Tag));
end;

end.
