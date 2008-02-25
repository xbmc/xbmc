
class CXBMC_PC
{
public:
  CXBMC_PC();
  ~CXBMC_PC();

  HRESULT Create( HINSTANCE hInstance );
  INT Run();
  HRESULT Render();
  HRESULT Cleanup();
  LRESULT MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
  BOOL ProcessMessage(MSG *msg);
  bool GetCursorPos(POINT &point);
  bool m_fullscreen;
  static INT_PTR CALLBACK ActivateWindowProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );

  HINSTANCE GetInstance() const { return m_hInstance; };
protected:
  void OnActivateWindow();
  void OnExecuteBuiltin();
  void OnResizeToPixel();
  void OnResizeToAspectRatio();
  void LoadSettings();
  void SaveSettings();
  bool m_mouseEnabled;
  bool m_active;
  bool m_focused;
  bool m_closing;
  bool m_inDialog;
  HWND m_hWnd;
  HINSTANCE m_hInstance;
  HACCEL m_hAccel;
  DWORD m_dwWindowStyle;
  DWORD m_dwCreationWidth;
  DWORD m_dwCreationHeight;
  CStdString m_strWindowTitle;
  RECT m_rcWindowBounds;
  RECT m_rcWindowClient;
};
