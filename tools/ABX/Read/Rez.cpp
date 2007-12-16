


//=============================
//		Rez.cpp - Mojo!
//=============================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
//#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"

//------------------------------------------------------------------------------

void Supported::RezRead(FILE* in){

fseek(in, 108514184, 0);   //108514184, 108502184
CItem.Files = 87;

for (int x = 0; x < CItem.Files; ++x)
{
	sys.PushOffset(in, 4); sys.PushChunk(in, 4);

	if (FItem.Chunk[x] <= 0)
	{
		FItem.Offset.pop_back(); FItem.Chunk.pop_back();
		--CItem.Files, --x;
	}
	else FItem.Chunk[x] -= 90;
	fseek(in, ftell(in) + 16, 0);
}

for (int x = 0; x < CItem.Files; ++x)
{
	fseek(in, FItem.Offset[x], 0);
	sys.PushChannel(in, 4);
	sys.PushSampleRate(in, 4);

	FItem.Offset[x] += 32;

	if (FItem.Channels[x] == 0) FItem.Channels[x] = 2;
	else FItem.Channels[x] <<= 1; // Multiply by 2 (shl 1)

	sys.PushAudioFormat("xadpcm"); sys.PushExtension(".wav"); sys.PushBits(16);
}   

}



