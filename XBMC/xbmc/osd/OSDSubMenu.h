#pragma once
#include "IOSDOption.h"
#include <vector>
using namespace std;
namespace OSD
{
	class COSDSubMenu
	{
	public:
		COSDSubMenu();
		COSDSubMenu(int iHeading,int iXpos, int iYpos);
		COSDSubMenu(const COSDSubMenu& submenu);
		const COSDSubMenu& operator = (const COSDSubMenu& submenu);
	
		virtual ~COSDSubMenu(void);
		COSDSubMenu* Clone() const;

		void Draw();
    void AddOption(const IOSDOption* option);
		bool OnAction(IExecutor& executor,const CAction& action);

		int  GetX() const;
		void SetX(int X);
		
		int  GetY() const;
		void SetY(int X) ;
    void SetValue(int iMessage, int iValue);
	private:
    void Clear();
		typedef	vector<IOSDOption*>::iterator ivecOptions;	
		typedef	vector<IOSDOption*>::const_iterator icvecOptions;						
		vector<IOSDOption*> m_vecOptions;

		int m_iCurrentOption;
		int m_iXPos;
		int m_iYPos;
    int m_iHeading;
    bool m_bOptionSelected;
	};
};