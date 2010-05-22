unit MP3export;

interface

Uses SysUtils, WinTypes, WinProcs, Messages, Classes, Graphics, Controls,
Forms, Dialogs, StdCtrls;

type
//type definitions
//typedef		unsigned long			HBE_STREAM;
//typedef		HBE_STREAM				*PHBE_STREAM;
//typedef		unsigned long			BE_ERR;
  THBE_STREAM = LongWord;
  PHBE_STREAM = ^PHBE_STREAM;
  BE_ERR = LongWord;

const
// encoding formats
//#define		BE_CONFIG_MP3			0
//#define		BE_CONFIG_LAME			256
  BE_CONFIG_MP3	 = 0;
  BE_CONFIG_LAME = 256;


// error codes
//#define    BE_ERR_SUCCESSFUL		        0x00000000
//#define    BE_ERR_INVALID_FORMAT		0x00000001
//#define    BE_ERR_INVALID_FORMAT_PARAMETERS	0x00000002
//#define    BE_ERR_NO_MORE_HANDLES		0x00000003
//#define    BE_ERR_INVALID_HANDLE		0x00000004
BE_ERR_SUCCESSFUL: LongWord = 0;
BE_ERR_INVALID_FORMAT: LongWord = 1;
BE_ERR_INVALID_FORMAT_PARAMETERS: LongWord = 2;
BE_ERR_NO_MORE_HANDLES: LongWord = 3;
BE_ERR_INVALID_HANDLE: LongWord = 4;

// other constants

BE_MAX_HOMEPAGE	= 256;

// format specific variables

BE_MP3_MODE_STEREO = 0;
BE_MP3_MODE_DUALCHANNEL = 2;
BE_MP3_MODE_MONO = 3;

type

  TMP3 = packed record
           dwSampleRate     : LongWord;
           byMode           : Byte;
           wBitRate         : Word;
           bPrivate         : LongWord;
           bCRC             : LongWord;
           bCopyright       : LongWord;
           bOriginal        : LongWord;
           end;

  TLHV1 = packed record
          // STRUCTURE INFORMATION
            dwStructVersion: DWORD;
            dwStructSize: DWORD;

          // BASIC ENCODER SETTINGS
            dwSampleRate: DWORD;	// ALLOWED SAMPLERATE VALUES DEPENDS ON dwMPEGVersion
            dwReSampleRate: DWORD;	// DOWNSAMPLERATE, 0=ENCODER DECIDES
            nMode: Integer;	  	// BE_MP3_MODE_STEREO, BE_MP3_MODE_DUALCHANNEL, BE_MP3_MODE_MONO
            dwBitrate: DWORD;		// CBR bitrate, VBR min bitrate
            dwMaxBitrate: DWORD;	// CBR ignored, VBR Max bitrate
            nQuality: Integer;   	// Quality setting (NORMAL,HIGH,LOW,VOICE)
            dwMpegVersion: DWORD;	// MPEG-1 OR MPEG-2
            dwPsyModel: DWORD;		// FUTURE USE, SET TO 0
            dwEmphasis: DWORD;		// FUTURE USE, SET TO 0

          // BIT STREAM SETTINGS
            bPrivate: LONGBOOL;		// Set Private Bit (TRUE/FALSE)
            bCRC: LONGBOOL;		// Insert CRC (TRUE/FALSE)
            bCopyright: LONGBOOL;	// Set Copyright Bit (TRUE/FALSE)
            bOriginal: LONGBOOL;	// Set Original Bit (TRUE/FALSE_

          // VBR STUFF
            bWriteVBRHeader: LONGBOOL;	// WRITE XING VBR HEADER (TRUE/FALSE)
            bEnableVBR: LONGBOOL;       // USE VBR ENCODING (TRUE/FALSE)
            nVBRQuality: Integer;	// VBR QUALITY 0..9

            btReserved: array[0..255] of Byte;	// FUTURE USE, SET TO 0
            end;

  TAAC = packed record
           dwSampleRate     : LongWord;
           byMode           : Byte;
           wBitRate         : Word;
           byEncodingMethod : Byte;
           end;

  TFormat = packed record
              case byte of
                1 : (mp3           : TMP3);
                2 : (lhv1          : TLHV1);
                3 : (aac           : TAAC);
              end;

  TBE_Config = packed record
                 dwConfig   : LongWord;
                 format     : TFormat;
                 end;


  PBE_Config = ^TBE_Config;

