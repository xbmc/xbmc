//  CAutorun		 -  Autorun for different Cd Media
//									like DVD Movies or XBOX Games
//
//	by Bobbin007 in 2003
//	
//	
//

#pragma once

class CAutorun
{
public:
	CAutorun();
	virtual			~CAutorun();
	void				HandleAutorun();
protected:
	void				ExecuteAutorun();
	void				RunXboxCd();
	void				RunCdda();
	void				RunISOMedia();
};