


//===========================================================
//	Pod.cpp - BloodRayne 2 - (Simple multi-format container)
//===========================================================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
//#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"

//------------------------------------------------------------------------------

void Supported::PodRead(FILE* in){

Supported file;

struct
{
	u_int 	index[4];
	u_int 	fileNameIndexSize[4];
	u_char	fileName[70];
}pod;


// Num of files
fseek(in, 88, 0);
fread(&CItem.Files, 4, 1, in);

// Index
fseek(in, 264, 0);
fread(pod.index, 4, 1, in);

// FileName index size
fseek(in, 272, 0);
fread(pod.fileNameIndexSize, 4, 1, in);


// Offsets / sizes
file.pos = *pod.index + 4;
for (int x = 0; x < CItem.Files; ++x, file.pos += 20)
{                              
	fseek(in, file.pos, 0);
	sys.PushChunk(in, 4);
	sys.PushOffset(in, 4);
}

// FileNames
file.pos = CItem.FileSize - *pod.fileNameIndexSize;
for (int x = 0; x < CItem.Files; ++x)
{
	fseek(in, file.pos, 0);
	fgets((char*)pod.fileName, sizeof(pod.fileName), in);

	file.pos += (long)strlen((char*)pod.fileName) + 1;
	FItem.FileName.push_back((char*)pod.fileName);
	FItem.Ext.push_back(ToLower("." + ExtractFileExt((char*)pod.fileName)));
}

sys.RiffSpecs(in, false);

}
