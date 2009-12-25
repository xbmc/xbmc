/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/


///////////////////////////////////////////////////
//
// PSM module loader
//
///////////////////////////////////////////////////
#include "stdafx.h"
#include "sndfile.h"

//#define PSM_LOG

#define PSM_ID_NEW	0x204d5350
#define PSM_ID_OLD	0xfe4d5350
#define IFFID_FILE	0x454c4946
#define IFFID_TITL	0x4c544954
#define IFFID_SDFT	0x54464453
#define IFFID_PBOD	0x444f4250
#define IFFID_SONG	0x474e4f53
#define IFFID_PATT	0x54544150
#define IFFID_DSMP	0x504d5344
#define IFFID_OPLH	0x484c504f

#pragma pack(1)

typedef struct _PSMCHUNK
{
	DWORD id;
	DWORD len;
	DWORD listid;
} PSMCHUNK;

typedef struct _PSMSONGHDR
{
	CHAR songname[8];	// "MAINSONG"
	BYTE reserved1;
	BYTE reserved2;
	BYTE channels;
} PSMSONGHDR;

typedef struct _PSMPATTERN
{
	DWORD size;
	DWORD name;
	WORD rows;
	WORD reserved1;
	BYTE data[4];
} PSMPATTERN;

typedef struct _PSMSAMPLE
{
	BYTE flags;
	CHAR songname[8];
	DWORD smpid;
	CHAR samplename[34];
	DWORD reserved1;
	BYTE reserved2;
	BYTE insno;
	BYTE reserved3;
	DWORD length;
	DWORD loopstart;
	DWORD loopend;
	WORD reserved4;
	BYTE defvol;
	DWORD reserved5;
	DWORD samplerate;
	BYTE reserved6[19];
} PSMSAMPLE;

#pragma pack()


