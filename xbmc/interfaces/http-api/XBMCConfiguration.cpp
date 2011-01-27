/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Settings.h"
#include "XBMCConfiguration.h"
#include "Util.h"
#include "URL.h"

CXbmcConfiguration::CXbmcConfiguration()
{
	xbmcCfgLoaded = false;
}

CXbmcConfiguration::~CXbmcConfiguration()
{

}

/*
 * Load sources.xml
 */
int CXbmcConfiguration::Load()
{
	if (!xbmcCfgLoaded)
	{
    if (!xbmcCfg.LoadFile(g_settings.GetSourcesFile())) return -1;
		xbmcCfgLoaded = true;
	}
	return 0;
}

/*
 * Retrieve size of bookmark type (type)
 * var type has to be set to a bookmark name (like video, music ...)
 */
int CXbmcConfiguration::BookmarkSize( int eid, webs_t wp, CStdString& response, int argc, char_t **argv)
{
	char_t *type = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T((char*)"%s"),&type) < 1)
	{
           if (eid!=-1) websError(wp, 500, T((char*)"Insufficient args\n"));
              else response="Error:Insufficient args";
		return -1;
	}

  VECSOURCES *pShares = g_settings.GetSourcesFromType(type);
  if (pShares)
  {
    char buffer[10];

    if (eid!=-1) 
      ejSetResult( eid, itoa(pShares->size(), buffer, 10));
    else
    {
      CStdString tmp;
      tmp.Format("%s", itoa(pShares->size(), buffer, 10));
      response="" + tmp;
    }

    return 0;
  }

  if (eid!=-1) websError(wp, 500, T((char*)"Bookmark type does not exist\n")); 
  else response="Error:Bookmark type does not exist";
  return -1;

/*	// load sources.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
    eid!=-1 ? websError(wp, 500, T("Could not load sources.xml\n")):
              response="Error:Could not load sources.xml";
		return -1;
	}

	// return number of
	TiXmlElement *pRootElement = xbmcCfg.RootElement();
	TiXmlNode *pNode = NULL;

	pNode = pRootElement->FirstChild(type);

	if (!pNode)
	{
    eid!=-1 ? websError(wp, 500, T("Bookmark type does not exist\n")):
              response="Error:Bookmark type does not exist";
		return -1;
	}

	TiXmlNode *pIt = NULL;
	char buffer[10];
	int counter = 0;

	while(pIt = pNode->IterateChildren("bookmark", pIt))	counter++;
  if (eid!=-1) 
    ejSetResult( eid, itoa(counter, buffer, 10));
  else
  {
    CStdString tmp;
    tmp.Format("%s", itoa(counter, buffer, 10));
    response="" + tmp;
  }
	return 0;*/
}

/*
 * Get bookmark (type, parameter, id)
 * var type has to be set to a bookmark name (like video, music ...)
 * var paramater = "name" or "path"
 * var id = position of bookmark
 */
