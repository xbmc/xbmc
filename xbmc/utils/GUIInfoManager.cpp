#include "stdafx.h"
#include "GUIInfoManager.h"
#include "Weather.h"
#include "../Application.h"
#include "../Util.h"

// stuff for current song
#include "../filesystem/CDDADirectory.h"
#include "../musicInfoTagLoaderFactory.h"
#include "../filesystem/SndtrkDirectory.h"

#include "FanController.h"

extern char g_szTitleIP[32];

CGUIInfoManager g_infoManager;

CGUIInfoManager::CGUIInfoManager(void)
{
  m_lastSysHeatInfoTime = 0;
  m_fanSpeed = 0;
  m_gpuTemp = 0;
  m_cpuTemp = 0;
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
	else if (strTest.Left(11) == "videoplayer")
		strLabel = GetVideoLabel(strTest.Mid(12));
	else if (strTest.Left(16) == "system.freespace")
		return GetFreeSpace(strTest.Mid(17,1).ToUpper());
  else if (strTest == "system.cputemperature")
    return GetSystemHeatInfo("cpu");
  else if (strTest == "system.gputemperature")
    return GetSystemHeatInfo("gpu");
  else if (strTest == "system.fanspeed")
    return GetSystemHeatInfo("fan");
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
  {
    if (!g_application.IsPlayingAudio()) return "";
		return m_currentSong.HasThumbnail() ? m_currentSong.GetThumbnailImage() : "music.jpg";
  }
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
	else if (strItem == "timeremaining") return GetCurrentPlayTimeRemaining();
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
	else if (strItem == "duration")
	{
		CStdString strDuration = "00:00";
		if (tag.GetDuration()>0)
			CUtil::SecondsToHMSString(tag.GetDuration(), strDuration);
		else
		{
			unsigned int iTotal = g_application.m_pPlayer->GetTotalTime();
			if (iTotal>0)
				CUtil::SecondsToHMSString(iTotal, strDuration, true);
		}
		return strDuration;
	}
	return "";
}

CStdString CGUIInfoManager::GetVideoLabel(const CStdString &strItem)
{
  // TODO: Move the SetCurrentFile() stuff out of VideoOverlay and into here.
  CIMDBMovie *pMovie = g_application.GetCurrentMovie();
  CFileItem item = g_application.CurrentFileItem();
  if (strItem == "title")
  {
    if (pMovie)
      return pMovie->m_strTitle;
    else if (item.IsVideo() && !item.GetLabel().IsEmpty())
      return item.GetLabel();
    else
      return "";
  }
	else if (strItem == "time") return GetCurrentPlayTime();
	else if (strItem == "timeremaining") return GetCurrentPlayTimeRemaining();
	else if (strItem == "timespeed")
	{
		CStdString strTime;
		if (g_application.GetPlaySpeed() != 1)
			strTime.Format("%s (%ix)", GetCurrentPlayTime().c_str(), g_application.GetPlaySpeed());
		else
			strTime = GetCurrentPlayTime();
		return strTime;
	}
	else if (strItem == "duration")
	{
		CStdString strDuration = "00:00:00";
		unsigned int iTotal = g_application.m_pPlayer->GetTotalTime();
		if (iTotal > 0)
			CUtil::SecondsToHMSString(iTotal, strDuration, true);
		return strDuration;
	}
	return "";
}

CStdString CGUIInfoManager::GetCurrentPlayTime()
{
	CStdString strTime;
	if (g_application.IsPlayingAudio())
	{
		__int64 lPTS=g_application.m_pPlayer->GetTime() - (g_infoManager.GetCurrentSongStart()*(__int64)1000)/75;
    if (lPTS < 0) lPTS = 0;
		CUtil::SecondsToHMSString((long)(lPTS/1000), strTime);
	}
	else if (g_application.IsPlayingVideo())
	{
		__int64 lPTS=g_application.m_pPlayer->GetTime();
		CUtil::SecondsToHMSString((long)(lPTS/1000), strTime, true);
	}
	return strTime;
}

CStdString CGUIInfoManager::GetCurrentPlayTimeRemaining()
{
	CStdString strTime;
	if (g_application.IsPlayingAudio())
	{
		int iTotalTime=g_application.m_pPlayer->GetTotalTime();
		if (m_currentSong.m_musicInfoTag.GetDuration()>0)
			iTotalTime=m_currentSong.m_musicInfoTag.GetDuration();
		else if (iTotalTime<0)
			iTotalTime=0;

		__int64 lPTS=g_application.m_pPlayer->GetTime() - (g_infoManager.GetCurrentSongStart()*(__int64)1000)/75;
    if (lPTS < 0) lPTS = 0;
		int iReverse=iTotalTime-(int)(lPTS/1000);
		CUtil::SecondsToHMSString(iReverse, strTime);
	}
	else if (g_application.IsPlayingVideo())
	{
		int iTotalTime=g_application.m_pPlayer->GetTotalTime();
		if (iTotalTime<0)
			iTotalTime=0;

		int iReverse=iTotalTime-(int)(g_application.m_pPlayer->GetTime()/1000);
		CUtil::SecondsToHMSString(iReverse, strTime, true);
	}
	return strTime;
}

