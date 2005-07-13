
#include "../stdafx.h"
#include "Weather.h"
#include "../FileSystem/ZipManager.h"
#include "../FileSystem/RarManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define SPEED_KMH 0
#define SPEED_MPH 1
#define SPEED_MPS 2

#define DEGREES_C 0
#define DEGREES_F 1

#define CONTROL_BTNREFRESH  2
#define CONTROL_SELECTLOCATION 3
#define CONTROL_LABELLOCATION 10
#define CONTROL_LABELUPDATED 11
#define CONTROL_IMAGELOGO  101

#define CONTROL_IMAGENOWICON 21
#define CONTROL_LABELNOWCOND 22
#define CONTROL_LABELNOWTEMP 23
#define CONTROL_LABELNOWFEEL 24
#define CONTROL_LABELNOWUVID 25
#define CONTROL_LABELNOWWIND 26
#define CONTROL_LABELNOWDEWP 27
#define CONTROL_LABELNOWHUMI 28

#define CONTROL_STATICTEMP  223
#define CONTROL_STATICFEEL  224
#define CONTROL_STATICUVID  225
#define CONTROL_STATICWIND  226
#define CONTROL_STATICDEWP  227
#define CONTROL_STATICHUMI  228

#define CONTROL_LABELD0DAY  31
#define CONTROL_LABELD0HI  32
#define CONTROL_LABELD0LOW  33
#define CONTROL_LABELD0GEN  34
#define CONTROL_IMAGED0IMG  35

#define DEGREE_CHARACTER  (char)176 //the degree 'o' character

#define PARTNER_ID    "1004124588"   //weather.com partner id
#define PARTNER_KEY    "079f24145f208494"  //weather.com partner key

#define MAX_LOCATION   3
#define LOCALIZED_TOKEN_FIRSTID   370
#define LOCALIZED_TOKEN_LASTID   395
#define LOCALIZED_TOKEN_FIRSTID2 1396
#define LOCALIZED_TOKEN_LASTID2   1410 
/*
FIXME'S
>strings are not centered
>weather.com dev account is mine not a general xbmc one
*/

/*const CStdString strBasePath = "Q:\\weather\\";
const bool bUseZip = false;
const bool bUseRar = false;
const CStdString strZipFile = "";
const CStdString strRarFile = "";*/

// FOR ZIP
/*const CStdString strBasePath = "Z:\\weather\\";
const bool bUseZip = true;
const bool bUseRar = false;
const CStdString strZipFile = "Q:\\weather\\weather.zip";
const CStdString strRarFile = "Q:\\weather\\weather.rar";*/

// OR THESE FOR RAR
const CStdString strBasePath = "Z:\\weather\\";
const bool bUseZip = false;
const bool bUseRar = true;
const CStdString strZipFile = "";
const CStdString strRarFile = "Q:\\media\\weather.rar";

CWeather g_weatherManager;

CBackgroundWeatherLoader::CBackgroundWeatherLoader(CWeather *pCallback, int iArea)
{
  m_pCallback = pCallback;
  m_iArea = iArea;
  m_bImagesOkay = false;
  CThread::Create(true);
}

CBackgroundWeatherLoader::~CBackgroundWeatherLoader()
{}

void CBackgroundWeatherLoader::Process()
{
  if (m_pCallback)
  {
    // Download our weather
    CLog::Log(LOGINFO, "WEATHER: Downloading weather");
    CHTTP httpUtil;
    CStdString strURL;

    char c_units; //convert from temp units to metric/standard
    if (g_guiSettings.GetInt("Weather.TemperatureUnits") == DEGREES_F) //we'll convert the speed later depending on what thats set to
      c_units = 's';
    else
      c_units = 'm';

    CStdString strSetting;
    strSetting.Format("Weather.AreaCode%i", m_iArea + 1);
    strURL.Format("http://xoap.weather.com/weather/local/%s?cc=*&unit=%c&dayf=4&prod=xoap&par=%s&key=%s",
                  g_guiSettings.GetString(strSetting), c_units, PARTNER_ID, PARTNER_KEY);
    CStdString strWeatherFile = "Z:\\curWeather.xml";
    if (httpUtil.Download(strURL, strWeatherFile))
    {
      CLog::Log(LOGINFO, "WEATHER: Weather download successful");
      if (!m_bImagesOkay)
      {
        if (bUseZip)
          g_ZipManager.ExtractArchive(strZipFile,strBasePath);
        else if (bUseRar)
          g_RarManager.ExtractArchive(strRarFile,strBasePath);
        m_bImagesOkay = true;
      }
      m_pCallback->LoadWeather(strWeatherFile);
    }
    else
      CLog::Log(LOGERROR, "WEATHER: Weather download failed!");

    // extract 
    
    // and we now die
    m_pCallback->LoaderFinished();
  }
}

