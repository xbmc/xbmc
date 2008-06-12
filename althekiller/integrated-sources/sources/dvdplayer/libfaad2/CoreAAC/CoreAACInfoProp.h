#if !defined(AFX_COREAACINFOPROP_H__2722D950_7796_4061_B127_B78F1B28B908__INCLUDED_)
#define AFX_COREAACINFOPROP_H__2722D950_7796_4061_B127_B78F1B28B908__INCLUDED_

class CCoreAACInfoProp : public CBasePropertyPage
{
public:
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);	
	CCoreAACInfoProp(LPUNKNOWN pUnk, HRESULT *phr);	
	virtual ~CCoreAACInfoProp();
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnDisconnect();
	HRESULT OnActivate();	
	HRESULT OnDeactivate();
	BOOL OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HRESULT OnApplyChanges();
	void SetDirty();

private:
	void CCoreAACInfoProp::RefreshDisplay(HWND hwnd);

	ICoreAACDec* m_pICoreAACDec;
	bool     m_DownMatrix;
	BOOL     m_fWindowInActive;          // TRUE ==> dialog is being destroyed
};

#endif // !defined(AFX_COREAACINFOPROP_H__2722D950_7796_4061_B127_B78F1B28B908__INCLUDED_)
