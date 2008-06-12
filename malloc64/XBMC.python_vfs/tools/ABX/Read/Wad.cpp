


//======================================================================
//	Wad.cpp - Disney's Extreme Skate Adventure - (Simple Wma container)
//======================================================================

#include "stdafx.h"
#include "../Supported.h"
#include "../helper.h"
//#pragma hdrstop
#include "../FileSpecs.h"
#include "../SystemX.h"
#include "../Gui.h"


//------------------------------------------------------------------------------

bool Supported::WadRead(){

Supported file;

FILE* in = fopen(ChangeFileExt(CItem.OpenFileName.c_str(),".dat").c_str(), "rb");

if (!in)
{
	gui.Error("Couldn't Find " + CItem.DirName + ".dat", sys.Delay(false), NULL);
	return false;
}

fread(&CItem.Files, 4, 1, in);

file.pos = 8;
for (int x = 0; x < CItem.Files; ++x, file.pos += 12)
{
	fseek(in, file.pos, 0);
	fread(file.offset, 4, 1, in);
	fread(file.chunk, 4, 1, in);
	FItem.Offset.push_back(*file.offset);
	FItem.Chunk.push_back(*file.chunk);
	FItem.Ext.push_back(".wma");
}

// Reset
fclose(in);
in = fopen(CItem.OpenFileName.c_str(), "rb");
if (!in)
{
	gui.Error("Couldn't Find " + CItem.DirName + ".wad", 1500, NULL);
	return false;
}

sys.WmaSpecs(in, false);
fclose(in);
return true;
}


