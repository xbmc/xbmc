{ *************************************************************************** }
{                                                                             }
{ Audio Tools Library (Freeware)                                              }
{ Class TMonkey - for manipulating with Monkey's Audio file information       }
{                                                                             }
{ Uses:                                                                       }
{   - Class TID3v1                                                            }
{   - Class TID3v2                                                            }
{   - Class TAPEtag                                                           }
{                                                                             }
{ Copyright (c) 2001,2002 by Jurgen Faul                                      }
{ E-mail: jfaul@gmx.de                                                        }
{ http://jfaul.de/atl                                                         }
{                                                                             }
{ Version 1.2 (21 April 2002)                                                 }
{   - Class TID3v2: support for ID3v2 tags                                    }
{   - Class TAPEtag: support for APE tags                                     }
{                                                                             }
{ Version 1.1 (11 September 2001)                                             }
{   - Added property Samples                                                  }
{   - Removed WAV header information                                          }
{                                                                             }
{ Version 1.0 (7 September 2001)                                              }
{   - Support for Monkey's Audio files                                        }
{   - Class TID3v1: reading & writing support for ID3v1.x tags                }
{                                                                             }
{ *************************************************************************** }

unit Monkey;

interface

uses
  Classes, SysUtils, ID3v1, ID3v2, APEtag;

const
  { Compression level codes }
  MONKEY_COMPRESSION_FAST = 1000;                               { Fast (poor) }
  MONKEY_COMPRESSION_NORMAL = 2000;                           { Normal (good) }
  MONKEY_COMPRESSION_HIGH = 3000;                          { High (very good) }
  MONKEY_COMPRESSION_EXTRA_HIGH = 4000;                   { Extra high (best) }

  { Compression level names }
  MONKEY_COMPRESSION: array [0..4] of string =
    ('Unknown', 'Fast', 'Normal', 'High', 'Extra High');

  { Format flags }
  MONKEY_FLAG_8_BIT = 1;                                        { Audio 8-bit }
  MONKEY_FLAG_CRC = 2;                            { New CRC32 error detection }
  MONKEY_FLAG_PEAK_LEVEL = 4;                             { Peak level stored }
  MONKEY_FLAG_24_BIT = 8;                                      { Audio 24-bit }
  MONKEY_FLAG_SEEK_ELEMENTS = 16;            { Number of seek elements stored }
  MONKEY_FLAG_WAV_NOT_STORED = 32;                    { WAV header not stored }

  { Channel mode names }
  MONKEY_MODE: array [0..2] of string =
    ('Unknown', 'Mono', 'Stereo');

type
  { Real structure of Monkey's Audio header }
  MonkeyHeader = record
    ID: array [1..4] of Char;                                 { Always "MAC " }
    VersionID: Word;                    { Version number * 1000 (3.91 = 3910) }
    CompressionID: Word;                             { Compression level code }
    Flags: Word;                                           { Any format flags }
    Channels: Word;                                      { Number of channels }
    SampleRate: Integer;                                   { Sample rate (hz) }
    HeaderBytes: Integer;                 { Header length (without header ID) }
    TerminatingBytes: Integer;                                { Extended data }
    Frames: Integer;                           { Number of frames in the file }
    FinalSamples: Integer;             { Number of samples in the final frame }
    PeakLevel: Integer;                              { Peak level (if stored) }
    SeekElements: Integer;              { Number of seek elements (if stored) }
  end;

  { Class TMonkey }
  TMonkey = class(TObject)
    private
      { Private declarations }
      FFileLength: Integer;
      FHeader: MonkeyHeader;
      FID3v1: TID3v1;
      FID3v2: TID3v2;
      FAPEtag: TAPEtag;
      procedure FResetData;
      function FGetValid: Boolean;
      function FGetVersion: string;
      function FGetCompression: string;
      function FGetBits: Byte;
      function FGetChannelMode: string;
      function FGetPeak: Double;
      function FGetSamplesPerFrame: Integer;
      function FGetSamples: Integer;
      function FGetDuration: Double;
      function FGetRatio: Double;
    public
      { Public declarations }
      constructor Create;                                     { Create object }
      destructor Destroy; override;                          { Destroy object }
      function ReadFromFile(const FileName: string): Boolean;   { Load header }
      property FileLength: Integer read FFileLength;    { File length (bytes) }
      property Header: MonkeyHeader read FHeader;     { Monkey's Audio header }
      property ID3v1: TID3v1 read FID3v1;                    { ID3v1 tag data }
      property ID3v2: TID3v2 read FID3v2;                    { ID3v2 tag data }
      property APEtag: TAPEtag read FAPEtag;                   { APE tag data }
      property Valid: Boolean read FGetValid;          { True if header valid }
      property Version: string read FGetVersion;            { Encoder version }
      property Compression: string read FGetCompression;  { Compression level }
      property Bits: Byte read FGetBits;                    { Bits per sample }
      property ChannelMode: string read FGetChannelMode;       { Channel mode }
      property Peak: Double read FGetPeak;             { Peak level ratio (%) }
      property Samples: Integer read FGetSamples;         { Number of samples }
      property Duration: Double read FGetDuration;       { Duration (seconds) }
      property Ratio: Double read FGetRatio;          { Compression ratio (%) }
  end;

implementation

{ ********************** Private functions & procedures ********************* }

procedure TMonkey.FResetData;
begin
  { Reset data }
  FFileLength := 0;
  FillChar(FHeader, SizeOf(FHeader), 0);
  FID3v1.ResetData;
  FID3v2.ResetData;
  FAPEtag.ResetData;
end;

{ --------------------------------------------------------------------------- }

function TMonkey.FGetValid: Boolean;
begin
  { Check for right Monkey's Audio file data }
  Result :=
    (FHeader.ID = 'MAC ') and
    (FHeader.SampleRate > 0) and
    (FHeader.Channels > 0);
end;

{ --------------------------------------------------------------------------- }

function TMonkey.FGetVersion: string;
begin
  { Get encoder version }
  if FHeader.VersionID = 0 then Result := ''
  else Str(FHeader.VersionID / 1000 : 4 : 2, Result);
end;

{ --------------------------------------------------------------------------- }

function TMonkey.FGetCompression: string;
begin
  { Get compression level }
  Result := MONKEY_COMPRESSION[FHeader.CompressionID div 1000];
end;

{ --------------------------------------------------------------------------- }

function TMonkey.FGetBits: Byte;
begin
  { Get number of bits per sample }
  if FGetValid then
  begin
    Result := 16;
    if FHeader.Flags and MONKEY_FLAG_8_BIT > 0 then Result := 8;
    if FHeader.Flags and MONKEY_FLAG_24_BIT > 0 then Result := 24;
  end
  else
    Result := 0;
end;

{ --------------------------------------------------------------------------- }

function TMonkey.FGetChannelMode: string;
begin
  { Get channel mode }
  Result := MONKEY_MODE[FHeader.Channels];
end;

{ --------------------------------------------------------------------------- }

function TMonkey.FGetPeak: Double;
begin
  { Get peak level ratio }
  if (FGetValid) and (FHeader.Flags and MONKEY_FLAG_PEAK_LEVEL > 0) then
    case FGetBits of
      16: Result := FHeader.PeakLevel / 32768 * 100;
      24: Result := FHeader.PeakLevel / 8388608 * 100;
      else Result := FHeader.PeakLevel / 128 * 100;
    end
  else
    Result := 0;
end;

{ --------------------------------------------------------------------------- }

function TMonkey.FGetSamplesPerFrame: Integer;
begin
  { Get number of samples in a frame }
  if FGetValid then
    if (FHeader.VersionID >= 3900) or
      ((FHeader.VersionID >= 3800) and
      (FHeader.CompressionID = MONKEY_COMPRESSION_EXTRA_HIGH)) then
      Result := 73728
    else
      Result := 9216
  else
    Result := 0;
end;

{ --------------------------------------------------------------------------- }

function TMonkey.FGetSamples: Integer;
begin
  { Get number of samples }
  if FGetValid then
    Result := (FHeader.Frames - 1) * FGetSamplesPerFrame + FHeader.FinalSamples
  else
    Result := 0;
end;

{ --------------------------------------------------------------------------- }

function TMonkey.FGetDuration: Double;
begin
  { Get song duration }
  if FGetValid then Result := FGetSamples / FHeader.SampleRate
  else Result := 0;
end;

{ --------------------------------------------------------------------------- }

function TMonkey.FGetRatio: Double;
begin
  { Get compression ratio }
  if FGetValid then
    Result := FFileLength /
      (FGetSamples * FHeader.Channels * FGetBits / 8 + 44) * 100
  else
    Result := 0;
end;

{ ********************** Public functions & procedures ********************** }

constructor TMonkey.Create;
begin
  { Create object }
  inherited;
  FID3v1 := TID3v1.Create;
  FID3v2 := TID3v2.Create;
  FAPEtag := TAPEtag.Create;
  FResetData;
end;

{ --------------------------------------------------------------------------- }

destructor TMonkey.Destroy;
begin
  { Destroy object }
  FID3v1.Free;
  FID3v2.Free;
  FAPEtag.Free;
  inherited;
end;

{ --------------------------------------------------------------------------- }

function TMonkey.ReadFromFile(const FileName: string): Boolean;
var
  SourceFile: file;
begin
  try
    { Reset data and search for file tag }
    FResetData;
    if (not FID3v1.ReadFromFile(FileName)) or
      (not FID3v2.ReadFromFile(FileName)) or
      (not FAPEtag.ReadFromFile(FileName)) then raise Exception.Create('');
    { Set read-access, open file and get file length }
    AssignFile(SourceFile, FileName);
    FileMode := 0;
    Reset(SourceFile, 1);
    FFileLength := FileSize(SourceFile);
    { Read Monkey's Audio header data }
    Seek(SourceFile, ID3v2.Size);
    BlockRead(SourceFile, FHeader, SizeOf(FHeader));
    if FHeader.Flags and MONKEY_FLAG_PEAK_LEVEL = 0 then
      FHeader.PeakLevel := 0;
    if FHeader.Flags and MONKEY_FLAG_SEEK_ELEMENTS = 0 then
      FHeader.SeekElements := 0;
    CloseFile(SourceFile);
    Result := true;
  except
    FResetData;
    Result := false;
  end;
end;

end.
