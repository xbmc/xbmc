


//================================================
//	DatRoller.cpp - RollerCoaster Tycoon (-)
//================================================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
//#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"

//------------------------------------------------------------------------------

void Supported::DatRollerRead(FILE* in){

fseek(in, 0, SEEK_SET);
fread(&CItem.Files, 4, 1, in);

for (int x = 0; x < CItem.Files; ++x)
	sys.PushOffset(in, 4);

for (int x = 0; x < CItem.Files; ++x)
{
	fseek(in, FItem.Offset[x], 0);
	sys.PushChunk(in, 4);

	fseek(in, FItem.Offset[x] + 6, 0);
	sys.PushChannel(in, 2);

	sys.PushSampleRate(in, 4);

	u_char bits;
	fseek(in, FItem.Offset[x] + 18, 0);
	fread(&bits, 1, 1, in);
	FItem.Bits.push_back(bits);

	if (bits == 16)
		sys.PushAudioFormat("pcm");
	else if (bits == 4)
		sys.PushAudioFormat("xadpcm");
	
	FItem.Ext.push_back(".wav");
	FItem.Offset[x] += 22;
}

}