//typedef struct	{
//	// BladeEnc DLL Version number
//
//	BYTE	byDLLMajorVersion;
//	BYTE	byDLLMinorVersion;
//
//	// BladeEnc Engine Version Number
//
//	BYTE	byMajorVersion;
//	BYTE	byMinorVersion;
//
//	// DLL Release date
//
//	BYTE	byDay;
//	BYTE	byMonth;
//	WORD	wYear;
//
//	// BladeEnc	Homepage URL
//
//	CHAR	zHomepage[BE_MAX_HOMEPAGE + 1];
//
//} BE_VERSION, *PBE_VERSION;

  TBE_Version = record
                  byDLLMajorVersion : Byte;
                  byDLLMinorVersion : Byte;

                  byMajorVersion    : Byte;
                  byMinorVersion    : Byte;

                  byDay             : Byte;
                  byMonth           : Byte;
                  wYear             : Word;

                  zHomePage         : Array[0..BE_MAX_HOMEPAGE + 1] of Char;
                  end;

  PBE_Version = ^TBE_Version;

//__declspec(dllexport) BE_ERR	beInitStream(PBE_CONFIG pbeConfig, PDWORD dwSamples, PDWORD dwBufferSize, PHBE_STREAM phbeStream);
//__declspec(dllexport) BE_ERR	beEncodeChunk(HBE_STREAM hbeStream, DWORD nSamples, PSHORT pSamples, PBYTE pOutput, PDWORD pdwOutput);
//__declspec(dllexport) BE_ERR	beDeinitStream(HBE_STREAM hbeStream, PBYTE pOutput, PDWORD pdwOutput);
//__declspec(dllexport) BE_ERR	beCloseStream(HBE_STREAM hbeStream);
//__declspec(dllexport) VOID	beVersion(PBE_VERSION pbeVersion);

{
Function beInitStream(var pbeConfig: TBE_CONFIG; var dwSample: LongWord; var dwBufferSize: LongWord; var phbeStream: THBE_STREAM ): BE_Err; cdecl; external 'Bladeenc.dll';
//Function beEncodeChunk(hbeStream: THBE_STREAM; nSamples: LongWord; pSample: PSmallInt;pOutput: PByte; var pdwOutput: LongWord): BE_Err; cdecl; external 'Bladeenc.dll';
Function beEncodeChunk(hbeStream: THBE_STREAM; nSamples: LongWord; var pSample;var pOutput; var pdwOutput: LongWord): BE_Err; stdcall; cdecl 'Bladeenc.dll';
Function beDeinitStream(hbeStream: THBE_STREAM; var pOutput; var pdwOutput: LongWord): BE_Err; cdecl; external 'Bladeenc.dll';
Function beCloseStream(hbeStream: THBE_STREAM): BE_Err; cdecl; external 'Bladeenc.dll';
Procedure beVersion(var pbeVersion: TBE_VERSION); cdecl; external 'Bladeenc.dll';
}

Function beInitStream(var pbeConfig: TBE_CONFIG; var dwSample: LongWord; var dwBufferSize: LongWord; var phbeStream: THBE_STREAM ): BE_Err; cdecl; external 'Lame_enc.dll';
//Function beEncodeChunk(hbeStream: THBE_STREAM; nSamples: LongWord; pSample: PSmallInt;pOutput: PByte; var pdwOutput: LongWord): BE_Err; cdecl; external 'Lame_enc.dll';
Function beEncodeChunk(hbeStream: THBE_STREAM; nSamples: LongWord; var pSample;var pOutput; var pdwOutput: LongWord): BE_Err; cdecl; external 'Lame_enc.dll';
Function beDeinitStream(hbeStream: THBE_STREAM; var pOutput; var pdwOutput: LongWord): BE_Err; cdecl; external 'Lame_enc.dll';
Function beCloseStream(hbeStream: THBE_STREAM): BE_Err; cdecl; external 'Lame_enc.dll';
Procedure beVersion(var pbeVersion: TBE_VERSION); cdecl; external 'Lame_enc.dll';

Procedure EncodeWavToMP3(fs, fd: Integer);
implementation

Uses InternetSnd, TraiteWav;

{----------------------------------------}
Procedure EncodeWavToMP3(fs, fd: Integer);
var
  err: Integer;
  beConfig: TBE_Config;
  dwSamples, dwSamplesMP3 : LongWord;
  hbeStream : THBE_STREAM;
  error: BE_ERR;
  pBuffer: PSmallInt;
  pMP3Buffer: PByte;
  Marque:PChar;

  done: LongWord;
  dwWrite: LongWord;
  ToRead: LongWord;
  ToWrite: LongWord;
  i:Integer;