int CXbmcConfiguration::GetBookmark( int eid, webs_t wp, CStdString& response, int argc, char_t **argv)
{
	char_t	*parameter, *type, *id = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T((char*)"%s %s %s"), &type, &parameter, &id) < 3) {
          if (eid!=-1) websError(wp, 500, T((char*)"Insufficient args\n"));
            else response="Error:Insufficient args";
		return -1;
	}

  int nr = 0;
  try { nr = atoi(id); }
  catch (...)
  {
    if (eid!=-1) websError(wp, 500, T((char*)"Id is not a number\n"));
      else response="Error:Id is not a number";
    return -1;
  }

  VECSOURCES* pShares = g_settings.GetSourcesFromType(type);
  if (!pShares)
  {
    if (eid!=-1) websError(wp, 500, T((char*)"Bookmark type does not exist\n"));
      else response="Error:Bookmark type does not exist";
    return -1;
  }
  if (nr > 0 && nr <= (int)pShares->size())
  {
    const CMediaSource& share = (*pShares)[nr-1];
    if (CStdString(parameter).Equals("path"))
    {
      if (eid!=-1)
        ejSetResult( eid, const_cast<char*>(share.strPath.c_str()));
      else
      {
        CStdString tmp;
        tmp.Format("%s",share.strPath);
        response="" + tmp;
      }
    }
    else if (CStdString(parameter).Equals("name"))
    {
      if (eid!=-1)
        ejSetResult( eid, const_cast<char*>(share.strName.c_str()));
      else
      {
        CStdString tmp;
        tmp.Format("%s",share.strName);
        response="" + tmp;
      }
    }
    else
    {
      if (eid!=-1) websError(wp, 500, T((char*)"Parameter not known\n")); 
        else response="Error:Parameter not known";
    }
    return 0;
  }

  if (eid!=-1) websError(wp, 500, T((char*)"Position not found\n"));
    else response="Error:Position not found";
  return -1;


	/*// load sources.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
    eid!=-1 ? websError(wp, 500, T("Could not load sources.xml\n")):
              response="Error:Could not load sources.xml";
		return -1;
	}

	// Return bookmark of
	TiXmlElement *pRootElement = xbmcCfg.RootElement();
	TiXmlNode *pNode = NULL;
	TiXmlNode *pIt = NULL;

	int nr = 0;
	try { nr = atoi(id); }
	catch (...)
	{
    eid!=-1 ? websError(wp, 500, T("Id is not a number\n")):
              response="Error:Id is not a number";
		return -1;
	}

	pNode = pRootElement->FirstChild(type);

	// if valid bookmark, find child at pos (id)
	if (pNode)
		for (int i = 0; i < nr; i++) pIt = pNode->IterateChildren("bookmark", pIt);
	if (pIt)
	{
		// user wants the name of the bookmark.
		if (!strcmp(parameter, "name"))
		{
			if (pIt->FirstChild("name"))
			{
        if (eid!=-1)
          ejSetResult( eid, (char*)pIt->FirstChild("name")->FirstChild()->Value());
        else
        {
          CStdString tmp;
          tmp.Format("%s",(char*)pIt->FirstChild("name")->FirstChild()->Value());
          response="" + tmp;
        }
			}
		}
		// user wants the path of the bookmark.
		else if (!strcmp(parameter, "path"))
		{
			if (pIt->FirstChild("path"))
			{
        if (eid!=-1)
          ejSetResult( eid, (char*)pIt->FirstChild("path")->FirstChild()->Value());
        else
        {
          CStdString tmp;
          tmp.Format("%s",(char*)pIt->FirstChild("path")->FirstChild()->Value());
          response="" + tmp ;
        }
			}
		}
    else
      eid!=-1 ? websError(wp, 500, T("Parameter not known\n")):
                response="Error:Parameter not known";
	}
  else
  {
    eid!=-1 ? websError(wp, 500, T("Position not found\n")):
              response="Error:Position not found";
    return -1;
  }
	return 0;*/
}

/*
 * Add a new bookmark (type, name, path)
 * Add a new bookmark (type, name, path, position)
 * Add a new bookmark (type, name, path, thumbnail, position)
 * var type has to be set to a bookmark name (like video, music ...)
 * var name = share name
 * var path = path
 * var thumbnail = thumbnail image (not required)
 * var postition = position where bookmark should be placed (not required)
 */
