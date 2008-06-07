


//=============================================================
//		StdExtract.cpp - Standard/Generic file extraction
//=============================================================

#include "stdafx.h"
#include <exception> // Try / Catch
#include "Supported.h"
#include "SystemX.h"
#include "FileSpecs.h"
#include "Helper.h"
#include "Gui.h"
//#include "Main.h"
//#include "Extract.h"

//#include "Adx2wav.c"
//#include "ConvertStx.h"
//#include "MediaPlayer.h"
//#include "LowLevel.h"
//#include "Strings.h"
//#include "ZlibPipe.c"
//#include <shlwapi.h> // PathFileExists - Won't link


// Audio files are written in scattered block form within map files,
// hence the need for an addon function
//bool Supported::MapExtract(int i, u_char* buffer, FILE* in, FILE* out){
//
//fseek(in, FItem.Offset[i], 0);
//
//if (FItem.Parts[i] > 1)
//{
//	int xPos = 0, a = 0;
//	while (a < i)
//	{
//		xPos += (FItem.Parts[a]);
//		++a;
//	}
//	fread(buffer, FItem.Chunk1st[i], 1, in);
//	fwrite(buffer, FItem.Chunk1st[i], 1, out);
//	sys.MemoryDel();
//
//	for (int z = 0; z < FItem.Parts[i]; ++z, ++xPos)
//	{
//		if (! (buffer = sys.MemoryAlloc(FItem.ChunkXtra[xPos], out)) )
//		{
//			fclose(in);
//			sys.ExtractError(
//			FItem.FileName[i] + FItem.Ext[i],sys.DirGet() + CItem.DirName);
//			return false;
//		}
//
//		fseek(in, FItem.OffsetXtra[xPos], 0);
//		fread(buffer, FItem.ChunkXtra[xPos], 1, in);
//		fwrite(buffer, FItem.ChunkXtra[xPos], 1, out);
//		sys.MemoryDel();
//
//		// Tmp fix/hack - Certain files will be too long without this
//		if (FItem.ChunkXtra[xPos] < 65520) break;
//	}
//
//}// End if parts < y
//else
//{
//	fread(buffer, FItem.Chunk[i], 1, in);
//	fwrite(buffer, FItem.Chunk[i], 1, out);
//	sys.MemoryDel();
//}
//
//return true;
//
//}