BOOL CSoundFile::ReadPSM(LPCBYTE lpStream, DWORD dwMemLength)
//-----------------------------------------------------------
{
	PSMCHUNK *pfh = (PSMCHUNK *)lpStream;
	DWORD dwMemPos, dwSongPos;
	DWORD smpnames[MAX_SAMPLES];
	DWORD patptrs[MAX_PATTERNS];
	BYTE samplemap[MAX_SAMPLES];
	UINT nPatterns;

	// Chunk0: "PSM ",filesize,"FILE"
	if (dwMemLength < 256) return FALSE;
	if (pfh->id == PSM_ID_OLD)
	{
	#ifdef PSM_LOG
		Log("Old PSM format not supported\n");
	#endif
		return FALSE;
	}
	if ((pfh->id != PSM_ID_NEW) || (pfh->len+12 > dwMemLength) || (pfh->listid != IFFID_FILE)) return FALSE;
	m_nType = MOD_TYPE_PSM;
	m_nChannels = 16;
	m_nSamples = 0;
	nPatterns = 0;
	dwMemPos = 12;
	dwSongPos = 0;
	for (UINT iChPan=0; iChPan<16; iChPan++)
	{
		UINT pan = (((iChPan & 3) == 1) || ((iChPan&3)==2)) ? 0xC0 : 0x40;
		ChnSettings[iChPan].nPan = pan;
	}
	while (dwMemPos+8 < dwMemLength)
	{
		PSMCHUNK *pchunk = (PSMCHUNK *)(lpStream+dwMemPos);
		if ((pchunk->len >= dwMemLength - 8) || (dwMemPos + pchunk->len + 8 > dwMemLength)) break;
		dwMemPos += 8;
		PUCHAR pdata = (PUCHAR)(lpStream+dwMemPos);
		ULONG len = pchunk->len;
		if (len) switch(pchunk->id)
		{
		// "TITL": Song title
		case IFFID_TITL:
			if (!pdata[0]) { pdata++; len--; }
			memcpy(m_szNames[0], pdata, (len>31) ? 31 : len);
			m_szNames[0][31] = 0;
			break;
		// "PBOD": Pattern
		case IFFID_PBOD:
			if ((len >= 12) && (nPatterns < MAX_PATTERNS))
			{
				patptrs[nPatterns++] = dwMemPos-8;
			}
			break;
		// "SONG": Song description
		case IFFID_SONG:
			if ((len >= sizeof(PSMSONGHDR)+8) && (!dwSongPos))
			{
				dwSongPos = dwMemPos - 8;
			}
			break;
		// "DSMP": Sample Data
		case IFFID_DSMP:
			if ((len >= sizeof(PSMSAMPLE)) && (m_nSamples+1 < MAX_SAMPLES))
			{
				m_nSamples++;
				MODINSTRUMENT *pins = &Ins[m_nSamples];
				PSMSAMPLE *psmp = (PSMSAMPLE *)pdata;
				smpnames[m_nSamples] = psmp->smpid;
				memcpy(m_szNames[m_nSamples], psmp->samplename, 31);
				m_szNames[m_nSamples][31] = 0;
				samplemap[m_nSamples-1] = (BYTE)m_nSamples;
				// Init sample
				pins->nGlobalVol = 0x40;
				pins->nC4Speed = psmp->samplerate;
				pins->nLength = psmp->length;
				pins->nLoopStart = psmp->loopstart;
				pins->nLoopEnd = psmp->loopend;
				pins->nPan = 128;
				pins->nVolume = (psmp->defvol+1) * 2;
				pins->uFlags = (psmp->flags & 0x80) ? CHN_LOOP : 0;
				if (pins->nLoopStart > 0) pins->nLoopStart--;
				// Point to sample data
				pdata += 0x60;
				len -= 0x60;
				// Load sample data
				if ((pins->nLength > 3) && (len > 3))
				{
					ReadSample(pins, RS_PCM8D, (LPCSTR)pdata, len);
				} else
				{
					pins->nLength = 0;
				}
			}
			break;
	#if 0
		default:
			{
				CHAR s[8], s2[64];
				*(DWORD *)s = pchunk->id;
				s[4] = 0;
				wsprintf(s2, "%s: %4d bytes @ %4d\n", s, pchunk->len, dwMemPos);
				OutputDebugString(s2);
			}
	#endif
		}
		dwMemPos += pchunk->len;
	}
	// Step #1: convert song structure
	PSMSONGHDR *pSong = (PSMSONGHDR *)(lpStream+dwSongPos+8);
	if ((!dwSongPos) || (pSong->channels < 2) || (pSong->channels > 32)) return TRUE;
	m_nChannels = pSong->channels;
	// Valid song header -> convert attached chunks
	{
		DWORD dwSongEnd = dwSongPos + 8 + *(DWORD *)(lpStream+dwSongPos+4);
		dwMemPos = dwSongPos + 8 + 11; // sizeof(PSMCHUNK)+sizeof(PSMSONGHDR)
		while (dwMemPos + 8 < dwSongEnd)
		{
			PSMCHUNK *pchunk = (PSMCHUNK *)(lpStream+dwMemPos);
			dwMemPos += 8;
			if ((pchunk->len > dwSongEnd) || (dwMemPos + pchunk->len > dwSongEnd)) break;
			PUCHAR pdata = (PUCHAR)(lpStream+dwMemPos);
			ULONG len = pchunk->len;
			switch(pchunk->id)
			{
			case IFFID_OPLH:
				if (len >= 0x20)
				{
					UINT pos = len - 3;
					while (pos > 5)
					{
						BOOL bFound = FALSE;
						pos -= 5;
						DWORD dwName = *(DWORD *)(pdata+pos);
						for (UINT i=0; i<nPatterns; i++)
						{
							DWORD dwPatName = ((PSMPATTERN *)(lpStream+patptrs[i]+8))->name;
							if (dwName == dwPatName)
							{
								bFound = TRUE;
								break;
							}
						}
						if ((!bFound) && (pdata[pos+1] > 0) && (pdata[pos+1] <= 0x10)
						 && (pdata[pos+3] > 0x40) && (pdata[pos+3] < 0xC0))
						{
							m_nDefaultSpeed = pdata[pos+1];
							m_nDefaultTempo = pdata[pos+3];
							break;
						}
					}
					UINT iOrd = 0;
					while ((pos+5<len) && (iOrd < MAX_ORDERS))
					{
						DWORD dwName = *(DWORD *)(pdata+pos);
						for (UINT i=0; i<nPatterns; i++)
						{
							DWORD dwPatName = ((PSMPATTERN *)(lpStream+patptrs[i]+8))->name;
							if (dwName == dwPatName)
							{
								Order[iOrd++] = i;
								break;
							}
						}
						pos += 5;
					}
				}
				break;
			}
			dwMemPos += pchunk->len;
		}
	}

	// Step #2: convert patterns
	for (UINT nPat=0; nPat<nPatterns; nPat++)
	{
		PSMPATTERN *pPsmPat = (PSMPATTERN *)(lpStream+patptrs[nPat]+8);
		ULONG len = *(DWORD *)(lpStream+patptrs[nPat]+4) - 12;
		UINT nRows = pPsmPat->rows;
		if (len > pPsmPat->size) len = pPsmPat->size;
		if ((nRows < 64) || (nRows > 256)) nRows = 64;
		PatternSize[nPat] = nRows;
		if ((Patterns[nPat] = AllocatePattern(nRows, m_nChannels)) == NULL) break;
		MODCOMMAND *m = Patterns[nPat];
		BYTE *p = pPsmPat->data;
		UINT pos = 0;
		UINT row = 0;
		UINT oldch = 0;
		BOOL bNewRow = FALSE;
	#ifdef PSM_LOG
		Log("Pattern %d at offset 0x%04X\n", nPat, (DWORD)(p - (BYTE *)lpStream));
	#endif
		while ((row < nRows) && (pos+1 < len))
		{
			UINT flags = p[pos++];
			UINT ch = p[pos++];
			
		#ifdef PSM_LOG
			//Log("flags+ch: %02X.%02X\n", flags, ch);
		#endif
			if (((flags & 0xf0) == 0x10) && (ch <= oldch) /*&& (!bNewRow)*/)
			{
				if ((pos+1<len) && (!(p[pos] & 0x0f)) && (p[pos+1] < m_nChannels))
				{
				#ifdef PSM_LOG
					//if (!nPat) Log("Continuing on new row\n");
				#endif
					row++;
					m += m_nChannels;
					oldch = ch;
					continue;
				}
			}
			if ((pos >= len) || (row >= nRows)) break;
			if (!(flags & 0xf0))
			{
			#ifdef PSM_LOG
				//if (!nPat) Log("EOR(%d): %02X.%02X\n", row, p[pos], p[pos+1]);
			#endif
				row++;
				m += m_nChannels;
				bNewRow = TRUE;
				oldch = ch;
				continue;
			}
			bNewRow = FALSE;
			if (ch >= m_nChannels)
			{
			#ifdef PSM_LOG
				if (!nPat) Log("Invalid channel row=%d (0x%02X.0x%02X)\n", row, flags, ch);
			#endif
				ch = 0;
			}
			// Note + Instr
			if ((flags & 0x40) && (pos+1 < len))
			{
				UINT note = p[pos++];
				UINT nins = p[pos++];
			#ifdef PSM_LOG
				//if (!nPat) Log("note+ins: %02X.%02X\n", note, nins);
				if ((!nPat) && (nins >= m_nSamples)) Log("WARNING: invalid instrument number (%d)\n", nins);
			#endif
				if ((note) && (note < 0x80)) note = (note>>4)*12+(note&0x0f)+12+1;
				m[ch].instr = samplemap[nins];
				m[ch].note = note;
			}
			// Volume
			if ((flags & 0x20) && (pos < len))
			{
				m[ch].volcmd = VOLCMD_VOLUME;
				m[ch].vol = p[pos++] / 2;
			}
			// Effect
			if ((flags & 0x10) && (pos+1 < len))
			{
				UINT command = p[pos++];
				UINT param = p[pos++];
				// Convert effects
				switch(command)
				{
				// 01: fine volslide up
				case 0x01:	command = CMD_VOLUMESLIDE; param |= 0x0f; break;
				// 04: fine volslide down
				case 0x04:	command = CMD_VOLUMESLIDE; param>>=4; param |= 0xf0; break;
				// 0C: portamento up
				case 0x0C:	command = CMD_PORTAMENTOUP; param = (param+1)/2; break;
				// 0E: portamento down
				case 0x0E:	command = CMD_PORTAMENTODOWN; param = (param+1)/2; break;
				// 33: Position Jump
				case 0x33:	command = CMD_POSITIONJUMP; break;
				// 34: Pattern break
				case 0x34:	command = CMD_PATTERNBREAK; break;
				// 3D: speed
				case 0x3D:	command = CMD_SPEED; break;
				// 3E: tempo
				case 0x3E:	command = CMD_TEMPO; break;
				// Unknown
				default:
				#ifdef PSM_LOG
					Log("Unknown PSM effect pat=%d row=%d ch=%d: %02X.%02X\n", nPat, row, ch, command, param);
				#endif
					command = param = 0;
				}
				m[ch].command = (BYTE)command;
				m[ch].param = (BYTE)param;
			}
			oldch = ch;
		}
	#ifdef PSM_LOG
		if (pos < len)
		{
			Log("Pattern %d: %d/%d[%d] rows (%d bytes) -> %d bytes left\n", nPat, row, nRows, pPsmPat->rows, pPsmPat->size, len-pos);
		}
	#endif
	}

	// Done (finally!)
	return TRUE;
}


