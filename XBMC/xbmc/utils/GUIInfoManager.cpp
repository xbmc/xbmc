#include "stdafx.h"
#include "GUIInfoManager.h"
#include "log.h"
#include "Weather.h"
#include "LocalizeStrings.h"
#include "../GUISettings.h"
#include "../Application.h"
#include "../Util.h"

// stuff for current song
#include "../filesystem/CDDADirectory.h"
#include "../url.h"
#include "../musicInfoTagLoaderFactory.h"
#include "../filesystem/SndtrkDirectory.h"

extern char g_szTitleIP[32];

CGUIInfoManager g_infoManager;

CGUIInfoManager::CGUIInfoManager(void)
{
}

CGUIInfoManager::~CGUIInfoManager(void)
{
}

/// \brief Obtains the label for the label control from whichever subsystem is needed
wstring CGUIInfoManager::GetLabel(const CStdString &strInfo)
{
	CStdString strLabel;
	CStdString strTest = strInfo;
	strTest = strTest.ToLower();
	if (strTest == "weather.conditions")
		strLabel = g_weatherManager.GetLabel(WEATHER_LABEL_CURRENT_COND);
	else if (strTest == "weather.temperature")
		strLabel = g_weatherManager.GetLabel(WEATHER_LABEL_CURRENT_TEMP);
	else if (strTest == "weather.location")
		strLabel = g_weatherManager.GetLabel(WEATHER_LABEL_LOCATION);
	else if (strTest == "system.time")
		return GetTime();
	else if (strTest == "system.date")
		return GetDate();
	else if (strTest.Left(11) == "musicplayer")
		strLabel = GetMusicLabel(strTest.Mid(12));
	else if (strTest.Left(16) == "system.freespace")
		return GetFreeSpace(strTest.Mid(17,1).ToUpper());
	else if (strTest == "network.ipaddress")
		strLabel = g_szTitleIP;
	// convert our CStdString to a wstring (which the label expects!)
	WCHAR szLabel[256];
	swprintf(szLabel,L"%S", strLabel.c_str() );
	wstring strReturn = szLabel;
	return strReturn;
}

/// \brief Obtains the filename of the image to show from whichever subsystem is needed
CStdString CGUIInfoManager::GetImage(const CStdString &strInfo)
{
	CStdString strTest = strInfo;
	strTest = strTest.ToLower();
	if (strTest == "weather.conditions")
		return g_weatherManager.GetCurrentIcon();
	if (strTest == "musicplayer.cover")
		return m_currentSong.GetThumbnailImage();
	return "";
}

wstring CGUIInfoManager::GetDate(bool bNumbersOnly)
{
	WCHAR szText[128];
	SYSTEMTIME time;
	GetLocalTime(&time);

	if (bNumbersOnly)
	{
		CStdString strDate;
		if (g_guiSettings.GetBool("LookAndFeel.SwapMonthAndDay"))
			swprintf(szText,L"%02.2i-%02.2i-%02.2i",time.wDay,time.wMonth,time.wYear);
		else
			swprintf(szText,L"%02.2i-%02.2i-%02.2i",time.wMonth,time.wDay,time.wYear);
	}
	else
	{
		const WCHAR* day;
		switch (time.wDayOfWeek)
		{
			case 1 :	day = g_localizeStrings.Get(11).c_str();	break;
			case 2 :	day = g_localizeStrings.Get(12).c_str();	break;
			case 3 :	day = g_localizeStrings.Get(13).c_str();	break;
			case 4 :	day = g_localizeStrings.Get(14).c_str();	break;
			case 5 :	day = g_localizeStrings.Get(15).c_str();	break;
			case 6 :	day = g_localizeStrings.Get(16).c_str();	break;
			default:	day = g_localizeStrings.Get(17).c_str();	break;
		}

		const WCHAR* month;
		switch (time.wMonth)
		{
			case 1 :	month= g_localizeStrings.Get(21).c_str();	break;
			case 2 :	month= g_localizeStrings.Get(22).c_str();	break;
			case 3 :	month= g_localizeStrings.Get(23).c_str();	break;
			case 4 :	month= g_localizeStrings.Get(24).c_str();	break;
			case 5 :	month= g_localizeStrings.Get(25).c_str();	break;
			case 6 :	month= g_localizeStrings.Get(26).c_str();	break;
			case 7 :	month= g_localizeStrings.Get(27).c_str();	break;
			case 8 :	month= g_localizeStrings.Get(28).c_str();	break;
			case 9 :	month= g_localizeStrings.Get(29).c_str();	break;
			case 10:	month= g_localizeStrings.Get(30).c_str();	break;
			case 11:	month= g_localizeStrings.Get(31).c_str();	break;
			default:	month= g_localizeStrings.Get(32).c_str();	break;
		}

		if (day && month)
		{
			if (g_guiSettings.GetBool("LookAndFeel.SwapMonthAndDay"))
				swprintf(szText,L"%s, %d %s", day, time.wDay, month);
			else
				swprintf(szText,L"%s, %s %d", day, month, time.wDay);
		}
		else
			swprintf(szText,L"no date");
	}
	wstring strText = szText;
	return strText;
}