CWeather::CWeather(void)
{
  m_bBusy = false;
  for (int i = 0; i < MAX_LOCATION; i++)
  {
    strcpy(m_szLocation[i], "");
  }
  // empty all our strings etc.
  strcpy(m_szLastUpdateTime, "");
  //strcpy(m_szCurrentIcon, "Q:\\weather\\128x128\\na.png");
  /*strcpy(m_szCurrentIcon,strBasePath.c_str());
  strcat(m_szCurrentIcon,"128x128\\na.png");*/
  strcpy(m_szCurrentIcon,"");
  strcpy(m_szCurrentConditions, "");
  strcpy(m_szCurrentTemperature, "");
  strcpy(m_szCurrentFeelsLike, "");

  strcpy(m_szCurrentWind, "");
  strcpy(m_szCurrentHumidity, "");
  strcpy(m_szCurrentUVIndex, "");
  strcpy(m_szCurrentDewPoint, "");

  //loop here as well
  for (int i = 0; i < NUM_DAYS; i++)
  {
    //strcpy(m_dfForcast[i].m_szIcon, "Q:\\weather\\64x64\\na.png");
    /*strcpy(m_dfForcast[i].m_szIcon,strBasePath.c_str());
    strcat(m_dfForcast[i].m_szIcon,"64x64\\na.png");*/
    strcat(m_dfForcast[i].m_szIcon,"");
    strcpy(m_dfForcast[i].m_szOverview, "");
    strcpy(m_dfForcast[i].m_szDay, "");
    strcpy(m_dfForcast[i].m_szHigh, "");
    strcpy(m_dfForcast[i].m_szLow, "");
  }
  for (int i = 0; i < MAX_LOCATION; i++)
  {
    strcpy(m_szLocation[i], "");
  }
  m_iCurWeather = 0;
  m_pBackgroundLoader = NULL;
  srand(timeGetTime());
  m_lRefreshTime = 0;
}

CWeather::~CWeather(void)
{
}

void CWeather::GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue, const CStdString& strDefaultValue)
{
  strcpy(szValue, "");
  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    CStdString strValue = pChild->FirstChild()->Value();
    if (strValue.size() )
    {
      if (strValue != "-")
        strcpy(szValue, strValue.c_str());
    }
  }
  if (strlen(szValue) == 0)
  {
    strcpy(szValue, strDefaultValue.c_str());
  }
}

void CWeather::GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue)
{
  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    iValue = atoi( pChild->FirstChild()->Value() );
  }
}

void CWeather::LocalizeOverviewToken(char *szToken, bool bAppendSpace)
{
  CStdString strLocStr = "";
  WCHAR wszText[1024];
  swprintf(wszText, L"%S", szToken );
  if (wcslen(wszText) > 0)
  {
    wstring strText = wszText;
    ilocalizedTokens i;
    i = m_localizedTokens.find(strText);
    if (i != m_localizedTokens.end())
    {
      strLocStr = g_localizeStrings.Get(i->second);
    }
  }
  if (strLocStr == "")
    strLocStr = szToken; //if not found, let fallback
  if (bAppendSpace)
    strLocStr += " ";     //append space if applicable
  strcpy(szToken, strLocStr.GetBuffer(strLocStr.GetLength()));
}

void CWeather::LocalizeOverview(char *szStr)
{
  char loc[256];
  char szToken[256];
  int intOffset = 0;
  char *pnt = NULL;
  memset(loc, '\0', sizeof(loc));

  while ((pnt = strstr(szStr + intOffset, " ")) != NULL)
  {
    //get the length of this token (everything before pnt)
    int iTokenLen = (int)(strlen(szStr) - strlen(pnt) - intOffset);
    strncpy(szToken, szStr + intOffset, iTokenLen); //extract the token
    szToken[iTokenLen] = '\0';      //stick an end on it
    LocalizeOverviewToken(szToken);     //localize
    strcpy(loc + strlen(loc), szToken);    //add it to the end of loc
    intOffset += iTokenLen + 1;      //update offset for next strstr search
  }
  strncpy(szToken, szStr + intOffset, strlen(szStr) - intOffset); //last word, copy the rest of the string
  szToken[strlen(szStr) - intOffset] = '\0';     //stick an end on it
  LocalizeOverviewToken(szToken);        //localize
  strcpy(loc + strlen(loc), szToken);       //add it to the end of loc
  strcpy(szStr, loc);           //copy loc over the original input string
  SplitLongString(szStr, 7, 15);    //split to 2 lines if needed
}