int CXbmcConfiguration::AddBookmark( int eid, webs_t wp, CStdString& response, int argc, char_t **argv)
{
  char_t    *type, *name, *path, *thumbnail = NULL, *position = NULL;
  int numParas;

  // asp function is called within a script, get arguments
  numParas=ejArgs(argc, argv, T((char*)"%s %s %s %s %s"), &type, &name, &path, &thumbnail, &position);
  if ( numParas< 3) 
  {
    if (eid!=-1)
       websError(wp, 500, T((char*)"Insufficient args\n use: function(command, type, name, path, [thumbnail], [position])"));
    else
       response="Error:Insufficient args, use: function(command, type, name, path, [thumbnail], [position])";
    return -1;
  }

  CMediaSource share;
  share.strName = name;
  if (numParas==4)
  {
	  position=thumbnail;
	  thumbnail=NULL;
  }
  if (numParas==5)
    share.m_strThumbnailImage = thumbnail;
  CStdString strPath=path;
  CUtil::AddSlashAtEnd(strPath);

  share.strPath = strPath;
  share.vecPaths.push_back(strPath.c_str());
  g_settings.AddShare(type,share);

  return 0;
/*
	// load sources.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
    eid!=-1 ? websError(wp, 500, T("Could not load sources.xml\n")):
              response="Error:Could not load sources.xml";
    return -1;
	}

	TiXmlElement *pRootElement = xbmcCfg.RootElement();
	TiXmlNode *pNode = NULL;
	TiXmlNode *pIt = NULL;

	pNode = pRootElement->FirstChild(type);
	
	// create a new Element
	TiXmlText xmlName(name);
	TiXmlText xmlPath(path);
	TiXmlElement eName("name");
	TiXmlElement ePath("path");
	eName.InsertEndChild(xmlName);
	ePath.InsertEndChild(xmlPath);

	TiXmlElement bookmark("bookmark");
	bookmark.InsertEndChild(eName);
	bookmark.InsertEndChild(ePath);

	//if postion == NULL add bookmark at end of the other bookmarks
	if (position)
	{
		//isert after postion 'position'
		int nr = 0;
		try { nr = atoi(position); }
		catch (...)
		{
      eid!=-1 ? websError(wp, 500, T("position is not a number\n")):
                response="Error:position is not a number";
			return -1;
		}

		// find bookmark at position
		if (pNode)
			for (int i = 0; i < nr; i++) pIt = pNode->IterateChildren("bookmark", pIt);
		if (pIt)
      pNode->ToElement()->InsertAfterChild(pIt, bookmark);
    else
    {
      eid!=-1 ? websError(wp, 500, T("Position not found\n")):
                response="Error:Position not found";
      return -1;
    }
	}
	else
	{
		pNode->ToElement()->InsertEndChild(bookmark);
	}
	return 0;*/
}

/*
 * Save bookmark (type, name, path, position)
 * var type has to be set to a bookmark name (like video, music ...)
 * var name = new share name
 * var path = new path
 * var postition = position where bookmark should be placed
 */
int CXbmcConfiguration::SaveBookmark( int eid, webs_t wp, CStdString& response, int argc, char_t **argv)
{
	char_t	*type, *name, *path, *position = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T((char*)"%s %s %s %s"), &type, &name, &path, &position) < 4) {
        if (eid!=-1) websError(wp, 500, T((char*)"Insufficient args\n use: function(command, type, name, path, postion)"));
          else response="Error:Insufficient args, use: function(command, type, name, path, postion)";
		return -1;
	}
  VECSOURCES* pShares = g_settings.GetSourcesFromType(type);
  int nr = 0;
	try { nr = atoi(position); }
	catch (...)
	{
          if (eid!=-1) websError(wp, 500, T((char*)"Id is not a number\n"));
              else response="Error:Id is not a number";
	  return -1;
	}

  if (nr > 0 && nr <= (int)pShares->size()) // update share
  {
    const CMediaSource& share = (*pShares)[nr-1];
    g_settings.UpdateSource(type, share.strName, "path", path);
    g_settings.UpdateSource(type, share.strName, "name", name);
    g_settings.SaveSources();
    return 0;
  }
  
  if (eid!=-1) websError(wp, 500, T((char*)"Position not found\n"));
    else response="Error:Position not found";
  return -1;


