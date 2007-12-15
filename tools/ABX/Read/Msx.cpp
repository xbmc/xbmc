


//=======================================================
//	Msx.cpp - MK Deception - (Simple audio container)
//=======================================================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
//#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"

//------------------------------------------------------------------------------

void Supported::MsxRead(FILE* in){

Supported file;

struct{
	u_short	headerSize[4];
}msx;

fseek(in, 4, 0);
fread(&CItem.Files, 4, 1, in);
--CItem.Files;

fread(msx.headerSize, 4, 1, in);

// Offsets / chunks
file.pos = 56;
for (int x = 0; x < CItem.Files; ++x, file.pos += 24)
{
	fseek(in, file.pos, 0); sys.PushOffset(in, 4); sys.PushChunk(in, 4);
	FItem.FileName.push_back(ExtractFileName(CItem.OpenFileName));
}

// Set offset 1
FItem.Offset[0] = *msx.headerSize;

// Audio specs
file.pos = ftell(in) + 12 + 20;
for (int x = 0; x < CItem.Files; ++x, file.pos += 40)
{
	fseek(in, file.pos, 0);
	fread(file.fmt, 2, 1, in);
	if (*file.fmt == 0x69) sys.PushAudioFormat("xadpcm");
	else sys.PushAudioFormat("pcm");

	sys.PushChannel(in, 2);
	sys.PushSampleRate(in, 4);

	fseek(in, ftell(in) +6, 0);
	fread(file.bits, 2, 1, in); sys.PushBits(*file.bits);
	sys.PushExtension(".wav");          
}

}

