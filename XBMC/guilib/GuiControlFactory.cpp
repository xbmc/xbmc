#include "stdafx.h"
#include "guicontrolfactory.h"
#include "localizestrings.h"
#include "guiButtoncontrol.h"
#include "guiSpinButtonControl.h"
#include "guiRadiobuttoncontrol.h"
#include "guiSpinControl.h"
#include "guiRSSControl.h"
#include "guiRAMControl.h"
#include "guiListControl.h"
#include "guiListControlEx.h"
#include "guiImage.h"
#include "GUILabelControl.h"
#include "GUIFadeLabelControl.h"
#include "GUICheckMarkControl.h"
#include "GUIThumbnailPanel.h"
#include "GUIMButtonControl.h"
#include "GUIToggleButtonControl.h" 
#include "GUITextBox.h" 
#include "guiVideoControl.h"
#include "GUIProgressControl.h"
#include "GUISliderControl.h"
#include "GUISelectButtonControl.h"

CGUIControlFactory::CGUIControlFactory(void)
{
}

CGUIControlFactory::~CGUIControlFactory(void)
{
}

bool CGUIControlFactory::GetHex(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwHexValue)
{

	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	sscanf(pNode->FirstChild()->Value(),"%x", &dwHexValue );
	return true;
}
bool CGUIControlFactory::GetDWORD(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwDWORDValue)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	dwDWORDValue= atol(pNode->FirstChild()->Value());
	return true;
}
bool CGUIControlFactory::GetLong(const TiXmlNode* pRootNode, const char* strTag, long& lLongValue)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	lLongValue = atol(pNode->FirstChild()->Value());
	return true;
}
bool CGUIControlFactory::GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	iIntValue = atoi(pNode->FirstChild()->Value());
	return true;
}

bool CGUIControlFactory::GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	CStdString strEnabled=pNode->FirstChild()->Value();
	strEnabled.ToLower();
	if (strEnabled=="off" || strEnabled=="no"||strEnabled=="disabled") bBoolValue=false;
	else bBoolValue=true;
	return true;
}

bool CGUIControlFactory::GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	strStringValue=pNode->FirstChild()->Value();
	return true;
}

bool CGUIControlFactory::GetPath(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringPath)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	strStringPath=pNode->FirstChild()->Value();
    strStringPath.Replace('/','\\');
	return true;
}

bool CGUIControlFactory::GetAlignment(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;

	CStdString strAlign=pNode->FirstChild()->Value();      
	if (strAlign=="right") dwAlignment=XBFONT_RIGHT;
	else if (strAlign=="center") dwAlignment=XBFONT_CENTER_X;
	else  dwAlignment=XBFONT_LEFT;
	return true;
}

bool CGUIControlFactory::GetAlignmentY(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode)
	{
		return false;
	}

	CStdString strAlign=pNode->FirstChild()->Value();      

	dwAlignment=0;
	if (strAlign=="center")
	{
		dwAlignment=XBFONT_CENTER_Y;
	}

	return true;
}