/*	// load sources.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
    eid!=-1 ? websError(wp, 500, T("Could not load sources.xml\n")):
              response="Error:Could not load sources.xml";
    return -1;
	}

	TiXmlElement *pRootElement = xbmcCfg.RootElement();
	TiXmlNode *pNode = NULL;
	TiXmlNode *pIt = NULL;

	pNode = pRootElement->FirstChild(type);

	int nr = 0;
	try { nr = atoi(position); }
	catch (...)
	{
    eid!=-1 ? websError(wp, 500, T("Id is not a number\n")):
              response="Error:Id is not a number";
		return -1;
	}

	// find bookmark at position
	if (pNode)
		for (int i = 0; i < nr; i++) pIt = pNode->IterateChildren("bookmark", pIt);
	if (pIt)
	{
		pIt->FirstChild("name")->FirstChild()->SetValue(name);
		pIt->FirstChild("path")->FirstChild()->SetValue(path);
	}
  else
  {
    eid!=-1 ? websError(wp, 500, T("Position not found\n")):
              response="Error:Position not found";
    return -1;
  }*/
  
	return 0;
}

/*
 * Remove bookmark (type, name, path, position)
 * var type has to be set to a bookmark name (like video, music ...)
 * var postition = bookmark at position that should be removed
 */
int CXbmcConfiguration::RemoveBookmark( int eid, webs_t wp, CStdString& response, int argc, char_t **argv)
{
	char_t	*type, *position = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T((char*)"%s %s"), &type, &position) < 2) {
          if(eid!=-1)
            websError(wp, 500, T((char*)"Insufficient args\n use: function(type, position)"));
          else
            response="Error:Insufficient args, use: function(type, position)";
	  return -1;
	}

	int nr = 0;
	try { nr = atoi(position); }
	catch (...)
	{
          if (eid!=-1) websError(wp, 500, T((char*)"Id is not a number\n"));
            else response="Error:position is not a number";
  	  return -1;
	}

  VECSOURCES* pShares = g_settings.GetSourcesFromType(type);
  const CMediaSource& share = (*pShares)[nr-1];
  if (g_settings.DeleteSource(type,share.strName,share.strPath))
    return 0;

  if (eid!=-1) websError(wp, 500, T((char*)"Position not found\n"));
    else response="Error:Position not found";
  return -1;
  /*
	// load sources.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
    eid!=-1 ? websError(wp, 500, T("Could not load sources.xml\n")):
              response="Error:Could not load sources.xml";
    return -1;
	}

	TiXmlElement *pRootElement = xbmcCfg.RootElement();
	TiXmlNode *pNode = NULL;
	TiXmlNode *pIt = NULL;

	pNode = pRootElement->FirstChild(type);

	int nr = 0;
	try { nr = atoi(position); }
	catch (...)
	{
    eid!=-1 ? websError(wp, 500, T("Id is not a number\n")):
              response="Error:position is not a number";
		return -1;
	}

	// find bookmark at position
	if (pNode)
		for (int i = 0; i < nr; i++) pIt = pNode->IterateChildren("bookmark", pIt);

	if (pIt)
    pNode->RemoveChild(pIt);
  else
  {
    eid!=-1 ? websError(wp, 500, T("Position not found\n")):
              response="Error:Position not found";
    return -1;
  }
	return 0;*/
}

/*
 * Save configuration to a file (filename)
 * var filename = filename to which the configuration has to be written
 * is only a filename is specified and no directory we save it in the same dir that
 * our executable is in.
 */
int CXbmcConfiguration::SaveConfiguration( int eid, webs_t wp, CStdString& response, int argc, char_t **argv)
{
  if (eid!=-1) websError(wp, 500, T((char*)"Deprecated\n"));
    else response="Error:Functino is deprecated";
  return -1;

  char_t	*filename = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T((char*)"%s"), &filename) < 1) {
           if (eid!=-1) websError(wp, 500, T((char*)"Insufficient args\n use: function(filename)"));
              else response="Error:Insufficient args, use: function(filename)";
  	   return -1;
	}

	// load sources.xml, write a messages if file could not be loaded
	if (Load() == -1) 
	{
          if (eid!=-1) websError(wp, 500, T((char*)"Could not load sources.xml\n"));
              else response="Error:Could not load sources.xml";
          return -1;
	}

	// Save configuration to file
	CStdString strPath(filename);
  if (!CURL::IsFullPath(strPath))
	{
		// only filename specified, so use our homedir as base.
    strPath = CUtil::AddFileToFolder("special://home/", filename);
	}

  if (!xbmcCfg.SaveFile(strPath))
	{
          if (eid!=-1) websError(wp, 500, T((char*)"Could not save to file\n"));
            else response="Error:Could not save to file";
 	  return -1;
	}
	return 0;
}

