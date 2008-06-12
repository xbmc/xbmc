


//=============================
//		FileSpecs.cpp
//=============================

#include "stdafx.h"

//#include "FileSpecs.h"

struct ContainerItem{

bool 		Finished,
			Loading,
			ScxMode,
			Extracting,
			IniMatch,
			HasNames,
			HasAudioInfo,
			HasFolders,
			NeedsWavHeader,
			Error;

int 		Files, IncompatileFmts, PlayableFmts;

u_long		FileSize;

string 		DirName,
			DirPath,
			OpenFileName,
			Format;
}CItem;


struct FileItem{

// Parts only used for map format
vector<short>	Channels,
				Bits,
				Parts;

vector<u_short>	SampleRate;

// OffsetXtra, ChunkXtra & Chunk1st only used for map format
vector<u_long>	Offset,
				Chunk,
				OffsetXtra,
				ChunkXtra,
				Chunk1st;

vector<string>	Format,
				FileName,
				FileNameBak,
				Ext;
}FItem;












