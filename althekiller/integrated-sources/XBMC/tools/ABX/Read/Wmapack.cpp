


//===========================================================
//	Wmapack.cpp - Forza Motorsport - (Simple wma container)
//===========================================================

#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
//#include "../Gui.h"
#include "../SystemX.h"
//#include "../Helper.h"

//------------------------------------------------------------------------------

void Supported::WmapackRead(FILE* in){

#define MAXPACKFILENAME 44
Supported file;

fseek(in, 8, 0);
fread(&CItem.Files, 4, 1, in);

file.pos = 20;

for (int x = 0; x < CItem.Files; ++x, file.pos += 64)
{
	fseek(in, file.pos, 0);
	sys.PushFileName(in, MAXPACKFILENAME);
	

	sys.PushOffset(in, 4);
	sys.PushChunk(in, 4);
	//sys.PushAudioFormat("wma");
	sys.PushExtension(".wma");
}

sys.WmaSpecs(in, 0);

}

