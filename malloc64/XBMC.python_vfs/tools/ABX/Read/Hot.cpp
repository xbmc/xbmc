


//====================================================
//	Hot.cpp - Voodoo Vince - (Messy audio container)
//====================================================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
//#include "Gui.h"
#include "../SystemX.h"
#include "../Helper.h"

//------------------------------------------------------------------------------

void Supported::HotRead(FILE* in){


#define MAXHOTFILENAMELEN 18
Supported file;

struct{
	int 	specsIndex;
	int 	filenameIndex;
	int 	firstOffset;
	u_char 	filename[MAXHOTFILENAMELEN];
	int 	filenameOffset;
	short	type;
}hot;


fseek(in, 8, 0);
fread(&hot.specsIndex, 4, 1, in);
fread(&hot.firstOffset, 4, 1, in);

//fseek(in, 32, 0);
//fread(&hot.type, 2, 1, in);

fseek(in, 20, 0);
fread(&hot.filenameIndex, 4, 1, in);
fread(&CItem.Files, 4, 1, in);


// Offsets
file.pos = 48;
for (int x = 0; x < CItem.Files; ++x, file.pos += 24)
{
	fseek(in, file.pos, 0);
	sys.PushOffset(in, 4);
}


// Filenames

// First one
fseek(in, hot.filenameIndex, 0);
fgets((char*)hot.filename, MAXHOTFILENAMELEN, in);
sys.PushFileName((char*)hot.filename);

// Rest
file.pos = 52;
for (int x = 0; x < CItem.Files -1; ++x, file.pos += 24)
{
	*hot.filename = 0x00;
	fseek(in, file.pos, 0);
	fread(&hot.filenameOffset, 4, 1, in);

	fseek(in, hot.filenameIndex + hot.filenameOffset, 0);
	fgets((char*)hot.filename, MAXHOTFILENAMELEN, in);
	sys.PushFileName((char*)hot.filename);
}


// Audio specs
//if (hot.type == 0x72)
//{
file.pos = hot.specsIndex;
for (int x = 0; x < CItem.Files; ++x, file.pos += 72)
{
fseek(in, file.pos + 68, 0);
fread(file.chunk, 4, 1, in);
FItem.Chunk.push_back(*file.chunk + 32 + 14);

fseek(in, file.pos + 20, 0);
fread(file.fmt, 2, 1, in);

if (*file.fmt == 0x69)
	sys.PushAudioFormat("xadpcm");
else if (*file.fmt == 0x01)
	sys.PushAudioFormat("pcm");

sys.PushChannel(in, 2);
sys.PushSampleRate(in, 4);

fseek(in, ftell(in) +6, 0);
fread(file.bits, 2, 1, in);
sys.PushBits(*file.bits);

sys.PushExtension(".wav");
}
//}
//else if (hot.type == 0x00)
//{
//
//for (int x = 0; x < Files; ++x)
//{
//Chunk.push_back(Offset[x+1] - Offset[x]);
//AudioFormat.push_back(NULL);
//Bits.push_back(NULL);
//Channels.push_back(NULL);
//SampleRate.push_back(NULL);
//sys.PushExtension(ExtractFileExt(FileName[x]));
//}
//Chunk.pop_back();
//Chunk.push_back(FileSize - Offset[Files - 1]);
//
//}


}


