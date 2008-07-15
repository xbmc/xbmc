#ifndef APE_APEINFODIALOG_H
#define APE_APEINFODIALOG_H

BOOL CALLBACK FileInfoDialogProcedureA(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	
class CAPEInfoDialog
{
public:

	CAPEInfoDialog();
	~CAPEInfoDialog();

	int ShowAPEInfoDialog(const str_utf16 * pFilename, HINSTANCE hInstance, const str_utf16 * lpszTemplateName, HWND hWndParent);

private:

	static LRESULT CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	int FillGenreComboBox(HWND hDlg, int nComboBoxID, char * pSelectedGenre);
	IAPEDecompress * m_pAPEDecompress;
};

#endif // #ifndef APE_APEINFODIALOG_H