int CWeather::ConvertSpeed(int curSpeed)
{
  //we might not need to convert at all
  if ((g_guiSettings.GetInt("Weather.TemperatureUnits") == DEGREES_C && g_guiSettings.GetInt("Weather.SpeedUnits") == SPEED_KMH) ||
      (g_guiSettings.GetInt("Weather.TemperatureUnits") == DEGREES_F && g_guiSettings.GetInt("Weather.SpeedUnits") == SPEED_MPH) )
    return curSpeed;

  //got through that so if temp is C, speed must be MPH or m/s
  if (g_guiSettings.GetInt("Weather.TemperatureUnits") == DEGREES_C)
  {
    if (g_guiSettings.GetInt("Weather.SpeedUnits") == SPEED_MPS)
      return (int)(curSpeed * (1000.0 / 3600.0) + 0.5);  //m/s
    else
      return (int)(curSpeed / (8.0 / 5.0));  //mph
  }
  else
  {
    if (g_guiSettings.GetInt("Weather.SpeedUnits") == SPEED_MPS)
      return (int)(curSpeed * (8.0 / 5.0) * (1000.0 / 3600.0) + 0.5);  //m/s
    else
      return (int)(curSpeed * (8.0 / 5.0));  //kph
  }
}

bool CWeather::LoadWeather(const CStdString &strWeatherFile)
{
  int iTmpInt;
  char iTmpStr[256];
  char szUnitTemp[2];
  char szUnitSpeed[5];
  SYSTEMTIME time;

  GetLocalTime(&time); //used when deciding what weather to grab for today

  // Load in our tokens if necessary
  if (!m_localizedTokens.size())
    LoadLocalizedToken();

  // load the xml file
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strWeatherFile))
  {
    CLog::Log(LOGERROR, "WEATHER: Unable to get data - invalid XML");
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (!pRootElement)
  {
    CLog::Log(LOGERROR, "WEATHER: Unable to get data - invalid XML");
    return false;
  }

  //if root element is 'error' display the error message
  if (strcmp(pRootElement->Value(), "error") == 0)
  {
    char szCheckError[256];
    GetString(pRootElement, "err", szCheckError, "Unknown Error"); //grab the error string
    CLog::Log(LOGERROR, "WEATHER: Unable to get data: %s", szCheckError);
    return false;
  }

  // units (C or F and mph or km/h or m/s)
  if (g_guiSettings.GetInt("Weather.TemperatureUnits") == DEGREES_C)
    strcpy(szUnitTemp, "C");
  else
    strcpy(szUnitTemp, "F");

  if (g_guiSettings.GetInt("Weather.SpeedUnits") == SPEED_MPH)
    strcpy(szUnitSpeed, "mph");
  else if (g_guiSettings.GetInt("Weather.SpeedUnits") == SPEED_KMH)
    strcpy(szUnitSpeed, "km/h");
  else
    strcpy(szUnitSpeed, "m/s");

  // location
  TiXmlElement *pElement = pRootElement->FirstChildElement("loc");
  if (pElement)
  {
    GetString(pElement, "dnam", m_szLocation[m_iCurWeather], "");
  }

  //current weather
  pElement = pRootElement->FirstChildElement("cc");
  if (pElement)
  {
    GetString(pElement, "lsup", m_szLastUpdateTime, "");

    GetString(pElement, "icon", iTmpStr, ""); //string cause i've seen it return N/A
    if (strcmp(iTmpStr, "N/A") == 0)
    {
      sprintf(m_szCurrentIcon, "%s128x128\\na.png",strBasePath.c_str());
    }
    else
      sprintf(m_szCurrentIcon, "%s128x128\\%s.png", strBasePath.c_str(),iTmpStr);

    GetString(pElement, "t", m_szCurrentConditions, "");   //current condition
    LocalizeOverview(m_szCurrentConditions);

    GetInteger(pElement, "tmp", iTmpInt);    //current temp
    sprintf(m_szCurrentTemperature, "%i%c%s", iTmpInt, DEGREE_CHARACTER, szUnitTemp);
    GetInteger(pElement, "flik", iTmpInt);    //current 'Feels Like'
    sprintf(m_szCurrentFeelsLike, "%i%c%s", iTmpInt, DEGREE_CHARACTER, szUnitTemp);

    TiXmlElement *pNestElement = pElement->FirstChildElement("wind"); //current wind
    if (pNestElement)
    {
      GetInteger(pNestElement, "s", iTmpInt);   //current wind strength
      iTmpInt = ConvertSpeed(iTmpInt);    //convert speed if needed
      GetString(pNestElement, "t", iTmpStr, "N");  //current wind direction

      //From <dir eg NW> at <speed> km/h   g_localizeStrings.Get(407)
      //This is a bit untidy, but i'm fed up with localization and string formats :)
      CStdString szWindFrom = g_localizeStrings.Get(407);
      CStdString szWindAt = g_localizeStrings.Get(408);
      CStdString szCalm = g_localizeStrings.Get(1410);

      if (strcmp(iTmpStr,"CALM") == 0)
        sprintf(m_szCurrentWind, "%s", szCalm.c_str());
      else
        sprintf(m_szCurrentWind, "%s %s %s %i %s",
              szWindFrom.GetBuffer(szWindFrom.GetLength()), iTmpStr,
              szWindAt.GetBuffer(szWindAt.GetLength()), iTmpInt, szUnitSpeed);
    }

    GetInteger(pElement, "hmid", iTmpInt);    //current humidity
    sprintf(m_szCurrentHumidity, "%i%%", iTmpInt);

    pNestElement = pElement->FirstChildElement("uv"); //current UV index
    if (pNestElement)
    {
      GetInteger(pNestElement, "i", iTmpInt);
      GetString(pNestElement, "t", iTmpStr, "");
      LocalizeOverviewToken(iTmpStr, false);
      sprintf(m_szCurrentUVIndex, "%i %s", iTmpInt, iTmpStr);
    }

    GetInteger(pElement, "dewp", iTmpInt);    //current dew point
    sprintf(m_szCurrentDewPoint, "%i%c%s", iTmpInt, DEGREE_CHARACTER, szUnitTemp);

  }
  //future forcast
  pElement = pRootElement->FirstChildElement("dayf");
  if (pElement)
  {
    TiXmlElement *pOneDayElement = pElement->FirstChildElement("day");;
    for (int i = 0; i < NUM_DAYS; i++)
    {
      if (pOneDayElement)
      {
        strcpy(m_dfForcast[i].m_szDay, pOneDayElement->Attribute("t"));
        LocalizeDay(m_dfForcast[i].m_szDay);

        GetString(pOneDayElement, "hi", iTmpStr, ""); //string cause i've seen it return N/A
        if (strcmp(iTmpStr, "N/A") == 0)
          strcpy(m_dfForcast[i].m_szHigh, "");
        else
          sprintf(m_dfForcast[i].m_szHigh, "%s%c%s", iTmpStr, DEGREE_CHARACTER, szUnitTemp);

        GetString(pOneDayElement, "low", iTmpStr, "");
        if (strcmp(iTmpStr, "N/A") == 0)
          strcpy(m_dfForcast[i].m_szHigh, "");
        else
          sprintf(m_dfForcast[i].m_szLow, "%s%c%s", iTmpStr, DEGREE_CHARACTER, szUnitTemp);

        TiXmlElement *pDayTimeElement = pOneDayElement->FirstChildElement("part"); //grab the first day/night part (should be day)
        if (i == 0 && (time.wHour < 7 || time.wHour >= 19)) //weather.com works on a 7am to 7pm basis so grab night if its late in the day
          pDayTimeElement = pDayTimeElement->NextSiblingElement("part");

        if (pDayTimeElement)
        {
          GetString(pDayTimeElement, "icon", iTmpStr, ""); //string cause i've seen it return N/A
          if (strcmp(iTmpStr, "N/A") == 0)
            sprintf(m_dfForcast[i].m_szIcon, "%s64x64\\na.png",strBasePath.c_str());
          else
            sprintf(m_dfForcast[i].m_szIcon, "%s64x64\\%s.png", strBasePath.c_str(),iTmpStr);

          GetString(pDayTimeElement, "t", m_dfForcast[i].m_szOverview, "");
          LocalizeOverview(m_dfForcast[i].m_szOverview);
        }
      }
      pOneDayElement = pOneDayElement->NextSiblingElement("day");
    }
  }
  return true;
}

