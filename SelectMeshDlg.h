#pragma once


// CSelectMeshDlg dialog

class CSelectMeshDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectMeshDlg)

public:
	CSelectMeshDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSelectMeshDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SELECTMESH };
#endif
	MeshType m_enMeshType = MeshType::MeshTypeSponza;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