//////////////////////////////////////////////////////////////
//
// PSM Old Format
//

/*

CONST
  c_PSM_MaxOrder   = $FF;
  c_PSM_MaxSample  = $FF;
  c_PSM_MaxChannel = $0F;

 TYPE
  PPSM_Header = ^TPSM_Header;
  TPSM_Header = RECORD
                 PSM_Sign                   : ARRAY[01..04] OF CHAR; { PSM + #254 }
                 PSM_SongName               : ARRAY[01..58] OF CHAR;
                 PSM_Byte00                 : BYTE;
                 PSM_Byte1A                 : BYTE;
                 PSM_Unknown00              : BYTE;
                 PSM_Unknown01              : BYTE;
                 PSM_Unknown02              : BYTE;
                 PSM_Speed                  : BYTE;
                 PSM_Tempo                  : BYTE;
                 PSM_Unknown03              : BYTE;
                 PSM_Unknown04              : WORD;
                 PSM_OrderLength            : WORD;
                 PSM_PatternNumber          : WORD;
                 PSM_SampleNumber           : WORD;
                 PSM_ChannelNumber          : WORD;
                 PSM_ChannelUsed            : WORD;
                 PSM_OrderPosition          : LONGINT;
                 PSM_ChannelSettingPosition : LONGINT;
                 PSM_PatternPosition        : LONGINT;
                 PSM_SamplePosition         : LONGINT;
                { *** perhaps there are some more infos in a larger header,
                      but i have not decoded it and so it apears here NOT }
                END;

  PPSM_Sample = ^TPSM_Sample;
  TPSM_Sample = RECORD
                 PSM_SampleFileName  : ARRAY[01..12] OF CHAR;
                 PSM_SampleByte00    : BYTE;
                 PSM_SampleName      : ARRAY[01..22] OF CHAR;
                 PSM_SampleUnknown00 : ARRAY[01..02] OF BYTE;
                 PSM_SamplePosition  : LONGINT;
                 PSM_SampleUnknown01 : ARRAY[01..04] OF BYTE;
                 PSM_SampleNumber    : BYTE;
                 PSM_SampleFlags     : WORD;
                 PSM_SampleLength    : LONGINT;
                 PSM_SampleLoopBegin : LONGINT;
                 PSM_SampleLoopEnd   : LONGINT;
                 PSM_Unknown03       : BYTE;
                 PSM_SampleVolume    : BYTE;
                 PSM_SampleC5Speed   : WORD;
                END;

  PPSM_SampleList = ^TPSM_SampleList;
  TPSM_SampleList = ARRAY[01..c_PSM_MaxSample] OF TPSM_Sample;

  PPSM_Order = ^TPSM_Order;
  TPSM_Order = ARRAY[00..c_PSM_MaxOrder] OF BYTE;

  PPSM_ChannelSettings = ^TPSM_ChannelSettings;
  TPSM_ChannelSettings = ARRAY[00..c_PSM_MaxChannel] OF BYTE;

 CONST
  PSM_NotesInPattern   : BYTE = $00;
  PSM_ChannelInPattern : BYTE = $00;

 CONST
  c_PSM_SetSpeed = 60;

 FUNCTION PSM_Size(FileName : STRING;FilePosition : LONGINT) : LONGINT;
  BEGIN
  END;

 PROCEDURE PSM_UnpackPattern(VAR Source,Destination;PatternLength : WORD);
  VAR
   Witz : ARRAY[00..04] OF WORD;
   I1,I2        : WORD;
   I3,I4        : WORD;
   TopicalByte  : ^BYTE;
   Pattern      : PUnpackedPattern;
   ChannelP     : BYTE;
   NoteP        : BYTE;
   InfoByte     : BYTE;
   CodeByte     : BYTE;
   InfoWord     : WORD;
   Effect       : BYTE;
   Opperand     : BYTE;
   Panning      : BYTE;
   Volume       : BYTE;
   PrevInfo     : BYTE;
   InfoIndex    : BYTE;
  BEGIN
   Pattern     := @Destination;
   TopicalByte := @Source;
  { *** Initialize patttern }
   FOR I2 := 0 TO c_Maximum_NoteIndex DO
    FOR I3 := 0 TO c_Maximum_ChannelIndex DO
     BEGIN
      Pattern^[I2,I3,c_Pattern_NoteIndex]     := $FF;
      Pattern^[I2,I3,c_Pattern_SampleIndex]   := $00;
      Pattern^[I2,I3,c_Pattern_VolumeIndex]   := $FF;
      Pattern^[I2,I3,c_Pattern_PanningIndex]  := $FF;
      Pattern^[I2,I3,c_Pattern_EffectIndex]   := $00;
      Pattern^[I2,I3,c_Pattern_OpperandIndex] := $00;
     END;
  { *** Byte-pointer on first pattern-entry }
   ChannelP    := $00;
   NoteP       := $00;
   InfoByte    := $00;
   PrevInfo    := $00;
   InfoIndex   := $02;
  { *** read notes in pattern }
   PSM_NotesInPattern   := TopicalByte^; INC(TopicalByte); DEC(PatternLength); INC(InfoIndex);
   PSM_ChannelInPattern := TopicalByte^; INC(TopicalByte); DEC(PatternLength); INC(InfoIndex);
  { *** unpack pattern }
   WHILE (INTEGER(PatternLength) > 0) AND (NoteP < c_Maximum_NoteIndex) DO
    BEGIN
    { *** Read info-byte }
     InfoByte := TopicalByte^; INC(TopicalByte); DEC(PatternLength); INC(InfoIndex);
     IF InfoByte <> $00 THEN
      BEGIN
       ChannelP := InfoByte AND $0F;
       IF InfoByte AND 128 = 128 THEN { note and sample }
        BEGIN
        { *** read note }
         CodeByte := TopicalByte^; INC(TopicalByte); DEC(PatternLength);
         DEC(CodeByte);
         CodeByte := CodeByte MOD 12 * 16 + CodeByte DIV 12 + 2;
         Pattern^[NoteP,ChannelP,c_Pattern_NoteIndex] := CodeByte;
        { *** read sample }
         CodeByte := TopicalByte^; INC(TopicalByte); DEC(PatternLength);
         Pattern^[NoteP,ChannelP,c_Pattern_SampleIndex] := CodeByte;
        END;
       IF InfoByte AND 64 = 64 THEN { Volume }
        BEGIN
         CodeByte := TopicalByte^; INC(TopicalByte); DEC(PatternLength);
         Pattern^[NoteP,ChannelP,c_Pattern_VolumeIndex] := CodeByte;
        END;
       IF InfoByte AND 32 = 32 THEN { effect AND opperand }
        BEGIN
         Effect   := TopicalByte^; INC(TopicalByte); DEC(PatternLength);
         Opperand := TopicalByte^; INC(TopicalByte); DEC(PatternLength);
         CASE Effect OF
          c_PSM_SetSpeed:
           BEGIN
            Effect := c_I_Set_Speed;
           END;
          ELSE
           BEGIN
            Effect   := c_I_NoEffect;
            Opperand := $00;
           END;
         END;
         Pattern^[NoteP,ChannelP,c_Pattern_EffectIndex]   := Effect;
         Pattern^[NoteP,ChannelP,c_Pattern_OpperandIndex] := Opperand;
        END;
      END ELSE INC(NoteP);
    END;
  END;

 PROCEDURE PSM_Load(FileName : STRING;FilePosition : LONGINT;VAR Module : PModule;VAR ErrorCode : WORD);
 { *** caution : Module has to be inited before!!!! }
  VAR
   Header             : PPSM_Header;
   Sample             : PPSM_SampleList;
   Order              : PPSM_Order;
   ChannelSettings    : PPSM_ChannelSettings;
   MultiPurposeBuffer : PByteArray;
   PatternBuffer      : PUnpackedPattern;
   TopicalParaPointer : WORD;

   InFile : FILE;
   I1,I2  : WORD;
   I3,I4  : WORD;
   TempW  : WORD;
   TempB  : BYTE;
   TempP  : PByteArray;
   TempI  : INTEGER;
  { *** copy-vars for loop-extension }
   CopySource      : LONGINT;
   CopyDestination : LONGINT;
   CopyLength      : LONGINT;
  BEGIN
  { *** try to open file }
   ASSIGN(InFile,FileName);
{$I-}
   RESET(InFile,1);
{$I+}
   IF IORESULT <> $00 THEN
    BEGIN
     EXIT;
    END;
{$I-}
  { *** seek start of module }
   IF FILESIZE(InFile) < FilePosition THEN
    BEGIN
     EXIT;
    END;
   SEEK(InFile,FilePosition);
  { *** look for enough memory for temporary variables }
   IF MEMAVAIL < SIZEOF(TPSM_Header)       + SIZEOF(TPSM_SampleList) +
                 SIZEOF(TPSM_Order)        + SIZEOF(TPSM_ChannelSettings) +
                 SIZEOF(TByteArray)        + SIZEOF(TUnpackedPattern)
   THEN
    BEGIN
     EXIT;
    END;
  { *** init dynamic variables }
   NEW(Header);
   NEW(Sample);
   NEW(Order);
   NEW(ChannelSettings);
   NEW(MultiPurposeBuffer);
   NEW(PatternBuffer);
  { *** read header }
   BLOCKREAD(InFile,Header^,SIZEOF(TPSM_Header));
  { *** test if this is a DSM-file }
   IF NOT ((Header^.PSM_Sign[1] = 'P') AND (Header^.PSM_Sign[2] = 'S')   AND
           (Header^.PSM_Sign[3] = 'M') AND (Header^.PSM_Sign[4] = #254)) THEN
    BEGIN
     ErrorCode := c_NoValidFileFormat;
     CLOSE(InFile);
     EXIT;
    END;
  { *** read order }
   SEEK(InFile,FilePosition + Header^.PSM_OrderPosition);
   BLOCKREAD(InFile,Order^,Header^.PSM_OrderLength);
  { *** read channelsettings }
   SEEK(InFile,FilePosition + Header^.PSM_ChannelSettingPosition);
   BLOCKREAD(InFile,ChannelSettings^,SIZEOF(TPSM_ChannelSettings));
  { *** read samplelist }
   SEEK(InFile,FilePosition + Header^.PSM_SamplePosition);
   BLOCKREAD(InFile,Sample^,Header^.PSM_SampleNumber * SIZEOF(TPSM_Sample));
  { *** copy header to intern NTMIK-structure }
   Module^.Module_Sign                 := 'MF';
   Module^.Module_FileFormatVersion    := $0100;
   Module^.Module_SampleNumber         := Header^.PSM_SampleNumber;
   Module^.Module_PatternNumber        := Header^.PSM_PatternNumber;
   Module^.Module_OrderLength          := Header^.PSM_OrderLength;
   Module^.Module_ChannelNumber        := Header^.PSM_ChannelNumber+1;
   Module^.Module_Initial_GlobalVolume := 64;
   Module^.Module_Initial_MasterVolume := $C0;
   Module^.Module_Initial_Speed        := Header^.PSM_Speed;
   Module^.Module_Initial_Tempo        := Header^.PSM_Tempo;
{ *** paragraph 01 start }
   Module^.Module_Flags                := c_Module_Flags_ZeroVolume        * BYTE(1) +
                                          c_Module_Flags_Stereo            * BYTE(1) +
                                          c_Module_Flags_ForceAmigaLimits  * BYTE(0) +
                                          c_Module_Flags_Panning           * BYTE(1) +
                                          c_Module_Flags_Surround          * BYTE(1) +
                                          c_Module_Flags_QualityMixing     * BYTE(1) +
                                          c_Module_Flags_FastVolumeSlides  * BYTE(0) +
                                          c_Module_Flags_SpecialCustomData * BYTE(0) +
                                          c_Module_Flags_SongName          * BYTE(1);
   I1 := $01;
   WHILE (Header^.PSM_SongName[I1] > #00) AND (I1 < c_Module_SongNameLength) DO
    BEGIN
     Module^.Module_Name[I1] := Header^.PSM_SongName[I1];
     INC(I1);
    END;
   Module^.Module_Name[c_Module_SongNameLength] := #00;
  { *** Init channelsettings }
   FOR I1 := 0 TO c_Maximum_ChannelIndex DO
    BEGIN
     IF I1 < Header^.PSM_ChannelUsed THEN
      BEGIN
      { *** channel enabled }
       Module^.Module_ChannelSettingPointer^[I1].ChannelSettings_GlobalVolume := 64;
       Module^.Module_ChannelSettingPointer^[I1].ChannelSettings_Panning      := (ChannelSettings^[I1]) * $08;
       Module^.Module_ChannelSettingPointer^[I1].ChannelSettings_Code         := I1 + $10 * BYTE(ChannelSettings^[I1] > $08) +
                                             c_ChannelSettings_Code_ChannelEnabled   * BYTE(1) +
                                             c_ChannelSettings_Code_ChannelDigital   * BYTE(1);
       Module^.Module_ChannelSettingPointer^[I1].ChannelSettings_Controls     :=
                                             c_ChannelSettings_Controls_EnhancedMode * BYTE(1) +
                                             c_ChannelSettings_Controls_SurroundMode * BYTE(0);
      END
     ELSE
      BEGIN
      { *** channel disabled }
       Module^.Module_ChannelSettingPointer^[I1].ChannelSettings_GlobalVolume := $00;
       Module^.Module_ChannelSettingPointer^[I1].ChannelSettings_Panning      := $00;
       Module^.Module_ChannelSettingPointer^[I1].ChannelSettings_Code         := $00;
       Module^.Module_ChannelSettingPointer^[I1].ChannelSettings_Controls     := $00;
      END;
    END;
  { *** init and copy order }
   FILLCHAR(Module^.Module_OrderPointer^,c_Maximum_OrderIndex+1,$FF);
   MOVE(Order^,Module^.Module_OrderPointer^,Header^.PSM_OrderLength);
  { *** read pattern }
   SEEK(InFile,FilePosition + Header^.PSM_PatternPosition);
   NTMIK_LoaderPatternNumber := Header^.PSM_PatternNumber-1;
   FOR I1 := 0 TO Header^.PSM_PatternNumber-1 DO
    BEGIN
     NTMIK_LoadPatternProcedure;
    { *** read length }
     BLOCKREAD(InFile,TempW,2);
    { *** read pattern }
     BLOCKREAD(InFile,MultiPurposeBuffer^,TempW-2);
    { *** unpack pattern and set notes per channel to 64 }
     PSM_UnpackPattern(MultiPurposeBuffer^,PatternBuffer^,TempW);
     NTMIK_PackPattern(MultiPurposeBuffer^,PatternBuffer^,PSM_NotesInPattern);
     TempW := WORD(256) * MultiPurposeBuffer^[01] + MultiPurposeBuffer^[00];
     GETMEM(Module^.Module_PatternPointer^[I1],TempW);
     MOVE(MultiPurposeBuffer^,Module^.Module_PatternPointer^[I1]^,TempW);
    { *** next pattern }
    END;
  { *** read samples }
   NTMIK_LoaderSampleNumber := Header^.PSM_SampleNumber;
   FOR I1 := 1 TO Header^.PSM_SampleNumber DO
    BEGIN
     NTMIK_LoadSampleProcedure;
    { *** get index for sample }
     I3 := Sample^[I1].PSM_SampleNumber;
    { *** clip PSM-sample }
     IF Sample^[I1].PSM_SampleLoopEnd > Sample^[I1].PSM_SampleLength
     THEN Sample^[I1].PSM_SampleLoopEnd := Sample^[I1].PSM_SampleLength;
    { *** init intern sample }
     NEW(Module^.Module_SamplePointer^[I3]);
     FILLCHAR(Module^.Module_SamplePointer^[I3]^,SIZEOF(TSample),$00);
     FILLCHAR(Module^.Module_SamplePointer^[I3]^.Sample_SampleName,c_Sample_SampleNameLength,#32);
     FILLCHAR(Module^.Module_SamplePointer^[I3]^.Sample_FileName,c_Sample_FileNameLength,#32);
    { *** copy informations to intern sample }
     I2 := $01;
     WHILE (Sample^[I1].PSM_SampleName[I2] > #00) AND (I2 < c_Sample_SampleNameLength) DO
      BEGIN
       Module^.Module_SamplePointer^[I3]^.Sample_SampleName[I2] := Sample^[I1].PSM_SampleName[I2];
       INC(I2);
      END;
     Module^.Module_SamplePointer^[I3]^.Sample_Sign              := 'DF';
     Module^.Module_SamplePointer^[I3]^.Sample_FileFormatVersion := $00100;
     Module^.Module_SamplePointer^[I3]^.Sample_Position          := $00000000;
     Module^.Module_SamplePointer^[I3]^.Sample_Selector          := $0000;
     Module^.Module_SamplePointer^[I3]^.Sample_Volume            := Sample^[I1].PSM_SampleVolume;
     Module^.Module_SamplePointer^[I3]^.Sample_LoopCounter       := $00;
     Module^.Module_SamplePointer^[I3]^.Sample_C5Speed           := Sample^[I1].PSM_SampleC5Speed;
     Module^.Module_SamplePointer^[I3]^.Sample_Length            := Sample^[I1].PSM_SampleLength;
     Module^.Module_SamplePointer^[I3]^.Sample_LoopBegin         := Sample^[I1].PSM_SampleLoopBegin;
     Module^.Module_SamplePointer^[I3]^.Sample_LoopEnd           := Sample^[I1].PSM_SampleLoopEnd;
    { *** now it's time for the flags }
     Module^.Module_SamplePointer^[I3]^.Sample_Flags :=
                                 c_Sample_Flags_DigitalSample      * BYTE(1) +
                                 c_Sample_Flags_8BitSample         * BYTE(1) +
                                 c_Sample_Flags_UnsignedSampleData * BYTE(1) +
                                 c_Sample_Flags_Packed             * BYTE(0) +
                                 c_Sample_Flags_LoopCounter        * BYTE(0) +
                                 c_Sample_Flags_SampleName         * BYTE(1) +
                                 c_Sample_Flags_LoopActive         *
                             BYTE(Sample^[I1].PSM_SampleFlags AND (LONGINT(1) SHL 15) = (LONGINT(1) SHL 15));
    { *** alloc memory for sample-data }
     E_Getmem(Module^.Module_SamplePointer^[I3]^.Sample_Selector,
              Module^.Module_SamplePointer^[I3]^.Sample_Position,
              Module^.Module_SamplePointer^[I3]^.Sample_Length + c_LoopExtensionSize);
    { *** read out data }
     EPT(TempP).p_Selector := Module^.Module_SamplePointer^[I3]^.Sample_Selector;
     EPT(TempP).p_Offset   := $0000;
     SEEK(InFile,Sample^[I1].PSM_SamplePosition);
     E_BLOCKREAD(InFile,TempP^,Module^.Module_SamplePointer^[I3]^.Sample_Length);
    { *** 'coz the samples are signed in a DSM-file -> PC-fy them }
     IF Module^.Module_SamplePointer^[I3]^.Sample_Length > 4 THEN
      BEGIN
       CopyLength := Module^.Module_SamplePointer^[I3]^.Sample_Length;
      { *** decode sample }
       ASM
        DB 066h; MOV CX,WORD PTR CopyLength
       { *** load sample selector }
                 MOV ES,WORD PTR TempP[00002h]
        DB 066h; XOR SI,SI
        DB 066h; XOR DI,DI
                 XOR AH,AH
       { *** conert all bytes }
                @@MainLoop:
        DB 026h; DB 067h; LODSB
                 ADD AL,AH
                 MOV AH,AL
        DB 067h; STOSB
        DB 066h; LOOP @@MainLoop
       END;
      { *** make samples unsigned }
       ASM
        DB 066h; MOV CX,WORD PTR CopyLength
       { *** load sample selector }
                 MOV ES,WORD PTR TempP[00002h]
        DB 066h; XOR SI,SI
        DB 066h; XOR DI,DI
       { *** conert all bytes }
                @@MainLoop:
        DB 026h; DB 067h; LODSB
                 SUB AL,080h
        DB 067h; STOSB
        DB 066h; LOOP @@MainLoop
       END;
      { *** Create Loop-Extension }
       IF Module^.Module_SamplePointer^[I3]^.Sample_Flags AND c_Sample_Flags_LoopActive = c_Sample_Flags_LoopActive THEN
        BEGIN
         CopySource      := Module^.Module_SamplePointer^[I3]^.Sample_LoopBegin;
         CopyDestination := Module^.Module_SamplePointer^[I3]^.Sample_LoopEnd;
         CopyLength      := CopyDestination - CopySource;
         ASM
         { *** load sample-selector }
                   MOV ES,WORD PTR TempP[00002h]
          DB 066h; MOV DI,WORD PTR CopyDestination
         { *** calculate number of full sample-loops to copy }
                   XOR DX,DX
                   MOV AX,c_LoopExtensionSize
                   MOV BX,WORD PTR CopyLength
                   DIV BX
                   OR AX,AX
                   JE @@NoFullLoop
         { *** copy some full-loops (size=bx) }
                   MOV CX,AX
                  @@InnerLoop:
                   PUSH CX
          DB 066h; MOV SI,WORD PTR CopySource
                   MOV CX,BX
          DB 0F3h; DB 026h,067h,0A4h { REP MOVS BYTE PTR ES:[EDI],ES:[ESI] }
                   POP CX
                   LOOP @@InnerLoop
                  @@NoFullLoop:
         { *** calculate number of rest-bytes to copy }
          DB 066h; MOV SI,WORD PTR CopySource
                   MOV CX,DX
          DB 0F3h; DB 026h,067h,0A4h { REP MOVS BYTE PTR ES:[EDI],ES:[ESI] }
         END;
        END
       ELSE
        BEGIN
         CopyDestination := Module^.Module_SamplePointer^[I3]^.Sample_Length;
         ASM
         { *** load sample-selector }
                   MOV ES,WORD PTR TempP[00002h]
          DB 066h; MOV DI,WORD PTR CopyDestination
         { *** clear extension }
                   MOV CX,c_LoopExtensionSize
                   MOV AL,080h
          DB 0F3h; DB 067h,0AAh       { REP STOS BYTE PTR ES:[EDI] }
         END;
        END;
      END;
    { *** next sample }
    END;
  { *** init period-ranges }
   NTMIK_MaximumPeriod := $0000D600 SHR 1;
   NTMIK_MinimumPeriod := $0000D600 SHR 8;
  { *** close file }
   CLOSE(InFile);
  { *** dispose all dynamic variables }
   DISPOSE(Header);
   DISPOSE(Sample);
   DISPOSE(Order);
   DISPOSE(ChannelSettings);
   DISPOSE(MultiPurposeBuffer);
   DISPOSE(PatternBuffer);
  { *** set errorcode to noerror }
   ErrorCode := c_NoError;
  END;

*/

