#pragma once


// CImageDlg dialog

class CImageDlg : public CDialog
{
	DECLARE_DYNAMIC(CImageDlg)
	void DrawImage(CImage* pImage, UINT nCtrlID);
	CImage* m_pImage = 0;
	CString m_strDuration;
public:
	CImageDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CImageDlg();
	void SetImage(CImage* pImage, long nNumSeconds);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IMAGEDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnBnClickedButton1();
};
