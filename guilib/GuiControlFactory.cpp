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

CGUIControlFactory::CGUIControlFactory(void)
{
}

CGUIControlFactory::~CGUIControlFactory(void)
{
}


CGUIControl* CGUIControlFactory::Create(DWORD dwParentId,const TiXmlNode* pControlNode)
{
  const TiXmlNode* pType=pControlNode->FirstChild("type");
  CStdString strType=pType->FirstChild()->Value();

  DWORD  dwPosX=0,dwPosY=0;
  DWORD  dwWidth=0, dwHeight=0;
  DWORD  dwID=0,left=0,right=0,up=0,down=0;
  TiXmlNode *pNode;
  dwID=atol(pControlNode->FirstChild("id" )->FirstChild()->Value());
  dwPosX=atol(pControlNode->FirstChild("posX" )->FirstChild()->Value());
  dwPosY=atol(pControlNode->FirstChild("posY" )->FirstChild()->Value());
  pNode=pControlNode->FirstChild("width" );
  if (pNode)
    dwWidth=atol(pNode->FirstChild()->Value());

  pNode=pControlNode->FirstChild("height" );
  if (pNode) dwHeight=atol(pNode->FirstChild()->Value());

  pNode=pControlNode->FirstChild("onup");
  if (pNode) up=atol(pNode->FirstChild()->Value());

  pNode=pControlNode->FirstChild("ondown");
  if (pNode) down=atol(pNode->FirstChild()->Value());

  pNode=pControlNode->FirstChild("onleft");
  if (pNode) left=atol(pNode->FirstChild()->Value());

  pNode=pControlNode->FirstChild("onright");
  if (pNode) right=atol(pNode->FirstChild()->Value());

  DWORD dwColorDiffuse=0xFFFFFFFF;
  pNode=pControlNode->FirstChild("colordiffuse" );
  if (pNode) sscanf(pNode->FirstChild()->Value(),"%x",&dwColorDiffuse);
  
  bool bVisible=true;
  pNode=pControlNode->FirstChild("visible" );
  if (pNode) 
  {
    CStdString strEnabled=pNode->FirstChild()->Value();
    strEnabled.ToLower();
    if (strEnabled=="no"||strEnabled=="disabled") bVisible=false;
  }
  

  if (strType=="label")
  {
      wstring strLabel;
      CStdString  strFont;
      D3DCOLOR dwTextColor;
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
      strLabel=g_localizeStrings.Get(dwLabelID);
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();

      DWORD dwAlign=XBFONT_LEFT;
      pNode=pControlNode->FirstChild("align");
      if (pNode)
      {
        CStdString strAlign=pControlNode->FirstChild("align")->FirstChild()->Value();      
        if (strAlign=="right") dwAlign=XBFONT_RIGHT;
      }

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
      CStdString  strFont;
      D3DCOLOR dwTextColor;
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();

      DWORD dwAlign=XBFONT_LEFT;
      pNode=pControlNode->FirstChild("align");
      if (pNode)
      {
        CStdString strAlign=pControlNode->FirstChild("align")->FirstChild()->Value();      
        if (strAlign=="right") dwAlign=XBFONT_RIGHT;
      }

      CGUIFadeLabelControl* pControl = new CGUIFadeLabelControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strFont,dwTextColor,dwAlign);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }

  if (strType=="button")
  {
      wstring strLabel;
      CStdString strFont,strTextureFocus,strTextureNoFocus;
      D3DCOLOR dwTextColor;
      D3DCOLOR dwDisabledColor;
      sscanf(pControlNode->FirstChild("disabledcolor" )->FirstChild()->Value(),"%x",&dwDisabledColor);
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      strTextureFocus=pControlNode->FirstChild("textureFocus")->FirstChild()->Value();
      strTextureNoFocus=pControlNode->FirstChild("textureNoFocus")->FirstChild()->Value();
      const char *szLabel=pControlNode->FirstChild("label")->FirstChild()->Value();
      if (szLabel[0]!='-')
      {
        DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
        strLabel=g_localizeStrings.Get(dwLabelID);
      }
      else strLabel=L"";

      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();

      CGUIButtonControl* pControl = new CGUIButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);

      pNode = pControlNode->FirstChild("hyperlink");
      if (pNode) 
      {
        int iHyperLink=atoi(pNode->FirstChild()->Value());
        pControl->SetHyperLink(iHyperLink);
      }
      pControl->SetVisible(bVisible);
      return pControl;
  }
  
  if (strType=="togglebutton")
  {
      wstring strLabel;
      CStdString strFont,strTextureFocus,strTextureNoFocus;
      CStdString strTextureAltFocus,strTextureAltNoFocus;
      D3DCOLOR dwTextColor;
      D3DCOLOR dwDisabledColor;
      sscanf(pControlNode->FirstChild("disabledcolor" )->FirstChild()->Value(),"%x",&dwDisabledColor);
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      strTextureFocus=pControlNode->FirstChild("textureFocus")->FirstChild()->Value();
      strTextureNoFocus=pControlNode->FirstChild("textureNoFocus")->FirstChild()->Value();
      strTextureAltFocus=pControlNode->FirstChild("AltTextureFocus")->FirstChild()->Value();
      strTextureAltNoFocus=pControlNode->FirstChild("AltTextureNoFocus")->FirstChild()->Value();
      const char *szLabel=pControlNode->FirstChild("label")->FirstChild()->Value();
      if (szLabel[0]!='-')
      {
        DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
        strLabel=g_localizeStrings.Get(dwLabelID);
      }
      else strLabel=L"";

      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();

      CGUIToggleButtonControl* pControl = new CGUIToggleButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus,strTextureAltFocus,strTextureAltNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);

      pNode = pControlNode->FirstChild("hyperlink");
      if (pNode) 
      {
        int iHyperLink=atoi(pNode->FirstChild()->Value());
        pControl->SetHyperLink(iHyperLink);
      }
      pControl->SetVisible(bVisible);
      return pControl;
  }

  if (strType=="buttonM")
  {
      wstring strLabel;
      CStdString strFont,strTextureFocus,strTextureNoFocus;
      D3DCOLOR dwTextColor;
      D3DCOLOR dwDisabledColor;
      DWORD dwItems;
      sscanf(pControlNode->FirstChild("bitmaps" )->FirstChild()->Value(),"%x",&dwItems);
      sscanf(pControlNode->FirstChild("disabledcolor" )->FirstChild()->Value(),"%x",&dwDisabledColor);
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      strTextureFocus=pControlNode->FirstChild("textureFocus")->FirstChild()->Value();
      strTextureNoFocus=pControlNode->FirstChild("textureNoFocus")->FirstChild()->Value();
      const char *szLabel=pControlNode->FirstChild("label")->FirstChild()->Value();
      if (szLabel[0]!='-')
      {
        DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
        strLabel=g_localizeStrings.Get(dwLabelID);
      }
      else strLabel=L"";

      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();

      CGUIMButtonControl* pControl = new CGUIMButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,dwItems,strTextureFocus,strTextureNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);

      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pNode = pControlNode->FirstChild("hyperlink");
      if (pNode) 
      {
        int iHyperLink=atoi(pNode->FirstChild()->Value());
        pControl->SetHyperLink(iHyperLink);
      }
      pControl->SetVisible(bVisible);
      return pControl;
  }
  
  if (strType=="checkmark")
  {
      wstring strLabel;
      CStdString strFont,strTextureCheckMark;
      D3DCOLOR dwTextColor;
      D3DCOLOR dwDisabledColor;
      DWORD dwCheckWidth, dwCheckHeight;
      sscanf(pControlNode->FirstChild("MarkWidth" )->FirstChild()->Value(),"%x",&dwCheckWidth);
      sscanf(pControlNode->FirstChild("MarkHeight" )->FirstChild()->Value(),"%x",&dwCheckHeight);
      sscanf(pControlNode->FirstChild("disabledcolor" )->FirstChild()->Value(),"%x",&dwDisabledColor);
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      strTextureCheckMark=pControlNode->FirstChild("textureCheckmark")->FirstChild()->Value();
      DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
      strLabel=g_localizeStrings.Get(dwLabelID);
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();
      DWORD dwAlign=XBFONT_LEFT;
      pNode=pControlNode->FirstChild("align");
      if (pNode)
      {
        CStdString strAlign=pControlNode->FirstChild("align")->FirstChild()->Value();      
        if (strAlign=="right") dwAlign=XBFONT_RIGHT;
      }

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
      wstring strLabel;
      CStdString strFont,strTextureFocus,strTextureNoFocus;
      CStdString strTextureRadioFocus,strTextureRadioNoFocus;
      D3DCOLOR dwTextColor;
      D3DCOLOR dwDisabledColor;
      sscanf(pControlNode->FirstChild("disabledcolor" )->FirstChild()->Value(),"%x",&dwDisabledColor);
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      strTextureFocus=pControlNode->FirstChild("textureFocus")->FirstChild()->Value();
      strTextureNoFocus=pControlNode->FirstChild("textureNoFocus")->FirstChild()->Value();
      strTextureRadioFocus=pControlNode->FirstChild("textureRadioFocus")->FirstChild()->Value();
      strTextureRadioNoFocus=pControlNode->FirstChild("textureRadioNoFocus")->FirstChild()->Value();
      DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
      strLabel=g_localizeStrings.Get(dwLabelID);
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();
      CGUIRadioButtonControl* pControl = new CGUIRadioButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus, strTextureRadioFocus,strTextureRadioNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);

      pNode = pControlNode->FirstChild("hyperlink");
      if (pNode) 
      {
        int iHyperLink=atoi(pNode->FirstChild()->Value());
        pControl->SetHyperLink(iHyperLink);
      }
      pControl->SetVisible(bVisible);
      return pControl;
  }
 

  if (strType=="spincontrol")
  {
      CStdString strFont,strUp,strDown;
      CStdString strUpFocus,strDownFocus;
      strUp=pControlNode->FirstChild("textureUp")->FirstChild()->Value();
      strDown=pControlNode->FirstChild("textureDown")->FirstChild()->Value();
      strUpFocus=pControlNode->FirstChild("textureUpFocus")->FirstChild()->Value();
      strDownFocus=pControlNode->FirstChild("textureDownFocus")->FirstChild()->Value();
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();
      D3DCOLOR dwTextColor;
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
  
      CStdString strSubType=pControlNode->FirstChild("subtype")->FirstChild()->Value();
      int iType;
      if ( strSubType=="int") iType=SPIN_CONTROL_TYPE_INT;
      else if ( strSubType=="float") iType=SPIN_CONTROL_TYPE_FLOAT;
      else iType=SPIN_CONTROL_TYPE_TEXT;

			DWORD dwAlign=XBFONT_RIGHT;
      pNode=pControlNode->FirstChild("align" );
      if (pNode)
			{
				CStdString strAlign=pNode->FirstChild()->Value();
				strAlign.ToLower();
				if (strAlign=="left") dwAlign=XBFONT_LEFT;
				if (strAlign=="center") dwAlign=XBFONT_CENTER_X;
			}

      CGUISpinControl* pControl = new CGUISpinControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strUp,strDown,strUpFocus,strDownFocus,strFont,dwTextColor,iType,dwAlign);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }
 
  if (strType=="image")
  {
      CStdString strTexture;
      strTexture=pControlNode->FirstChild("texture")->FirstChild()->Value();
  
      D3DCOLOR dwColorKey=0xffffffff;
      pNode=pControlNode->FirstChild("colorkey" );
      if (pNode)
        sscanf(pControlNode->FirstChild("colorkey" )->FirstChild()->Value(),"%x",&dwColorKey);

      CGUIImage* pControl = new CGUIImage(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTexture,dwColorKey);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }
 if (strType=="listcontrol")
 {
      CStdString strFont,strUp,strDown;
      CStdString strUpFocus,strDownFocus;
      strUp=pControlNode->FirstChild("textureUp")->FirstChild()->Value();
      strDown=pControlNode->FirstChild("textureDown")->FirstChild()->Value();
      strUpFocus=pControlNode->FirstChild("textureUpFocus")->FirstChild()->Value();
      strDownFocus=pControlNode->FirstChild("textureDownFocus")->FirstChild()->Value();
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();
      D3DCOLOR dwTextColor,dwSpinColor, dwSelectedColor;
      sscanf(pControlNode->FirstChild("spinColor" )->FirstChild()->Value(),"%x",&dwSpinColor);
      sscanf(pControlNode->FirstChild("selectedColor" )->FirstChild()->Value(),"%x",&dwSelectedColor);
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      

      DWORD dwSpinWidth=atol(pControlNode->FirstChild("spinWidth" )->FirstChild()->Value());
      DWORD dwSpinHeight=atol(pControlNode->FirstChild("spinHeight" )->FirstChild()->Value());
      DWORD dwSpinPosX=atol(pControlNode->FirstChild("spinPosX" )->FirstChild()->Value());
			DWORD dwSpinPosY=atol(pControlNode->FirstChild("spinPosY" )->FirstChild()->Value());

      CStdString strButton=pControlNode->FirstChild("textureNoFocus")->FirstChild()->Value();
      CStdString strButtonFocus=pControlNode->FirstChild("textureFocus")->FirstChild()->Value();

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
      pNode=pControlNode->FirstChild("suffix" );
      if (pNode)
      {
        CStdString strSuffix=pControlNode->FirstChild("suffix" )->FirstChild()->Value();
        pControl->SetScrollySuffix(strSuffix);
      }
			int iTextXOff=0;
			int iTextYOff=0;
			int iTextXOff2=0;
			int iTextYOff2=0;
			pNode=pControlNode->FirstChild("textXOff" );
			if (pNode)
				iTextXOff=atol(pNode->FirstChild()->Value());
			pNode=pControlNode->FirstChild("textYOff" );
			if (pNode)
				iTextYOff=atol(pNode->FirstChild()->Value());
			pNode=pControlNode->FirstChild("textXOff2" );
			if (pNode)
				iTextXOff2=atol(pNode->FirstChild()->Value());
			pNode=pControlNode->FirstChild("textYOff2" );
			if (pNode)
				iTextYOff2=atol(pNode->FirstChild()->Value());


			pControl->SetTextOffsets(iTextXOff,iTextYOff, iTextXOff2,iTextYOff2);

			DWORD dwitemWidth=16, dwitemHeight=16;
			pNode=pControlNode->FirstChild("itemWidth" );
			if (pNode) dwitemWidth=atol(pNode->FirstChild()->Value());

			pNode=pControlNode->FirstChild("itemHeight" );
			if (pNode) dwitemHeight=atol(pNode->FirstChild()->Value());
      
			int iSpace=2;
			int iTextureHeight=30;
			
			pNode=pControlNode->FirstChild("spaceBetweenItems" );
			if (pNode) iSpace=atol(pNode->FirstChild()->Value());

			
			pNode=pControlNode->FirstChild("textureHeight" );
			if (pNode) 
			{
				iTextureHeight=atol(pNode->FirstChild()->Value());
			}

      pControl->SetVisible(bVisible);
			pControl->SetImageDimensions(dwitemWidth, dwitemHeight);
			pControl->SetItemHeight(iTextureHeight);
			pControl->SetSpace(iSpace);

			DWORD dwTextColor2=dwTextColor;
			DWORD dwSelectedColor2=dwSelectedColor;
			pNode=pControlNode->FirstChild("selectedColor2" );
			if (pNode) sscanf(pNode->FirstChild()->Value(),"%x",&dwSelectedColor2);

			pNode=pControlNode->FirstChild("textcolor2" );
			if (pNode) sscanf(pNode->FirstChild()->Value(),"%x",&dwTextColor2);
			pControl->SetColors2(dwTextColor2, dwSelectedColor2);

			pNode=pControlNode->FirstChild("font2" );
			if (pNode)
				pControl->SetFont2( pNode->FirstChild()->Value() );
      return pControl;
  }

 if (strType=="textbox")
 {
      CStdString strFont,strUp,strDown;
      CStdString strUpFocus,strDownFocus;
      strUp=pControlNode->FirstChild("textureUp")->FirstChild()->Value();
      strDown=pControlNode->FirstChild("textureDown")->FirstChild()->Value();
      strUpFocus=pControlNode->FirstChild("textureUpFocus")->FirstChild()->Value();
      strDownFocus=pControlNode->FirstChild("textureDownFocus")->FirstChild()->Value();
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();
      D3DCOLOR dwTextColor,dwSpinColor;
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      sscanf(pControlNode->FirstChild("spinColor" )->FirstChild()->Value(),"%x",&dwSpinColor);
      
      DWORD dwSpinWidth=atol(pControlNode->FirstChild("spinWidth" )->FirstChild()->Value());
      DWORD dwSpinHeight=atol(pControlNode->FirstChild("spinHeight" )->FirstChild()->Value());
      DWORD dwSpinPosX=atol(pControlNode->FirstChild("spinPosX" )->FirstChild()->Value());
			DWORD dwSpinPosY=atol(pControlNode->FirstChild("spinPosY" )->FirstChild()->Value());

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
      CStdString strFont,strUp,strDown;
      CStdString strUpFocus,strDownFocus, strSuffix;



      strUp=pControlNode->FirstChild("textureUp")->FirstChild()->Value();
      strDown=pControlNode->FirstChild("textureDown")->FirstChild()->Value();
      strUpFocus=pControlNode->FirstChild("textureUpFocus")->FirstChild()->Value();
      strDownFocus=pControlNode->FirstChild("textureDownFocus")->FirstChild()->Value();
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();
      D3DCOLOR dwTextColor,dwSpinColor,dwSelectedColor;
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      sscanf(pControlNode->FirstChild("spinColor" )->FirstChild()->Value(),"%x",&dwSpinColor);
      sscanf(pControlNode->FirstChild("selectedColor" )->FirstChild()->Value(),"%x",&dwSelectedColor);
      
      DWORD dwitemWidth=64, dwitemHeight=64;
      sscanf(pControlNode->FirstChild("itemWidth" )->FirstChild()->Value(),"%i",&dwitemWidth);
      sscanf(pControlNode->FirstChild("itemHeight" )->FirstChild()->Value(),"%i",&dwitemHeight);
      
      CStdString strImage=pControlNode->FirstChild("imageFolder")->FirstChild()->Value();
      CStdString strImageFocus=pControlNode->FirstChild("imageFolderFocus")->FirstChild()->Value();

      DWORD dwSpinWidth=atol(pControlNode->FirstChild("spinWidth" )->FirstChild()->Value());
      DWORD dwSpinHeight=atol(pControlNode->FirstChild("spinHeight" )->FirstChild()->Value());
      DWORD dwSpinPosX=atol(pControlNode->FirstChild("spinPosX" )->FirstChild()->Value());
			DWORD dwSpinPosY=atol(pControlNode->FirstChild("spinPosY" )->FirstChild()->Value());
      

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
      pNode=pControlNode->FirstChild("suffix" );
      if (pNode)
      {
        strSuffix=pControlNode->FirstChild("suffix" )->FirstChild()->Value();
        pControl->SetScrollySuffix(strSuffix);
      }
      pControl->SetVisible(bVisible);
			int iTextureWidth=80;
			int iTextureHeight=80;
			pNode=pControlNode->FirstChild("textureWidth" );
			if (pNode) 
				iTextureWidth=atol(pNode->FirstChild()->Value() );
			pNode=pControlNode->FirstChild("textureHeight" );
			if (pNode) 
				iTextureHeight=atol(pNode->FirstChild()->Value() );
			pControl->SetTextureDimensions(iTextureWidth,iTextureHeight);
      return pControl;
  }
 
  return NULL;
}