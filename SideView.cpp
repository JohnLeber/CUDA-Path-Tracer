// SideView.cpp : implementation file
//

#include "pch.h"
#include "RayTracer.h"
#include "SideView.h"
#include "RayTracerView.h"
#include "MainFrm.h" 
#define M_PI 3.1415926535897932384626433832795// *** Added for VS2012

//-----------------------------------------------------------------------//
// CSideView
//-----------------------------------------------------------------------//
IMPLEMENT_DYNCREATE(CSideView, CFormView)
//-----------------------------------------------------------------------//
CSideView::CSideView()
	: CFormView(IDD_SIDEVIEW)
{

}
//-----------------------------------------------------------------------//
CSideView::~CSideView()
{
}
//-----------------------------------------------------------------------//
void CSideView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}
//-----------------------------------------------------------------------//
BEGIN_MESSAGE_MAP(CSideView, CFormView)
	ON_BN_CLICKED(IDC_RENDER, &CSideView::OnBnClickedRender)
    ON_BN_CLICKED(IDC_CALC_RAY, &CSideView::OnBnClickedCalcRay) 
    ON_BN_CLICKED(IDC_TOGGLE_MESH, &CSideView::OnBnClickedToggleMesh)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER1, &CSideView::OnNMCustomdrawSlider1)
    ON_WM_HSCROLL()
    ON_BN_CLICKED(IDC_WIREFRAME, &CSideView::OnBnClickedWireframe)
    ON_BN_CLICKED(IDC_GI, &CSideView::OnBnClickedGi)
    ON_MESSAGE(WM_PROGRESS_UPDATE, OnProgressUpdate)
    ON_MESSAGE(WM_RENDER_START, OnRenderStart)
    ON_MESSAGE(WM_RENDER_END, OnRenderEnd)
END_MESSAGE_MAP()
//-----------------------------------------------------------------------//
// CSideView diagnostics
#ifdef _DEBUG
void CSideView::AssertValid() const
{
	CFormView::AssertValid();
}
//-----------------------------------------------------------------------//
#ifndef _WIN32_WCE
void CSideView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG 
//-----------------------------------------------------------------------//
LRESULT CSideView::OnRenderStart(WPARAM wp, LPARAM lp)
{
    GetDlgItem(IDC_CALC_RAY)->EnableWindow(FALSE);
    m_Progress.SetPos(0);
    DoPump();
    return 0;
}
//-----------------------------------------------------------------------//
LRESULT CSideView::OnRenderEnd(WPARAM wp, LPARAM lp)
{
    m_Progress.SetPos(0);
    GetDlgItem(IDC_CALC_RAY)->EnableWindow(TRUE);
    DoPump();
    return 0;
}
//-----------------------------------------------------------------------//
LRESULT CSideView::OnProgressUpdate(WPARAM wp, LPARAM lp)
{
    m_Progress.SetPos(wp * 100 / lp);
    DoPump();
    return 0;
}
//-----------------------------------------------------------------------//
void CSideView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
    gGlobalData->m_hSideWnd = m_hWnd;
    SetDlgItemInt(IDC_SAMPLES, 8);
    m_SunPos.SubclassDlgItem(IDC_SLIDER1, this);
    m_SunPos.SetRange(1, 2000);
    m_SunPos.SetPos(1000);
    m_Progress.SubclassDlgItem(IDC_PROGRESS1, this);
    m_Progress.SetRange(0, 100);
}
//-----------------------------------------------------------------------//
bool CSideView::DoPump()
{
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return false;
        }
        if (!AfxGetApp()->PreTranslateMessage(&msg))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
    return true;
}
 
//-----------------------------------------------------------------------//
void CSideView::OnBnClickedRender()
{  
}
//--------------------------------------------------------------------//
void CSideView::OnBnClickedCalcRay()
{
    CWaitCursor wait;
    long nNumSamples = GetDlgItemInt(IDC_SAMPLES);
    bool bUseTextures = IsDlgButtonChecked(IDC_USE_TEXTURES) == BST_CHECKED;
    if (IsDlgButtonChecked(IDC_CUDA) == BST_CHECKED) {
        ((CMainFrame*)AfxGetMainWnd())->GetRightPane()->CalcRayCUDA(nNumSamples);
    }
    else   {
        ((CMainFrame*)AfxGetMainWnd())->GetRightPane()->CalcRayCPU(nNumSamples, bUseTextures);
    }
} 
//--------------------------------------------------------------------//
void CSideView::OnBnClickedToggleMesh()
{
    ((CMainFrame*)AfxGetMainWnd())->GetRightPane()->m_bShowMesh = !((CMainFrame*)AfxGetMainWnd())->GetRightPane()->m_bShowMesh;
}
//--------------------------------------------------------------------//
void CSideView::OnNMCustomdrawSlider1(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
    *pResult = 0;
}
//--------------------------------------------------------------------//
void CSideView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    if ((CSliderCtrl*)pScrollBar == &m_SunPos)
    {
        float nPos = m_SunPos.GetPos() / 2000.0f; 
        float nAngle = M_PI * nPos; 
        ((CMainFrame*)AfxGetMainWnd())->GetRightPane()->SetSunPos(nAngle);
         
    }
    CFormView::OnHScroll(nSBCode, nPos, pScrollBar);
}
//--------------------------------------------------------------------//
void CSideView::OnBnClickedWireframe()
{
    ((CMainFrame*)AfxGetMainWnd())->GetRightPane()->m_bWireframe = !((CMainFrame*)AfxGetMainWnd())->GetRightPane()->m_bWireframe;
}
//--------------------------------------------------------------------//
void CSideView::OnBnClickedGi()
{
    ((CMainFrame*)AfxGetMainWnd())->GetRightPane()->m_bGlobalIllumination = !((CMainFrame*)AfxGetMainWnd())->GetRightPane()->m_bGlobalIllumination;
}
//--------------------------------------------------------------------//
