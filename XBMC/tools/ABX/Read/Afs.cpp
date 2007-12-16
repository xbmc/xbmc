


//=============================
//		Afs.cpp - Multi
//=============================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"
#include "../Convert/Adx2wav.c"

//------------------------------------------------------------------------------

bool Supported::AfsRead(FILE* in){ //Old/crude code - Needs cleaning up

struct
{
	u_long	index[4];
	u_char	fileName[48];
}afs;

Supported file;


fseek(in, 4, 0);
fread(&CItem.Files, 4, 1, in);

fseek(in, 8, 0);
for (int x = 0; x < CItem.Files; ++x)
{
	fread(file.offset, 4, 1, in);
	fread(file.chunk, 4, 1, in);
	FItem.Offset.push_back(*file.offset);
	FItem.Chunk.push_back(*file.chunk);
}

// Get fileName index
fread(afs.index, 4, 1, in); // Check end of last offset pos
if (*afs.index < CItem.FileSize && *afs.index > 0)
{
	fseek(in, *afs.index, 0);
}
else // Check 8 bytes before 1st offset
{
	fseek(in, FItem.Offset[0] - 8, 0);
	fread(afs.index, 4, 1, in);
	if (*afs.index != 0) fseek(in, *afs.index, 0);

	else if (ExtractFileName (CItem.OpenFileName) == "AFS02.AFS")
	{
		fseek(in, 138225664, 0); // Marvel Vs. Capcom 2 fix
	}
	else
	{
		//gui.Error("Error: Afs fileName index not found", 1500, in);
		return false;
	}
}


// Read filenames
for (int x = 0; x < CItem.Files; ++x)
{
	//memset(afs.fileName, 48, 0x00);
	fread(afs.fileName, 48, 1, in);

	int pos = (int)string((char*)afs.fileName).find("/");

	if (pos == -1) 
	{
		FItem.Ext.push_back("." + ExtractFileExt(ToLower((char*)afs.fileName)));
		FItem.FileName.push_back(RemoveFileExt((char*)afs.fileName));
	}
	else // Fix for Soul Calibur II
	{
		FItem.FileName.push_back("");
		if (ExtractFileExt (FItem.FileName[x]) == "")
			FItem.Ext.push_back(".unknown");
		else
			FItem.Ext.push_back(ExtractFileExt(ToLower(FItem.FileName[x])));
		CItem.HasNames = 0;
	}
	
}

// Audio specs
u_char buf[16];
for (int x = 0; x < CItem.Files; ++x)
{	
	//Msg(x);
	//gui.Status(FItem.FileName[x], 0);

	if(FItem.Ext[x] == ".adx")
	{
		//gui.Status("adx", 0);
		fseek(in, FItem.Offset[x], 0);
		fread(buf, 16, 1, in);

		FItem.Channels.push_back((short)buf[7]);
		FItem.SampleRate.push_back((u_short)read_long(buf +8));
		FItem.Format.push_back("adx");
		FItem.Bits.push_back(16);
	}
	else
	{
		//gui.Status("misc", 0);
		sys.PushAudioFormat("");
		sys.PushChannel(NULL);
		sys.PushSampleRate(NULL);
		sys.PushBits(NULL);

	}
}

return true;
}