wstring CGUIInfoManager::GetTime(bool bSeconds)
{
	WCHAR szText[128];
	SYSTEMTIME time;
	GetLocalTime(&time);

	INT iHour = time.wHour;

	if (g_guiSettings.GetBool("LookAndFeel.Clock12Hour"))
	{
		if (iHour>11)
		{
			iHour-=(12*(iHour>12));
			if (bSeconds)
				swprintf(szText,L"%2d:%02d:%02d PM", iHour, time.wMinute, time.wSecond);
			else
				swprintf(szText,L"%2d:%02d PM", iHour, time.wMinute);
		}
		else
		{
			iHour+=(12*(iHour<1));
			if (bSeconds)
				swprintf(szText,L"%2d:%02d:%02d AM", iHour, time.wMinute, time.wSecond);
			else
				swprintf(szText,L"%2d:%02d AM", iHour, time.wMinute);
		}
	}
	else
	{
		if (bSeconds)
			swprintf(szText,L"%2d:%02d:%02d", iHour, time.wMinute, time.wSecond);
		else
			swprintf(szText,L"%02d:%02d", iHour, time.wMinute);
	}
	wstring strText = szText;
	return strText;
}

CStdString CGUIInfoManager::GetMusicLabel(const CStdString &strItem)
{
	CMusicInfoTag& tag = m_currentSong.m_musicInfoTag;
	if (!tag.Loaded()) return "";
	if (strItem == "title") return tag.GetTitle();
	else if (strItem == "album") return tag.GetAlbum();
	else if (strItem == "artist") return tag.GetArtist();
	else if (strItem == "year") return tag.GetYear();
	else if (strItem == "genre") return tag.GetGenre();
	else if (strItem == "time") return GetCurrentPlayTime();
	else if (strItem == "timespeed")
	{
		CStdString strTime;
		if (g_application.GetPlaySpeed() != 1)
			strTime.Format("%s (%ix)", GetCurrentPlayTime().c_str(), g_application.GetPlaySpeed());
		else
			strTime = GetCurrentPlayTime();
		return strTime;
	}
	else if (strItem == "tracknumber")
	{
		CStdString strTrack;
		if (tag.GetTrackNumber()>0)
			strTrack.Format("%02i", tag.GetTrackNumber());
		return strTrack;
	}
	else if (strItem == "duration" && tag.GetDuration()>0)
	{
		CStdString strDuration;
		CUtil::SecondsToHMSString(tag.GetDuration(), strDuration);
		return strDuration;
	}
	return "";
}

CStdString CGUIInfoManager::GetVideoLabel(const CStdString &strItem)
{
	if (strItem == "time")
	{
		CStdString strTime;
		__int64 lPTS=10*g_application.m_pPlayer->GetTime();
		int hh = (int)(lPTS / 36000) % 100;
		int mm = (int)((lPTS / 600) % 60);
		int ss = (int)((lPTS /  10) % 60);
		if (hh >=1)
		{
			strTime.Format("%02.2i:%02.2i:%02.2i",hh,mm,ss);
		}
		else
		{
			strTime.Format("%02.2i:%02.2i",mm,ss);
		}
		return strTime;
	}
	return "";
}

CStdString CGUIInfoManager::GetCurrentPlayTime()
{
	CStdString strTime;
  __int64 lPTS=g_application.m_pPlayer->GetPTS() - (g_infoManager.GetCurrentSongStart()*10)/75;
	CUtil::SecondsToHMSString((long)(lPTS/10), strTime);
	return strTime;
}