//splitStart + End are the chars to search between for a space to replace with a \n
void CWeather::SplitLongString(char *szString, int splitStart, int splitEnd)
{
  //search chars 10 to 15 for a space
  //if we find one, replace it with a newline
  for (int i = splitStart; i < splitEnd && i < (int)strlen(szString); i++)
  {
    if (szString[i] == ' ')
    {
      szString[i] = '\n';
      return ;
    }
  }
}


//convert weather.com day strings into localized string id's
void CWeather::LocalizeDay(char *szDay)
{
  CStdString strLocDay;

  if (strcmp(szDay, "Monday") == 0)   //monday is localized string 11
    strLocDay = g_localizeStrings.Get(11);
  else if (strcmp(szDay, "Tuesday") == 0)
    strLocDay = g_localizeStrings.Get(12);
  else if (strcmp(szDay, "Wednesday") == 0)
    strLocDay = g_localizeStrings.Get(13);
  else if (strcmp(szDay, "Thursday") == 0)
    strLocDay = g_localizeStrings.Get(14);
  else if (strcmp(szDay, "Friday") == 0)
    strLocDay = g_localizeStrings.Get(15);
  else if (strcmp(szDay, "Saturday") == 0)
    strLocDay = g_localizeStrings.Get(16);
  else if (strcmp(szDay, "Sunday") == 0)
    strLocDay = g_localizeStrings.Get(17);
  else
    strLocDay = "";

  strcpy(szDay, strLocDay.GetBuffer(strLocDay.GetLength()));
}


