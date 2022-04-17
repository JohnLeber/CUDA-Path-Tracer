// ImageDlg.cpp : implementation file
//

#include "pch.h"
#include "RayTracer.h"
#include "ImageDlg.h"
#include "afxdialogex.h"
#include <OpenImageDenoise/oidn.hpp>


//-----------------------------------------------------------------------//
// CImageDlg dialog
//-----------------------------------------------------------------------//
IMPLEMENT_DYNAMIC(CImageDlg, CDialog)
//-----------------------------------------------------------------------//
CImageDlg::CImageDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_IMAGEDLG, pParent)
{

}
//-----------------------------------------------------------------------//
CImageDlg::~CImageDlg()
{
}
//-----------------------------------------------------------------------//
void CImageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}
//-----------------------------------------------------------------------//
BEGIN_MESSAGE_MAP(CImageDlg, CDialog)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BUTTON1, &CImageDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_DENOISE, &CImageDlg::OnBnClickedDenoise)
END_MESSAGE_MAP()
//-----------------------------------------------------------------------//
void CImageDlg::SetImage(CImage* pImage, long nNumSeconds)
{
	COleDateTimeSpan span;
	span.SetDateTimeSpan(0, 0, 0, nNumSeconds);
	 
	m_strDuration.Format(L"%d hours : %d minutes : %d seconds", span.GetHours(), span.GetMinutes(), span.GetSeconds());
 
	m_pImage = pImage;
}
//-----------------------------------------------------------------------//
void CImageDlg::DrawImage(CImage* pImage, UINT nCtrlID)
{
	if (!pImage) return;
	CDC* pDC = GetDC();
	if (pDC == 0)
	{
		ATLASSERT(0);
		return;
	}
	HGDIOBJ original = 0;
	CRect rtClient;
	GetDlgItem(nCtrlID)->GetWindowRect(&rtClient);
	ScreenToClient(&rtClient);
	long nImageWidth = pImage->GetWidth();
	long nImageHeight = pImage->GetHeight();
	float nAR = (float)nImageWidth / (float)nImageHeight;
	float nAR2 = (float)rtClient.Width() / (float)rtClient.Height();
	if (nAR < nAR2)
	{
		float nAR = (float)nImageWidth / (float)nImageHeight;
		float nScaledImageHeight = rtClient.Height();
		float nScaledImageWidth = nScaledImageHeight * nAR;

		original = pDC->SelectObject(GetStockObject(GRAY_BRUSH));
		long nW = (rtClient.Width() - nScaledImageWidth) / 2;
		long nH = rtClient.Height();
		CRect rt1(rtClient.left, rtClient.top, rtClient.left + nW, rtClient.bottom);
		pDC->Rectangle(rt1);
		CRect rt2(rtClient.left + nW + nScaledImageWidth, rtClient.top, rtClient.right, rtClient.bottom);
		pDC->Rectangle(rt2);
		pImage->Draw(pDC->m_hDC, rtClient.left + nW, rtClient.top, nScaledImageWidth, rtClient.Height(), 0, 0, nImageWidth, nImageHeight);
	}
	else
	{
		float nAR = (float)nImageWidth / (float)nImageHeight;
		float nScaledImageWidth = rtClient.Width();
		float nScaledImageHeight = nScaledImageWidth / nAR;
		original = pDC->SelectObject(GetStockObject(GRAY_BRUSH));
		long nH = (rtClient.Height() - nScaledImageHeight) / 2;
		long nW = rtClient.Width();
		CRect rt1(rtClient.left, rtClient.top, rtClient.right, rtClient.top + nH);
		pDC->Rectangle(rt1);
		CRect rt2(rtClient.left, rtClient.top + nH + nScaledImageHeight, rtClient.right, rtClient.bottom);
		pDC->Rectangle(rt2);
		pImage->Draw(pDC->m_hDC, rtClient.left, rtClient.top + nH, rtClient.Width(), nScaledImageHeight, 0, 0, nImageWidth, nImageHeight);
	}

	if (original != 0) pDC->SelectObject(original);
	ReleaseDC(pDC);
}
//-----------------------------------------------------------------------//
void CImageDlg::OnPaint()
{
	CPaintDC dc(this); 
	DrawImage(m_pImage, IDC_BORDER);
	SetDlgItemText(IDC_DURATION, m_strDuration);
}
//-----------------------------------------------------------------------//
void CImageDlg::OnBnClickedButton1()
{
	if (OpenClipboard())
	{
		EmptyClipboard();
		//create some data
		CBitmap* pTempBitmap = new CBitmap();
		if (pTempBitmap)
		{
			CClientDC cdc(this);
			CDC dc;
			dc.CreateCompatibleDC(&cdc);
			CImage* pImage = m_pImage;
			long nHeight = pImage->GetHeight();
			long nWidth = pImage->GetWidth();
			if (nWidth == 0 || nHeight == 0) return;

			BYTE* pBMP = new BYTE[nWidth * nHeight * 4];
			BYTE* pArray = (BYTE*)pImage->GetBits();
			int nPitch = pImage->GetPitch();
			int nBitCount = pImage->GetBPP() / 8;
			for (int h = 0; h < nHeight; h++) {
				for (int j = 0; j < nWidth; j++) {
					pBMP[h * nWidth * 4 + j * 4 + 0] = *(pArray + nPitch * h + j * nBitCount + 0);
					pBMP[h * nWidth * 4 + j * 4 + 1] = *(pArray + nPitch * h + j * nBitCount + 1);
					pBMP[h * nWidth * 4 + j * 4 + 2] = *(pArray + nPitch * h + j * nBitCount + 2);
					pBMP[h * nWidth * 4 + j * 4 + 3] = 255;
				}
			}
			CRect client(0, 0, nWidth, nHeight);
			pTempBitmap->CreateCompatibleBitmap(&cdc, nWidth, nHeight);
			dc.SelectObject(pTempBitmap);
			//call draw routine here that makes GDI calls
			pTempBitmap->SetBitmapBits(nWidth * nHeight * 4, pBMP);
			//put the data on the clipboard
			SetClipboardData(CF_BITMAP, pTempBitmap->m_hObject);
			CloseClipboard();
			//copy has been made on clipboard so we can delete
			delete pTempBitmap;
			delete[] pBMP;
		}
	}
}
//-----------------------------------------------------------------------//
void CImageDlg::OnBnClickedDenoise()
{
	long nWidth = m_pImage->GetWidth();
	long nHeight = m_pImage->GetHeight();
	float* pBMP = new float[nWidth * nHeight * 3];
	float* pOutput = new float[nWidth * nHeight * 3];
	BYTE* pArray = (BYTE*)m_pImage->GetBits();
	int nPitch = m_pImage->GetPitch();
	int nBitCount = m_pImage->GetBPP() / 8;
	for (int h = 0; h < nHeight; h++) {
		for (int j = 0; j < nWidth; j++) {
			pBMP[h * nWidth * 3 + j * 3 + 0] = (*(pArray + nPitch * h + j * nBitCount + 0)) / 255.0f;
			pBMP[h * nWidth * 3 + j * 3 + 1] = (*(pArray + nPitch * h + j * nBitCount + 1)) / 255.0f;
			pBMP[h * nWidth * 3 + j * 3 + 2] = (*(pArray + nPitch * h + j * nBitCount + 2)) / 255.0f;
		}
	}


	// Create an Intel Open Image Denoise device
	oidn::DeviceRef device = oidn::newDevice();
	device.commit();
	// Create a filter for denoising a beauty (color) image using optional auxiliary images too
	oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
	filter.setImage("color", pBMP, oidn::Format::Float3, nWidth, nHeight); // beauty
	//filter.setImage("albedo", albedoPtr, oidn::Format::Float3, nWidth, nHeight); // auxiliary
	//filter.setImage("normal", normalPtr, oidn::Format::Float3, nWidth, nHeight); // auxiliary
	filter.setImage("output", pOutput, oidn::Format::Float3, nWidth, nHeight); // denoised beauty
	filter.set("hdr", true); // beauty image is HDR
	filter.commit();
	// Filter the image
	filter.execute();
	// Check for errors
	const char* errorMessage;
	if (device.getError(errorMessage) != oidn::Error::None) {
		std::cout << "Error: " << errorMessage << std::endl;
	}
	 
	for (int h = 0; h < nHeight; h++) {
		for (int j = 0; j < nWidth; j++) {
			{
				*(pArray + nPitch * h + j * nBitCount + 0) = pOutput[h * nWidth * 3 + j * 3 + 0] * 255;
				*(pArray + nPitch * h + j * nBitCount + 1) = pOutput[h * nWidth * 3 + j * 3 + 1] * 255;
				*(pArray + nPitch * h + j * nBitCount + 2) = pOutput[h * nWidth * 3 + j * 3 + 2] * 255;
			}
		}
	}

	delete[] pBMP;
	delete[] pOutput;

	DrawImage(m_pImage, IDC_BORDER);
}
 