void CGUIInfoManager::SetCurrentSong(const CFileItem &item)
{
	//	No audio file, we are finished here
	if (!CUtil::IsAudio(item.m_strPath) )
		return;

	m_currentSong = item;

	//	Get a reference to the item's tag
	CMusicInfoTag& tag=m_currentSong.m_musicInfoTag;
	CURL url(m_currentSong.m_strPath);
	// check if we don't have the tag already loaded
	if (!tag.Loaded())
	{
		//	if the file is a cdda track, ...
		if (url.GetProtocol()=="cdda" )
		{
			VECFILEITEMS  items;
			CCDDADirectory dir;
			//	... use the directory of the cd to 
			//	get its cddb information...
			if (dir.GetDirectory("D:",items))
			{
				for (int i=0; i < (int)items.size(); ++i)
				{
					CFileItem* pItem=items[i];
					if (pItem->m_strPath==m_currentSong.m_strPath)
					{
						//	...and find current track to use
						//	cddb information for display.
						m_currentSong=*pItem;
					}
				}
			}
			{
				CFileItemList itemlist(items);	//	cleanup everything
			}
		}
		else
		{
			//	we have a audio file.
			//	Look if we have this file in database...
			bool bFound=false;
			CSong song;
			if (g_musicDatabase.Open())
			{
				bFound=g_musicDatabase.GetSongByFileName(m_currentSong.m_strPath, song);
				g_musicDatabase.Close();
			}
			// always get id3 info for the overlay
			if (!bFound)// && g_stSettings.m_bUseID3)
			{
				//	...no, try to load the tag of the file.
				CMusicInfoTagLoaderFactory factory;
				auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(m_currentSong.m_strPath));
				//	Do we have a tag loader for this file type?
				if (NULL != pLoader.get())
				{
					// yes, load its tag
					if ( !pLoader->Load(m_currentSong.m_strPath,tag))
					{
						//	Failed!
						tag.SetLoaded(false);
						//	just to be sure :-)
					}
				}
			}
			else
			{
				//	...yes, this file is found in database
				//	fill the tag of our fileitem
				tag.SetSong(song);
			}
		}
	}

	//	If we have tag information, ...
	if (tag.Loaded())
	{
		if (!tag.GetTitle().size())
		{
			//	No title in tag, show filename only
			CSndtrkDirectory dir;
			char NameOfSong[64];
			if (dir.FindTrackName(item.m_strPath,NameOfSong))
				tag.SetTitle(NameOfSong);
			else
				tag.SetTitle( CUtil::GetFileName(m_currentSong.m_strPath) );
		}
	}	//	if (tag.Loaded())
	else 
	{
		//	If we have a cdda track without cddb information,...
		if ( url.GetProtocol()=="cdda" )
		{
			//	we have the tracknumber...
			int iTrack=tag.GetTrackNumber();
			if (iTrack >=1)
			{
				CStdString strText=g_localizeStrings.Get(435);	//	"Track"
				if (strText.GetAt(strText.size()-1) != ' ')
					strText+=" ";
				CStdString strTrack;
				strTrack.Format(strText+"%i", iTrack);
				tag.SetTitle(strTrack);
				tag.SetLoaded(true);
			}
		}	//	if (!tag.Loaded() && url.GetProtocol()=="cdda" )
		else
		{	// at worse, set our title as the filename
			tag.SetTitle( CUtil::GetFileName(m_currentSong.m_strPath) );
		}	// we now have at least the title
		tag.SetLoaded(true);
	}

	//	Find a thumb for this file.
	CUtil::SetMusicThumb(&m_currentSong);
	CUtil::FillInDefaultIcon(&m_currentSong);
}

wstring CGUIInfoManager::GetFreeSpace(const CStdString &strDrive)
{
	ULARGE_INTEGER lTotalFreeBytes;
	WCHAR wszHD[64];
	wstring strReturn;

	CStdString strDriveFind = strDrive + ":\\";
	const WCHAR *pszDrive=g_localizeStrings.Get(155).c_str();
	const WCHAR *pszFree=g_localizeStrings.Get(160).c_str();
	const WCHAR *pszUnavailable=g_localizeStrings.Get(161).c_str();
	if (GetDiskFreeSpaceEx( strDriveFind.c_str(), NULL, NULL, &lTotalFreeBytes))
	{
		swprintf(wszHD, L"%s %c: %u Mb ", pszDrive, strDrive[0], lTotalFreeBytes.QuadPart/1048576); //To make it MB
		wcscat(wszHD,pszFree);
	} 
	else
	{
		swprintf(wszHD, L"%s %c: ",pszDrive, strDrive[0]);
		wcscat(wszHD,pszUnavailable);
	}
	strReturn = wszHD;
	return strReturn;
}