void CWeather::LoadLocalizedToken()
{
  // We load the english strings in to get our tokens
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile("Q:\\language\\english\\strings.xml") )
  {
    return ;
  }
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("strings")) return ;
  const TiXmlNode *pChild = pRootElement->FirstChild();
  while (pChild)
  {
    CStdString strValue = pChild->Value();
    if (strValue == "string")
    {
      const TiXmlNode *pChildID = pChild->FirstChild("id");
      const TiXmlNode *pChildText = pChild->FirstChild("value");
      DWORD dwID = atoi(pChildID->FirstChild()->Value());
      if ( (LOCALIZED_TOKEN_FIRSTID <= dwID && dwID <= LOCALIZED_TOKEN_LASTID) ||
           (LOCALIZED_TOKEN_FIRSTID2 <= dwID && dwID <= LOCALIZED_TOKEN_LASTID2) )
      {
        WCHAR wszText[1024];
        swprintf(wszText, L"%S", pChildText->FirstChild()->Value() );
        if (wcslen(wszText) > 0)
        {
          wstring strText = wszText;
          m_localizedTokens.insert(std::pair<wstring, DWORD>(strText, dwID));
        }

      }
    }
    pChild = pChild->NextSibling();
  }
}

bool CWeather::GetSearchResults(const CStdString &strSearch, CStdString &strResult)
{
  // Check to see if the user entered a weather.com code
  if (strSearch.size() == 8)
  {
    strResult = "";
    int i = 0;
    for (i = 0; i < 4; ++i)
    {
      strResult += toupper(strSearch[i]);
      if (!isalpha(strSearch[i]))
        break;
    }
    if (i == 4)
    {
      for ( ; i < 8; ++i)
      {
        strResult += strSearch[i];
        if (!isdigit(strSearch[i]))
          break;
      }
      if (i == 8)
      {
        return true; // match
      }
    }
    // no match, wipe string
    strResult = "";
  }

  CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  CGUIDialogProgress *pDlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  //do the download
  CHTTP httpUtil;
  CStdString strURL;
  //  CStdString strResultsFile = "Z:\\searchresults.xml";
  CStdString strXML;

  if (pDlgProgress)
  {
    pDlgProgress->SetHeading(410);       //"Accessing Weather.com"
    pDlgProgress->SetLine(0, 194);       //"Searching"
    pDlgProgress->SetLine(1, strSearch);
    pDlgProgress->SetLine(2, "");
    pDlgProgress->StartModal(m_gWindowManager.GetActiveWindow());
    pDlgProgress->Progress();
  }

  strURL.Format("http://xoap.weather.com/search/search?where=%s", strSearch);

  if (!httpUtil.Get(strURL, strXML))
  {
    if (pDlgProgress) pDlgProgress->Close();
    return false;
  }

  //some select dialog init stuff
  if (!pDlgSelect)
  {
    if (pDlgProgress) pDlgProgress->Close();
    return false;
  }

  pDlgSelect->SetHeading(396); //"Select Location"
  pDlgSelect->Reset();

  ///////////////////////////////
  // load the xml file
  ///////////////////////////////
  TiXmlDocument xmlDoc;
  xmlDoc.Parse(strXML.c_str());
  if (xmlDoc.Error())
    return false;

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  TiXmlElement *pElement = pRootElement->FirstChildElement("loc");
  CStdString strItemTmp;
  while (pElement)
  {
    strItemTmp.Format("%s - %s", pElement->Attribute("id"), pElement->FirstChild()->Value());
    pDlgSelect->Add(strItemTmp);
    pElement = pElement->NextSiblingElement("loc");
  }

  if (pDlgProgress) pDlgProgress->Close();

  pDlgSelect->EnableButton(TRUE);
  pDlgSelect->SetButtonLabel(222); //'Cancel' button returns to weather settings
  pDlgSelect->DoModal(m_gWindowManager.GetActiveWindow());

  if (pDlgSelect->GetSelectedLabel() < 0)
  {
    if (pDlgSelect->IsButtonPressed())
    {
      pDlgSelect->Close(); //close the select dialog and return to weather settings
      return true;
    }
  }

  //copy the selected code into the settings
  if (pDlgSelect->GetSelectedLabel() >= 0)
  {
    CStdString areacode;
    areacode = pDlgSelect->GetSelectedLabelText();
    strResult = areacode.substr(0, areacode.Find("-") - 1);
  }

  if (pDlgProgress) pDlgProgress->Close();

  return true;
}

