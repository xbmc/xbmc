#include ".\htmlutil.h"
using namespace HTML;


CHTMLUtil::CHTMLUtil(void)
{
}

CHTMLUtil::~CHTMLUtil(void)
{
}

int CHTMLUtil::FindTag(const CStdString& strHTML,const CStdString& strTag, CStdString& strtagFound, int iPos) const
{
	CStdString strHTMLLow=strHTML;
	CStdString strTagLow=strTag;
	strHTMLLow.ToLower();
	strTagLow.ToLower();
	strtagFound="";
	int iStart=strHTMLLow.Find(strTag,iPos);
	if (iStart < 0) return -1;
	int iEnd=strHTMLLow.Find(">",iStart);
	if (iEnd < 0) iEnd=(int)strHTMLLow.size();
	strtagFound=strHTMLLow.Mid(iStart,(iEnd+1)-iStart);
	return iStart;
}

void CHTMLUtil::getValueOfTag(const CStdString& strTagAndValue, CStdString& strValue)
{
	// strTagAndValue contains:
	// like <a href=blablabla.....>value</a>
	strValue=strTagAndValue;
	int iStart=strTagAndValue.Find(">");
	int iEnd=strTagAndValue.Find("<",iStart+1);
	if (iStart>=0 && iEnd>=0)
	{
		iStart++;
		strValue=strTagAndValue.Mid(iStart,iEnd-iStart);
	}
}

void  CHTMLUtil::getAttributeOfTag(const CStdString& strTagAndValue, const CStdString& strTag,CStdString& strValue)
{
	// strTagAndValue contains:
	// like <a href=""value".....
	strValue=strTagAndValue;
	int iStart=strTagAndValue.Find(strTag);
	if (iStart< 0) return;
	iStart+=(int)strTag.size();
	while (strTagAndValue[iStart+1] == 0x20 || strTagAndValue[iStart+1] == 0x27 || strTagAndValue[iStart+1] == 34) iStart++;
	int iEnd=iStart+1;
	while (strTagAndValue[iEnd] != 0x27 && strTagAndValue[iEnd] != 0x20 && strTagAndValue[iEnd] != 34&& strTagAndValue[iEnd] != '>') iEnd++;
	if (iStart>=0 && iEnd>=0)
	{
		strValue=strTagAndValue.Mid(iStart,iEnd-iStart);
	}
}

void CHTMLUtil::RemoveTags(CStdString& strHTML)
{
	int iNested=0;
	CStdString strReturn="";
	for (int i=0; i < (int) strHTML.size(); ++i)
	{
		if (strHTML[i] == '<') iNested++;
		else if (strHTML[i] == '>') iNested--;
		else
		{
			if (!iNested)
			{
				strReturn+=strHTML[i];
			}
		}
	}
	strHTML=strReturn;
}


void CHTMLUtil::ConvertHTMLToAnsi(const CStdString& strHTML, string& strStripped)
{
	int i=0; 
	if (strHTML.size()==0)
	{
		strStripped="";
		return;
	}
	int iAnsiPos=0;
	char *szAnsi = new char[strHTML.size()*2];

	while (i < (int)strHTML.size() )
	{
		char kar=strHTML[i];
		if (kar=='&')
		{
			if (strHTML[i+1]=='#')
			{
				int ipos=0;
				i+=2;
				char szDigit[12];
				while ( ipos < 12 && strHTML[i] && isdigit(strHTML[i])) 
				{
					szDigit[ipos]=strHTML[i];
					szDigit[ipos+1]=0;
					ipos++;
					i++;
				}
				szAnsi[iAnsiPos++] = (char)(atoi(szDigit));
				i++;
			}
			else
			{
				i++;
				int ipos=0;
				char szKey[112];
				while (strHTML[i] && strHTML[i] != ';' && ipos < 12)
				{
					szKey[ipos]=tolower((unsigned char)strHTML[i]);
					szKey[ipos+1]=0;
					ipos++;
					i++;
				}
				i++;
				if (strcmp(szKey,"amp")==0) szAnsi[iAnsiPos++] ='&';
				if (strcmp(szKey,"nbsp")==0) szAnsi[iAnsiPos++] =' ';
			}
		}
		else
		{
			szAnsi[iAnsiPos++] =kar;
			i++;
		}
	}
	szAnsi[iAnsiPos++]=0;
	strStripped=szAnsi;
	delete [] szAnsi;;
}