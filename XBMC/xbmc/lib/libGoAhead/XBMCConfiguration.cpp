#include "stdafx.h"

#pragma once

#include "XBMCConfiguration.h"
#include "..\..\xbox\iosupport.h"
#include "..\..\util.h"

#pragma code_seg("WEB_TEXT")
#pragma data_seg("WEB_DATA")
#pragma bss_seg("WEB_BSS")
#pragma const_seg("WEB_RD")

CXbmcConfiguration::CXbmcConfiguration()
{
	xbmcCfg = NULL;
	xbmcCfgLoaded = false;
}

CXbmcConfiguration::~CXbmcConfiguration()
{

}

/*
 * Load XboxMediaCenter.xml
 */
int CXbmcConfiguration::Load()
{
	if (!xbmcCfgLoaded)
	{
		// note, we don't use 'Q:\\' here since 'Q:\\' is always mapped to our xbmc home dir
		// and when using xbmc as dash our configfile has to be loaded from 'c:\\'
		CStdString strPath;
		char szXBEFileName[1024];
		CIoSupport helper;
		helper.GetXbePath(szXBEFileName);
		strrchr(szXBEFileName,'\\')[0] = 0;
		strPath.Format("%s\\%s", szXBEFileName, "XboxMediaCenter.xml");

		if (!xbmcCfg.LoadFile(strPath)) return -1;
		xbmcCfgLoaded = true;
	}
	return 0;
}

/*
 * Retrieve size of bookmark type (type)
 * var type has to be set to a bookmark name (like video, music ...)
 */
int CXbmcConfiguration::BookmarkSize( int eid, webs_t wp, int argc, char_t **argv)
{
	char_t *type = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T("%s"),&type) < 1)
	{
		websError(wp, 500, T("Insufficient args\n"));
		return -1;
	}

	// load xboxmediacenter.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
		websError(wp, 500, T("Could not load XboxMediaCenter.xml\n"));
		return -1;
	}

	// return number of
	TiXmlElement *pRootElement = xbmcCfg.RootElement();
	TiXmlNode *pNode = NULL;

	pNode = pRootElement->FirstChild(type);

	if (!pNode)
	{
		websError(wp, 500, T("Bookmark type doesn oet exist\n"));
		return -1;
	}

	TiXmlNode *pIt = NULL;
	char buffer[10];
	int counter = 0;

	while(pIt = pNode->IterateChildren("bookmark", pIt))	counter++;
	ejSetResult( eid, itoa(counter, buffer, 10));

	return 0;
}

/*
 * Get bookmark (type, parameter, id)
 * var type has to be set to a bookmark name (like video, music ...)
 * var paramater = "name" or "path"
 * var id = position of bookmark
 */
int CXbmcConfiguration::GetBookmark( int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*parameter, *type, *id = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T("%s %s %s"), &type, &parameter, &id) < 3) {
		websError(wp, 500, T("Insufficient args\n"));
		return -1;
	}

	// load xboxmediacenter.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
		websError(wp, 500, T("Could not load XboxMediaCenter.xml\n"));
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
		websError(wp, 500, T("Id is not a number\n"));
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
				ejSetResult( eid, (char*)pIt->FirstChild("name")->FirstChild()->Value());
			}
		}
		// user wants the path of the bookmark.
		if (!strcmp(parameter, "path"))
		{
			if (pIt->FirstChild("path"))
			{
				ejSetResult( eid, (char*)pIt->FirstChild("path")->FirstChild()->Value());
			}
		}
	}
	return 0;
}

/*
 * Add a new bookmark (type, name, path, position)
 * var type has to be set to a bookmark name (like video, music ...)
 * var name = share name
 * var path = path
 * var postition = position where bookmark should be placed (not required)
 */
