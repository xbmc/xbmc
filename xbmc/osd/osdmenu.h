#pragma once
#include "OSDSubMenu.h"

namespace OSD
{
	class COSDMenu
	{
	public:
		COSDMenu();
		COSDMenu(int iXpos, int iYpos);
		COSDMenu(const COSDMenu& menu);
		const COSDMenu& operator = (const COSDMenu& menu);

		virtual ~COSDMenu(void);
		COSDMenu* Clone();

    void AddSubMenu(const COSDSubMenu& submenu);
		void Draw();
		bool OnAction(IExecutor& executor,const CAction& action);

		int  GetX() const;
		void SetX(int X);
		
		int  GetY() const;
		void SetY(int Y) ;

    int  GetSelectedMenu() const;
    void Clear();
    void SetValue(int iMessage, int iValue);
    void SetLabel(int iMessage, const CStdString& strLabel);
	private:
		typedef	vector<COSDSubMenu*>::iterator ivecSubMenus;
		typedef	vector<COSDSubMenu*>::const_iterator icvecSubMenus;						
		vector<COSDSubMenu*> m_vecSubMenus;

		int m_iCurrentSubMenu;
		int m_iXPos;
		int m_iYPos;
	};
};