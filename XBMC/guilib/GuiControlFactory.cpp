#include "guicontrolfactory.h"
#include "localizestrings.h"
#include "guibuttoncontrol.h"
#include "guiRadiobuttoncontrol.h"
#include "guiSpinControl.h"
#include "guiListControl.h"
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

CGUIControl* CGUIControlFactory::Create(DWORD dwParentId,const TiXmlNode* pControlNode, CGUIControl* pReference)
{
	CStdString strType;
	GetString(pControlNode,"type",strType);

  DWORD  			dwPosX=0,dwPosY=0;
  DWORD  			dwWidth=0, dwHeight=0;
  DWORD  			dwID=0,left=0,right=0,up=0,down=0;
	DWORD	 			dwColorDiffuse=0xFFFFFFFF;
	bool   			bVisible=true;
	wstring			strLabel=L"";
	CStdString  strFont="";
	CStdString  strTmp;
	DWORD     	dwTextColor=0xFFFFFFFF;
	DWORD				dwAlign=XBFONT_LEFT;
	CStdString  strTextureFocus,strTextureNoFocus;
	CStdString	strTextureAltFocus,strTextureAltNoFocus;
	DWORD				dwDisabledColor;
	int					iHyperLink=-1;
	DWORD				dwItems;
	CStdString  strUp,strDown;
	CStdString  strUpFocus,strDownFocus;
	DWORD				dwSpinColor;
	DWORD				dwSpinWidth,dwSpinHeight,dwSpinPosX,dwSpinPosY;
	CStdString  strTextureCheckMark;
	DWORD				dwCheckWidth, dwCheckHeight;
	CStdString	strTextureRadioFocus,strTextureRadioNoFocus;
	CStdString	strSubType;
	int					iType=SPIN_CONTROL_TYPE_TEXT;
	bool				bReverse=false;
	CStdString	strTextureBg, strLeft,strRight,strMid;
	CStdString	strTexture;
	DWORD 			dwColorKey=0xffffffff;
	DWORD 			dwSelectedColor;
	CStdString 	strButton,strButtonFocus;
	CStdString 	strSuffix="";
	CStdString 	strFont2="";
	int 				iTextXOff=0;
	int 				iTextYOff=0;
	int 				iTextXOff2=0;
	int 				iTextYOff2=0;
	DWORD			 	dwitemWidth=16, dwitemHeight=16;
	DWORD			 	dwTextColor2=dwTextColor;
	DWORD			 	dwSelectedColor2;
	int        	iSpace=2;
	int				 	iTextureHeight=30;
	CStdString 	strImage,strImageFocus;
	int				 	iTextureWidth=80;
	
	// get defaults from reference control
	if (pReference)
	{
		bVisible			= pReference->IsVisible();
		dwColorDiffuse= pReference->GetColourDiffuse();
		dwPosX				= pReference->GetXPosition();
		dwPosY				= pReference->GetYPosition();
		dwWidth				= pReference->GetWidth();
		dwHeight			= pReference->GetHeight();
		if (strType=="label")
		{
			strFont			= ((CGUILabelControl*)pReference)->GetFontName();
			strLabel		= ((CGUILabelControl*)pReference)->GetLabel();
			dwTextColor = ((CGUILabelControl*)pReference)->GetTextColor();
			dwAlign			= ((CGUILabelControl*)pReference)->GetAlignment();
		}
		if (strType=="fadelabel")
		{
			strFont			= ((CGUIFadeLabelControl*)pReference)->GetFontName();
			dwTextColor = ((CGUIFadeLabelControl*)pReference)->GetTextColor();
			dwAlign			= ((CGUIFadeLabelControl*)pReference)->GetAlignment();
		}
		if (strType=="button")
		{
			strTextureFocus	 = ((CGUIButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus= ((CGUIButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont					 = ((CGUIButtonControl*)pReference)->GetFontName();
			strLabel				 = ((CGUIButtonControl*)pReference)->GetLabel();
			dwTextColor			 = ((CGUIButtonControl*)pReference)->GetTextColor();
			dwDisabledColor  = ((CGUIButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink			 = ((CGUIButtonControl*)pReference)->GetHyperLink();
		}
		if (strType=="togglebutton")
		{
			strTextureAltFocus  = ((CGUIToggleButtonControl*)pReference)->GetTexutureAltFocusName();
			strTextureAltNoFocus= ((CGUIToggleButtonControl*)pReference)->GetTexutureAltNoFocusName();
			strTextureFocus			= ((CGUIToggleButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus		= ((CGUIToggleButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont							= ((CGUIToggleButtonControl*)pReference)->GetFontName();
			strLabel						= ((CGUIToggleButtonControl*)pReference)->GetLabel();
			dwTextColor					= ((CGUIToggleButtonControl*)pReference)->GetTextColor();
			dwDisabledColor			= ((CGUIToggleButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink					= ((CGUIToggleButtonControl*)pReference)->GetHyperLink();
		}
		if (strType=="buttonM")
		{
			dwItems							= ((CGUIMButtonControl*)pReference)->GetItems();
			strTextureFocus			= ((CGUIMButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus		= ((CGUIMButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont							= ((CGUIMButtonControl*)pReference)->GetFontName();
			strLabel						= ((CGUIMButtonControl*)pReference)->GetLabel();
			dwTextColor					= ((CGUIMButtonControl*)pReference)->GetTextColor();
			dwDisabledColor			= ((CGUIMButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink					= ((CGUIMButtonControl*)pReference)->GetHyperLink();
		}
		if (strType=="checkmark")
		{
			strTextureCheckMark	= ((CGUICheckMarkControl*)pReference)->GetCheckMarkTextureName();
			dwCheckWidth				= ((CGUICheckMarkControl*)pReference)->GetCheckMarkWidth();
			dwCheckHeight				= ((CGUICheckMarkControl*)pReference)->GetCheckMarkHeight();
			dwAlign							= ((CGUICheckMarkControl*)pReference)->GetAlignment();
			strFont							= ((CGUICheckMarkControl*)pReference)->GetFontName();
			strLabel						= ((CGUICheckMarkControl*)pReference)->GetLabel();
			dwTextColor					= ((CGUICheckMarkControl*)pReference)->GetTextColor();
			dwDisabledColor			= ((CGUICheckMarkControl*)pReference)->GetDisabledColor();
		}
		if (strType=="radiobutton")
		{
			strTextureRadioFocus	= ((CGUIRadioButtonControl*)pReference)->GetTexutureRadioFocusName();;
			strTextureRadioNoFocus= ((CGUIRadioButtonControl*)pReference)->GetTexutureRadioNoFocusName();;
			strTextureFocus				= ((CGUIRadioButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus			= ((CGUIRadioButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont								= ((CGUIRadioButtonControl*)pReference)->GetFontName();
			strLabel							= ((CGUIRadioButtonControl*)pReference)->GetLabel();
			dwTextColor						= ((CGUIRadioButtonControl*)pReference)->GetTextColor();
			dwDisabledColor				= ((CGUIRadioButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink						= ((CGUIRadioButtonControl*)pReference)->GetHyperLink();
		}
		if (strType=="spincontrol")
		{
			strFont								= ((CGUISpinControl*)pReference)->GetFontName();
			dwTextColor						= ((CGUISpinControl*)pReference)->GetTextColor();
			dwAlign							  = ((CGUISpinControl*)pReference)->GetAlignment();
			strUp									= ((CGUISpinControl*)pReference)->GetTexutureUpName();
			strDown								= ((CGUISpinControl*)pReference)->GetTexutureDownName();
			strUpFocus						= ((CGUISpinControl*)pReference)->GetTexutureUpFocusName();
			strDownFocus					= ((CGUISpinControl*)pReference)->GetTexutureDownFocusName();
			iType									= ((CGUISpinControl*)pReference)->GetType();
			dwWidth								= ((CGUISpinControl*)pReference)->GetSpinWidth();
			dwHeight							= ((CGUISpinControl*)pReference)->GetSpinHeight();

		}
		if (strType=="progress")
		{
			strTextureBg	= ((CGUIProgressControl*)pReference)->GetBackGroundTextureName();
			strLeft				= ((CGUIProgressControl*)pReference)->GetBackGroundTextureName();
			strMid				= ((CGUIProgressControl*)pReference)->GetBackTextureRightName();
			strRight			= ((CGUIProgressControl*)pReference)->GetBackTextureMidName();
		}
		if (strType=="image")
		{
			strTexture		= ((CGUIImage *)pReference)->GetFileName();
			dwColorKey		= ((CGUIImage *)pReference)->GetColorKey();
		}
		if (strType=="listcontrol")
		{
			strFont					=	((CGUIListControl*)pReference)->GetFontName();
			dwSpinWidth			=	((CGUIListControl*)pReference)->GetSpinWidth();
			dwSpinHeight		=	((CGUIListControl*)pReference)->GetSpinHeight();
			strUp						=	((CGUIListControl*)pReference)->GetTexutureUpName();
			strDown					=	((CGUIListControl*)pReference)->GetTexutureDownName();
			strUpFocus			=	((CGUIListControl*)pReference)->GetTexutureUpFocusName();
			strDownFocus		=	((CGUIListControl*)pReference)->GetTexutureDownFocusName();
			dwSpinColor			=	((CGUIListControl*)pReference)->GetSpinTextColor();
			dwSpinPosX			=	((CGUIListControl*)pReference)->GetSpinX();
			dwSpinPosY			=	((CGUIListControl*)pReference)->GetSpinY();
			dwTextColor			=	((CGUIListControl*)pReference)->GetTextColor();
			dwSelectedColor = ((CGUIListControl*)pReference)->GetSelectedColor();
			strButton				=	((CGUIListControl*)pReference)->GetButtonNoFocusName();
			strButtonFocus	=	((CGUIListControl*)pReference)->GetButtonFocusName();
			strSuffix				=	((CGUIListControl*)pReference)->GetSuffix();
			iTextXOff				=	((CGUIListControl*)pReference)->GetTextOffsetX();
			iTextYOff				=	((CGUIListControl*)pReference)->GetTextOffsetY();
			iTextXOff2			=	((CGUIListControl*)pReference)->GetTextOffsetX2();
			iTextYOff2			=	((CGUIListControl*)pReference)->GetTextOffsetY2();
			dwitemWidth			=	((CGUIListControl*)pReference)->GetImageWidth();
			dwitemHeight		=	((CGUIListControl*)pReference)->GetImageHeight();
			iTextureHeight	=	((CGUIListControl*)pReference)->GetItemHeight();
			iSpace					=	((CGUIListControl*)pReference)->GetSpace();
			dwTextColor2		=	((CGUIListControl*)pReference)->GetTextColor2(); 
			dwSelectedColor2=	((CGUIListControl*)pReference)->GetSelectedColor2();
			strFont2				=	((CGUIListControl*)pReference)->GetFontName2();
		}
		if (strType=="textbox")
		{
				strFont				= ((CGUITextBox*)pReference)->GetFontName();
				dwTextColor		= ((CGUITextBox*)pReference)->GetTextColor();
				dwSpinWidth		= ((CGUITextBox*)pReference)->GetSpinWidth();
				dwSpinHeight	= ((CGUITextBox*)pReference)->GetSpinHeight();
				strUp					= ((CGUITextBox*)pReference)->GetTexutureUpName();
				strDown				= ((CGUITextBox*)pReference)->GetTexutureDownName();
				strUpFocus		= ((CGUITextBox*)pReference)->GetTexutureUpFocusName();
				strDownFocus	= ((CGUITextBox*)pReference)->GetTexutureDownFocusName();
				dwSpinColor		= ((CGUITextBox*)pReference)->GetSpinTextColor();
				dwSpinPosX		= ((CGUITextBox*)pReference)->GetSpinX();
				dwSpinPosY		= ((CGUITextBox*)pReference)->GetSpinY();
		}
		if (strType=="thumbnailpanel")
		{
			strFont					= ((CGUIThumbnailPanel*)pReference)->GetFontName();
			strImage				= ((CGUIThumbnailPanel*)pReference)->GetNoFocusName();
			strImageFocus		= ((CGUIThumbnailPanel*)pReference)->GetFocusName();
			dwitemWidth			= ((CGUIThumbnailPanel*)pReference)->GetItemWidth();
			dwitemHeight		= ((CGUIThumbnailPanel*)pReference)->GetItemHeight();
			dwSpinWidth			= ((CGUIThumbnailPanel*)pReference)->GetSpinWidth();
			dwSpinHeight		= ((CGUIThumbnailPanel*)pReference)->GetSpinHeight();
			strUp						= ((CGUIThumbnailPanel*)pReference)->GetTexutureUpName();
			strDown					= ((CGUIThumbnailPanel*)pReference)->GetTexutureDownName();
			strUpFocus			= ((CGUIThumbnailPanel*)pReference)->GetTexutureUpFocusName();
			strDownFocus		= ((CGUIThumbnailPanel*)pReference)->GetTexutureDownFocusName();
			dwSpinColor			= ((CGUIThumbnailPanel*)pReference)->GetSpinTextColor();
			dwSpinPosX			= ((CGUIThumbnailPanel*)pReference)->GetSpinX();
			dwSpinPosY			= ((CGUIThumbnailPanel*)pReference)->GetSpinY();
			dwTextColor			= ((CGUIThumbnailPanel*)pReference)->GetTextColor();
			dwSelectedColor = ((CGUIThumbnailPanel*)pReference)->GetSelectedColor();
			iTextureWidth		= ((CGUIThumbnailPanel*)pReference)->GetTextureWidth();
			iTextureHeight	= ((CGUIThumbnailPanel*)pReference)->GetTextureHeight();
		}
	}


  if (!GetDWORD(pControlNode, "id", dwID)) return NULL; // NO id????
  
	GetDWORD(pControlNode, "posX", dwPosX);
	GetDWORD(pControlNode, "posY", dwPosY);
	GetDWORD(pControlNode, "width", dwWidth);
	GetDWORD(pControlNode, "height", dwHeight);

	if (!GetDWORD(pControlNode, "onup"   , up  )) up    = dwID-1;
	if (!GetDWORD(pControlNode, "ondown" , down)) down  = dwID+1;
	if (!GetDWORD(pControlNode, "onleft" , left  )) left  = dwID;
	if (!GetDWORD(pControlNode, "onright", right)) right = dwID;

  GetHex(pControlNode, "colordiffuse", dwColorDiffuse);
 	GetBoolean(pControlNode,"visible",bVisible);
	GetString(pControlNode,"font", strFont);
	GetAlignment(pControlNode,"align", dwAlign);
	GetInt(pControlNode,"hyperlink",iHyperLink);
	GetHex(pControlNode,"disabledcolor",dwDisabledColor);
	GetString(pControlNode,"textureFocus",strTextureFocus);
	GetString(pControlNode,"textureNoFocus",strTextureNoFocus);
	GetString(pControlNode,"AltTextureFocus",strTextureAltFocus);
	GetString(pControlNode,"AltTextureNoFocus",strTextureAltNoFocus);
	GetDWORD(pControlNode,"bitmaps", dwItems);
	GetHex(pControlNode, "textcolor", dwTextColor);

	GetString(pControlNode,"textureUp",strUp);
	GetString(pControlNode,"textureDown",strDown);
	GetString(pControlNode,"textureUpFocus",strUpFocus);
	GetString(pControlNode,"textureDownFocus",strDownFocus);

	GetHex(pControlNode,"spinColor",dwSpinColor);
	GetDWORD(pControlNode,"spinWidth",dwSpinWidth);
	GetDWORD(pControlNode,"spinHeight",dwSpinHeight);
	GetDWORD(pControlNode,"spinPosX",dwSpinPosX);
	GetDWORD(pControlNode,"spinPosY",dwSpinPosY);      

	GetDWORD(pControlNode,"MarkWidth",dwCheckWidth);
	GetDWORD(pControlNode,"MarkHeight",dwCheckHeight);
	GetString(pControlNode,"textureCheckmark",strTextureCheckMark);

	GetString(pControlNode,"textureRadioFocus",strTextureRadioFocus);
	GetString(pControlNode,"textureRadioNoFocus",strTextureRadioNoFocus);

	if (GetString(pControlNode,"subtype",strSubType))
	{
		strSubType.ToLower();
		if ( strSubType=="int") iType=SPIN_CONTROL_TYPE_INT;
		else if ( strSubType=="float") iType=SPIN_CONTROL_TYPE_FLOAT;
		else iType=SPIN_CONTROL_TYPE_TEXT;
	}
	GetBoolean(pControlNode,"reverse",bReverse);

	GetString(pControlNode,"texturebg",strTextureBg);
	GetString(pControlNode,"lefttexture",strLeft);
	GetString(pControlNode,"midtexture",strMid);
	GetString(pControlNode,"righttexture",strRight);
	GetString(pControlNode,"texture",strTexture);
	GetHex(pControlNode,"colorkey",dwColorKey);

	GetHex  (pControlNode,"selectedColor",dwSelectedColor);
	dwSelectedColor2=dwSelectedColor;

	GetString(pControlNode,"textureNoFocus",strButton);
	GetString(pControlNode,"textureFocus",strButtonFocus);
	GetString(pControlNode,"suffix",strSuffix);
	GetInt(pControlNode,"textXOff",iTextXOff);
	GetInt(pControlNode,"textYOff",iTextYOff);
	GetInt(pControlNode,"textXOff2",iTextXOff2);
	GetInt(pControlNode,"textYOff2",iTextYOff2);

	GetDWORD(pControlNode,"itemWidth",dwitemWidth);
	GetDWORD(pControlNode,"itemHeight",dwitemHeight);
	GetInt(pControlNode,"spaceBetweenItems",iSpace);

	GetHex(pControlNode,"selectedColor2",dwSelectedColor2);
	GetHex(pControlNode,"textcolor2",dwTextColor2);
	GetString(pControlNode,"font2",strFont2);
	GetHex(pControlNode,"selectedColor",dwSelectedColor);

	GetString(pControlNode,"imageFolder",strImage);
	GetString(pControlNode,"imageFolderFocus",strImageFocus);
	GetInt(pControlNode,"textureWidth",iTextureWidth);
	GetInt(pControlNode,"textureHeight",iTextureHeight);


	if ( GetString(pControlNode, "label", strTmp))
	{
		if (strTmp.size() > 0)
		{
			if (strTmp[0] != '-') 
			{
				DWORD dwLabelID=atol(strTmp);
				strLabel=g_localizeStrings.Get(dwLabelID);
			}
		}
	}

  if (strType=="label")
  {
      CGUILabelControl* pControl = new CGUILabelControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strFont,strLabel,dwTextColor,dwAlign);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }

  if (strType=="videowindow")
  {
    CGUIVideoControl* pControl = new CGUIVideoControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight);
    return pControl;
  }

	if (strType=="fadelabel")
  {
      CGUIFadeLabelControl* pControl = new CGUIFadeLabelControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strFont,dwTextColor,dwAlign);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }

  if (strType=="button")
	{
      CGUIButtonControl* pControl = new CGUIButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
			pControl->SetHyperLink(iHyperLink);
      pControl->SetVisible(bVisible);
      return pControl;
  }
  
  if (strType=="togglebutton")
  {
      CGUIToggleButtonControl* pControl = new CGUIToggleButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus,strTextureAltFocus,strTextureAltNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
			pControl->SetHyperLink(iHyperLink);
      pControl->SetVisible(bVisible);
      return pControl;
  }

  if (strType=="buttonM")
  {
      CGUIMButtonControl* pControl = new CGUIMButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,dwItems,strTextureFocus,strTextureNoFocus);
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
			CGUICheckMarkControl* pControl = new CGUICheckMarkControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureCheckMark,dwCheckWidth,dwCheckHeight,dwAlign);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }


  if (strType=="radiobutton")
  {
      CGUIRadioButtonControl* pControl = new CGUIRadioButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus, strTextureRadioFocus,strTextureRadioNoFocus);
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

      CGUISpinControl* pControl = new CGUISpinControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strUp,strDown,strUpFocus,strDownFocus,strFont,dwTextColor,iType,dwAlign);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
			pControl->SetReverse(bReverse);
      return pControl;
  }

  if (strType=="progress")
  {
      CGUIProgressControl* pControl= new CGUIProgressControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureBg,strLeft,strMid,strRight);
      pControl->SetVisible(bVisible);
      return pControl;
  } 

  if (strType=="image")
  {

			CGUIImage* pControl = new CGUIImage(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTexture,dwColorKey);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }
 if (strType=="listcontrol")
 {

      CGUIListControl* pControl = new CGUIListControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
                                                      strFont,
                                                      dwSpinWidth,dwSpinHeight,
                                                      strUp,strDown,
                                                      strUpFocus,strDownFocus,
                                                      dwSpinColor,dwSpinPosX,dwSpinPosY,
                                                      strFont,dwTextColor,dwSelectedColor,
                                                      strButton,strButtonFocus);
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

 if (strType=="textbox")
 {
      
      CGUITextBox* pControl = new CGUITextBox(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
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
      CGUIThumbnailPanel* pControl = new CGUIThumbnailPanel(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
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
      return pControl;
  }
 
  return NULL;
}
