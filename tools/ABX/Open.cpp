#include "stdafx.h" 

#include "Supported.h"
#include "FileSpecs.h"
#include "SystemX.h"
#include "Gui.h"
#include "Helper.h"
//#include "Main.h"
#include "Open.h"
//#include "MediaPlayer.h"
#include "FormatScan.h"
//#include "List.h"
#include "Strings.h"
//#include "Xct.h"
//#include "Settings.h"
//=============================
//		Open.cpp
//=============================

bool Open::DoOpen(const std::string& fileName){
	
	//if(Form1->ComboBufSize->Enabled)
	//	gui.ScxLblSetup();

	// Reset progressbar
	//gui.ProgressFxOff();

	CItem.ScxMode = false;
	CItem.Finished = false;
	//gui.ProgressReset();
	//Form1->ComboBufSize->Enabled = false;
	//Form1->ComboSmpRate->Enabled = true;
	//Form1->ComboFormat->Enabled = true;
	//Form1->ComboChan->Enabled = true;
	//Form1->ComboBits->Enabled = true;

	//if (Form1->ChkAutoEditClr->Checked)
	//	Form1->EditFolName->Clear();


	//Form1->BtnOpen->Update();
	//gui.CancelAllSet(false);
	sys.TimerStart();

	//int z = 0;
	CItem.OpenFileName = fileName;

	//FILE *in = fopen(CItem.OpenFileName.c_str(), "rb");

	//for (; z < Form1->OpenDialog->Files->Count && !gui.CancelAllGet(); ++z)
	//for (; z < 1000; ++z) // warning 1000 = hardcoded val
	//{
	
	//ProcessMessages();

	// Reset
	sys.ClearVariables();
	CItem.IniMatch = false;
	CItem.Error = false;
	CItem.Files = 0;

	//string s = ExtractFileName("c:\\filename.wmapack");
	//string dir = ExtractFileDir("c:\\stuff\\thedir\\filename.wmapack");
	//Msg(s);
	//Msg(dir);
	//Msg("debug");

	//CItem.OpenFileName = Form1->OpenDialog->Files->operator [](z);

	//CItem.DirName = "ExtractDir"; //HardCoded
	//sys.DirSet("C:\\Forza");  //HardCoded
	sys.DirNameSet();
	sys.DirPathSet();
	//Msg(CItem.DirPath);
	//Msg(CItem.DirName);
	FILE *in = fopen(CItem.OpenFileName.c_str(), "rb");

	if (!in){
		++CItem.IncompatileFmts;
		gui.Error("Couldn't Open: "
		+ ExtractFileName(CItem.OpenFileName), sys.Delay(false), in);
		//continue;
		return false;
	}


	//if (MediaPlayer.GetIsPlaying())
	//	CItem.FileSize = sys.GetFileSize(in, false);
	//else
		CItem.FileSize = sys.GetFileSize(in, true);


	if (CItem.FileSize == 0)  //128
	{
		++CItem.IncompatileFmts;
		gui.Error("Error: Zero Byte File", sys.Delay(false), in);
		//continue;
		return false;
	}

	//if (MediaPlayer.IsValidFormat(CItem.OpenFileName))
	//{
	//	fclose(in);


	//	if (!MediaPlayer.PlayListGetSize()){
	//		MediaPlayer.Stop();
	//		gui.ClearInfo();
	//	}

	//	if (ExtractFileExt(CItem.OpenFileName) == ".m3u"){
	//		Supported fmt;
	//		fmt.M3uRead();
	//	}
	//	else
	//		MediaPlayer.PlayListAddEntry(CItem.OpenFileName);

	//	if (!MediaPlayer.PlayListGetSize()){
	//		gui.ClearInfo();
	//	}

	//	continue;
	//}

	//string 	archiveVal 	= Form1->LblArchiveVal->Caption,
	//		audioVal	= Form1->LblAudioVal->Caption,
	//		sizeVal		= Form1->LblSizeVal->Caption;


	//if (MediaPlayer.GetIsPlaying() && !MediaPlayer.GetIsPaused()) // Pause if playing
	//	MediaPlayer.Pause();

	
	FormatScan fmtScan;

	// Format detect
	if (!fmtScan.Detect(in)){
		fclose(in);

		//if (MediaPlayer.GetIsPlaying() && !MediaPlayer.GetIsPaused()) // Pause if playing
		//	MediaPlayer.Pause();
		gui.Status("Unknown Format", 0);

		//continue;
		return false;
	}


	CItem.Loading = true;

	if (!sys.DirPathIsSet())
		sys.DirPathSetSilent(ExtractFileDir(CItem.OpenFileName) + "\\");


	// App caption
	//if (!MediaPlayer.GetIsPlaying()){  
		//gui.Status("Loading File..", 0, true);

		//if (Form1->OpenDialog->Files->Count > 1 && !MediaPlayer.PlayListGetSize())
		//	gui.Caption(ExtractFileName(CItem.OpenFileName) +
		//	  " (" + string(z +1)
		//	+ "/" + Form1->OpenDialog->Files->Count + ")" );
		//else
		//	gui.NameCaption(CItem.OpenFileName);
	//}

	// Parse
	if (!fmtScan.Parse(in) || CItem.Error || CItem.Files <= 0){

		//if (MediaPlayer.GetIsPlaying()){ // Revert progressbar
		//	gui.ProgressSetMax(MediaPlayer.GetDuration());
		//		if (MediaPlayer.GetIsPlaying() && MediaPlayer.GetIsPaused()) // UnPause if playing
		//			MediaPlayer.Pause();

		//	Form1->LblArchiveVal->Caption	= archiveVal;
		//	Form1->LblAudioVal->Caption		= audioVal;
		//	Form1->LblSizeVal->Caption		= sizeVal;
		//}

		fclose(in);
		//continue;
		return false;
	}

	//if (MediaPlayer.GetIsPlaying()){ // Revert progressbar
	//	gui.ProgressSetMax(MediaPlayer.GetDuration());
	//		if (MediaPlayer.GetIsPlaying()) // UnPause if playing
	//			MediaPlayer.Pause();
	//}


	// Close input stream as we are not using a global instance
	fclose(in);


	//MediaPlayer.PlayListClear();


	#ifndef DEBUG
	// If all read file chunks added together are greater than filesize
	// file wasn't read correctly
	__int64 total = 0;
	for (int x = 0; x < CItem.Files; ++x)
		total += FItem.Chunk[x];

	if (total > CItem.FileSize)
	{
		++CItem.IncompatileFmts;
		//gui.Error(" File wasn't read correctly", sys.Delay(false), in);
		//continue;
		return false;
	}
	#endif


	//// Give archives without extensions theresuch
	//if (!sys.GetOpenFileNameExtLen())
	//{
	//	RenameFile(CItem.OpenFileName, CItem.OpenFileName + "." + CItem.Format);
	//	CItem.OpenFileName = CItem.OpenFileName + "." + CItem.Format;
	//	Form1->OpenDialog->Files->operator [](z) = CItem.OpenFileName;
	//	if (Form1->OpenDialog->Files->Count > 1 && !MediaPlayer.PlayListGetSize())
	//		gui.Caption(ExtractFileName(CItem.OpenFileName) +
	//		" (File " + string(z +1)
	//		+ " of " + Form1->OpenDialog->Files->Count + ")" );
	//	else
	//		gui.NameCaption(CItem.OpenFileName);
	//}

	// Show num of files contained therein
	//Form1->LblFilesVal->Caption = CItem.Files;

	// Make filenames if none exist
	if (!CItem.HasNames)
	{
		gui.Status("Making Names", 0);
		int x = 0;
		while(x < CItem.Files)
		{
			FItem.FileName.push_back("");
			str.AutoNumList(0, FItem.FileName[x], x, CItem.Files);
			gui.Status("Naming: "+ FItem.FileName[x], 0);
			++x;
		}
		gui.Status("Finished Making Names", 0);
	}


	// Remove file ext's & lowercasify theresuch
	for (int x = 0; x < CItem.Files; ++x)
	{
		if (ExtractFileExt(FItem.FileName[x]) != "")
			FItem.FileName[x] = RemoveFileExt(FItem.FileName[x]);
		//gui.Status(FItem.FileName[x], 50);
		//Msg(CItem.Files);
		if (!FItem.Ext.empty()){
			FItem.Ext[x] = ToLower(FItem.Ext[x]);
		}
	}


	// Display audio type on AudioLabel (based on extension)
	//gui.ExtInfo(CItem.Files, FItem.Ext);

	// Make a copy of fileName vector array for reverting from renamed files
	FItem.FileNameBak = FItem.FileName;

	//// Xct - (Custom filename database format)
	//if (Form1->ChkXct->Checked)
	//{
	//	Xct xct; xct.FindMatch(CItem.DirName, CItem.IniMatch);
	//}
	//else if (Form1->EditRenFiles->Text != "")// Rename files according to editbox
	//{
	//	string text = Form1->EditRenFiles->Text;
	//	for (int x = 0; x < CItem.Files; ++x)
	//		FItem.FileName[x] =
	//		str.stringReplacer(text, x, CItem.Files);
	//}


	//// CleanNames
	//if (Form1->ChkTidyName->Checked && !CItem.IniMatch)
	//{
	//	int x = 0;
	//	while (x < CItem.Files)
	//	{
	//		str.StrClean(FItem.FileName[x]);
	//		++x;
	//	}
	//}


	//// Caption setup
	//if (CItem.IniMatch)
	//	gui.Caption(CItem.DirName);

	//// Form specs
	//if (Form1->Width != gui.MaxWidth && gui.AutoXpandGet() == true)
	//{
	//	Form1->Visible = false;
	//	gui.NormHeightMode(false);
	//	gui.MaxWidthMode();
	//	gui.CenterForm();
	//	Form1->Visible = true;
	//}

			  
	//if (Form1->ChkList->Checked)  //Form1->ChkAutoXpand->Checked
	//{
	//	Form1->List.Load();
	//}


	//if (Form1->OpenDialog->Files->Count == z + 1)
	//	CItem.Finished = true;


	CItem.Loading = false;

											
	//// No batch mode
	//if (Form1->OpenDialog->Files->Count == 1)
	//{
	//	gui.Status("File Loaded In " + string(sys.TimerStop()) + " Sec/s", 0, false);
	//}
	//else
	//{
	//	Supported fmt; fmt.StdExtract();
	//}
	
	//Supported fmt; fmt.StdExtract();

	//}// End main while batch loop

	CItem.Loading = false;
	//gui.Close();
	//if (CItem.Error && !MediaPlayer.PlayListGetSize() && !MediaPlayer.GetIsPlaying())
	if (CItem.Error)
	{
		//gui.ClearInfo();
		// UnCommentMeSoon
		//int compat = Form1->OpenDialog->Files->Count - CItem.IncompatileFmts;
		int compat = CItem.IncompatileFmts;	// FixMeSoon
		//gui.Caption("Finished");
		//gui.Error(string(compat) + " Compatible Format/s Found",
		gui.Error("Format Read Problem", 0, 0);
		sys.Delay(true);
		return false;
	}
	else
		return true;

	//CItem.IncompatileFmts = 0;

	//if (!gui.CancelAllGet() && Form1->OpenDialog->Files->Count != 1)
	//{
	//	gui.ProgressUpdate();
	//}


	//if (MediaPlayer.PlayListGetSize())
	//{
	//	Form1->OpenDialog->Files->Clear();
	//	if (!MediaPlayer.GetIsPlaying())  //needs a fix
	//		MediaPlayer.PlayListLoad();
	//}

					 
}
