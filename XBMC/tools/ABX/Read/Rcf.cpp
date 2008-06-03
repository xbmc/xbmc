


//=============================================================================
//	Rcf.cpp - Multi (Needs work - Error reading 'dialoge.rcf' - Tetris Worlds)
//=============================================================================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
//#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"
#include "../FormatHdrs.h"

//------------------------------------------------------------------------------

void Supported::RcfRead(FILE* in){

Supported file;

fseek(in, 36, 0);
fread(rcf.index, 4, 1, in);

fseek(in, *rcf.index, 0);
fread(&CItem.Files, 4, 1, in);

fread(rcf.fileNameIndex, 4, 1, in);

// Offsets / chunks
file.pos = *rcf.index +8;

//fseek(in, file.pos, 0);
//fread((char*)off, 4, 1, in);   Offset.push_back(*off);

file.pos += 12;
for (int x = 0; x < CItem.Files; ++x, file.pos += 12)
{
	fseek(in, file.pos, 0);
	fread(file.offset, 4, 1, in);
	fread(file.chunk, 4, 1, in);
	FItem.Offset.push_back(*file.offset);
	FItem.Chunk.push_back(*file.chunk);
}

// Check last offset
if (FItem.Offset[CItem.Files - 1] >= CItem.FileSize)
	FItem.Chunk[CItem.Files - 1] = NULL;

// Filenames
file.pos = *rcf.fileNameIndex + 8;
for(int x = 0; x < CItem.Files; ++x)
{
	fseek(in, file.pos, 0);
	fread(rcf.fileNameLength, 4, 1, in);
	fread(rcf.fileName, *rcf.fileNameLength, 1, in);

	FItem.FileName.push_back((char*) rcf.fileName);
	//FItem.FileName[x] = ExtractFileName(FItem.FileName[x]);
	//FItem.FileName[x] = FItem.FileName[x].Trim();
	FItem.Ext.push_back("." + ExtractFileExt(ToLower(FItem.FileName[x])));
	if (FItem.Ext[x] == ".rsd")
		FItem.Ext[x] = ".wav";

	file.pos = file.pos + *rcf.fileNameLength + 8;
}


// Custom sort algorithm (Req. in order for filenames to match offsets)
u_long tmpOffset, tmpChunk;

for (int x = 0, y = 1; x < CItem.Files; ++x, y = 0)
{
	while (y < CItem.Files)
	{
		if (FItem.Offset[x] < FItem.Offset[y])// Swapped < with > don't know why it works this way
		{
			tmpOffset = FItem.Offset[y];  tmpChunk = FItem.Chunk[y];
			FItem.Offset[y] = FItem.Offset[x];  FItem.Chunk[y] = FItem.Chunk[x];
			FItem.Offset[x] = tmpOffset;  FItem.Chunk[x] = tmpChunk;
		}
	++y;
	}
}

rcf.fileNameIndexEnd = file.pos;

// Rsd4pcm specs
for (int x = 0; x < CItem.Files; ++x)
{
	fseek(in, FItem.Offset[x], 0);
	fread(rsd4.header, 7, 1, in);

	if (!strncmp((char*)rsd4.header, "RSD4PCM", 7) && FItem.Ext[x] == ".wav")
	{
		fseek(in, FItem.Offset[x] + rsd4.channelsOffset, 0);
		fread(rsd4.channels, 2, 1, in);

		fseek(in, FItem.Offset[x] + rsd4.bitsOffset, 0);
		fread(rsd4.bits, 2, 1, in);

		fseek(in, FItem.Offset[x] + rsd4.sampleRateOffset, 0);
		fread(rsd4.sampleRate, 4, 1, in);

		FItem.Format.push_back("pcm");
		FItem.Channels.push_back(*rsd4.channels);
		FItem.Bits.push_back(*rsd4.bits);
		FItem.SampleRate.push_back(*rsd4.sampleRate);
	}
	else
	{
		FItem.Format.push_back("");
		FItem.Channels.push_back(NULL);
		FItem.Bits.push_back(NULL);
		FItem.SampleRate.push_back(NULL);
	}
}

// Trim headers & padding from rsd's
u_char buf[8];
u_char xPad[8];
memset(xPad, 0x2D, 8);

for (int x = 0; x < CItem.Files; ++x)
{
	if (FItem.Format[x] == "pcm")
	{
		FItem.Offset[x] += 128;
		if (FItem.Chunk[x] > 128) FItem.Chunk[x] -= 128;
		// Remove '-' padding as found in Simpsons Hit & Run
		fseek(in, FItem.Offset[x] + 256, 0);
		fread(buf, 8, 1, in);
		if (*buf == *xPad)
		{
			FItem.Offset[x] += 1920;
			if (FItem.Chunk[x] > 1920) FItem.Chunk[x] -= 1920;
		}
	}
}

}

