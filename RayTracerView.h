
// RayTracerView.h : interface of the CRayTracerView class
//

#pragma once
#include "Camera.h" 
#include "xnacollision.h"
#include "vertex.h"
#include "PTProgress.h"

 
struct CUDAMesh;
class CRayTracerView : public CView, CPTCallback
{
protected: // create from serialization only
	CRayTracerView() noexcept;
	DECLARE_DYNCREATE(CRayTracerView)

// Attributes
public:
	CRayTracerDoc* GetDocument() const;

// Operations
public:
	void CalcRayCUDA(long nNumSamples, bool bUseTextures, long nDiv, bool bGlobalIllumination);
	void CalcRayCPU(long nNumSamples, bool bUseTextures, long nDiv, bool bGlobalIllumination);
	bool Init();

	void SetSunPos(float nAngle);
	void Render(float fps, float mspf);
	 
	void UpdateScene(float dt);
 
	bool m_bShowMesh = true;
	bool m_bWireframe = false;  
	
// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	void UpdateProgress(long nProgress, long nTotal);
protected:
	virtual void OnInitialUpdate(); // called first time after construct
	long m_nNumSamples = 4;
	void DrawRay();
	void DrawSun();
	void DrawLight(CLight* light);
	inline float deg2rad(const float& deg)
	{
		return deg * 3.14159267f / 180.0f;
	}

	void CopyMesh(CUDAMesh* pDst, CMesh* src);
	void FreeMesh(CUDAMesh* pDst);
// Implementation
public:
	virtual ~CRayTracerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void Init(IDWriteTextFormat* pTextFormat, ID2D1Factory* pD2DFactory, DXGI_SWAP_CHAIN_DESC sd, bool bEnable4xMsaa, UINT n4xMsaaQuality, ID3D11Device* pD3DDevice, ID3D11DeviceContext* pD3DImmediateContext);
	void OnDraw(CDC* /*pDC*/);
	void PositionCameras();
protected:
	CLight m_Sun;
	CMesh m_SunMesh;
	float m_fps = 0;
	float m_mspf = 0;
	long m_nSunIndexCount = 0;
	DirectionalLight mDirLights[3]; 
	ID3D11Buffer* mSunVB = 0;
	ID3D11Buffer* mSunIB = 0; 
	ID3D11Buffer* mRayVB = 0;

	 
	//debug ray
	bool m_bRayValid = false;
 
	DirectX::XMVECTOR m_rayStart;
	DirectX::XMVECTOR m_rayDir;

	/*void SetupBuffers(CString strPath);
	void LoadTextureFromFile(CString strPath);*/ 
	POINT mLastMousePos = { 0, 0 };
 	//DXGI
	IDXGISwapChain* m_pSwapChain = 0;
	DXGI_SWAP_CHAIN_DESC m_sd;
	//Direct3D
	ID3D11Device* m_pD3DDevice = 0;
	long m_nID = -1;
	bool m_bEnable4xMsaa = false;
	UINT m_4xMsaaQuality = 0;
	ID3D11Texture2D* m_pDepthStencilBuffer = 0;
	ID3D11DeviceContext* m_pD3DImmediateContext = 0;
	ID3D11RenderTargetView* m_pRenderTargetView = 0;
	ID3D11DepthStencilView* m_pDepthStencilView = 0;
	ID2D1SolidColorBrush* m_pSolidBrush = 0;
	ID2D1SolidColorBrush* m_pTextBrush = 0;
	//Direct2D
	ID2D1Factory* m_pD2DFactory = 0;
	ID2D1RenderTarget* m_pBackBufferRT = 0;
	IDWriteTextFormat* m_pTextFormat = 0;
	long m_nClientWidth = 0;
	long m_nClientHeight = 0;
	void ReCreateBuffers(int w, int h);
	void DiscardDeviceResources();
	void CreateDeviceResources();
	void InitEffects(ID3D11Device* pD3DDevice);
	float GetAspectRatio();
	Camera m_Camera;
	float m_nNearZ = 1.0f;
	float m_nFarZ = 6000.0f;
// Generated message map functions
protected: 
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	 
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};

#ifndef _DEBUG  // debug version in RayTracerView.cpp
inline CRayTracerDoc* CRayTracerView::GetDocument() const
   { return reinterpret_cast<CRayTracerDoc*>(m_pDocument); }
#endif