void CGUIInfoManager::SetCurrentSong(const CFileItem &item)
{
	//	No audio file, we are finished here
	if (!item.IsAudio())
		return;

	m_currentSong = item;

	//	Get a reference to the item's tag
	CMusicInfoTag& tag=m_currentSong.m_musicInfoTag;
	// check if we don't have the tag already loaded
	if (!tag.Loaded())
	{
		//	we have a audio file.
		//	Look if we have this file in database...
		bool bFound=false;
		if (g_musicDatabase.Open())
		{
			CSong song;
			bFound=g_musicDatabase.GetSongByFileName(m_currentSong.m_strPath, song);
			m_currentSong.m_musicInfoTag.SetSong(song);
			g_musicDatabase.Close();
		}

		if (!bFound)
		{
			// always get id3 info for the overlay
			CMusicInfoTagLoaderFactory factory;
			auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(m_currentSong.m_strPath));
			//	Do we have a tag loader for this file type?
			if (NULL != pLoader.get())
				pLoader->Load(m_currentSong.m_strPath,tag);
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
			if (dir.FindTrackName(m_currentSong.m_strPath,NameOfSong))
				tag.SetTitle(NameOfSong);
			else
				tag.SetTitle( CUtil::GetFileName(m_currentSong.m_strPath) );
		}
	}	//	if (tag.Loaded())
	else 
	{
		//	If we have a cdda track without cddb information,...
		if (m_currentSong.IsCDDA())
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
			tag.SetTitle( CUtil::GetTitleFromPath(m_currentSong.m_strPath) );
		}	// we now have at least the title
		tag.SetLoaded(true);
	}

	//	Find a thumb for this file.
	m_currentSong.SetMusicThumb();
	m_currentSong.FillInDefaultIcon();
}

wstring CGUIInfoManager::GetSystemHeatInfo(const CStdString &strInfo)
{
	if(timeGetTime() - m_lastSysHeatInfoTime >= 1000)
	{ // update our variables
		m_lastSysHeatInfoTime = timeGetTime();
    m_fanSpeed = CFanController::Instance()->GetFanSpeed();
    m_gpuTemp   = CFanController::Instance()->GetGPUTemp();
    m_cpuTemp  = CFanController::Instance()->GetCPUTemp();
	}
  if (strInfo == "cpu")
  {
    WCHAR CPUText[32];
    if(g_guiSettings.GetInt("Weather.TemperatureUnits") == 1 /*DEGREES_F*/)
      swprintf(CPUText, L"%s %2.2f%cF", g_localizeStrings.Get(140).c_str(), ((9.0 / 5.0) * m_cpuTemp) + 32.0, 176);	
    else
      swprintf(CPUText, L"%s %2.2f%cC", g_localizeStrings.Get(140).c_str(), m_cpuTemp, 176);	
    return CPUText;
  }
  else if (strInfo == "gpu")
  {
    WCHAR GPUText[32];
    if(g_guiSettings.GetInt("Weather.TemperatureUnits") == 1 /*DEGREES_F*/)
      swprintf(GPUText, L"%s %2.2f%cF", g_localizeStrings.Get(141).c_str(), ((9.0 / 5.0) * m_gpuTemp) + 32.0, 176);	
    else
      swprintf(GPUText, L"%s %2.2f%cC", g_localizeStrings.Get(141).c_str(), m_gpuTemp, 176);	
    return GPUText;
  }
  else if (strInfo == "fan")
  {
    WCHAR FanText[32];
    swprintf(FanText, L"%s %i%%", g_localizeStrings.Get(13300).c_str(), m_fanSpeed * 2);	
    return FanText;
  }
  return L"";
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
CStdString CGUIInfoManager::GetVersion()
{
	CStdString tmp = g_localizeStrings.Get(6).c_str();
	tmp = tmp.substr(18, tmp.size()-14);
	return tmp;
}
CStdString CGUIInfoManager::GetBuild()
{
	WCHAR wszDate[32];
	CStdString tmp;
	mbstowcs(wszDate, __DATE__, sizeof(wszDate));
	return wszDate;
}