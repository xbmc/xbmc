


//==================================================
//	Piz.cpp - Mashed - (Simple non audio container)
//==================================================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
//#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"

//------------------------------------------------------------------------------

void Supported::PizRead(FILE* in){

struct
{
	u_char fileName[116];
}piz;

Supported file;

file.pos = 8;
fseek(in, file.pos, 0);
fread(&CItem.Files, 4, 1, in);

file.pos = 2048;
for (int x = 0; x < CItem.Files; ++x, file.pos += 128)
{
	fseek(in, file.pos, 0);
	fread(piz.fileName, 116, 1, in);
	fread(file.offset, 4, 1, in);
	fread(file.chunk, 4, 1, in);
	FItem.FileName.push_back((char*)piz.fileName);
	FItem.Ext.push_back("." + ExtractFileExt((char*)piz.fileName));
	FItem.Offset.push_back(*file.offset); 
	FItem.Chunk.push_back(*file.chunk);
	FItem.Format.push_back("");
	//FItem.FileName = ReplaceAll((char*)piz.fileName, "/", "\\")
}


}