const char *CWeather::GetLabel(DWORD dwLabel)
{
  if (!g_guiSettings.GetBool("Network.EnableInternet"))
    return "";

  // ok, now let's check if we have a background loader running
  if (m_lRefreshTime < timeGetTime())
  { // need to refresh!
    Refresh(m_iCurWeather);
  }
  if (m_bBusy)
  {
    CStdString strBusy = g_localizeStrings.Get(503);
    strncpy(m_szBusyString, strBusy.c_str(), 256);
    return m_szBusyString;
  }
  // ok, here is where the fun deciphering goes
  if (dwLabel == WEATHER_LABEL_CURRENT_COND) return m_szCurrentConditions;
  else if (dwLabel == WEATHER_LABEL_CURRENT_TEMP) return m_szCurrentTemperature;
  else if (dwLabel == WEATHER_LABEL_CURRENT_FEEL) return m_szCurrentFeelsLike;
  else if (dwLabel == WEATHER_LABEL_CURRENT_UVID) return m_szCurrentUVIndex;
  else if (dwLabel == WEATHER_LABEL_CURRENT_WIND) return m_szCurrentWind;
  else if (dwLabel == WEATHER_LABEL_CURRENT_DEWP) return m_szCurrentDewPoint;
  else if (dwLabel == WEATHER_LABEL_CURRENT_HUMI) return m_szCurrentHumidity;
  else if (dwLabel == WEATHER_LABEL_LOCATION) return m_szLocation[m_iCurWeather];
  return "";
}

void CWeather::Refresh(int iArea)
{
  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("Network.EnableInternet")) return ;

  m_iCurWeather = iArea;
  // all we do is start the background loader if it's not already running.
  // this just runs until it has the info, then exits, setting it's pointer to NULL.
  if (!m_pBackgroundLoader)
  {
    m_pBackgroundLoader = new CBackgroundWeatherLoader(this, iArea);
    if (!m_pBackgroundLoader)
    {
      CLog::Log(LOGERROR, "Unable to start the background weather loader");
      return ;
    }
  }
  m_bBusy = true;
}


void CWeather::LoaderFinished()
{
  m_lRefreshTime = timeGetTime() + (DWORD)(g_guiSettings.GetInt("Weather.RefreshTime") * 60000);
  m_pBackgroundLoader = NULL;
  m_bBusy = false;
}

char *CWeather::GetCurrentIcon()
{
  if (!g_guiSettings.GetBool("Network.EnableInternet"))
    return "";

  // ok, now let's check if we have a background loader running
  if (m_lRefreshTime < timeGetTime())
  { // need to refresh!
    Refresh(m_iCurWeather);
  }
  //if (m_bBusy) return "Q:\\weather\\128x128\\na.png";
  if (m_bBusy) 
  {
    sprintf(m_szNAIcon,"%s128x128\\na.png",strBasePath.c_str());
    return m_szNAIcon;
  }
  return m_szCurrentIcon;
}
