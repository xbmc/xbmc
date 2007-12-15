


//===========================================================================
//	Samp.cpp - Namco Museum 50th Anniversary - (Simple container, requires
//												Namco50.uber for offsets)
//===========================================================================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"
#include "../FormatHdrs.h"

//------------------------------------------------------------------------------

bool Supported::SampRead(){


struct{
	u_int sDirOffset[4];
}samp;


Supported file;

FILE* in =
fopen(ChangeFileExt(CItem.OpenFileName.c_str(), ".uber").c_str(), "rb");

if (!in)
{
	gui.Error("Couldn't Find " + CItem.DirName + ".uber", 1500, in);
	return false;
}

fseek(in, 40, 0);
fread(&CItem.Files, 4, 1, in);

fseek(in, 16, 0);
fread(samp.sDirOffset, 4, 1, in);

file.pos = *samp.sDirOffset + 44;
for (int x = 0; x < CItem.Files; ++x, file.pos += 16)
{
	fseek(in, file.pos, 0);
	fread(file.offset, 4, 1, in);
	//fread(file.chunk, 4, 1, in);
	FItem.Offset.push_back(*file.offset); //Chunk.push_back(*file.chunk);
}


// We need to calc the filesizes as the info contained in .uber is off
for (int x = 0; x < CItem.Files-1; ++x)
{
	FItem.Chunk.push_back(FItem.Offset[x + 1] - FItem.Offset[x]);
}

FItem.Chunk.push_back(CItem.FileSize - FItem.Offset[CItem.Files - 1]);

fclose(in);
in = fopen(CItem.OpenFileName.c_str(), "rb");

for (int x = 0; x < CItem.Files; ++x)
{
	fseek(in, FItem.Offset[x], 0);
	fread(file.fmt, 2, 1, in);

	if (*file.fmt == 0x69)
	{
		FItem.Format.push_back("xadpcm");
		FItem.Bits.push_back(4);
	}
	else if (*file.fmt == 0x01)
	{
		FItem.Format.push_back("pcm");
		FItem.Bits.push_back(16);
	}

	fread(file.chan, 2, 1, in);
	FItem.Channels.push_back(*file.chan);

	fread(file.sr, 4, 1, in);
	FItem.SampleRate.push_back(*file.sr);

	FItem.Ext.push_back(".wav");
	FItem.Offset[x] += 20;
	FItem.Chunk[x] -= 20;
}

return true;
}