begin
  beConfig.dwConfig := BE_CONFIG_LAME;

{
  beConfig.Format.mp3.dwSampleRate := WavInfo.SamplesPerSec;
  beConfig.Format.mp3.byMode := BE_MP3_MODE_STEREO;
  beConfig.Format.mp3.wBitrate := strToInt(MainFrm.Mp3BitRate.Text);
  beConfig.Format.mp3.bCopyright := 0;
  beConfig.Format.mp3.bCRC := $00000000;
  beConfig.Format.mp3.bOriginal := 0;
  beConfig.Format.mp3.bPrivate := 0;
}
//Structure information
  beConfig.Format.lhv1.dwStructVersion := 1;
  beConfig.Format.lhv1.dwStructSize := SizeOf(beConfig);
//Basic encoder setting
  beConfig.Format.lhv1.dwSampleRate := WavInfo.SamplesPerSec;
  beConfig.Format.lhv1.dwReSampleRate := 44100;
  beConfig.Format.lhv1.nMode := BE_MP3_MODE_STEREO;
  beConfig.Format.lhv1.dwBitrate := strToInt(MainFrm.Mp3BitRate.Text);
  beConfig.Format.lhv1.dwMaxBitrate := strToInt(MainFrm.Mp3BitRate.Text);
  beConfig.Format.lhv1.nQuality := 2;
  beConfig.Format.lhv1.dwMPegVersion := 1; //MPEG1
  beConfig.Format.lhv1.dwPsyModel := 0;
  beConfig.Format.lhv1.dwEmphasis := 0;
//Bit Stream Settings
  beConfig.Format.lhv1.bPrivate := False;
  beConfig.Format.lhv1.bCRC := False;
  beConfig.Format.lhv1.bCopyright := True;
  beConfig.Format.lhv1.bOriginal := True;
//VBR Stuff
  beConfig.Format.lhv1.bWriteVBRHeader := false;
  beConfig.Format.lhv1.bEnableVBR := false;
  beConfig.Format.lhv1.nVBRQuality := 0;

  i := 0;
  error := beInitStream(beConfig, dwSamples, dwSamplesMP3, hbeStream);
  if error = BE_ERR_SUCCESSFUL
    then begin
         pBuffer := AllocMem(dwSamples*2);
         pMP3Buffer := AllocMem(dwSamplesMP3);
         try
           done := 0;

           error := FileSeek(fs, 0, 0);
           While (done < TotalSize) do
             begin
               if (done + dwSamples*2 < TotalSize)
                 then ToRead := dwSamples*2
                 else begin
                      ToRead := TotalSize-done;
                      //FillChar(buf[0],dwSamples*2,0);
                      FillChar(pbuffer^,dwSamples,0);
                      end;

               //if FileRead(fs, buf[0], toread) = -1
               if FileRead(fs, pbuffer^, toread) = -1
                 then raise Exception.Create('Erreur de lecture');

               //error := beEncodeChunk(hbeStream, toRead div 2, Buf[0], TmpBuf[0], toWrite);
               error := beEncodeChunk(hbeStream, toRead div 2, pBuffer^, pMP3Buffer^, toWrite);

               if error <> BE_ERR_SUCCESSFUL
                 then begin
                      beCloseStream(hbeStream);
                      raise Exception.Create('Echec de l''encodage');
                      end;

               //if FileWrite(fd, TmpBuf[0], toWrite) = -1
               if FileWrite(fd, pMP3Buffer^, toWrite) = -1
                 then raise Exception.Create('Erreur d''écriture');

               done := done + toread;
               inc(i);
               if i mod 64 = 0
                 then begin
                      MainFrm.ProgressBar1.Position := round(100*done/Totalsize);
                      Application.ProcessMessages;
                      end;
             end;

           error := beDeInitStream(hbeStream, pMP3Buffer^, dwWrite);
           //error := beDeInitStream(hbeStream, TmpBuf[0], dwWrite);

           if error <> BE_ERR_SUCCESSFUL
             then begin
                  beCloseStream(hbeStream);
                  raise Exception.Create('Echec à la sortie');
                  end;

           if dwWrite <> 0
             then begin
                  //if FileWrite(fd, TmpBuf[0], dwWrite) = -1
                  if FileWrite(fd, pMP3Buffer^, dwWrite) = -1
                    then raise Exception.Create('Erreur à la dernière écriture');
                  end;

           beCloseStream(hbeStream);
           finally
             FreeMem(pBuffer);
             FreeMem(pMP3Buffer);
             end;
         end
    else begin

         end;
end;

end.
