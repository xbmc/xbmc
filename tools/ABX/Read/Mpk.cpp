


//=============================================================
//	Mpk.cpp - Mission Impossible - (Simple audio container)
//=============================================================



#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"

//------------------------------------------------------------------------------

void Supported::MpkRead(FILE* in){


Supported file;

struct
{
	u_char	fileName[100];
	u_int	fileNameIndex[4];
	u_int	fileNameIndexLen[4];
}mpk;



// Num of files
fseek(in, 8, 0);
fread(&CItem.Files, 4, 1, in);

// FileName index length
fseek(in, 12, 0);
fread(mpk.fileNameIndexLen, 4, 1, in);

// FileName index
fseek(in, 24, 0);
fread(mpk.fileNameIndex, 4, 1, in);
*mpk.fileNameIndex += 16;


// Offsets / sizes
file.pos = 16;
for (int x = 0; x < CItem.Files; ++x, file.pos += 12)
{
	fseek(in, file.pos, 0);
	sys.PushOffset(in, 4);
	sys.PushChunk(in, 4);
}

// FileNames
file.pos = *mpk.fileNameIndex;
for (int x = 0; x < CItem.Files; ++x)
{
	fseek(in, file.pos, 0);
	fgets((char*)mpk.fileName, sizeof(mpk.fileName), in);

	FItem.FileName.push_back((char*)mpk.fileName);
	FItem.Ext.push_back("." + ToLower(ExtractFileExt((char*)mpk.fileName)));

	file.pos += 8 + (long)strlen((char*)mpk.fileName) + 1;
	//file.pos += 8 + (long)strlen((char*)big.name) + 1;
}

sys.RiffSpecs(in, false);

}


