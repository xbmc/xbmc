


//=====================================
//		Adf.cpp - GTA: Vice City (PC)
//=====================================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
//#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"

//------------------------------------------------------------------------------

void Supported::AdfRead(FILE* in){
           
sys.PushFiles(1);
sys.PushAudioFormat("mp3");
sys.PushOffset(0);
sys.PushChunk(sys.GetFileSize(in, false));
sys.PushFileName(ExtractFileName(CItem.OpenFileName));
sys.PushExtension(".mp3");
sys.PushChannel(2);
sys.PushSampleRate(44100);
sys.PushBits(16);   

}




