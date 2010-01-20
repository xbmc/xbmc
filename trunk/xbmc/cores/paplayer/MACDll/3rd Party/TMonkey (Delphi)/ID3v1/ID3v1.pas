{ *************************************************************************** }
{                                                                             }
{ Audio Tools Library (Freeware)                                              }
{ Class TID3v1 - for manipulating with ID3v1 tags                             }
{                                                                             }
{ Copyright (c) 2001,2002 by Jurgen Faul                                      }
{ E-mail: jfaul@gmx.de                                                        }
{ http://jfaul.de/atl                                                         }
{                                                                             }
{ Version 1.0 (25 July 2001)                                                  }
{   - Reading & writing support for ID3v1.x tags                              }
{   - Tag info: title, artist, album, track, year, genre, comment             }
{                                                                             }
{ *************************************************************************** }

unit ID3v1;

interface

uses
  Classes, SysUtils;

const
  MAX_MUSIC_GENRES = 148;                       { Max. number of music genres }
  DEFAULT_GENRE = 255;                              { Index for default genre }

  { Used with VersionID property }
  TAG_VERSION_1_0 = 1;                                { Index for ID3v1.0 tag }
  TAG_VERSION_1_1 = 2;                                { Index for ID3v1.1 tag }

var
  MusicGenre: array [0..MAX_MUSIC_GENRES - 1] of string;        { Genre names }

type
  { Used in TID3v1 class }
  String04 = string[4];                          { String with max. 4 symbols }
  String30 = string[30];                        { String with max. 30 symbols }

  { Class TID3v1 }
  TID3v1 = class(TObject)
    private
      { Private declarations }
      FExists: Boolean;
      FVersionID: Byte;
      FTitle: String30;
      FArtist: String30;
      FAlbum: String30;
      FYear: String04;
      FComment: String30;
      FTrack: Byte;
      FGenreID: Byte;
      procedure FSetTitle(const NewTitle: String30);
      procedure FSetArtist(const NewArtist: String30);
      procedure FSetAlbum(const NewAlbum: String30);
      procedure FSetYear(const NewYear: String04);
      procedure FSetComment(const NewComment: String30);
      procedure FSetTrack(const NewTrack: Byte);
      procedure FSetGenreID(const NewGenreID: Byte);
      function FGetGenre: string;
    public
      { Public declarations }
      constructor Create;                                     { Create object }
      procedure ResetData;                                   { Reset all data }
      function ReadFromFile(const FileName: string): Boolean;      { Load tag }
      function RemoveFromFile(const FileName: string): Boolean;  { Delete tag }
      function SaveToFile(const FileName: string): Boolean;        { Save tag }
      property Exists: Boolean read FExists;              { True if tag found }
      property VersionID: Byte read FVersionID;                { Version code }
      property Title: String30 read FTitle write FSetTitle;      { Song title }
      property Artist: String30 read FArtist write FSetArtist;  { Artist name }
      property Album: String30 read FAlbum write FSetAlbum;      { Album name }
      property Year: String04 read FYear write FSetYear;               { Year }
      property Comment: String30 read FComment write FSetComment;   { Comment }
      property Track: Byte read FTrack write FSetTrack;        { Track number }
      property GenreID: Byte read FGenreID write FSetGenreID;    { Genre code }
      property Genre: string read FGetGenre;                     { Genre name }
  end;

implementation

type
  { Real structure of ID3v1 tag }
  TagRecord = record
    Header: array [1..3] of Char;                { Tag header - must be "TAG" }
    Title: array [1..30] of Char;                                { Title data }
    Artist: array [1..30] of Char;                              { Artist data }
    Album: array [1..30] of Char;                                { Album data }
    Year: array [1..4] of Char;                                   { Year data }
    Comment: array [1..30] of Char;                            { Comment data }
    Genre: Byte;                                                 { Genre data }
  end;

{ ********************* Auxiliary functions & procedures ******************** }

function ReadTag(const FileName: string; var TagData: TagRecord): Boolean;
var
  SourceFile: file;
begin
  try
    Result := true;
    { Set read-access and open file }
    AssignFile(SourceFile, FileName);
    FileMode := 0;
    Reset(SourceFile, 1);
    { Read tag }
    Seek(SourceFile, FileSize(SourceFile) - 128);
    BlockRead(SourceFile, TagData, 128);
    CloseFile(SourceFile);
  except
    { Error }
    Result := false;
  end;
end;

{ --------------------------------------------------------------------------- }

function RemoveTag(const FileName: string): Boolean;
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
    Seek(SourceFile, FileSize(SourceFile) - 128);
    Truncate(SourceFile);
    CloseFile(SourceFile);
  except
    { Error }
    Result := false;
  end;
end;

{ --------------------------------------------------------------------------- }

function SaveTag(const FileName: string; TagData: TagRecord): Boolean;
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
    { Write tag }
    Seek(SourceFile, FileSize(SourceFile));
    BlockWrite(SourceFile, TagData, SizeOf(TagData));
    CloseFile(SourceFile);
  except
    { Error }
    Result := false;
  end;
end;

{ --------------------------------------------------------------------------- }

function GetTagVersion(const TagData: TagRecord): Byte;
begin
  Result := TAG_VERSION_1_0;
  { Terms for ID3v1.1 }
  if ((TagData.Comment[29] = #0) and (TagData.Comment[30] <> #0)) or
    ((TagData.Comment[29] = #32) and (TagData.Comment[30] <> #32)) then
    Result := TAG_VERSION_1_1;
end;

{ ********************** Private functions & procedures ********************* }

procedure TID3v1.FSetTitle(const NewTitle: String30);
begin
  FTitle := TrimRight(NewTitle);
end;

{ --------------------------------------------------------------------------- }

procedure TID3v1.FSetArtist(const NewArtist: String30);
begin
  FArtist := TrimRight(NewArtist);
end;

{ --------------------------------------------------------------------------- }

procedure TID3v1.FSetAlbum(const NewAlbum: String30);
begin
  FAlbum := TrimRight(NewAlbum);
end;

{ --------------------------------------------------------------------------- }

procedure TID3v1.FSetYear(const NewYear: String04);
begin
  FYear := TrimRight(NewYear);
end;

{ --------------------------------------------------------------------------- }

procedure TID3v1.FSetComment(const NewComment: String30);
begin
  FComment := TrimRight(NewComment);
end;

{ --------------------------------------------------------------------------- }

procedure TID3v1.FSetTrack(const NewTrack: Byte);
begin
  FTrack := NewTrack;
end;

{ --------------------------------------------------------------------------- }

procedure TID3v1.FSetGenreID(const NewGenreID: Byte);
begin
  FGenreID := NewGenreID;
end;

{ --------------------------------------------------------------------------- }

function TID3v1.FGetGenre: string;
begin
  Result := '';
  { Return an empty string if the current GenreID is not valid }
  if FGenreID in [0..MAX_MUSIC_GENRES - 1] then Result := MusicGenre[FGenreID];
end;

{ ********************** Public functions & procedures ********************** }

constructor TID3v1.Create;
begin
  inherited;
  ResetData;
end;

{ --------------------------------------------------------------------------- }

procedure TID3v1.ResetData;
begin
  FExists := false;
  FVersionID := TAG_VERSION_1_0;
  FTitle := '';
  FArtist := '';
  FAlbum := '';
  FYear := '';
  FComment := '';
  FTrack := 0;
  FGenreID := DEFAULT_GENRE;
end;

{ --------------------------------------------------------------------------- }

function TID3v1.ReadFromFile(const FileName: string): Boolean;
var
  TagData: TagRecord;
begin
  { Reset and load tag data from file to variable }
  ResetData;
  Result := ReadTag(FileName, TagData);
  { Process data if loaded and tag header OK }
  if (Result) and (TagData.Header = 'TAG') then
  begin
    FExists := true;
    FVersionID := GetTagVersion(TagData);
    { Fill properties with tag data }
    FTitle := TrimRight(TagData.Title);
    FArtist := TrimRight(TagData.Artist);
    FAlbum := TrimRight(TagData.Album);
    FYear := TrimRight(TagData.Year);
    if FVersionID = TAG_VERSION_1_0 then
      FComment := TrimRight(TagData.Comment)
    else
    begin
      FComment := TrimRight(Copy(TagData.Comment, 1, 28));
      FTrack := Ord(TagData.Comment[30]);
    end;
    FGenreID := TagData.Genre;
  end;
end;

{ --------------------------------------------------------------------------- }

function TID3v1.RemoveFromFile(const FileName: string): Boolean;
var
  TagData: TagRecord;
begin
  { Find tag }
  Result := ReadTag(FileName, TagData);
  { Delete tag if loaded and tag header OK }
  if (Result) and (TagData.Header = 'TAG') then Result := RemoveTag(FileName);
end;

{ --------------------------------------------------------------------------- }

function TID3v1.SaveToFile(const FileName: string): Boolean;
var
  TagData: TagRecord;
begin
  { Prepare tag record }
  FillChar(TagData, SizeOf(TagData), 0);
  TagData.Header := 'TAG';
  Move(FTitle[1], TagData.Title, Length(FTitle));
  Move(FArtist[1], TagData.Artist, Length(FArtist));
  Move(FAlbum[1], TagData.Album, Length(FAlbum));
  Move(FYear[1], TagData.Year, Length(FYear));
  Move(FComment[1], TagData.Comment, Length(FComment));
  if FTrack > 0 then
  begin
    TagData.Comment[29] := #0;
    TagData.Comment[30] := Chr(FTrack);
  end;
  TagData.Genre := FGenreID;
  { Delete old tag and write new tag }
  Result := (RemoveFromFile(FileName)) and (SaveTag(FileName, TagData));
end;

{ ************************** Initialize music genres ************************ }

initialization
begin
  { Standard genres }
  MusicGenre[0] := 'Blues';
  MusicGenre[1] := 'Classic Rock';
  MusicGenre[2] := 'Country';
  MusicGenre[3] := 'Dance';
  MusicGenre[4] := 'Disco';
  MusicGenre[5] := 'Funk';
  MusicGenre[6] := 'Grunge';
  MusicGenre[7] := 'Hip-Hop';
  MusicGenre[8] := 'Jazz';
  MusicGenre[9] := 'Metal';
  MusicGenre[10] := 'New Age';
  MusicGenre[11] := 'Oldies';
  MusicGenre[12] := 'Other';
  MusicGenre[13] := 'Pop';
  MusicGenre[14] := 'R&B';
  MusicGenre[15] := 'Rap';
  MusicGenre[16] := 'Reggae';
  MusicGenre[17] := 'Rock';
  MusicGenre[18] := 'Techno';
  MusicGenre[19] := 'Industrial';
  MusicGenre[20] := 'Alternative';
  MusicGenre[21] := 'Ska';
  MusicGenre[22] := 'Death Metal';
  MusicGenre[23] := 'Pranks';
  MusicGenre[24] := 'Soundtrack';
  MusicGenre[25] := 'Euro-Techno';
  MusicGenre[26] := 'Ambient';
  MusicGenre[27] := 'Trip-Hop';
  MusicGenre[28] := 'Vocal';
  MusicGenre[29] := 'Jazz+Funk';
  MusicGenre[30] := 'Fusion';
  MusicGenre[31] := 'Trance';
  MusicGenre[32] := 'Classical';
  MusicGenre[33] := 'Instrumental';
  MusicGenre[34] := 'Acid';
  MusicGenre[35] := 'House';
  MusicGenre[36] := 'Game';
  MusicGenre[37] := 'Sound Clip';
  MusicGenre[38] := 'Gospel';
  MusicGenre[39] := 'Noise';
  MusicGenre[40] := 'AlternRock';
  MusicGenre[41] := 'Bass';
  MusicGenre[42] := 'Soul';
  MusicGenre[43] := 'Punk';
  MusicGenre[44] := 'Space';
  MusicGenre[45] := 'Meditative';
  MusicGenre[46] := 'Instrumental Pop';
  MusicGenre[47] := 'Instrumental Rock';
  MusicGenre[48] := 'Ethnic';
  MusicGenre[49] := 'Gothic';
  MusicGenre[50] := 'Darkwave';
  MusicGenre[51] := 'Techno-Industrial';
  MusicGenre[52] := 'Electronic';
  MusicGenre[53] := 'Pop-Folk';
  MusicGenre[54] := 'Eurodance';
  MusicGenre[55] := 'Dream';
  MusicGenre[56] := 'Southern Rock';
  MusicGenre[57] := 'Comedy';
  MusicGenre[58] := 'Cult';
  MusicGenre[59] := 'Gangsta';
  MusicGenre[60] := 'Top 40';
  MusicGenre[61] := 'Christian Rap';
  MusicGenre[62] := 'Pop/Funk';
  MusicGenre[63] := 'Jungle';
  MusicGenre[64] := 'Native American';
  MusicGenre[65] := 'Cabaret';
  MusicGenre[66] := 'New Wave';
  MusicGenre[67] := 'Psychadelic';
  MusicGenre[68] := 'Rave';
  MusicGenre[69] := 'Showtunes';
  MusicGenre[70] := 'Trailer';
  MusicGenre[71] := 'Lo-Fi';
  MusicGenre[72] := 'Tribal';
  MusicGenre[73] := 'Acid Punk';
  MusicGenre[74] := 'Acid Jazz';
  MusicGenre[75] := 'Polka';
  MusicGenre[76] := 'Retro';
  MusicGenre[77] := 'Musical';
  MusicGenre[78] := 'Rock & Roll';
  MusicGenre[79] := 'Hard Rock';
  { Extended genres }
  MusicGenre[80] := 'Folk';
  MusicGenre[81] := 'Folk-Rock';
  MusicGenre[82] := 'National Folk';
  MusicGenre[83] := 'Swing';
  MusicGenre[84] := 'Fast Fusion';
  MusicGenre[85] := 'Bebob';
  MusicGenre[86] := 'Latin';
  MusicGenre[87] := 'Revival';
  MusicGenre[88] := 'Celtic';
  MusicGenre[89] := 'Bluegrass';
  MusicGenre[90] := 'Avantgarde';
  MusicGenre[91] := 'Gothic Rock';
  MusicGenre[92] := 'Progessive Rock';
  MusicGenre[93] := 'Psychedelic Rock';
  MusicGenre[94] := 'Symphonic Rock';
  MusicGenre[95] := 'Slow Rock';
  MusicGenre[96] := 'Big Band';
  MusicGenre[97] := 'Chorus';
  MusicGenre[98] := 'Easy Listening';
  MusicGenre[99] := 'Acoustic';
  MusicGenre[100]:= 'Humour';
  MusicGenre[101]:= 'Speech';
  MusicGenre[102]:= 'Chanson';
  MusicGenre[103]:= 'Opera';
  MusicGenre[104]:= 'Chamber Music';
  MusicGenre[105]:= 'Sonata';
  MusicGenre[106]:= 'Symphony';
  MusicGenre[107]:= 'Booty Bass';
  MusicGenre[108]:= 'Primus';
  MusicGenre[109]:= 'Porn Groove';
  MusicGenre[110]:= 'Satire';
  MusicGenre[111]:= 'Slow Jam';
  MusicGenre[112]:= 'Club';
  MusicGenre[113]:= 'Tango';
  MusicGenre[114]:= 'Samba';
  MusicGenre[115]:= 'Folklore';
  MusicGenre[116]:= 'Ballad';
  MusicGenre[117]:= 'Power Ballad';
  MusicGenre[118]:= 'Rhythmic Soul';
  MusicGenre[119]:= 'Freestyle';
  MusicGenre[120]:= 'Duet';
  MusicGenre[121]:= 'Punk Rock';
  MusicGenre[122]:= 'Drum Solo';
  MusicGenre[123]:= 'A capella';
  MusicGenre[124]:= 'Euro-House';
  MusicGenre[125]:= 'Dance Hall';
  MusicGenre[126]:= 'Goa';
  MusicGenre[127]:= 'Drum & Bass';
  MusicGenre[128]:= 'Club-House';
  MusicGenre[129]:= 'Hardcore';
  MusicGenre[130]:= 'Terror';
  MusicGenre[131]:= 'Indie';
  MusicGenre[132]:= 'BritPop';
  MusicGenre[133]:= 'Negerpunk';
  MusicGenre[134]:= 'Polsk Punk';
  MusicGenre[135]:= 'Beat';
  MusicGenre[136]:= 'Christian Gangsta Rap';
  MusicGenre[137]:= 'Heavy Metal';
  MusicGenre[138]:= 'Black Metal';
  MusicGenre[139]:= 'Crossover';
  MusicGenre[140]:= 'Contemporary Christian';
  MusicGenre[141]:= 'Christian Rock';
  MusicGenre[142]:= 'Merengue';
  MusicGenre[143]:= 'Salsa';
  MusicGenre[144]:= 'Trash Metal';
  MusicGenre[145]:= 'Anime';
  MusicGenre[146]:= 'JPop';
  MusicGenre[147]:= 'Synthpop';
end;

end.