int CXbmcConfiguration::AddBookmark( int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*type, *name, *path, *position = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T("%s %s %s %s"), &type, &name, &path, &position) < 3) {
		websError(wp, 500, T("Insufficient args\n use: function(command, type, name, path, [postion])"));
		return -1;
	}

	// load xboxmediacenter.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
		websError(wp, 500, T("Could not load XboxMediaCenter.xml\n"));
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
			websError(wp, 500, T("position is not a number\n"));
			return -1;
		}

		// find bookmark at position
		if (pNode)
			for (int i = 0; i < nr; i++) pIt = pNode->IterateChildren("bookmark", pIt);
		if (pIt) pNode->ToElement()->InsertAfterChild(pIt, bookmark);
	}
	else
	{
		pNode->ToElement()->InsertEndChild(bookmark);
	}
	return 0;
}

/*
 * Save bookmark (type, name, path, position)
 * var type has to be set to a bookmark name (like video, music ...)
 * var name = new share name
 * var path = new path
 * var postition = position where bookmark should be placed
 */
int CXbmcConfiguration::SaveBookmark( int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*type, *name, *path, *position = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T("%s %s %s %s"), &type, &name, &path, &position) < 4) {
		websError(wp, 500, T("Insufficient args\n use: function(command, type, name, path, postion)"));
		return -1;
	}

	// load xboxmediacenter.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
		websError(wp, 500, T("Could not load XboxMediaCenter.xml\n"));
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
		websError(wp, 500, T("Id is not a number\n"));
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
	return 0;
}

/*
 * Remove bookmark (type, name, path, position)
 * var type has to be set to a bookmark name (like video, music ...)
 * var postition = bookmark at position that should be removed
 */
int CXbmcConfiguration::RemoveBookmark( int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*type, *position = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T("%s %s"), &type, &position) < 2) {
		websError(wp, 500, T("Insufficient args\n use: function(type, postion)"));
		return -1;
	}

	// load xboxmediacenter.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
		websError(wp, 500, T("Could not load XboxMediaCenter.xml\n"));
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
		websError(wp, 500, T("Id is not a number\n"));
		return -1;
	}

	// find bookmark at position
	if (pNode)
		for (int i = 0; i < nr; i++) pIt = pNode->IterateChildren("bookmark", pIt);

	if (pIt) pNode->RemoveChild(pIt);
	return 0;
}

/*
 * Save configuration to a file (filename)
 * var filename = filename to which the configuration has to be written
 * is only a filename is specified and no directory we save it in the same dir that
 * our executable is in.
 */
int CXbmcConfiguration::SaveConfiguration( int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*filename = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T("%s"), &filename) < 1) {
		websError(wp, 500, T("Insufficient args\n use: function(filename)"));
		return -1;
	}

	// load xboxmediacenter.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
		websError(wp, 500, T("Could not load XboxMediaCenter.xml\n"));
		return -1;
	}

	// Save configuration to file
	CStdString strPath(filename);
	if (strPath.find(":\\") == -1)
	{
		// only filename specified, this means whe have to lookup the directory where
		// our executable is in and add the filename to it
		// note, we don't use 'Q:\\' here since 'Q:\\' is always mapped to our xbmc home dir
		// and when using xbmc as dash our configfile has to be saved to 'c:\\'
		char szXBEFileName[1024];
		CIoSupport helper;
		helper.GetXbePath(szXBEFileName);
		strrchr(szXBEFileName,'\\')[0] = 0;
		strPath.Format("%s\\%s", szXBEFileName, filename);
	}

	if (!xbmcCfg.SaveFile(strPath))
	{
		websError(wp, 500, T("Could not save to file\n"));
		return -1;
	}
	return 0;
}

/*
 * Get value from configuration (name)
 * var name = option name
 */
