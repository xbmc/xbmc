


//===========================================================
//		Rws.cpp - Multi (Built on shitty hacks, which work)
//===========================================================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"
#include "../FormatHdrs.h"

//------------------------------------------------------------------------------

bool Supported::RwsRead(FILE* in){


Supported file;

struct
{
	u_char	header[5];
	u_char 	type[7];
	u_int 	check [4];
	u_short	magic[2];
}	rws = { 0x0D, 0x08, 0x00, 0x00, 0xF4 };


//if (rwsType == 1)
fseek(in, 56, 0);
//else if (rwsType == 2)
//fseek(in, 6, 0);
fread(&CItem.Files, 4, 1, in);

// Magic
//u_int magic[2];
//fseek(in,6,0);
//fread((char*)magic, 2, 1, in);

//if (rwsType == 1)
fseek(in, 72, 0);
//else if (rwsType == 2)
//fseek(in, 12, 0);
fread(file.firstOffset, 2, 1, in);

// Get Index Offset
fseek(in, 144, 0);
fread(rws.check, 4, 1, in);
if (*rws.check != NULL) file.pos = 144;
else file.pos = 160;

for (int x = 0; x < CItem.Files; ++x, file.pos += 32)
{
	fseek(in, file.pos, 0);
	fread(file.chunk, 4, 1, in);
	fread(file.offset, 4, 1, in);
	FItem.Offset.push_back(*file.offset);
	FItem.Chunk.push_back(*file.chunk);

	FItem.Chunk[x] -= *file.firstOffset;
	FItem.Offset[x] += *file.firstOffset;
	//Offset[x] += 2048 + 6048;
	//Chunk[x] -= 2048;
	//Offset[x] = Offset[x] + 2062 + *magic + 1;
	if (feof(in))
	{
		gui.Error("Rws Read Error", 500, in);
		return false;
	}
}
		//msg.info(file.pos);
// Magic
fseek(in, 24, 0);
fread((char*)rws.magic, 2, 1, in);

// Rws type
fseek(in, 5, 0);
fread(rws.type, 7, 1, in);

// Madagascar
if (rws.type[0] == 0x87 && rws.type[5] == 0x02 && rws.type[6] == 0x1C)
	fseek(in, *rws.magic - 52, 0);

// Neighbours from hell
else if (rws.type[0] == 0xD7 && rws.type[5] == 0x02 && rws.type[6] == 0x1C)
	fseek(in, *rws.magic - 68, 0);

//// Yourslef Fitness
//else if (rws.type[1] == 0x1D && rws.type[2] == 0x00 && rws.type[3] == 0x0E)
//{
//	fseek(in, 16, 0);
//	fread((char*)rws.magic, 2, 1, in);
//	fseek(in, *rws.magic + 68, 0);
//}


// Max Payne || Party Blast
else //if (rws.type[5] == 0x03 && rws.type[6] == 0x18)
	fseek(in, *rws.magic - 36, 0);

fread(file.sr, 4, 1, in);

if (*file.sr >= 11025 && *file.sr <= 48000)
{
	for (int x = 0; x < CItem.Files; ++x)
	{
		FItem.SampleRate.push_back(*file.sr);
		FItem.Ext.push_back(".wav");
	}

fseek(in, ftell(in) + 8, 0);
fread(file.bits, 1, 1, in);
fread(file.chan, 1, 1, in);

if (*file.bits == 4 || *file.bits == 10 || *file.bits == 16)
{
	for (int x = 0; x < CItem.Files; ++x)
	{
	if (*file.chan <= 2 && *file.chan > 0)
		FItem.Channels.push_back(*file.chan);
		if (*file.bits == 4)
		{
			FItem.Bits.push_back(*file.bits);
			FItem.Format.push_back("xadpcm");
		}
			else if (*file.bits == 10 || *file.bits == 16)
			{
				if (*file.bits == 10) *file.bits = (16); // Fix
				FItem.Bits.push_back(*file.bits);
				FItem.Format.push_back("pcm");
			}
			if (FItem.SampleRate[x] == 22052)
				FItem.SampleRate[x] = 22050; //Fix
	}
}
else

{
	FItem.Ext.push_back("");
	FItem.Format.push_back("");
	FItem.Channels.push_back(0);
	FItem.Bits.push_back(0);
	FItem.SampleRate.push_back(0);
}

//Set Def Channels
//for (int x = 0; x < Files; ++x)
//{
//Channels[x] = 2;
//}

}
else
for (int x = 0; x < CItem.Files; ++x)
{
	FItem.Ext.push_back("");
	FItem.Format.push_back("");
	FItem.Channels.push_back(0);
	FItem.Bits.push_back(0);
	FItem.SampleRate.push_back(0);
}

//		//Get File Names
//		ContainsNames = 1;
//		u_char fName[24];
//		file.pos = 36;
//		for (int x = 0; x < CItem.Files; ++x, file.pos += 24)
//		{
//			fseek(in, file.pos, 0);
//			fread(fName, 24, 1, in);
//			FItem.FileName.push_back((char*)fName);
//			FItem.Ext[x] 	= ExtractFileExt(FItem.FileName[x]);
//		}

return true;
}