CGUIControl* CGUIControlFactory::Create(DWORD dwParentId,const TiXmlNode* pControlNode, CGUIControl* pReference, RESOLUTION res)
{
	CStdString strType;
	GetString(pControlNode,"type",strType);

	DWORD		dwPosX=0,dwPosY=0;
	DWORD  		dwWidth=0, dwHeight=0;
	DWORD  		dwID=0,left=0,right=0,up=0,down=0;
	DWORD	 	dwColorDiffuse=0xFFFFFFFF;
	bool   		bVisible=true;
	wstring		strLabel=L"";
	CStdString  strFont="";
	CStdString  strTmp;
	DWORD     	dwTextColor=0xFFFFFFFF;
	DWORD		dwAlign=XBFONT_LEFT;
	DWORD		dwAlignY=XBFONT_CENTER_Y;
	CStdString  strTextureFocus,strTextureNoFocus,strTextureUpFocus,strTextureDownFocus;
	CStdString	strTextureAltFocus,strTextureAltNoFocus;
	DWORD		dwDisabledColor=0xffffffff;;
	int			iHyperLink=WINDOW_INVALID;
	DWORD		dwItems;
	CStdString  strUp,strDown;
	CStdString  strUpFocus,strDownFocus;
	DWORD		dwSpinColor=0xffffffff;
	DWORD		dwSpinWidth,dwSpinHeight,dwSpinPosX,dwSpinPosY;
	CStdString  strTextureCheckMark;
	CStdString  strTextureCheckMarkNF;
	DWORD		dwCheckWidth, dwCheckHeight;
	CStdString	strTextureRadioFocus,strTextureRadioNoFocus;
	CStdString	strSubType;
	int			iType=SPIN_CONTROL_TYPE_TEXT;
	bool		bReverse=false;
	bool        bShadow;
	CStdString	strTextureBg, strLeft,strRight,strMid,strMidFocus;
	CStdString	strLeftFocus, strRightFocus;
	CStdString	strTexture;
	DWORD 		dwColorKey=0xffffffff;
	DWORD 		dwSelectedColor;
	CStdString 	strButton,strButtonFocus;
	CStdString 	strSuffix="";
	CStdString 	strFont2="";
	
	DWORD		dwTextOffsetX	 = 10;
	DWORD		dwTextOffsetY	 = 2;
	DWORD		dwControlOffsetX = 0;
	DWORD		dwControlOffsetY = 0;

	int 		iTextXOff=0;
	int 		iTextYOff=0;
	int 		iTextXOff2=0;
	int 		iTextYOff2=0;
	DWORD		dwitemWidth=16, dwitemHeight=16;
	DWORD       textureWidthBig=128;
	DWORD       textureHeightBig=128;
	DWORD       itemWidthBig=150;
	DWORD       itemHeightBig=150;
	DWORD		dwDisposition=0;
	DWORD		dwTextColor2=dwTextColor;
	DWORD		dwTextColor3=dwTextColor;
	DWORD		dwSelectedColor2;
	int        	iSpace=2;
	int			iTextureHeight=30;
	CStdString 	strImage,strImageFocus;
	int			iTextureWidth=80;
	bool		bHasPath=false;
	CStdString	strScriptAction="";
	CStdString	strRSSUrl="";
	CStdString  strTitle="";

	DWORD		dwThumbWidth = 80;
	DWORD		dwThumbHeight = 128;
	DWORD		dwThumbSpaceX = 6;
	DWORD		dwThumbSpaceY	= 25;
	DWORD		dwTextSpaceY = 12;
	CStdString	strDefaultThumb;

	int         iThumbXPos=4;
	int         iThumbYPos=10;
	int         iThumbWidth=64;
	int         iThumbHeight=64;

	int         iThumbXPosBig=14;
	int         iThumbYPosBig=14;
	int         iThumbWidthBig=100;
	int         iThumbHeightBig=100;
	DWORD		dwBuddyControlID=0;
	
	/////////////////////////////////////////////////////////////////////////////
	// Read default properties from reference controls
	//

	if (pReference)
	{
		bVisible				= pReference->IsVisible();
		dwColorDiffuse			= pReference->GetColourDiffuse();
		dwPosX					= pReference->GetXPosition();
		dwPosY					= pReference->GetYPosition();
		dwWidth					= pReference->GetWidth();
		dwHeight				= pReference->GetHeight();
		if (strType=="label")
		{
			strFont				= ((CGUILabelControl*)pReference)->GetFontName();
			strLabel			= ((CGUILabelControl*)pReference)->GetLabel();
			dwTextColor			= ((CGUILabelControl*)pReference)->GetTextColor();
			dwAlign				= ((CGUILabelControl*)pReference)->m_dwTextAlign;
			dwDisabledColor		= ((CGUILabelControl*)pReference)->GetDisabledColor();
		}
		else if (strType=="fadelabel")
		{
			strFont				= ((CGUIFadeLabelControl*)pReference)->GetFontName();
			dwTextColor			= ((CGUIFadeLabelControl*)pReference)->GetTextColor();
			dwAlign				= ((CGUIFadeLabelControl*)pReference)->GetAlignment();
		}
		else if (strType=="rss")
		{
			strFont				= ((CGUIRSSControl*)pReference)->GetFontName();
			strRSSUrl			= ((CGUIRSSControl*)pReference)->GetUrl();
			dwTextColor3		= ((CGUIRSSControl*)pReference)->GetChannelTextColor();
			dwTextColor2		= ((CGUIRSSControl*)pReference)->GetHeadlineTextColor();
			dwTextColor			= ((CGUIRSSControl*)pReference)->GetNormalTextColor();
		}
		else if (strType=="ram")
		{
			strFont				= ((CGUIRAMControl*)pReference)->GetFontName();
			strFont2			= ((CGUIRAMControl*)pReference)->GetFont2Name();
			dwTextColor3		= ((CGUIRAMControl*)pReference)->GetTitleTextColor();
			dwTextColor			= ((CGUIRAMControl*)pReference)->GetNormalTextColor();
			dwSelectedColor		= ((CGUIRAMControl*)pReference)->GetSelectedTextColor();
			dwTextOffsetX		= ((CGUIRAMControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY		= ((CGUIRAMControl*)pReference)->GetTextOffsetY();
			dwTextSpaceY		= ((CGUIRAMControl*)pReference)->GetTextSpacing();

			((CGUIRAMControl*)pReference)->GetThumbAttributes(dwThumbWidth,dwThumbHeight,dwThumbSpaceX,dwThumbSpaceY,strDefaultThumb);

		}
		else if (strType=="button")
		{
			strTextureFocus		= ((CGUIButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus	= ((CGUIButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont				= ((CGUIButtonControl*)pReference)->GetFontName();
			strLabel			= ((CGUIButtonControl*)pReference)->GetLabel();
			dwTextColor			= ((CGUIButtonControl*)pReference)->GetTextColor();
			dwDisabledColor		= ((CGUIButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink			= ((CGUIButtonControl*)pReference)->GetHyperLink();
			strScriptAction		= ((CGUIButtonControl*)pReference)->GetScriptAction();
			dwTextOffsetX		= ((CGUIButtonControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY		= ((CGUIButtonControl*)pReference)->GetTextOffsetY();
		}
		else if (strType=="spinbutton")
		{
			iType				= ((CGUISpinButtonControl*)pReference)->GetType();
			strTextureDownFocus	= ((CGUISpinButtonControl*)pReference)->GetTexutureDownFocusName();
			strTextureUpFocus	= ((CGUISpinButtonControl*)pReference)->GetTexutureUpFocusName();
			strTextureFocus		= ((CGUISpinButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus	= ((CGUISpinButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont				= ((CGUISpinButtonControl*)pReference)->GetFontName();
			dwTextColor			= ((CGUISpinButtonControl*)pReference)->GetTextColor();
			dwDisabledColor		= ((CGUISpinButtonControl*)pReference)->GetDisabledColor() ;
		}
		else if (strType=="togglebutton")
		{
			strTextureAltFocus	= ((CGUIToggleButtonControl*)pReference)->GetTexutureAltFocusName();
			strTextureAltNoFocus= ((CGUIToggleButtonControl*)pReference)->GetTexutureAltNoFocusName();
			strTextureFocus		= ((CGUIToggleButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus	= ((CGUIToggleButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont				= ((CGUIToggleButtonControl*)pReference)->GetFontName();
			strLabel			= ((CGUIToggleButtonControl*)pReference)->GetLabel();
			dwTextColor			= ((CGUIToggleButtonControl*)pReference)->GetTextColor();
			dwDisabledColor		= ((CGUIToggleButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink			= ((CGUIToggleButtonControl*)pReference)->GetHyperLink();
			dwTextOffsetX		= ((CGUIToggleButtonControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY		= ((CGUIToggleButtonControl*)pReference)->GetTextOffsetY();
		}
		else if (strType=="buttonM")
		{
			dwItems				= ((CGUIMButtonControl*)pReference)->GetItems();
			strTextureFocus		= ((CGUIMButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus	= ((CGUIMButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont				= ((CGUIMButtonControl*)pReference)->GetFontName();
			strLabel			= ((CGUIMButtonControl*)pReference)->GetLabel();
			dwTextColor			= ((CGUIMButtonControl*)pReference)->GetTextColor();
			dwDisabledColor		= ((CGUIMButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink			= ((CGUIMButtonControl*)pReference)->GetHyperLink();
			dwTextOffsetX		= ((CGUIMButtonControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY		= ((CGUIMButtonControl*)pReference)->GetTextOffsetY();
		}
		else if (strType=="checkmark")
		{
			bShadow             = ((CGUICheckMarkControl*)pReference)->GetShadow();
			strTextureCheckMark	= ((CGUICheckMarkControl*)pReference)->GetCheckMarkTextureName();
			strTextureCheckMarkNF= ((CGUICheckMarkControl*)pReference)->GetCheckMarkTextureNameNF();
			dwCheckWidth		= ((CGUICheckMarkControl*)pReference)->GetCheckMarkWidth();
			dwCheckHeight		= ((CGUICheckMarkControl*)pReference)->GetCheckMarkHeight();
			dwAlign				= ((CGUICheckMarkControl*)pReference)->GetAlignment();
			strFont				= ((CGUICheckMarkControl*)pReference)->GetFontName();
			strLabel			= ((CGUICheckMarkControl*)pReference)->GetLabel();
			dwTextColor			= ((CGUICheckMarkControl*)pReference)->GetTextColor();
			dwDisabledColor		= ((CGUICheckMarkControl*)pReference)->GetDisabledColor();
		}
		else if (strType=="radiobutton")
		{
			strTextureRadioFocus= ((CGUIRadioButtonControl*)pReference)->GetTexutureRadioFocusName();;
			strTextureRadioNoFocus= ((CGUIRadioButtonControl*)pReference)->GetTexutureRadioNoFocusName();;
			strTextureFocus		= ((CGUIRadioButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus	= ((CGUIRadioButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont				= ((CGUIRadioButtonControl*)pReference)->GetFontName();
			strLabel			= ((CGUIRadioButtonControl*)pReference)->GetLabel();
			dwTextColor			= ((CGUIRadioButtonControl*)pReference)->GetTextColor();
			dwDisabledColor		= ((CGUIRadioButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink			= ((CGUIRadioButtonControl*)pReference)->GetHyperLink();
			dwTextOffsetX		= ((CGUIRadioButtonControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY		= ((CGUIRadioButtonControl*)pReference)->GetTextOffsetY();
		}
		else if (strType=="spincontrol")
		{
			strFont				= ((CGUISpinControl*)pReference)->GetFontName();
			dwTextColor			= ((CGUISpinControl*)pReference)->GetTextColor();
			dwAlign				= ((CGUISpinControl*)pReference)->GetAlignment();
			dwAlignY			= ((CGUISpinControl*)pReference)->GetAlignmentY();
			strUp				= ((CGUISpinControl*)pReference)->GetTexutureUpName();
			strDown				= ((CGUISpinControl*)pReference)->GetTexutureDownName();
			strUpFocus			= ((CGUISpinControl*)pReference)->GetTexutureUpFocusName();
			strDownFocus		= ((CGUISpinControl*)pReference)->GetTexutureDownFocusName();
			iType				= ((CGUISpinControl*)pReference)->GetType();
			dwWidth				= ((CGUISpinControl*)pReference)->GetSpinWidth();
			dwHeight			= ((CGUISpinControl*)pReference)->GetSpinHeight();
			dwDisabledColor		= ((CGUISpinControl*)pReference)->GetDisabledColor() ;
			dwTextOffsetX		= ((CGUISpinControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY		= ((CGUISpinControl*)pReference)->GetTextOffsetY();
		}
		else if (strType=="slider")
		{
			strTextureBg		= ((CGUISliderControl*)pReference)->GetBackGroundTextureName();
			strMid				= ((CGUISliderControl*)pReference)->GetBackTextureMidName();
			dwControlOffsetX	= ((CGUISliderControl*)pReference)->GetControlOffsetX();
			dwControlOffsetY	= ((CGUISliderControl*)pReference)->GetControlOffsetY();
		}
		else if (strType=="progress")
		{
			strTextureBg		= ((CGUIProgressControl*)pReference)->GetBackGroundTextureName();
			strLeft				= ((CGUIProgressControl*)pReference)->GetBackTextureLeftName();
			strMid				= ((CGUIProgressControl*)pReference)->GetBackTextureMidName();
			strRight			= ((CGUIProgressControl*)pReference)->GetBackTextureRightName();
		}
		else if (strType=="image")
		{
			strTexture			= ((CGUIImage *)pReference)->GetFileName();
			dwColorKey			= ((CGUIImage *)pReference)->GetColorKey();
		}
		else if (strType=="listcontrol")
		{
			strFont				= ((CGUIListControl*)pReference)->GetFontName();
			dwSpinWidth			= ((CGUIListControl*)pReference)->GetSpinWidth();
			dwSpinHeight		= ((CGUIListControl*)pReference)->GetSpinHeight();
			strUp				= ((CGUIListControl*)pReference)->GetTexutureUpName();
			strDown				= ((CGUIListControl*)pReference)->GetTexutureDownName();
			strUpFocus			= ((CGUIListControl*)pReference)->GetTexutureUpFocusName();
			strDownFocus		= ((CGUIListControl*)pReference)->GetTexutureDownFocusName();
			dwSpinColor			= ((CGUIListControl*)pReference)->GetSpinTextColor();
			dwSpinPosX			= ((CGUIListControl*)pReference)->GetSpinX();
			dwSpinPosY			= ((CGUIListControl*)pReference)->GetSpinY();
			dwTextColor			= ((CGUIListControl*)pReference)->GetTextColor();
			dwSelectedColor		= ((CGUIListControl*)pReference)->GetSelectedColor();
			strButton			= ((CGUIListControl*)pReference)->GetButtonNoFocusName();
			strButtonFocus		= ((CGUIListControl*)pReference)->GetButtonFocusName();
			strSuffix			= ((CGUIListControl*)pReference)->GetSuffix();
			iTextXOff			= ((CGUIListControl*)pReference)->GetTextOffsetX();
			iTextYOff			= ((CGUIListControl*)pReference)->GetTextOffsetY();
			iTextXOff2			= ((CGUIListControl*)pReference)->GetTextOffsetX2();
			iTextYOff2			= ((CGUIListControl*)pReference)->GetTextOffsetY2();
			dwitemWidth			= ((CGUIListControl*)pReference)->GetImageWidth();
			dwitemHeight		= ((CGUIListControl*)pReference)->GetImageHeight();
			iTextureHeight		= ((CGUIListControl*)pReference)->GetItemHeight();
			iSpace				= ((CGUIListControl*)pReference)->GetSpace();
			dwTextColor2		= ((CGUIListControl*)pReference)->GetTextColor2(); 
			dwSelectedColor2	= ((CGUIListControl*)pReference)->GetSelectedColor2();
			strFont2			= ((CGUIListControl*)pReference)->GetFontName2();
			dwTextOffsetX		= ((CGUIListControl*)pReference)->GetButtonTextOffsetX();
			dwTextOffsetY		= ((CGUIListControl*)pReference)->GetButtonTextOffsetY();
		}
		else if (strType=="listcontrolex")
		{
			strFont				= ((CGUIListControlEx*)pReference)->GetFontName();
			dwSpinWidth			= ((CGUIListControlEx*)pReference)->GetSpinWidth();
			dwSpinHeight		= ((CGUIListControlEx*)pReference)->GetSpinHeight();
			strUp				= ((CGUIListControlEx*)pReference)->GetTexutureUpName();
			strDown				= ((CGUIListControlEx*)pReference)->GetTexutureDownName();
			strUpFocus			= ((CGUIListControlEx*)pReference)->GetTexutureUpFocusName();
			strDownFocus		= ((CGUIListControlEx*)pReference)->GetTexutureDownFocusName();
			dwSpinColor			= ((CGUIListControlEx*)pReference)->GetSpinTextColor();
			dwSpinPosX			= ((CGUIListControlEx*)pReference)->GetSpinX();
			dwSpinPosY			= ((CGUIListControlEx*)pReference)->GetSpinY();
			dwTextColor			= ((CGUIListControlEx*)pReference)->GetTextColor();
			dwSelectedColor		= ((CGUIListControlEx*)pReference)->GetSelectedColor();
			strButton			= ((CGUIListControlEx*)pReference)->GetButtonNoFocusName();
			strButtonFocus		= ((CGUIListControlEx*)pReference)->GetButtonFocusName();
			strSuffix			= ((CGUIListControlEx*)pReference)->GetSuffix();
			dwitemWidth			= ((CGUIListControlEx*)pReference)->GetImageWidth();
			dwitemHeight		= ((CGUIListControlEx*)pReference)->GetImageHeight();
			iTextureHeight		= ((CGUIListControlEx*)pReference)->GetItemHeight();
			iSpace				= ((CGUIListControlEx*)pReference)->GetSpace();
			dwTextColor2		= ((CGUIListControlEx*)pReference)->GetTextColor2(); 
			dwSelectedColor2	= ((CGUIListControlEx*)pReference)->GetSelectedColor2();
			strFont2			= ((CGUIListControlEx*)pReference)->GetFontName2();
			dwTextOffsetX		= ((CGUIListControlEx*)pReference)->GetTextOffsetX();
			dwTextOffsetY		= ((CGUIListControlEx*)pReference)->GetTextOffsetY();
		}
		else if (strType=="textbox")
		{
			strFont				= ((CGUITextBox*)pReference)->GetFontName();
			dwTextColor			= ((CGUITextBox*)pReference)->GetTextColor();
			dwSpinWidth			= ((CGUITextBox*)pReference)->GetSpinWidth();
			dwSpinHeight		= ((CGUITextBox*)pReference)->GetSpinHeight();
			strUp				= ((CGUITextBox*)pReference)->GetTexutureUpName();
			strDown				= ((CGUITextBox*)pReference)->GetTexutureDownName();
			strUpFocus			= ((CGUITextBox*)pReference)->GetTexutureUpFocusName();
			strDownFocus		= ((CGUITextBox*)pReference)->GetTexutureDownFocusName();
			dwSpinColor			= ((CGUITextBox*)pReference)->GetSpinTextColor();
			dwSpinPosX			= ((CGUITextBox*)pReference)->GetSpinX();
			dwSpinPosY			= ((CGUITextBox*)pReference)->GetSpinY();
			dwSpinWidth			= ((CGUITextBox*)pReference)->GetSpinWidth();
			dwSpinHeight 		= ((CGUITextBox*)pReference)->GetSpinHeight();
		}
		else if (strType=="thumbnailpanel")
		{      
			textureWidthBig		= ((CGUIThumbnailPanel*)pReference)->GetTextureWidthBig();
			textureHeightBig	= ((CGUIThumbnailPanel*)pReference)->GetTextureHeightBig();
			itemWidthBig		= ((CGUIThumbnailPanel*)pReference)->GetItemWidthBig();
			itemHeightBig		= ((CGUIThumbnailPanel*)pReference)->GetItemHeightBig();
			strFont				= ((CGUIThumbnailPanel*)pReference)->GetFontName();
			strImage			= ((CGUIThumbnailPanel*)pReference)->GetNoFocusName();
			strImageFocus		= ((CGUIThumbnailPanel*)pReference)->GetFocusName();
			dwitemWidth			= ((CGUIThumbnailPanel*)pReference)->GetItemWidth();
			dwitemHeight		= ((CGUIThumbnailPanel*)pReference)->GetItemHeight();
			dwSpinWidth			= ((CGUIThumbnailPanel*)pReference)->GetSpinWidth();
			dwSpinHeight		= ((CGUIThumbnailPanel*)pReference)->GetSpinHeight();
			strUp				= ((CGUIThumbnailPanel*)pReference)->GetTexutureUpName();
			strDown				= ((CGUIThumbnailPanel*)pReference)->GetTexutureDownName();
			strUpFocus			= ((CGUIThumbnailPanel*)pReference)->GetTexutureUpFocusName();
			strDownFocus		= ((CGUIThumbnailPanel*)pReference)->GetTexutureDownFocusName();
			dwSpinColor			= ((CGUIThumbnailPanel*)pReference)->GetSpinTextColor();
			dwSpinPosX			= ((CGUIThumbnailPanel*)pReference)->GetSpinX();
			dwSpinPosY			= ((CGUIThumbnailPanel*)pReference)->GetSpinY();
			dwTextColor			= ((CGUIThumbnailPanel*)pReference)->GetTextColor();
			dwSelectedColor		= ((CGUIThumbnailPanel*)pReference)->GetSelectedColor();
			iTextureWidth		= ((CGUIThumbnailPanel*)pReference)->GetTextureWidth();
			iTextureHeight		= ((CGUIThumbnailPanel*)pReference)->GetTextureHeight();
			((CGUIThumbnailPanel*)pReference)->GetThumbDimensions(iThumbXPos, iThumbYPos,iThumbWidth, iThumbHeight);
			((CGUIThumbnailPanel*)pReference)->GetThumbDimensionsBig(iThumbXPosBig, iThumbYPosBig,iThumbWidthBig, iThumbHeightBig);      
		}
		else if (strType=="selectbutton")
		{
			strTextureBg  		= ((CGUISelectButtonControl*)pReference)->GetTextureBackground();
			strLeft				= ((CGUISelectButtonControl*)pReference)->GetTextureLeft();
			strLeftFocus		= ((CGUISelectButtonControl*)pReference)->GetTextureLeftFocus();
			strRight			= ((CGUISelectButtonControl*)pReference)->GetTextureRight();
			strRightFocus		= ((CGUISelectButtonControl*)pReference)->GetTextureRightFocus();
			strTextureFocus		= ((CGUISelectButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus	= ((CGUISelectButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont				= ((CGUISelectButtonControl*)pReference)->GetFontName();
			strLabel			= ((CGUISelectButtonControl*)pReference)->GetLabel();
			dwTextColor			= ((CGUISelectButtonControl*)pReference)->GetTextColor();
			dwDisabledColor		= ((CGUISelectButtonControl*)pReference)->GetDisabledColor() ;
			dwTextOffsetX		= ((CGUISelectButtonControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY		= ((CGUISelectButtonControl*)pReference)->GetTextOffsetY();
		}
	}
	
	/////////////////////////////////////////////////////////////////////////////
	// Read control properties from XML
	//

	if (!GetDWORD(pControlNode, "id", dwID))
	{
		return NULL; // NO id????
	}
  
	if (GetDWORD(pControlNode, "posX", dwPosX)) g_graphicsContext.ScaleXCoord(dwPosX, res);
	if (GetDWORD(pControlNode, "posY", dwPosY)) g_graphicsContext.ScaleYCoord(dwPosY, res);
	if (GetDWORD(pControlNode, "width", dwWidth)) g_graphicsContext.ScaleXCoord(dwWidth, res);
	if (GetDWORD(pControlNode, "height", dwHeight)) g_graphicsContext.ScaleYCoord(dwHeight, res);
	if (GetDWORD(pControlNode, "textOffsetX", dwTextOffsetX)) g_graphicsContext.ScaleXCoord(dwTextOffsetX, res);
	if (GetDWORD(pControlNode, "textOffsetY", dwTextOffsetY)) g_graphicsContext.ScaleYCoord(dwTextOffsetY, res);
	if (GetDWORD(pControlNode, "textSpaceY",  dwTextSpaceY)) g_graphicsContext.ScaleYCoord(dwTextSpaceY, res);

	if (GetDWORD(pControlNode, "controlOffsetX", dwControlOffsetX)) g_graphicsContext.ScaleXCoord(dwControlOffsetX, res);
	if (GetDWORD(pControlNode, "controlOffsetY", dwControlOffsetY)) g_graphicsContext.ScaleYCoord(dwControlOffsetY, res);

	if (GetDWORD(pControlNode, "gfxThumbWidth",  dwThumbWidth)) g_graphicsContext.ScaleXCoord(dwThumbWidth, res);
	if (GetDWORD(pControlNode, "gfxThumbHeight", dwThumbHeight)) g_graphicsContext.ScaleYCoord(dwThumbHeight, res);
	if (GetDWORD(pControlNode, "gfxThumbSpaceX", dwThumbSpaceX)) g_graphicsContext.ScaleXCoord(dwThumbSpaceX, res);
	if (GetDWORD(pControlNode, "gfxThumbSpaceY", dwThumbSpaceY)) g_graphicsContext.ScaleYCoord(dwThumbSpaceY, res);
	GetString(pControlNode,"gfxThumbDefault",strDefaultThumb);

	if (!GetDWORD(pControlNode, "onup"   , up  ))
	{
		up    = dwID-1;
	}
	if (!GetDWORD(pControlNode, "ondown" , down))
	{
		down  = dwID+1;
	}
	if (!GetDWORD(pControlNode, "onleft" , left  ))
	{
		left  = dwID;
	}
	if (!GetDWORD(pControlNode, "onright", right))
	{
		right = dwID;
	}

	GetHex(pControlNode, "colordiffuse", dwColorDiffuse);
 	GetBoolean(pControlNode,"visible",bVisible);
	GetString(pControlNode,"font", strFont);
	GetAlignment(pControlNode,"align", dwAlign);
	GetAlignmentY(pControlNode,"alignY", dwAlignY);
	GetInt(pControlNode,"hyperlink",iHyperLink);
	// windows are referenced from WINDOW_HOME
	if (iHyperLink != WINDOW_INVALID) iHyperLink += WINDOW_HOME;

	GetString(pControlNode,"script", strScriptAction);
	GetHex(pControlNode,"disabledcolor",dwDisabledColor);
	GetPath(pControlNode,"textureDownFocus",strTextureDownFocus);
	GetPath(pControlNode,"textureUpFocus",strTextureUpFocus);
	GetPath(pControlNode,"textureFocus",strTextureFocus);
	GetPath(pControlNode,"textureNoFocus",strTextureNoFocus);
	GetPath(pControlNode,"AltTextureFocus",strTextureAltFocus);
	GetPath(pControlNode,"AltTextureNoFocus",strTextureAltNoFocus);
	GetDWORD(pControlNode,"bitmaps", dwItems);
	GetHex(pControlNode, "textcolor", dwTextColor);

 	GetBoolean(pControlNode,"hasPath",bHasPath);
 	GetBoolean(pControlNode,"shadow",bShadow);

	GetPath(pControlNode,"textureUp",strUp);
	GetPath(pControlNode,"textureDown",strDown);
	GetPath(pControlNode,"textureUpFocus",strUpFocus);
	GetPath(pControlNode,"textureDownFocus",strDownFocus);

	GetPath(pControlNode,"textureLeft",strLeft);
	GetPath(pControlNode,"textureRight",strRight);
	GetPath(pControlNode,"textureLeftFocus",strLeftFocus);
	GetPath(pControlNode,"textureRightFocus",strRightFocus);

	GetHex(pControlNode,"spinColor",dwSpinColor);
	if (GetDWORD(pControlNode,"spinWidth",dwSpinWidth)) g_graphicsContext.ScaleXCoord(dwSpinWidth, res);
	if (GetDWORD(pControlNode,"spinHeight",dwSpinHeight)) g_graphicsContext.ScaleYCoord(dwSpinHeight, res);
	if (GetDWORD(pControlNode,"spinPosX",dwSpinPosX)) g_graphicsContext.ScaleXCoord(dwSpinPosX, res);
	if (GetDWORD(pControlNode,"spinPosY",dwSpinPosY)) g_graphicsContext.ScaleYCoord(dwSpinPosY, res);      

	if (GetDWORD(pControlNode,"MarkWidth",dwCheckWidth)) g_graphicsContext.ScaleXCoord(dwCheckWidth, res);
	if (GetDWORD(pControlNode,"MarkHeight",dwCheckHeight)) g_graphicsContext.ScaleYCoord(dwCheckHeight, res);
	GetPath(pControlNode,"textureCheckmark",strTextureCheckMark);
	GetPath(pControlNode,"textureCheckmarkNoFocus",strTextureCheckMarkNF);
	GetPath(pControlNode,"textureRadioFocus",strTextureRadioFocus);
	GetPath(pControlNode,"textureRadioNoFocus",strTextureRadioNoFocus);

	GetPath(pControlNode,"textureSliderBar", strTextureBg);
	GetPath(pControlNode,"textureSliderNib", strMid);
	GetPath(pControlNode,"textureSliderNibFocus", strMidFocus);
	GetDWORD(pControlNode,"disposition",dwDisposition);
	GetString(pControlNode,"feed",strRSSUrl);
	GetString(pControlNode,"title",strTitle);
	GetHex(pControlNode, "headlinecolor", dwTextColor2);
	GetHex(pControlNode, "titlecolor", dwTextColor3);

	if (GetString(pControlNode,"subtype",strSubType))
	{
		strSubType.ToLower();

		if ( strSubType=="int")
		{
			iType=SPIN_CONTROL_TYPE_INT;
		}
		else if ( strSubType=="float")
		{
			iType=SPIN_CONTROL_TYPE_FLOAT;
		}
		else
		{
			iType=SPIN_CONTROL_TYPE_TEXT;
		}
	}

	GetBoolean(pControlNode,"reverse",bReverse);

	GetPath(pControlNode,"texturebg",strTextureBg);
	GetPath(pControlNode,"lefttexture",strLeft);
	GetPath(pControlNode,"midtexture",strMid);
	GetPath(pControlNode,"righttexture",strRight);
	GetPath(pControlNode,"texture",strTexture);
	GetHex(pControlNode,"colorkey",dwColorKey);

	GetHex(pControlNode,"selectedColor",dwSelectedColor);
	dwSelectedColor2=dwSelectedColor;

	GetPath(pControlNode,"textureNoFocus",strButton);
	GetPath(pControlNode,"textureFocus",strButtonFocus);
	GetString(pControlNode,"suffix",strSuffix);
	if (GetInt(pControlNode,"textXOff",iTextXOff)) g_graphicsContext.ScaleXCoord(iTextXOff, res);
	if (GetInt(pControlNode,"textYOff",iTextYOff)) g_graphicsContext.ScaleYCoord(iTextYOff, res);
	if (GetInt(pControlNode,"textXOff2",iTextXOff2)) g_graphicsContext.ScaleXCoord(iTextXOff2, res);
	if (GetInt(pControlNode,"textYOff2",iTextYOff2)) g_graphicsContext.ScaleYCoord(iTextYOff2, res);

	if (GetDWORD(pControlNode,"itemWidth",dwitemWidth)) g_graphicsContext.ScaleXCoord(dwitemWidth, res);
	if (GetDWORD(pControlNode,"itemHeight",dwitemHeight)) g_graphicsContext.ScaleYCoord(dwitemHeight, res);
	if (GetInt(pControlNode,"spaceBetweenItems",iSpace)) g_graphicsContext.ScaleYCoord(iSpace, res);

	GetHex(pControlNode,"selectedColor2",dwSelectedColor2);
	GetHex(pControlNode,"textcolor2",dwTextColor2);
	GetString(pControlNode,"font2",strFont2);
	GetHex(pControlNode,"selectedColor",dwSelectedColor);

	GetPath(pControlNode,"imageFolder",strImage);
	GetPath(pControlNode,"imageFolderFocus",strImageFocus);
	if (GetInt(pControlNode,"textureWidth",iTextureWidth)) g_graphicsContext.ScaleXCoord(iTextureWidth, res);
	if (GetInt(pControlNode,"textureHeight",iTextureHeight)) g_graphicsContext.ScaleYCoord(iTextureHeight, res);
  
	if (GetInt(pControlNode,"thumbWidth",iThumbWidth)) g_graphicsContext.ScaleXCoord(iThumbWidth, res);
	if (GetInt(pControlNode,"thumbHeight",iThumbHeight)) g_graphicsContext.ScaleYCoord(iThumbHeight, res);
	if (GetInt(pControlNode,"thumbPosX",iThumbXPos)) g_graphicsContext.ScaleXCoord(iThumbXPos, res);
	if (GetInt(pControlNode,"thumbPosY",iThumbYPos)) g_graphicsContext.ScaleYCoord(iThumbYPos, res);
  
	if (GetInt(pControlNode,"thumbWidthBig",iThumbWidthBig)) g_graphicsContext.ScaleXCoord(iThumbWidthBig, res);
	if (GetInt(pControlNode,"thumbHeightBig",iThumbHeightBig)) g_graphicsContext.ScaleYCoord(iThumbHeightBig, res);
	if (GetInt(pControlNode,"thumbPosXBig",iThumbXPosBig)) g_graphicsContext.ScaleXCoord(iThumbXPosBig, res);
	if (GetInt(pControlNode,"thumbPosYBig",iThumbYPosBig)) g_graphicsContext.ScaleYCoord(iThumbYPosBig, res);

	if (GetDWORD(pControlNode,"textureWidthBig",textureWidthBig)) g_graphicsContext.ScaleXCoord(textureWidthBig, res);
	if (GetDWORD(pControlNode,"textureHeightBig",textureHeightBig)) g_graphicsContext.ScaleYCoord(textureHeightBig, res);
	if (GetDWORD(pControlNode,"itemWidthBig",itemWidthBig)) g_graphicsContext.ScaleXCoord(itemWidthBig, res);
	if (GetDWORD(pControlNode,"itemHeightBig",itemHeightBig)) g_graphicsContext.ScaleYCoord(itemHeightBig, res);
	GetDWORD(pControlNode,"buddycontrolid",dwBuddyControlID);
	

	if ( GetString(pControlNode, "label", strTmp))
	{
		if (strTmp.size() > 0)
		{
			if (strTmp[0] != '-') 
			{
				if ((strTmp[0]>='A')&&(strTmp[0]<='z'))
				{
					WCHAR wszTmp[256];
					swprintf(wszTmp,L"%S",strTmp.c_str());
					strLabel = wszTmp;
				}
				else
				{
					DWORD dwLabelID=atol(strTmp);
					strLabel=g_localizeStrings.Get(dwLabelID);
				}
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	// Instantiate a new control using the properties gathered above
	//

	if (strType=="label")
	{
		CGUILabelControl* pControl = new CGUILabelControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strFont,strLabel,dwTextColor,dwDisabledColor,
					dwAlign, bHasPath);

		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		return pControl;
	}
	if (strType=="videowindow")
	{
		CGUIVideoControl* pControl = new CGUIVideoControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight);

		return pControl;
	}
	if (strType=="fadelabel")
	{
		CGUIFadeLabelControl* pControl = new CGUIFadeLabelControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strFont,dwTextColor,dwAlign);

		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		return pControl;
	}  
	if (strType=="spinbutton")
	{
		CGUISpinButtonControl* pControl = new CGUISpinButtonControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strTextureNoFocus,strTextureFocus,
					strTextureUpFocus,strTextureDownFocus,
					strFont,dwTextColor,(int)dwDisposition);

		pControl->SetDisabledColor(dwDisabledColor);
		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		return pControl;
	}
  
	if (strType=="rss")
	{
		CGUIRSSControl* pControl = new CGUIRSSControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strFont,dwTextColor3,dwTextColor2,dwTextColor,strRSSUrl);

		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		return pControl;
	}
	if (strType=="ram")
	{
		CGUIRAMControl* pControl = new CGUIRAMControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strFont,strFont2,dwTextColor3,dwTextColor,
					dwSelectedColor,dwTextOffsetX,dwTextOffsetY);

		pControl->SetTextSpacing(dwTextSpaceY);
		pControl->SetThumbAttributes(dwThumbWidth,dwThumbHeight,dwThumbSpaceX,dwThumbSpaceY,strDefaultThumb);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
	    pControl->SetNavigation(up,down,left,right);
		return pControl;
	}
	if (strType=="button")
	{
		CGUIButtonControl* pControl = new CGUIButtonControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strTextureFocus,strTextureNoFocus,
					dwTextOffsetX,dwTextOffsetY);

		pControl->SetLabel(strFont,strLabel,dwTextColor);
		pControl->SetDisabledColor(dwDisabledColor);
		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetHyperLink(iHyperLink);
		pControl->SetScriptAction(strScriptAction);
		pControl->SetVisible(bVisible);
		return pControl;
	}
	if (strType=="togglebutton")
	{
		CGUIToggleButtonControl* pControl = new CGUIToggleButtonControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strTextureFocus,strTextureNoFocus,
					strTextureAltFocus,strTextureAltNoFocus);

		pControl->SetLabel(strFont,strLabel,dwTextColor);
		pControl->SetDisabledColor(dwDisabledColor);
		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetHyperLink(iHyperLink);
		pControl->SetVisible(bVisible);
		pControl->SetTextOffsetX(dwTextOffsetX);
		pControl->SetTextOffsetY(dwTextOffsetY);
		return pControl;
	}
	if (strType=="buttonM")
	{
		CGUIMButtonControl* pControl = new CGUIMButtonControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					dwItems,strTextureFocus,strTextureNoFocus,
					dwTextOffsetX,dwTextOffsetY);

		pControl->SetLabel(strFont,strLabel,dwTextColor);
		pControl->SetDisabledColor(dwDisabledColor);
		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetHyperLink(iHyperLink);
		pControl->SetVisible(bVisible);
		return pControl;
	}
	if (strType=="checkmark")
	{
		CGUICheckMarkControl* pControl = new CGUICheckMarkControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strTextureCheckMark,strTextureCheckMarkNF,
					dwCheckWidth,dwCheckHeight,dwAlign);

		pControl->SetLabel(strFont,strLabel,dwTextColor);
		pControl->SetDisabledColor(dwDisabledColor);
		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		pControl->SetShadow(bShadow);
		return pControl;
	}
	if (strType=="radiobutton")
	{
		CGUIRadioButtonControl* pControl = new CGUIRadioButtonControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strTextureFocus,strTextureNoFocus,
					dwTextOffsetX,dwTextOffsetY,
					strTextureRadioFocus,strTextureRadioNoFocus);

		pControl->SetLabel(strFont,strLabel,dwTextColor);
		pControl->SetDisabledColor(dwDisabledColor);
		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetHyperLink(iHyperLink);
		pControl->SetVisible(bVisible);
		return pControl;
	}
 	if (strType=="spincontrol")
	{
		CGUISpinControl* pControl = new CGUISpinControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strUp,strDown,strUpFocus,strDownFocus,
					strFont,dwTextColor,iType,dwAlign);

		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		pControl->SetReverse(bReverse);
		pControl->SetBuddyControlID(dwBuddyControlID);
		pControl->SetDisabledColor(dwDisabledColor);
		pControl->SetTextOffsetX(dwTextOffsetX);
		pControl->SetTextOffsetY(dwTextOffsetY);
		pControl->SetAlignmentY(dwAlignY);
		return pControl;
	}
	if (strType=="slider")
	{
		CGUISliderControl* pControl= new CGUISliderControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strTextureBg,strMid,strMidFocus,iType);

		pControl->SetVisible(bVisible);
		pControl->SetNavigation(up,down,left,right);
		pControl->SetControlOffsetX(dwControlOffsetX);
		pControl->SetControlOffsetY(dwControlOffsetY);
		return pControl;
	} 
	if (strType=="progress")
	{
		CGUIProgressControl* pControl= new CGUIProgressControl(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,
					strTextureBg,strLeft,strMid,strRight);

		pControl->SetVisible(bVisible);
		return pControl;
	} 
	if (strType=="image")
	{
		CGUIImage* pControl = new CGUIImage(
			dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,strTexture,dwColorKey);

		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		return pControl;
	}
	if (strType=="listcontrol")
	{
		CGUIListControl* pControl = new CGUIListControl(
						dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
						strFont,
						dwSpinWidth,dwSpinHeight,
						strUp,strDown,
						strUpFocus,strDownFocus,
						dwSpinColor,dwSpinPosX,dwSpinPosY,
						strFont,dwTextColor,dwSelectedColor,
						strButton,strButtonFocus,
						dwTextOffsetX, dwTextOffsetY);

		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetScrollySuffix(strSuffix);
		pControl->SetTextOffsets(iTextXOff,iTextYOff, iTextXOff2,iTextYOff2);
		pControl->SetVisible(bVisible);
		pControl->SetImageDimensions(dwitemWidth, dwitemHeight);
		pControl->SetItemHeight(iTextureHeight);
		pControl->SetSpace(iSpace);
		pControl->SetColors2(dwTextColor2, dwSelectedColor2);
		pControl->SetFont2( strFont2 );
		return pControl;
	}
	if (strType=="listcontrolex")
	{
		CGUIListControlEx* pControl = new CGUIListControlEx(
						dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
						strFont,
						dwSpinWidth,dwSpinHeight,
						strUp,strDown,
						strUpFocus,strDownFocus,
						dwSpinColor,dwSpinPosX,dwSpinPosY,
						strFont,dwTextColor,dwSelectedColor,
						strButton,strButtonFocus,
						dwTextOffsetX, dwTextOffsetY);

		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetScrollySuffix(strSuffix);
		pControl->SetVisible(bVisible);
		pControl->SetImageDimensions(dwitemWidth, dwitemHeight);
		pControl->SetItemHeight(iTextureHeight);
		pControl->SetSpace(iSpace);
		pControl->SetColors2(dwTextColor2, dwSelectedColor2);
		pControl->SetFont2( strFont2 );
		return pControl;
	}
	if (strType=="textbox")
	{
		CGUITextBox* pControl = new CGUITextBox(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
					strFont,
					dwSpinWidth,dwSpinHeight,
					strUp,strDown,
					strUpFocus,strDownFocus,
					dwSpinColor,dwSpinPosX,dwSpinPosY,
					strFont,dwTextColor);

		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		return pControl;
	}
	if (strType=="thumbnailpanel")
	{
		CGUIThumbnailPanel* pControl = new CGUIThumbnailPanel(
					dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
					strFont,
					strImage,strImageFocus,
					dwitemWidth,dwitemHeight,
					dwSpinWidth,dwSpinHeight,
					strUp,strDown,
					strUpFocus,strDownFocus,
					dwSpinColor,dwSpinPosX,dwSpinPosY,
					strFont,dwTextColor,dwSelectedColor);

		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		pControl->SetTextureDimensions(iTextureWidth,iTextureHeight);
		pControl->SetThumbDimensions(iThumbXPos, iThumbYPos,iThumbWidth, iThumbHeight);
		pControl->SetTextureWidthBig(textureWidthBig);
		pControl->SetTextureHeightBig(textureHeightBig);
		pControl->SetItemWidthBig(itemWidthBig);
		pControl->SetItemHeightBig(itemHeightBig);
		pControl->SetTextureWidthLow(iTextureWidth);
		pControl->SetTextureHeightLow(iTextureHeight);
		pControl->SetItemWidthLow(dwitemWidth);
		pControl->SetItemHeightLow(dwitemHeight);
		pControl->SetThumbDimensionsLow(iThumbXPos, iThumbYPos,iThumbWidth, iThumbHeight);
		pControl->SetThumbDimensionsBig(iThumbXPosBig, iThumbYPosBig,iThumbWidthBig, iThumbHeightBig);
		return pControl;
	}
	if (strType=="selectbutton")
	{
		CGUISelectButtonControl* pControl = new CGUISelectButtonControl(
					dwParentId,dwID,dwPosX,dwPosY,
					dwWidth, dwHeight, strTextureFocus,strTextureNoFocus, 
					dwTextOffsetX, dwTextOffsetY,
					strTextureBg, strLeft, strLeftFocus, strRight, strRightFocus);

		pControl->SetLabel(strFont,strLabel,dwTextColor);
		pControl->SetDisabledColor(dwDisabledColor);
		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		return pControl;
	}

	return NULL;
}
