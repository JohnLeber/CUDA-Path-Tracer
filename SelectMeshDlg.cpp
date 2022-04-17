// SelectMeshDlg.cpp : implementation file
//

#include "pch.h"
#include "RayTracer.h"
#include "SelectMeshDlg.h"
#include "afxdialogex.h"


// CSelectMeshDlg dialog

IMPLEMENT_DYNAMIC(CSelectMeshDlg, CDialog)

CSelectMeshDlg::CSelectMeshDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_SELECTMESH, pParent)
{

}
//--------------------------------------------------------------//
CSelectMeshDlg::~CSelectMeshDlg()
{
}
//--------------------------------------------------------------//
void CSelectMeshDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}
//--------------------------------------------------------------//
BEGIN_MESSAGE_MAP(CSelectMeshDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CSelectMeshDlg::OnBnClickedOk)
END_MESSAGE_MAP()
//--------------------------------------------------------------//
// CSelectMeshDlg message handlers
BOOL CSelectMeshDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO1);

	long nItem = pCombo->AddString(L"Sponza");
	pCombo->SetItemData(nItem, 1);
	  
	/*nItem = pCombo->AddString(L"Cornel Box");
	pCombo->SetItemData(nItem, 0); */

	nItem = pCombo->AddString(L"Crytek Sponza");
	pCombo->SetItemData(nItem, 2);
	pCombo->SetCurSel(nItem);
	return TRUE; 
}
//--------------------------------------------------------------//
void CSelectMeshDlg::OnBnClickedOk()
{
	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO1);
	
	if (pCombo->GetItemData(pCombo->GetCurSel()) == 0) {
		m_enMeshType = MeshType::MeshTypeCornelBox;
	}
	else if (pCombo->GetItemData(pCombo->GetCurSel()) == 1) {
		m_enMeshType = MeshType::MeshTypeSponza;
	}
	else if (pCombo->GetItemData(pCombo->GetCurSel()) == 2) {
		m_enMeshType = MeshType::MeshTypeCrytekSponza;
	}

	CDialog::OnOK();
}
//--------------------------------------------------------------//
