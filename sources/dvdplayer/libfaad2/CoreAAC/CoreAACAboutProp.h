#if !defined(AFX_COREAACABOUTPROP_H__D1E17C99_2135_46E3_A2D6_7A9845F1A296__INCLUDED_)
#define AFX_COREAACABOUTPROP_H__D1E17C99_2135_46E3_A2D6_7A9845F1A296__INCLUDED_

class CCoreAACAboutProp : public CBasePropertyPage
{
public:
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);	
	CCoreAACAboutProp(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CCoreAACAboutProp();
	HRESULT OnActivate();	
	BOOL OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


#endif // !defined(AFX_COREAACABOUTPROP_H__D1E17C99_2135_46E3_A2D6_7A9845F1A296__INCLUDED_)
