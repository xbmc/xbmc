#include "stdafx.h"
#include "pyutil.h"
#include <wchar.h>
#include "stdstring.h"
#include "graphiccontext.h"
#include "..\..\..\..\guilib\tinyXML\tinyxml.h"
#include "skininfo.h"

static int iPyGUILockRef = 0;
static TiXmlDocument pySkinReferences;
static int iPyLoadedSkinReferences = 0;

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

namespace PYXBMC
{
	int PyGetUnicodeString(wstring& buf, PyObject* pObject, int pos)
	{
		if(PyUnicode_Check(pObject))
		{
			buf = PyUnicode_AsUnicode(pObject);
			return 1;
		}
		if(PyString_Check(pObject))
		{
			buf = CStdStringW(PyString_AsString(pObject));
			return 1;
		}
		// object is not an unicode ar normal string
		buf = L"";
		if (pos != -1) PyErr_Format(PyExc_TypeError, "argument %.200i must be unicode or str", pos);
		return 0;
	}

	void PyGUILock()
	{
		if (iPyGUILockRef == 0) g_graphicsContext.Lock();
		iPyGUILockRef++;
	}

	void PyGUIUnlock()
	{
		if (iPyGUILockRef > 0)
		{
			iPyGUILockRef--;
			if (iPyGUILockRef == 0) g_graphicsContext.Unlock();	
		}
	}

	/*
	 * Looks in references.xml for image name
	 * If none exist return default image name
	 * iPyLoadedSkinReferences:
	 * - 0 xml not loaded
	 * - 1 tried to load xml, but it failed
	 * - 2 xml loaded
	 */
	const char* GetDefaultImage(char* cControlType, char* cTextureType, char* cDefault)
	{
		if (iPyLoadedSkinReferences == 0)
		{
			// xml not loaded. We only try to load references.xml once
			RESOLUTION res;
			string strPath = g_SkinInfo.GetSkinPath("references.xml", &res);
			if (pySkinReferences.LoadFile(strPath.c_str()))	iPyLoadedSkinReferences = true;
			else return cDefault; // return default value
				
		}
		else if (iPyLoadedSkinReferences == 1) return cDefault;

		TiXmlElement *pControls = pySkinReferences.RootElement();

		TiXmlNode *pNode = NULL;
		TiXmlElement *pElement = NULL;

		//find control element
		while(pNode = pControls->IterateChildren("control", pNode))
		{
			pElement = pNode->FirstChildElement("type");
			if (pElement) if (!strcmp(pElement->FirstChild()->Value(), cControlType)) break;
		}
		if (!pElement) return cDefault;

		// get texture element
		TiXmlElement *pTexture = pNode->FirstChildElement(cTextureType);
		if (pTexture)
		{
			// found our textureType
			pNode = pTexture->FirstChild();
			if (pNode && pNode->Value()[0] != '-') return pNode->Value();
			else
			{
				//set default
				return cDefault;
			}
		}
		return cDefault;
	}
}
