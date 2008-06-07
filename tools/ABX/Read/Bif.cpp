


//===========================================================
//	Bif.cpp - Star Wars: KOTOR - (Simple container)
//===========================================================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
//#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"

//------------------------------------------------------------------------------

void Supported::BifRead(FILE* in){

Supported file;

fseek(in, 8, 0);
fread(&CItem.Files, 4, 1, in);

file.pos = 24;

for (int x = 0; x < CItem.Files; ++x, file.pos += 16)
{
	fseek(in, file.pos, 0);

	sys.PushOffset(in, 4);
	sys.PushChunk(in, 4);

	// Check format
	u_char fmt[4];
	memset(fmt, 4, 0x00);
	fseek(in, FItem.Offset[x], 0);
	fread(fmt, 4, 1, in);

	if (fmt[0] == 'R' && fmt[1] == 'I' && fmt[2] == 'F' && fmt[3] == 'F')
	//if (!strcmp((char*)fmt, "RIFF"))
		sys.PushExtension(".wav");
	else
		sys.PushExtension(".dat");
}

sys.RiffSpecs(in, false);

}