/*
 * Get value from configuration (name)
 * var name = option name
 */
int CXbmcConfiguration::GetOption( int eid, webs_t wp, CStdString& response, int argc, char_t **argv)
{
  if (eid!=-1) websError(wp, 500, T((char*)"Deprecated\n"));
    else response="Error:Functino is deprecated";
return -1;

 /* 
  char_t* name = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T("%s"), &name) < 1) {
    eid!=-1 ? websError(wp, 500, T("Insufficient args\n")):
              response="Error:Insufficient args";
		return -1;
	}

	// load sources.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
    eid!=-1 ? websError(wp, 500, T("Could not load sources.xml\n")):
              response="Error:Could not load sources.xml";;
		return -1;
	}

	// get first option from xml file
	// we have to check if there arent any other childs in this element
	TiXmlElement *pRootElement = xbmcCfg.RootElement();
	TiXmlElement *pElement = NULL;
	pElement = pRootElement->FirstChildElement(name);

	if (pElement)
	{
		if (pElement->FirstChild() && pElement->FirstChild()->FirstChild() == NULL)
		{
			char* value = (char*)pElement->FirstChild()->Value();
			if (value) 
        if (eid!=-1)
          ejSetResult(eid, value);
        else
        {
          CStdString tmp;
          tmp.Format("%s",value);
          response="" + tmp;
        }
		}
		// option exist, but no value is set. Default is "-"
		else 
                {
                   if (eid!=-1) 
                      ejSetResult(eid, "-");
                   else
                      response="";
                }
	}
	else
	{
		// option not found in xml file
		// set value to "-"
                if (eid!=-1) 
                   ejSetResult(eid, "");
                else
                   response="Error:Not found";
  }
	return 0;
*/
}

/*
 * Set value for option in configuration (name, value)
 * var name = option name
 * var value = new value
 */
int CXbmcConfiguration::SetOption( int eid, webs_t wp, CStdString& response, int argc, char_t **argv)
{
  if (eid!=-1) websError(wp, 500, T((char*)"Deprecated\n"));
    else response="Error:Functino is deprecated";

  return -1;
}

/*
 * Check if option is a valid one in sources.xml
 */
bool CXbmcConfiguration::IsValidOption(char* option)
{
	if (!strcmp("subtitles", option)) return true;
	if (!strcmp("thumbnails", option)) return true;
	if (!strcmp("shortcuts", option)) return true;
	if (!strcmp("albums", option)) return true;
	if (!strcmp("recordings", option)) return true;
	if (!strcmp("screenshots", option)) return true;
	return false;
}

bool XbmcWebConfigInit()
{
  if (!pXbmcWebConfig) {
    pXbmcWebConfig = new CXbmcConfiguration();
	return true;
  }
  else
	return false;
}

void XbmcWebConfigRelease()
{
  if (pXbmcWebConfig)
  {
	delete pXbmcWebConfig;
	pXbmcWebConfig=NULL;
  }
}

int XbmcWebsHttpAPIConfigBookmarkSize(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->BookmarkSize(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigGetBookmark(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->GetBookmark(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigAddBookmark(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->AddBookmark(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigSaveBookmark(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->SaveBookmark(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigRemoveBookmark(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->RemoveBookmark(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigSaveConfiguration(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->SaveConfiguration(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigGetOption(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->GetOption(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigSetOption(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->SetOption(-1, NULL, response, argc, argv) : -1; }