int CXbmcConfiguration::GetOption( int eid, webs_t wp, int argc, char_t **argv)
{
	char_t* name = NULL;

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T("%s"), &name) < 1) {
		websError(wp, 500, T("Insufficient args\n"));
		return -1;
	}

	// load xboxmediacenter.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
		websError(wp, 500, T("Could not load XboxMediaCenter.xml\n"));
		return -1;
	}

	// get first option from xml file
	// we have to check if there arent any other childs in this element
	TiXmlElement *pRootElement = xbmcCfg.RootElement();
	TiXmlElement *pElement = NULL;
	pElement = pRootElement->FirstChildElement(name);

	if (pElement)
	{
		// if we have another child, this isn't an option like
		// <xboxmediacenter>
		//	<home>c:\</home>
		// <xboxmediacenter>
		if (pElement->FirstChild() && pElement->FirstChild()->FirstChild() == NULL)
		{
			char* value = (char*)pElement->FirstChild()->Value();
			if (value) ejSetResult(eid, value);
		}
		// option exist, but no value is set. Default is "-"
		else ejSetResult(eid, "-");
	}
	else
	{
		// option not found in xml file
		// set value to "-"
		ejSetResult(eid, "");
	}
	return 0;
}

/*
 * Set value for option in configuration (name, value)
 * var name = option name
 * var value = new value
 */
int CXbmcConfiguration::SetOption( int eid, webs_t wp, int argc, char_t **argv)
{
	char_t *name, *value = NULL;

	// load xboxmediacenter.xml, write a messages if file could not be loaded
	if (Load() == -1)
	{
		websError(wp, 500, T("Could not load XboxMediaCenter.xml\n"));
		return -1;
	}

	// asp function is called within a script, get arguments
	if (ejArgs(argc, argv, T("%s %s"), &name, &value) < 2)
	{
		websError(wp, 500, T("Insufficient args\n"));
		return -1;
	}

	// get first option from xml file
	// we have to check if there arent any other childs in this element
	TiXmlElement *pRootElement = xbmcCfg.RootElement();
	TiXmlElement *pElement = NULL;
	pElement = pRootElement->FirstChildElement(name);

	if (pElement)
	{
		// if we have another child, this isn't an option like
		// <xboxmediacenter>
		//	<home>c:\</home>
		// <xboxmediacenter>
		if (pElement->FirstChild() && pElement->FirstChild()->FirstChild() == NULL)
		{
			pElement->FirstChild()->SetValue(value);
		}
		
		else
		{
			// option exist, but no child exists.  set value or "-" is value = NULL
			pElement->InsertEndChild(TiXmlText(value ? value : "-"));
		}
	}
	else if (IsValidOption(name))
	{
		// option not found in xml file
		// create new element and set value or "-" is value = NULL

		// create a new Element
		TiXmlElement xmlOption(name);
		xmlOption.InsertEndChild(TiXmlText(value ? value : "-"));

		// add element to configuration
		pRootElement->InsertEndChild(xmlOption);
	}
	return 0;
}

/*
 * Check if option is a valid one in xboxmediacenter.xml
 */
bool CXbmcConfiguration::IsValidOption(char* option)
{
	if (!strcmp("home", option)) return true;
	if (!strcmp("ipadres", option)) return true;
	if (!strcmp("netmask", option)) return true;
	if (!strcmp("defaultgateway", option)) return true;
	if (!strcmp("nameserver", option)) return true;
	if (!strcmp("CDDBIpAdres", option)) return true;
	if (!strcmp("useFDrive", option)) return true;
	if (!strcmp("useFDrive", option)) return true;
	if (!strcmp("httpproxy", option)) return true;
	if (!strcmp("httpproxyport", option)) return true;
	if (!strcmp("timeserver", option)) return true;
	if (!strcmp("dashboard", option)) return true;
	if (!strcmp("dvdplayer", option)) return true;
	if (!strcmp("subtitles", option)) return true;
	if (!strcmp("startwindow", option)) return true;
	if (!strcmp("pictureextensions", option)) return true;
	if (!strcmp("musicextensions", option)) return true;
	if (!strcmp("videoextensions", option)) return true;
	if (!strcmp("thumbnails", option)) return true;
	if (!strcmp("shortcuts", option)) return true;
	if (!strcmp("albums", option)) return true;
	if (!strcmp("recordings", option)) return true;
	if (!strcmp("screenshots", option)) return true;
	if (!strcmp("displayremotecodes", option)) return true;
	return false;
}
