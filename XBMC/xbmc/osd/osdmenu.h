#pragma once
#include "key.h"
#include "OSDSubMenu.h"
#include <vector>
using namespace std;
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
		bool OnAction(const CAction& action);

		int  GetX() const;
		void SetX(int X);
		
		int  GetY() const;
		void SetY(int Y) ;

    int  GetSelectedMenu() const;
	private:
    void Clear();
		typedef	vector<COSDSubMenu*>::iterator ivecSubMenus;
		typedef	vector<COSDSubMenu*>::const_iterator icvecSubMenus;						
		vector<COSDSubMenu*> m_vecSubMenus;

		int m_iCurrentSubMenu;
		int m_iXPos;
		int m_iYPos;
	};
};