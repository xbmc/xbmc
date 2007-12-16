


//=============================
//		FormatScan.cpp
//=============================

//#include <vcl.h>
#include "stdafx.h" 

#include "FormatScan.h"
#include "Supported.h"
#include "FileSpecs.h"
#include "Strings.h"
#include "SystemX.h"
#include "Gui.h"
#include "FormatHdrs.h"
#include "Helper.h"
//#pragma hdrstop

//#include "Main.h"
//#include "MediaPlayer.h"

//------------------------------------------------------------------------------

bool FormatScan::Detect(FILE* in){

gui.Status("Detecting Format", 0);
Xwb360 = false;
Offset = 0;
char Name[128];
strcpy(Name, ToLower(ExtractFileName(CItem.OpenFileName)).c_str());

//CItem.OpenFileName=str.(CItem.OpenFileName);
// UnCommentMeSoon
//Name = ToLower(ExtractFileName(CItem.OpenFileName));
// Fix me - how to change to lower case using std::string
// UnCommentMeSoon
Ext = ExtractFileExt(CItem.OpenFileName);
Ext = str.ToLower(Ext);
fseek(in, 0, 0);
fread(Header, 22, 1, in);

//------------------------------------------------------------------------------
// Adf - (GTA: Vice City)
if ( Ext == "adf" && Header[8] == 0x22 && Header[9] == 0x22 )
{
	CItem.Format = "adf"; //Form1->LblArchiveVal->Caption = "Adf";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
//------------------------------------------------------------------------------
// Adx - (Multi)
else if ( Ext == "adx" ) //Header[0] != 0x80 && Header[1] != 0x00
{
	CItem.Format = "adx"; //Form1->LblArchiveVal->Caption = "Adx";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
//------------------------------------------------------------------------------
// Afs - (Multi)
else if (  !memcmp(Header, "AFS", 3) && Ext == "afs" )
{
	CItem.Format = "afs"; //Form1->LblArchiveVal->Caption = "Afs";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
//------------------------------------------------------------------------------
// Arc - (Sims2)
else if ( Ext == "arc" )
{
	CItem.Format = "arc"; //Form1->LblArchiveVal->Caption = "Arc";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Asf/Mus - (Multi)
else if ( (Ext == "asf" || Ext == "mus") && Header[0] == 0x53 && Header[1] == 0x43
		&& Header[2] == 0x48 && Header[3] == 0x6C)
{
	CItem.Format = "mus"; //Form1->LblArchiveVal->Caption = "Mus";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Bif (Star Wars: KOTOR)
else if ( !memcmp(Header, "BIFFV1", 6) )
{
	CItem.Format = "bif"; //Form1->LblArchiveVal->Caption = "Bif";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
//------------------------------------------------------------------------------
// Big (CodeMasters) - (Colin McRae Rally 2005)
else if ( Header[9] == 0x8 && Header[13] == 0x8 && !memcmp(Header, "BIGF", 4) )
{
	CItem.Format = "bigcode"; //Form1->LblArchiveVal->Caption = "Big (CM)";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
//------------------------------------------------------------------------------
// Big (EA) - (Fight Night Round 2)
else if ( Header[9] == 0x0 && Header[13] == 0x0 && !memcmp(Header, "BIGF", 4) )
{
	CItem.Format = "bigea"; //Form1->LblArchiveVal->Caption = "Big (EA)";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
//------------------------------------------------------------------------------
#ifdef DEBUG
// Bor (Senile) - (Beats Of Rage)
else if ( !memcmp(Header, "BOR ", 4) )
{
	CItem.Format = "bor"; //Form1->LblArchiveVal->Caption = "Bor";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
#endif
//------------------------------------------------------------------------------
// Cab - (Forza)
else if ( Ext == "cab"
	&& Header[0] == 0xAA && Header[1] == 0xAA && Header[2] == 0xC0)
{                     
	CItem.Format = "cabforza"; //Form1->LblArchiveVal->Caption = "CabForza";
	CItem.HasNames = true, CItem.HasAudioInfo = false, CItem.NeedsWavHeader = false;
	CItem.HasFolders = true;
}
//------------------------------------------------------------------------------
// DatRoller (RollerCoaster Tycoon)
else if (Ext == "dat" && Header[0] == 0x30)
{
	CItem.Format = "datroller"; //Form1->LblArchiveVal->Caption = "DatRoller";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Fsb3 - Flatout 2,  BioShock (PC)
else if ( !memcmp(Header, "FSB3", 4) )
{
	CItem.Format = "fsb"; //Form1->LblArchiveVal->Caption = "Fsb3";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Hot- (Voodoo Vince)
else if ( !memcmp(Header, "HOT", 3) )
{
	fseek(in, 32, 0);
	fread(Header, 1, 1, in);
	if (Header[0] != 0x48)
	{
		//gui.Error("Error: Only audio hot containers are supported",
		sys.Delay(false);
		fclose(in);
		return false;
	}
	CItem.Format = "hot"; //Form1->LblArchiveVal->Caption = "Hot";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Hwx- (Star Wars Episode III: Revenge Of The Sith)
else if ( Ext == "hwx" )
{
	CItem.Format = "hwx"; //Form1->LblArchiveVal->Caption = "Hwx";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Lug - (Fable)
else if (Ext == "lug" )
{
	CItem.Format = "lug"; //Form1->LblArchiveVal->Caption = "Lug";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
	CItem.HasFolders = true;
}
//------------------------------------------------------------------------------
// Map - (Halo / Stubbs)
else if ( !memcmp(Header, "daeh", 4) )
{
	fseek(in, 4, 0);
	fread(Header, 1, 1, in);
	if (Header[0] == 0x7)
	{
		//gui.Error("Error: PC Halo maps arn't supported", sys.Delay(false), in);
		return false;
	}
	CItem.Format = "maphalo"; //Form1->LblArchiveVal->Caption = "Map (Halo)";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
	CItem.HasFolders = true;
}
//------------------------------------------------------------------------------
// Mpk - (Mission Impossible)
else if ( !memcmp(Header, "MPAK", 4) )
{
	CItem.Format = "mpk"; //Form1->LblArchiveVal->Caption = "Mpk";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
//------------------------------------------------------------------------------
// Msx - (MK Deception)
else if (Ext == "msx" ) //header[0] == 0x02
{
	CItem.Format = "msx"; //Form1->LblArchiveVal->Caption = "Msx";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// PakGoblin - (Goblin Commander)
else if (!memcmp(Header, "RIFF", 4) && !memcmp(Name, "audxbox", 4))
{
	CItem.Format = "pakgoblin"; //Form1->LblArchiveVal->Caption = "PakGob";
	gui.Status("detected", 0);
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
//------------------------------------------------------------------------------
#ifdef DEBUG
// PakDream - (Dreamfall: The Longest Journey)
else if ( !memcmp(Header, "tlj_", 4) )
{
	CItem.Format = "pakdream"; //Form1->LblArchiveVal->Caption = "PakDrm";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
#endif
//------------------------------------------------------------------------------
// Piz - (Mashed)
else if ( !memcmp(Header, "PIZ", 3) )
{
	CItem.Format = "piz"; //Form1->LblArchiveVal->Caption = "Piz";
	CItem.HasNames = true, CItem.HasAudioInfo = false, CItem.NeedsWavHeader = true;
	CItem.HasFolders = true;
}
//------------------------------------------------------------------------------
// Pod - (BloodRayne 2)
else if ( !memcmp(Header, "POD3", 4) )
{
	CItem.Format = "pod"; //Form1->LblArchiveVal->Caption = "Pod";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
	CItem.HasFolders = true;
}
//------------------------------------------------------------------------------
// Rcf (Radcore) - (Multi)
else if ( !memcmp(Header, "RADCORE", 4) )
{
	CItem.Format = "rcf"; //Form1->LblArchiveVal->Caption = "Rcf";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
	CItem.HasFolders = true;
}
//------------------------------------------------------------------------------
// Rez - (Mojo!)
else if ( Header[0] == 0x80 && Header[4] == 0x8C && Ext == "rez" )
{
	CItem.Format = "rez"; //Form1->LblArchiveVal->Caption = "Rez";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Rfa - (RalliSport Challenge)
else if ( !memcmp(Header, "Refractor", 6) ) //!memcmp(ext, ".rfa", 4)
{
	CItem.Format = "rfa"; //Form1->LblArchiveVal->Caption = "Rfa";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
	CItem.HasFolders = true;
}
//------------------------------------------------------------------------------
// Rws - (Multi)
else if ( Ext == "rws" )//else if (!memcmp(header, rws.header, 5))
{
	// Compatiblity Check
	if (Header[20] == 0x0E && Header[21] == 0x00)
	{
		//if ( !(header[0] == 0x09 && header[1] == 0x08) )
		//{
			//gui.Error("Error: Unsupported Rws Type", sys.Delay(false), in);
			return false;
		//}
	}
	//Contains names but not are yet being parsed
	CItem.Format = "rws"; //Form1->LblArchiveVal->Caption = "Rws";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;

	//if (header[20] == 0x0E)
	//	iRws.Read(in, 2);
	//else
}
//------------------------------------------------------------------------------
// Samp - (Namco Museum 50th Anniversary)
else if ( Header[0] == 0x69 && Header[1] == 0x00 && Ext == "samp" )
{
	CItem.Format = "samp"; //Form1->LblArchiveVal->Caption = "Samp";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Sh4 - (Silent Hill 4)
else if ( Header[0] == 0x53 && Header[1] == 0x48 && Header[2] == 0x34 )
{
	CItem.Format = "sh4"; //Form1->LblArchiveVal->Caption = "Sh4";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Sr - (Multi)
else if ( Header[15] == 0x20 && Header[16] == 0x20
		&& Ext == "sr" ) // && header[17] == 0x20
{
	CItem.Format = "sr"; //Form1->LblArchiveVal->Caption = "Sr";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Stx - (Kakuto Chojin)
else if ( !memcmp(Header, "STHD", 4) )
{
	CItem.Format = "stx"; //Form1->LblArchiveVal->Caption = "Stx";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Sxb/Vxb - (Ford Racing 3)
else if ( Ext == "sxb" || Ext == "vxb" )
{
	fseek(in, 4, 0);
	fread(&Offset, 4, 1, in);

	fseek(in, Offset, 0);
	fread(Header, 4, 1, in);

	if (memcmp(Header, "WBND", 4)){
		//gui.Error("Sxb/Vxb fault", sys.Delay(false), in);
		return false;
	}
	CItem.Format = "xwb"; //Form1->LblArchiveVal->Caption = "Xwb";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Wad - (Disney's Extreme Skate Adventure)
else if ( Ext == "wad" && !memcmp(Header, wma.header, 16) )
{
	CItem.Format = "wad"; //Form1->LblArchiveVal->Caption = "Wad";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
//------------------------------------------------------------------------------
// WavRaw - (Testing only -> Used to write headers on raw wav's)
#ifdef DEBUG
else if ( Ext == "eksz" )
{
	CItem.Format = "wavraw"; //Form1->LblArchiveVal->Caption = "Wav";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;

	//a_str(" (EkszBox).wav")

	//OpenFileName = OpenFileName.SetLength(OpenFileName.Length() - 5) +
	//" (EkszBox).wav";

	//Form1->OpenDialog->FileName = OpenFileName;
	//in = fopen(OpenFileName.c_str(), "rb");

	//EkszBox::Formats::Wav iWav; iWav.Read(in);

	//FileSize = sys.GetFileSize(in);

	//gui.Status("Finished Processing", 0, true);
	//return false;
}
#endif
//------------------------------------------------------------------------------
// Wmapack - (Forza Motorsport)
else if ( Header[0] == 0x01 && Header[5] == 0x08 && Ext == "wmapack" )
{
	CItem.Format = "wmapack"; //Form1->LblArchiveVal->Caption = "Wmapack";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
//------------------------------------------------------------------------------
// Wpx - (Capcom Vs Snk 2 EO)
else if ( Ext == "wpx" && Header[2] == 0x00 )
{
	CItem.Format = "wpx"; //Form1->LblArchiveVal->Caption = "Wpx";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Xbp - (Multi)
else if ( Ext == "xbp" )
{
	CItem.Format = "xbp"; //Form1->LblArchiveVal->Caption = "Xbp";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
}
//------------------------------------------------------------------------------
//#ifdef DEBUG
//// Xpr - (Multi)
//else if  ( !memcmp(header, "XPR1", 4) ) //!memcmp(ext, ".xwc", 4)
//{
//	CItem.Format = "xpr"; Form1->LblArchiveVal->Caption = "Xpr";
//	CItem.HasNames=1, CItem.HasAudioInfo=0, CItem.NeedsWavHeader=0;
//	EkszBox::Formats::Xpr iXpr; iXpr.Read(in);
//}
//#endif
//------------------------------------------------------------------------------
#ifdef DEBUG
// Xiso - (Multi)
else if ( Ext == "iso" )
{
	fseek(in, 65536, 0);
	fread(Header, 20, 1, in);

	// This works backwards for some reason??
	if ( memcmp(Header, "MICROSOFT*XBOX*MEDIA", 20) )
	{
		//gui.Error("Unsupported Iso Type", sys.Delay(false), in);
		return false;
	}


	//gui.Error("Xiso support disabled until it's more stable",
	//sys.Delay(false), NULL);
	//return false;

	CItem.Format = "xiso"; //Form1->LblArchiveVal->Caption = "Xbox Iso";
	CItem.HasNames = true, CItem.HasAudioInfo = false, CItem.NeedsWavHeader = false;
	CItem.HasFolders = true;
	//Supported fmt;
	//if (fmt.XisoRead(in) == -1) return false;
}
#endif
//------------------------------------------------------------------------------
// Xwb - (Multi)
else if ( Ext == "xwb" || !memcmp(Header, "WBND", 4) )
{

if (memcmp(Header, "WBND", 4) && memcmp(Header, "DNBW", 4))
{
	//gui.Status("Scanning For Xwb Header", 0, true);

	// Scan for xbox xwb signature
	Offset = sys.HeaderScan(0, sys.GetFileSize(in, false), "WBND", 4, in);

	// Xwb signature found
	if (Offset == -1)
	{
		//gui.Status("Scanning For Xwb360 Header", 0, true);
		Offset = sys.HeaderScan(0, sys.GetFileSize(in, false), "DNBW", 4, in);
		if (Offset != -1)
		{
			Xwb360 = true;
		}
		else
		{
			//gui.Error("Xwb header not found", sys.Delay(false), in);
			return false;
		}
	}
}
else if (!memcmp(Header, "DNBW", 4)) Xwb360 = true;

	CItem.Format = "xwb"; //Form1->LblArchiveVal->Caption = "Xwb";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Xwc - (Chronicles Of Riddick: Escape From Butcher Bay)
else if  ( !memcmp(Header, "MOS DATA", 5) ) //!memcmp(ext, ".xwc", 4)
{
	if ( Ext == "xtc")
	{
		//gui.Error("Incompatible Format", sys.Delay(false), in);
		return false;
	}
	CItem.Format = "xwc"; //Form1->LblArchiveVal->Caption = "Xwc";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
#ifdef DEBUG
// Sz - (OutRun 2006 - Coast 2 Coast)
else if ( Ext == "sz" )
{
	CItem.Format = "sz"; //Form1->LblArchiveVal->Caption = "Sz";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
#endif
//------------------------------------------------------------------------------
// Zsm - (Multi)
else if  ( !memcmp(Header, "ZSNDXBOX", 8) )
{
	CItem.Format = "zsm"; //Form1->LblArchiveVal->Caption = "Zsm";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Zwb- ((Unknown)
else if ( Ext == "zwb" )
{
	CItem.Format = "zwb"; //Form1->LblArchiveVal->Caption = "Zwb";
	CItem.HasNames = false, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = true;
}
//------------------------------------------------------------------------------
// Misc formats
else
{

#ifdef DEBUG
// Misc - (Multi)
if ( Ext == "csc" )
{
	CItem.Format = "csc"; //Form1->LblArchiveVal->Caption = "Csc";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
	return true;
}
#endif
//------------------------------------------------------------------------------
// Wav - (Multi)
#ifndef DEBUG
	if (!memcmp(Header, "RIFF", 4) && Header[8] == 0x57 && Ext != "wav")
#endif
#ifdef DEBUG
	if (!memcmp(Header, "RIFF", 4) && Header[8] == 0x57)
#endif
		//&& memcmp(Ext, ".wav", 4) && memcmp(Ext, ".csc", 4)
{
	CItem.Format = "wav"; //Form1->LblArchiveVal->Caption = "Wav";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
	return true;
}
//------------------------------------------------------------------------------
// Wma - (Multi)  (Used for WmaSpecs testing)
#ifndef DEBUG
	else if (!memcmp(Header, wma.header, 16) && Ext != "wma")
#endif
#ifdef DEBUG
	else if (!memcmp(Header, wma.header, 16))
#endif
{
	CItem.Format = "wma"; //Form1->LblArchiveVal->Caption = "Wma";
	CItem.HasNames = true, CItem.HasAudioInfo = true, CItem.NeedsWavHeader = false;
	return true;
}
//------------------------------------------------------------------------------
// Gta5 - (Gta: San Andreas)
u_char chk;
fseek(in, 7984, 0);
fread(&chk, 1, 1, in);

fseek(in, 8016, 0);
fread(Header, 4, 1, in);
if ( !memcmp(Header, "OggS", 4) && chk == 0xFF )
{
	CItem.Format = "gta5"; //Form1->LblArchiveVal->Caption = "Gta5";
	CItem.HasNames = false, CItem.HasAudioInfo = false, CItem.NeedsWavHeader = false;
	return true;
}
//--------------------------------------------------------------------------
else  // Incompatible
{
	// UnCommentMeSoon
	//if (Form1->ChkAutoScx->Checked)
	//{
	//	CItem.Format = "unknown"; //Form1->LblArchiveVal->Caption = "Unknown";
	//	CItem.HasNames = true, CItem.HasAudioInfo = false, CItem.NeedsWavHeader = false;
	//	return true;
	//   }

	++CItem.IncompatileFmts;
	// UnCommentMeSoon
	//if (!MediaPlayer.PlayListGetSize()){ //test
	//	gui.Error("Incompatible Format, try using SCX", sys.Delay(true), in);
	//	gui.ProgressReset();
	//}
	return false;
}
//--------------------------------------------------------------------------
}

gui.Status("Recognized Format Found: " + CItem.Format, 0);
return true;

}
//End FormatScan::Detect
//--------------------------------------------------------------------------


// FormatScan - Parse
//-------------------

bool FormatScan::Parse(FILE* in){

Supported fmt;
gui.Status("Reading Format.." , 0);

if (CItem.Format == "adf")
	fmt.AdfRead(in);
//else if (CItem.Format == "adx")
//	fmt.AdxRead(in);
else if (CItem.Format == "afs" && !fmt.AfsRead(in))
	return false;
//else if (CItem.Format == "arc")
//	fmt.ArcRead(in);
//else if (CItem.Format == "mus" && !fmt.AsfMusRead(in))
//	return false;
else if (CItem.Format == "bif")
	fmt.BifRead(in);
else if (CItem.Format == "bigcode")
	fmt.BigCodeRead(in);
else if (CItem.Format == "bigea" && !fmt.BigEaRead(in))
	return false;
//#ifdef DEBUG
//else if (CItem.Format == "bor")
//	fmt.BorRead(in);
//#endif
//else if (CItem.Format == "cabforza")
//	fmt.CabForzaRead(in);
else if (CItem.Format == "datroller")
	fmt.DatRollerRead(in);
else if (CItem.Format == "fsb")
	fmt.FsbRead(in);
else if (CItem.Format == "hot")
	fmt.HotRead(in);
else if (CItem.Format == "hwx")
	fmt.HwxRead(in);
else if (CItem.Format == "lug")
	fmt.LugRead();
//else if (CItem.Format == "maphalo" && !fmt.MapHaloRead(in))
//	return false;
else if (CItem.Format == "mpk")
	fmt.MpkRead(in);
else if (CItem.Format == "msx")
	fmt.MsxRead(in);
else if (CItem.Format == "pakgoblin" && !fmt.PakGoblinRead(in))
	return false;
//#ifdef DEBUG
//else if (CItem.Format == "pakdream")
//	fmt.PakDreamRead(in);
//#endif
else if (CItem.Format == "piz")
	fmt.PizRead(in);
else if (CItem.Format == "pod")
	fmt.PodRead(in);
else if (CItem.Format == "rcf")
	fmt.RcfRead(in);
else if (CItem.Format == "rez")
	fmt.RezRead(in);
else if (CItem.Format == "rfa")
	fmt.RfaRead(in);
else if (CItem.Format == "rws" && !fmt.RwsRead(in))
	return false;
else if (CItem.Format == "samp" && !fmt.SampRead())
	return false;
//else if (CItem.Format == "sh4")
//	fmt.Sh4Read(in);
//else if (CItem.Format == "sr")
//	fmt.SrRead();
//else if (CItem.Format == "stx")
//	fmt.StxRead(in);
//#ifdef DEBUG
//else if (CItem.Format == "sz" && !fmt.XwbRead(in, Offset, Xwb360))
//	return false;
//#endif
else if (CItem.Format == "wad" && !fmt.WadRead())
	return false;
//#ifdef DEBUG
//else if (CItem.Format == "wavraw")
//	fmt.WavRawRead(in);
//#endif
//else if (CItem.Format == "wav" && !fmt.WavRead(in))
//	return false;
//else if (CItem.Format == "wmapack")
if (CItem.Format == "wmapack")
	fmt.WmapackRead(in);
//else if (CItem.Format == "wpx")
//	fmt.WpxRead(in);
//else if (CItem.Format == "xbp" && !fmt.XbpRead())
//	return false;
//#ifdef DEBUG
//else if (CItem.Format == "xiso" && !fmt.XisoRead(in))
//	return false;
//#endif
//else if (CItem.Format == "xwb" && !fmt.XwbRead(in, Offset, Xwb360))
//	return false;
//else if (CItem.Format == "xwc")
//	fmt.XwcRead(in);
//else if (CItem.Format == "zsm")
//	fmt.ZsmRead(in);
//else if (CItem.Format == "zwb" && !fmt.XwbRead(in, 4, 0))
//	return false;


//// Miscellaneous
//#ifdef DEBUG
//else if (CItem.Format == "csc" && !fmt.MiscRead())
//	return false;
//#endif
//else if (CItem.Format == "wma" && !fmt.WmaRead(in)) {
//	return false;
//}
//else if (CItem.Format == "gta5" && !fmt.Gta5Read())
//	return false;
//
//else if (CItem.Format == "unknown" && !fmt.MiscRead())
//	return false;

return true;

}
