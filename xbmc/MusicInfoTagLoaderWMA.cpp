#include "musicinfotagloaderWMA.h"
#include "stdstring.h"
#include "sectionloader.h"

using namespace MUSIC_INFO;
using namespace XFILE;

CMusicInfoTagLoaderWMA::CMusicInfoTagLoaderWMA(void)
{
}

CMusicInfoTagLoaderWMA::~CMusicInfoTagLoaderWMA()
{
}

// Based on MediaInfo
// by Jérôme Martinez, Zen@MediaArea.net
// http://sourceforge.net/projects/mediainfo/
bool CMusicInfoTagLoaderWMA::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  tag.SetLoaded(false);
  CFile file;
  if (!file.Open(strFileName.c_str())) return false;

  unsigned char* Tout=new unsigned char[65536];
  file.Read(Tout, 65536);
  int Offset=0; //Offset debut
  unsigned int* ToutI;

  //Play time
  Offset=0;
  ToutI=(unsigned int*)Tout;
  while (!(ToutI[0]==0x75B22630 && ToutI[1]==0x11CF668E && ToutI[2]==0xAA00D9A6 && ToutI[3]==0x6CCE6200) && Offset<=65536-4)
  {
		Offset++;
		ToutI=(unsigned int*)(Tout+Offset);
  }
  if (Offset>65536-4)
		return false;

  //General(ZT("Format"))=ZT("WM");
  //General(ZT("Format_String"))=ZT("Windows Media");
  //General(ZT("Format_Extensions"))=ZT("ASF WMA WMV");

  //Play time
  Offset=0;
  ToutI=(unsigned int*)Tout;
  while (!(ToutI[0]==0x8CABDCA1 && ToutI[1]==0x11CFA947 && ToutI[2]==0xC000E48E && ToutI[3]==0x6553200C) && Offset<=65536-4)
  {
		Offset++;
		ToutI=(unsigned int*)(Tout+Offset);
  }
  if (Offset<=65536-4)
  {
		Offset+=64;
		ToutI=(unsigned int*)(Tout+Offset);
		float F1=(float)ToutI[1];
		F1=F1*0x10000*0x10000+ToutI[0];
		tag.SetDuration((long)((F1/10000)/1000));	//	from milliseconds to seconds
  }

  //Description  Title
  Offset=0;
  ToutI=(unsigned int*)Tout;
  while (!(ToutI[0]==0x75B22633 && ToutI[1]==0x11CF668E && ToutI[2]==0xAA00D9A6 && ToutI[3]==0x6CCE6200) && Offset<=65536-4)
  {
		Offset++;
		ToutI=(unsigned int*)(Tout+Offset);
  }
  if (Offset<=65536-4)
  {
		Offset+=24;
		int Taille0=Tout[Offset+0]+Tout[Offset+1]*0x100;
		int Taille1=Tout[Offset+2]+Tout[Offset+3]*0x100;
		int Taille2=Tout[Offset+4]+Tout[Offset+5]*0x100;
		int Taille3=Tout[Offset+6]+Tout[Offset+7]*0x100;

		Offset+=10;
		//Conversion Unicode
		for (int I2=0; I2<Taille0+Taille1+Taille2+Taille3; I2+=2)
				Tout[Offset+I2/2]=Tout[Offset+I2];

		//General(ZT("Title"))=wxString((char*)Tout+Offset,wxConvUTF8).c_str();
		tag.SetTitle(CStdString((char*)Tout+Offset));	// titel
		//General(ZT("Author"))=wxString((char*)Tout+Offset+Taille0/2,wxConvUTF8).c_str();
		tag.SetArtist(CStdString((char*)Tout+Offset+Taille0/2)); //	author
		//General(ZT("Copyright"))=wxString((char*)Tout+Offset+(Taille0+Taille1)/2,wxConvUTF8).c_str();
		//General(ZT("Comments"))=wxString((char*)Tout+Offset+(Taille0+Taille1+Taille2)/2,wxConvUTF8).c_str();
  }

	//	Maybe these information can be usefull in the future

  //Info audio
  //Offset=0;
  //ToutI=(unsigned int*)Tout;
  //while (!(ToutI[0]==0xF8699E40 && ToutI[1]==0x11CF5B4D && ToutI[2]==0x8000FDA8 && ToutI[3]==0x2B445C5F) && Offset<=65536-4)
  //{
		//Offset++;
		//ToutI=(unsigned int*)(Tout+Offset);
  //}
  //if (Offset<=65536-4)
  //{
		//Offset+=54;
		////Codec
		//TCHAR C1[30]; _itoa(Tout[Offset]+Tout[Offset+1]*0x100, C1, 16);
		//CStdString Codec=C1;
		//while (Codec.size()<4)
		//		Codec='0'+Codec;
		//Audio[0](ZT("Codec"))=Codec;
		//Audio[0](ZT("Channels"))=Tout[Offset+2]; //2 octets
		//ToutI=(unsigned int*)(Tout+Offset);
		//Audio[0](ZT("SamplingRate"))=ToutI[1];
		//Audio[0](ZT("BitRate"))=ToutI[2]*8;
  //}

  //Info video
  //Offset=0;
  //ToutI=(unsigned int*)Tout;
  //while (!(ToutI[0]==0xBC19EFC0 && ToutI[1]==0x11CF5B4D && ToutI[2]==0x8000FDA8 && ToutI[3]==0x2B445C5F) && Offset<=65536-4)
  //{
		//Offset++;
		//ToutI=(unsigned int*)(Tout+Offset);
  //}
  //if (Offset<=65536-4)
  //{
		//Offset+=54;
		//Offset+=15;
		//ToutI=(unsigned int*)(Tout+Offset);
		//Video[0](ZT("Width"))=ToutI[0];
		//Video[0](ZT("Height"))=ToutI[1];
		//Codec
		//unsigned char C1[5]; C1[4]='\0';
		//C1[0]=Tout[Offset+12+0]; C1[1]=Tout[Offset+12+1]; C1[2]=Tout[Offset+12+2]; C1[3]=Tout[Offset+12+3];
		//Video[0](ZT("Codec"))=wxString((char*)C1,wxConvUTF8).c_str();
  //}


  //Extended
  Offset=0;
  ToutI=(unsigned int*)Tout;
  while (!(ToutI[0]==0xD2D0A440 && ToutI[1]==0x11D2E307 && ToutI[2]==0xA000F097 && ToutI[3]==0x50A85EC9) && Offset<=65536-4)
  {
		Offset++;
		ToutI=(unsigned int*)(Tout+Offset);
  }

  if (Offset<=65536-4)
  {
    Offset+=24;
    int Nb=Tout[Offset]+Tout[Offset+1];
    Offset+=2;
    for (int Pos=0; Pos<Nb; Pos++)
    {
			int TailleNom=Tout[Offset]+Tout[Offset+1];
			Offset+=2;
			//Conversion Unicode
			for (int I2=0; I2<TailleNom; I2+=2)
					Tout[Offset+I2/2]=Tout[Offset+I2];
			CStdString Nom=(char*)Tout+Offset; // UTF8?
			Offset+=TailleNom;
			int Type=Tout[Offset]+Tout[Offset+1];
			Offset+=2;//2 octets du type
			int TailleValeur=Tout[Offset]+Tout[Offset+1];
			Offset+=2;
			//Gestion des type
			CStdString Valeur;
			if (Type==0) //Unicode
			{
					for (int I3=0; I3<TailleValeur; I3+=2)
							Tout[Offset+I3/2]=Tout[Offset+I3];
					Valeur=CStdString((char*)Tout+Offset); // UTF8?
			}
			else if (Type==1) //Byte
					Valeur=CStdString((char*)Tout+Offset); // UTF8?
			else if (Type==2)//Bool
					Valeur=(int)Tout[Offset];
			else if (Type==3)//DWord
					Valeur=Tout[Offset]+Tout[Offset+1]*0x100+Tout[Offset+2]*0x10000+Tout[Offset+3]*0x1000000;
			else if (Type==4)//QWord
					Valeur=Tout[Offset]+Tout[Offset+1]*0x100+Tout[Offset+2]*0x10000+Tout[Offset+3]*0x1000000;
			else if (Type==5)//Word
					Valeur=Tout[Offset]+Tout[Offset+1]*0x100;
			//Gestion des noms
			if (Nom=="WM/AlbumTitle")
				tag.SetAlbum(Valeur);
			else if (Nom=="WM/AlbumArtist")
			{
				if (tag.GetArtist().IsEmpty())
					tag.SetArtist(Valeur);
			}
			else if (Nom=="WM/TrackNumber")
			{
				if (tag.GetTrackNumber()<=0)
					tag.SetTrackNumber(Valeur[0]);
			}
			else if (Nom=="WM/Track")
			{
				//	Which Tracknumber is right, this or the above ?
				//if (tag.GetTrackNumber()<=0)
				//	tag.SetTrackNumber(Valeur[0]);
			}
			else if (Nom=="WM/Year")
			{
				SYSTEMTIME dateTime;
				dateTime.wYear=atoi(Valeur);
				tag.SetReleaseDate(dateTime);
			}
			else if (Nom=="WM/Genre")
				tag.SetGenre(Valeur);
//		else if (Nom=="isVBR"){}// && Valeur==ZT("1")) A VOIR
//               Audio_BitrateMode[0]=ZT("VBR");

			Offset+=TailleValeur;
		}
  }

	tag.SetLoaded(true);
  return true;
}