bool Supported::StdExtract(const int& pFileNum){ // Needs cleaning up

//if (MediaPlayer.GetIsPlaying())  // No Extract support if file is playing
//	return false;

if (pFileNum > CItem.Files || pFileNum == 0) 
	return false;

int filesChecked = 0;
int* checked = new int [CItem.Files];
for (int x = 0; x < CItem.Files; ++x)
{
	if (x == pFileNum-1 || pFileNum == -1)
	{
		checked[filesChecked++] = x;
	}

}

//if (Form1->ChkList->Checked)
//{
//int x = 0;
//	while (x < CItem.Files)
//	{
//		if (Form1->ListView1->Items->operator [](x)->Checked)
//		{
//			checked[filesChecked++] = x;
//		}
//	++x;
//	}
//	if (filesChecked <= 0) return false;
//}
//else
//{
//int x = 0;
//	while (x < CItem.Files)
//	{
//		checked[x++] = x;
//	}
//filesChecked = CItem.Files;
//}


// Make Dir
//if (Form1->EditFolName->Text != "") CItem.DirName = Form1->EditFolName->Text;

string dirName = sys.DirPathGet() + "\\" + CItem.DirName + "\\"; 
//					| c:\xbox\	| xwb	=	c:\xbox\xwb

//Msg(sys.DirGet());
//Msg(CItem.DirName);
//Msg(dirName);

//if (!DirectoryExists(dirName.c_str()))
if (!CreateDirectory(dirName.c_str(), NULL))
{
	//if (mkdir(dirName.c_str()) == -1)
	//if (_mkdir(dirName.c_str()) == -1)
	//if (!CreateDir(dirName))
	gui.Error("Couldn't Make Directory: " + dirName, sys.Delay(true), 0);
	//return false;
}


FILE* in = fopen(CItem.OpenFileName.c_str(), "rb");
if (in == NULL)
{
	//sys.ExtractError(CItem.OpenFileName, sys.DirGet() + CItem.DirName);
	//gui.Error
	//(
	//"Couldn't Open: " + ExtractFileName(CItem.OpenFileName), sys.Delay(true), in
	//);
	gui.Error("Couldn't Open: " + ExtractFileName(CItem.OpenFileName), 0, in);
	return false;
}


bool
autoOn = 1, pcmOn = 0, xadpcmOn = 0,
chanAuto = 1, smpRateAuto = 1, bitsAuto = 1;
//autoOn = 0, pcmOn = 0, xadpcmOn = 0,
//chanAuto = 0, smpRateAuto = 0, bitsAuto = 0;
//if (CItem.NeedsWavHeader)
//{
//	if (Form1->ComboChan->Text == "Auto")
//		chanAuto = 1;
//	if (Form1->ComboSmpRate->Text == "Auto")
//		smpRateAuto = 1;
//	if (Form1->ComboBits->Text == "Auto")
//		bitsAuto = 1;
//}

//if (Form1->ComboFormat->Text == "PCM")
//	pcmOn = 1;
//else if (Form1->ComboFormat->Text == "XADPCM")
//	xadpcmOn  = 1;
//else //if (Form1->ComboFormat->Text == "Auto")
//	autoOn = 1;


//gui.ProgressStepSize((float) 100 / filesChecked);
//gui.Open("Extracting/Converting Files", "", "Writing...");
gui.Status("Extracting/Converting Files", 0);


// Extract files
int files;
if (pFileNum == -1)
	files = CItem.Files;
else
	files = 1;

//static 
int extracted = 0;

for (int x = 0, i; x < files; // && ExtractDlg->Showing
++x, ++extracted) // && !feof(in)
//++x, ++extracted, ++filesTotal) // && !feof(in)
{
	
	//Application->ProcessMessages();
	i = checked[x];

	string name = dirName + FItem.FileName[i] + FItem.Ext[i];

	gui.Status(name, 0);

	if ((FItem.Format[i] == "adx" && pcmOn) 
		|| (FItem.Format[i] == "adx" && CItem.Format == "adx"))
		name = ChangeFileExt(name, ".wav");

	if (name.length() >= MAX_PATH)
	{
		//_rmdir(String(name.c_str());
		RemoveDirectory(name.c_str());
		//RemoveDir(name);
		//gui.Error("Error: Max path (259) limit reached", sys.Delay(false), NULL);
		continue;
	}

	//gui.ProgressCaptionBasic(FItem.FileName[i] + FItem.Ext[i], false);
	//gui.ProgressStep();



	if (CItem.HasFolders || FItem.FileName[i].find("\\"))
		sys.BuildDirStruct(name.c_str());

	//ShowMessage(name);

	FILE* out = fopen(name.c_str(), "wb");
	if (!out){
		//gui.Error("Error: Output file could not be created", sys.Delay(false), out);
		continue;
	}
	//ShowMessage(name);
	
	try{
		if (CItem.NeedsWavHeader)
		{
			//if (!chanAuto)
			//{
			//	if (Form1->ComboChan->Text == "Mono") FItem.Channels[i] = 1;
			//	else FItem.Channels[i] = 2;
			//}
			//if (!smpRateAuto)
			//	FItem.SampleRate[i] = Form1->ComboSmpRate->Text.ToInt();


		// Pcm header
		if (pcmOn || (autoOn && FItem.Format[i] == "pcm") )
		{
			//if (!bitsAuto) FItem.Bits[i] = Form1->ComboBits->Text.ToInt();
			sys.WritePcmHeader
			(
				FItem.Chunk[i], FItem.Channels[i],
				FItem.Bits[i], FItem.SampleRate[i], out
			);
		}
		// Xadpcm header
		else if ( FItem.Format[i] == "xadpcm" && (xadpcmOn || autoOn) )
		{
			sys.WriteXadpcmHeader
			(
				FItem.Chunk[i], FItem.Channels[i], FItem.SampleRate[i], out
			);
		}


		}// End write header

		//if (CItem.Format == "stx")
		//{       
		//	bool surround = CItem.Files > 1;
		//	StxToXbadpcm(in, out, FItem.Channels[x], FItem.Offset[x], surround, false);
		//	fclose(out);
		//	//gui.ProgressReset();
		//	continue;
		//}
		//else if ((FItem.Format[i] == "adx" && pcmOn) // Adx to pcm - container format
		//|| (FItem.Format[i] == "adx" && CItem.Format == "adx")) // Adx to pcm
		//{
		//	adx2wav(FItem.Channels[i],FItem.SampleRate[i],FItem.Offset[i],in, out);
		//	fclose(out);
		//	continue;
		//}
		//else if (CItem.Format == "cabforza")
		//{
		//	fseek(in, FItem.Offset[i], 0);
		//	decomp(in, out);
		//	fclose(out);
		//	continue;
		//}

		////const int BUFSIZE = BUFSIZ;
		////const int& BUFSIZE = FItem.Chunk[i];
		u_char* buffer;
					   
		if (! (buffer = (u_char*) malloc(FItem.Chunk[i])) )
		//if (! (buffer = sys.MemoryAlloc(FItem.Chunk[i], out)) )
		{
			sys.ExtractError
			(
				FItem.FileName[i] + FItem.Ext[i], sys.DirPathGet() + CItem.DirName
			);
			fclose(out);
			continue;
		}

		//if (CItem.Format == "maphalo"){
		//	if (!MapExtract(i, buffer, in, out)) return false;
		//	fclose(out);
		//	continue;
		//}

		fseek(in, FItem.Offset[i], 0);
		fread(buffer, FItem.Chunk[i], 1, in);

		// Vice city decryption
		if (CItem.Format == "adf")
		{
			int bufSize = FItem.Chunk[i] -1; // Use a copy as value is changed
			XORBUFC(bufSize, buffer, 0x22); // LowLevel.cpp/h
		}

		fwrite(buffer, FItem.Chunk[i], 1, out);
		free(buffer);
		buffer = NULL;

	}
	// End try

	catch(exception &e)
	{
		exception f = e;
		remove(CItem.OpenFileName.c_str());
		--extracted;
	};

	fclose(out);

}
// End main while loop


delete [] checked;
fclose(in);


              
//if (CItem.Finished)
//{
	double time = sys.TimerStop();
	gui.Status("Finished Processing", 0);
	//std::cout<<"Finished in" + string(time);
//	Form1->DoubleBuffered = true;
//	if (Form1->ChkAutoGuiClr->Checked){
//		gui.ClearInfo();
//		sys.ClearVariables();
//	}
//	gui.FinishDisplay(extracted, filesTotal, time, "Saved", "Finished");
//	gui.NameCaption("Finished Processing");
//	Form1->DoubleBuffered = false;
//}

return true;
}

