
// RayTracerView.cpp : implementation of the CRayTracerView class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "RayTracer.h"
#endif

#include "Vertex.h"
#include "Effects.h" 
#include "RayTracerDoc.h"
#include "RayTracerView.h" 
//#include "WICTextureLoader11.h"
#include "RenderStates.h" 
 
#include "GeometryGenerator.h"

#include "vec.h"

#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "PathTracer.h"
#include "ImageDlg.h"
#include "CPUPathTracer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define M_PI 3.1415926535897932384626433832795// *** Added for VS2012
 

//-----------------------------------------------------------------------//
// CRayTracerView

IMPLEMENT_DYNCREATE(CRayTracerView, CView)

BEGIN_MESSAGE_MAP(CRayTracerView, CView)
	//ON_WM_STYLECHANGED()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()
//-----------------------------------------------------------------------//
// CRayTracerView construction/destruction
CRayTracerView::CRayTracerView() noexcept
{
	// TODO: add construction code here

}
//-----------------------------------------------------------------------//
CRayTracerView::~CRayTracerView()
{
	DiscardDeviceResources();
	 
	if (gGlobalData)
	{
		for (auto& i : gGlobalData->m_vMeshes)
		{
			i.Clear();
		}
		for (auto& i : gGlobalData->m_vMaterials)
		{
			if (i.mDiffuseMapSRV)
			{
				i.mDiffuseMapSRV->Release();
				i.mDiffuseMapSRV = 0;
			}
			if (i.m_pTexData)
			{
				delete[] i.m_pTexData;
				i.m_pTexData = 0;
			}
		}
		delete gGlobalData;
		gGlobalData = 0;
	}
	if (mRayVB)
	{
		mRayVB->Release();
	}
	if (mSunVB)
	{
		mSunVB->Release();
	}
	if (mSunIB)
	{
		mSunIB->Release();
	}
	if (m_pSwapChain)
	{
		m_pSwapChain->Release();
		m_pSwapChain = 0;
	}
	if (m_pRenderTargetView)
	{
		m_pRenderTargetView->Release();
		m_pRenderTargetView = 0;
	}
	if (m_pDepthStencilBuffer)
	{
		m_pDepthStencilBuffer->Release();
		m_pDepthStencilBuffer = 0;
	}
	if (m_pDepthStencilView)
	{
		m_pDepthStencilView->Release();
		m_pDepthStencilView = 0;
	}
	if (m_pBackBufferRT)
	{
		m_pBackBufferRT->Release();
		m_pBackBufferRT = 0;
	}
	Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}
//-----------------------------------------------------------------------//
BOOL CRayTracerView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}
//-----------------------------------------------------------------------//
void CRayTracerView::OnInitialUpdate()
{
	mDirLights[0].Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Diffuse = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Specular = DirectX::XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
	mDirLights[0].Direction = DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	mDirLights[1].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[1].Diffuse = DirectX::XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	mDirLights[1].Specular = DirectX::XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	mDirLights[1].Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	mDirLights[2].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[2].Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Direction = DirectX::XMFLOAT3(0.0f, -0.707f, -0.707f);

	CView::OnInitialUpdate();
}
//-----------------------------------------------------------------------//
// CRayTracerView diagnostics

#ifdef _DEBUG
void CRayTracerView::AssertValid() const
{
	CView::AssertValid();
}
//-----------------------------------------------------------------------//
void CRayTracerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
//-----------------------------------------------------------------------//
CRayTracerDoc* CRayTracerView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRayTracerDoc)));
	return (CRayTracerDoc*)m_pDocument;
}
#endif //_DEBUG 
//-----------------------------------------------------------------------//
// CDX360View message handlers
BOOL CRayTracerView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return FALSE;
}
//-----------------------------------------------------------------------//
void CRayTracerView::OnDraw(CDC* /*pDC*/)
{
	//CRayTracerDoc* pDoc = GetDocument();
	//ASSERT_VALID(pDoc);
	//if (!pDoc)
	//	return;

	//// TODO: add draw code for native data here
}
//-----------------------------------------------------------------------//
void CRayTracerView::Init(IDWriteTextFormat* pTextFormat, ID2D1Factory* pD2DFactory, DXGI_SWAP_CHAIN_DESC sd, bool bEnable4xMsaa, UINT n4xMsaaQuality, ID3D11Device* pD3DDevice, ID3D11DeviceContext* pD3DImmediateContext)
{
	m_pTextFormat = pTextFormat;
	m_pD2DFactory = pD2DFactory;
	m_pD3DImmediateContext = pD3DImmediateContext;
	m_pD3DDevice = pD3DDevice;
	m_sd = sd;
	m_bEnable4xMsaa = bEnable4xMsaa;
	m_4xMsaaQuality = n4xMsaaQuality;
	InitEffects(pD3DDevice);
	CRect R;
	GetClientRect(&R);
	ReCreateBuffers(R.Width(), R.Height());

	m_Camera.Pitch(0);
	m_Camera.SetLens(0.25f * MathHelper::Pi, GetAspectRatio(), m_nNearZ, m_nFarZ);
	m_Camera.SetPosition(-1280, 138, -50);
	m_Camera.RotateY(3.1416 / 2);

	/*float nPos = m_SunPos.GetPos() / 2000.0f;
	float nDist = 1;
	float nAngle = M_PI * nPos;
	DirectX::XMFLOAT3 sunpos(nDist * cos(nAngle), nDist * sin(nAngle), 0);*/

	SetSunPos(M_PI / 2);//noon 
}
//-----------------------------------------------------------------------//
float CRayTracerView::GetAspectRatio()
{
	return static_cast<float>(m_nClientWidth) / m_nClientHeight;
}
//-----------------------------------------------------------------------//
void CRayTracerView::InitEffects(ID3D11Device* pD3DDevice)
{
	Effects::InitAll(pD3DDevice);
	InputLayouts::InitAll(pD3DDevice);
	RenderStates::InitAll(pD3DDevice);
}
//-----------------------------------------------------------------------//
void CRayTracerView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if (m_pD3DDevice && cx > 0 && cy > 0)
	{
		ReCreateBuffers(cx, cy);
		Render(m_fps, m_mspf);
	}
}
//-----------------------------------------------------------------------//
void CRayTracerView::ReCreateBuffers(int w, int h)
{
	if (m_pSwapChain)
	{
		m_pSwapChain->Release();
		m_pSwapChain = 0;
	}
	if (m_pRenderTargetView)
	{
		m_pRenderTargetView->Release();
		m_pRenderTargetView = 0;
	}
	if (m_pDepthStencilBuffer)
	{
		m_pDepthStencilBuffer->Release();
		m_pDepthStencilBuffer = 0;
	}
	if (m_pDepthStencilView)
	{
		m_pDepthStencilView->Release();
		m_pDepthStencilView = 0;
	}
	if (m_pBackBufferRT)
	{
		m_pBackBufferRT->Release();
		m_pBackBufferRT = 0;
	}
	IDXGIDevice* pDXGIDevice = 0;
	IDXGIAdapter* pDXGIAdapter = 0;
	IDXGIFactory* pDXGIFactory = 0;

	HRESULT hr = m_pD3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);
	if (FAILED(hr))
	{
		ATLASSERT(0);
	}
	hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pDXGIAdapter);
	if (FAILED(hr))
	{
		ATLASSERT(0);
	}
	hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pDXGIFactory);
	if (FAILED(hr))
	{
		ATLASSERT(0);
	}

	m_sd.OutputWindow = GetSafeHwnd();
	m_sd.BufferDesc.Width = w;
	m_sd.BufferDesc.Height = h;
	hr = pDXGIFactory->CreateSwapChain(m_pD3DDevice, &m_sd, &m_pSwapChain);
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	m_nClientWidth = w;
	m_nClientHeight = h;
	depthStencilDesc.Width = w;
	depthStencilDesc.Height = h;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if (m_bEnable4xMsaa)
	{
		depthStencilDesc.SampleDesc.Count = 4;
		depthStencilDesc.SampleDesc.Quality = m_4xMsaaQuality - 1;
	}
	// No MSAA
	else
	{
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
	}

	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	hr = m_pD3DDevice->CreateTexture2D(&depthStencilDesc, 0, &m_pDepthStencilBuffer);

	hr = m_pSwapChain->ResizeBuffers(1, w, h, m_sd.BufferDesc.Format, 0);
	ID3D11Texture2D* pBackBuffer = 0;
	hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	hr = m_pD3DDevice->CreateRenderTargetView(pBackBuffer, 0, &m_pRenderTargetView);
	if (pBackBuffer)
	{
		pBackBuffer->Release();
		pBackBuffer = 0;
	}
	hr = m_pD3DDevice->CreateDepthStencilView(m_pDepthStencilBuffer, 0, &m_pDepthStencilView);
	long nStop = 0;


	// Create the DXGI Surface Render Target.
	FLOAT dpiX;
	FLOAT dpiY;
	//m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);
	dpiX = (FLOAT)GetDpiForWindow(::GetDesktopWindow());
	dpiY = dpiX;

	D2D1_RENDER_TARGET_PROPERTIES props =
		D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
			dpiX,
			dpiY
		);

	// Create a Direct2D render target which can draw into the surface in the swap chain
	IDXGISurface* pD2DBackBuffer = 0;
	hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pD2DBackBuffer));
	hr = m_pD2DFactory->CreateDxgiSurfaceRenderTarget(
		pD2DBackBuffer,
		&props,
		&m_pBackBufferRT
	);
	if (pD2DBackBuffer)
	{
		pD2DBackBuffer->Release();
	}
	if (pDXGIDevice)
	{
		pDXGIDevice->Release();
	}
	if (pDXGIAdapter)
	{
		pDXGIAdapter->Release();
	}
	if (pDXGIFactory)
	{
		pDXGIFactory->Release();
	}

	DiscardDeviceResources();
	CreateDeviceResources();

	m_Camera.SetLens(0.25f * MathHelper::Pi, GetAspectRatio(), m_nNearZ, m_nFarZ);
}
//-----------------------------------------------------------------------//
void CRayTracerView::CreateDeviceResources()
{
	if (!m_pBackBufferRT) return;

	D2D1_COLOR_F clr;
	clr.a = 1.0f;
	clr.r = 255.0f / 255.0f;
	clr.g = 0.0f / 255.0f;
	clr.b = 0.0f / 255.0f;

	HRESULT hr = m_pBackBufferRT->CreateSolidColorBrush(clr, &m_pSolidBrush);

	clr.r = 0.0f / 255.0f;
	clr.g = 0.0f / 255.0f;
	clr.b = 0.0f / 255.0f;

	hr = m_pBackBufferRT->CreateSolidColorBrush(clr, &m_pTextBrush);
}
//-----------------------------------------------------------------------//
void CRayTracerView::DiscardDeviceResources()
{
	if (m_pTextBrush)
	{
		m_pTextBrush->Release();
		m_pTextBrush = 0;
	}
	if (m_pSolidBrush)
	{
		m_pSolidBrush->Release();
		m_pSolidBrush = 0;
	}
}
//-----------------------------------------------------------------------//
void CRayTracerView::UpdateScene(float dt)
{
	float nScale = 10;
	POINT s;
	GetCursorPos(&s);
	ScreenToClient(&s);
	CRect rt;
	GetClientRect(&rt);
	if (rt.PtInRect(s))
	{
		if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		{
			nScale = 1200;
		}
		if (GetAsyncKeyState('W') & 0x8000)
			m_Camera.Walk(1.0f * dt * nScale);

		if (GetAsyncKeyState('S') & 0x8000)
			m_Camera.Walk(-1.0f * dt * nScale);

		if (GetAsyncKeyState('A') & 0x8000)
			m_Camera.Strafe(-1.0f * dt * nScale);

		if (GetAsyncKeyState('D') & 0x8000)
			m_Camera.Strafe(1.0f * dt * nScale);

	}
}
//-----------------------------------------------------------------------//
void CRayTracerView::Render(float fps, float mspf)
{
	if (Effects::BasicFX == 0) return;

	if (abs(fps) < 0.00001) fps = m_fps;
	if (abs(mspf) < 0.00001) mspf = m_mspf;

	m_fps = fps;
	m_mspf = mspf;

	if (m_pDepthStencilView == 0) return;
	if (m_pRenderTargetView == 0) return;

	DirectX::XMVECTORF32 color = Colors::LightSteelBlue;


	m_pD3DImmediateContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);
	D3D11_VIEWPORT Viewport;
	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;
	Viewport.Width = (FLOAT)m_nClientWidth;
	Viewport.Height = (FLOAT)m_nClientHeight;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;
	m_pD3DImmediateContext->RSSetViewports(1, &Viewport);

	//render background with DirectX
	float nDefault = 0;
	m_pD3DImmediateContext->ClearRenderTargetView(m_pRenderTargetView, reinterpret_cast<const float*>(&color));
	m_pD3DImmediateContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);



	m_pD3DImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_Camera.UpdateViewMatrix();

	DirectX::XMMATRIX view = m_Camera.View();
	DirectX::XMMATRIX proj = m_Camera.Proj();
	DirectX::XMMATRIX viewProj = m_Camera.ViewProj();
	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();

	DirectX::XMFLOAT4X4 TexTransform;
	XMStoreFloat4x4(&TexTransform, I);


	Effects::BasicFX->SetDirLights(mDirLights);
	Effects::BasicFX->SetEyePosW(m_Camera.GetPosition());
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX worldInvTranspose = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX worldViewProj = DirectX::XMMatrixIdentity();


	if (m_bShowMesh)
	{
		for (auto& m : gGlobalData->m_vMeshes)
		{
			if (m.bLight) continue;

			ID3DX11EffectTechnique* activeSliderTech = Effects::BasicFX->SliderTech;
			m_pD3DImmediateContext->IASetInputLayout(InputLayouts::Basic32);
			D3DX11_TECHNIQUE_DESC techDesc;
			activeSliderTech->GetDesc(&techDesc);

			m_pD3DImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			UINT stride = sizeof(Vertex::Basic32);
			UINT offset = 0;

			for (UINT p = 0; p < techDesc.Passes; ++p)
			{
				DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
				DirectX::XMMATRIX worldViewProj = world * view * proj;

				Effects::BasicFX->SetWorld(world);

				Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
				Effects::BasicFX->SetWorldViewProj(worldViewProj);

				Material mat;
				mat.Ambient.x = m.Ka[0];
				mat.Ambient.y = m.Ka[1];
				mat.Ambient.z = m.Ka[2];
				mat.Diffuse.x = m.Kd[0];
				mat.Diffuse.y = m.Kd[1];
				mat.Diffuse.z = m.Kd[2];
				mat.Specular.x = m.Ks[0];
				mat.Specular.y = m.Ks[1];
				mat.Specular.z = m.Ks[2];
				Effects::BasicFX->SetMaterial(mat);

				for (auto mt : gGlobalData->m_vMaterials)
				{
					if (mt.strName == m.strMaterial)
					{
						Effects::BasicFX->SetDiffuseMap(mt.mDiffuseMapSRV);
						break;
					}
				}


				Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&TexTransform)); 
				m_pD3DImmediateContext->RSSetState(RenderStates::NoCullRS);

				if(m_bWireframe)
				{
					m_pD3DImmediateContext->RSSetState(RenderStates::WireframeRS);
				}

				float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
				m_pD3DImmediateContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);

				m_pD3DImmediateContext->IASetVertexBuffers(0, 1, &m.pVB, &stride, &offset);
				m_pD3DImmediateContext->IASetIndexBuffer(m.pIB, DXGI_FORMAT_R32_UINT, 0);

				activeSliderTech->GetPassByIndex(p)->Apply(0, m_pD3DImmediateContext);
				m_pD3DImmediateContext->DrawIndexed(m.nNumTriangles * 3, 0, 0);
				// Restore default blend state
				m_pD3DImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
			}
		}
	}
	/*if (m_bRayValid) {
		DrawRay();
	}*/
	DrawSun();
	//Render foreground with Direct2D
	/*DXGI_SWAP_CHAIN_DESC swapDesc;
	HRESULT hr = m_pSwapChain->GetDesc(&swapDesc);*/

	CString strText;
	DirectX::XMFLOAT3 pos = m_Camera.GetPosition();
	strText.Format(L"fps = %.1f   ms per frame= %.3f \nx=%.1f y=%.1f z=%.1f \n", m_fps, m_mspf, pos.x, pos.y, pos.z);
	//display text at bottom left of screen
	m_pBackBufferRT->BeginDraw();
	D2D1_RECT_F rect = D2D1::RectF(0, 0, m_nClientWidth * 2, 64);
	m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	m_pBackBufferRT->DrawText(strText, strText.GetLength(), m_pTextFormat, rect, m_pTextBrush, D2D1_DRAW_TEXT_OPTIONS::D2D1_DRAW_TEXT_OPTIONS_CLIP);

	m_pBackBufferRT->EndDraw();

	HRESULT hr = m_pSwapChain->Present(0, 0);
}
//-----------------------------------------------------------------------//
void CRayTracerView::DrawRay()
{
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX worldInvTranspose = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX worldViewProj = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX view = m_Camera.View();
	DirectX::XMMATRIX proj = m_Camera.Proj();

	m_pD3DImmediateContext->IASetInputLayout(InputLayouts::VertexPos);
	ID3DX11EffectTechnique* activeTech = Effects::BasicFX->LineTech;
	D3DX11_TECHNIQUE_DESC techDesc;
	activeTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		// Draw the slice. 
		UINT stride2 = sizeof(Vertex::VertexPos);
		UINT offset2 = 0;

		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetLineColor(Colors::Red);
		m_pD3DImmediateContext->IASetVertexBuffers(0, 1, &mRayVB, &stride2, &offset2);
		m_pD3DImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
		activeTech->GetPassByIndex(p)->Apply(0, m_pD3DImmediateContext);
		m_pD3DImmediateContext->Draw(2, 0);

	}
	m_pD3DImmediateContext->OMSetDepthStencilState(0, 0);
}
//-----------------------------------------------------------------------//
void CRayTracerView::DrawSun()
{
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX worldInvTranspose = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX worldViewProj = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX view = m_Camera.View();
	DirectX::XMMATRIX proj = m_Camera.Proj();

	m_pD3DImmediateContext->IASetInputLayout(InputLayouts::PosNormal);
	ID3DX11EffectTechnique* activeTech = Effects::BasicFX->SunTech;
	D3DX11_TECHNIQUE_DESC techDesc;
	activeTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		// Draw the slice. 
		UINT stride2 = sizeof(Vertex::PosNormal);
		UINT offset2 = 0;

		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		Effects::BasicFX->SetDirLights(mDirLights);
		Effects::BasicFX->SetEyePosW(m_Camera.GetPosition());
 

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		//Effects::BasicFX->SetLineColor(Colors::Red);
		m_pD3DImmediateContext->IASetVertexBuffers(0, 1, &mSunVB, &stride2, &offset2);
		m_pD3DImmediateContext->IASetIndexBuffer(mSunIB, DXGI_FORMAT_R32_UINT, 0);
		m_pD3DImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		activeTech->GetPassByIndex(p)->Apply(0, m_pD3DImmediateContext);
		m_pD3DImmediateContext->DrawIndexed(m_nSunIndexCount, 0, 0);
	}
	 
	m_pD3DImmediateContext->OMSetDepthStencilState(0, 0);
}
//--------------------------------------------------------------------//
void CRayTracerView::FreeMesh(CUDAMesh* pDst)
{
	if (pDst)
	{
		CCUDAVertex* pVert = pDst->pVertices;
		if (pDst->pMesh) {
			for (int h = 0; h < 8; h++) {
				FreeMesh(&pDst->pMesh[h]);
			}
			delete[] pDst->pMesh;
		}
		delete[] pVert;
	}
}
//-----------------------------------------------------------------------//
void CRayTracerView::CopyMesh(CUDAMesh* pVB, CMesh* pSrc)
{
	if (pSrc->m_pMesh)
	{
		pVB->pMesh = new CUDAMesh[8];
		for (int h = 0; h < 8; h++)
		{
			CopyMesh(&pVB->pMesh[h], &(pSrc->m_pMesh[h]));
		}
	}
	pVB->bbmin.x = pSrc->bbmin.x;
	pVB->bbmin.y = pSrc->bbmin.y;
	pVB->bbmin.z = pSrc->bbmin.z;
	pVB->bbmax.x = pSrc->bbmax.x;
	pVB->bbmax.y = pSrc->bbmax.y;
	pVB->bbmax.z = pSrc->bbmax.z;
	pVB->nNumTriangles = pSrc->nNumTriangles;
	if (pVB->nNumTriangles == 0)
	{
		long nStop = 0;
	}
	pVB->bLight = pSrc->bLight;
	pVB->BB.center.x = pSrc->boundingbox.Center.x;
	pVB->BB.center.y = pSrc->boundingbox.Center.y;
	pVB->BB.center.z = pSrc->boundingbox.Center.z;
	pVB->BB.extents.x = pSrc->boundingbox.Extents.x;
	pVB->BB.extents.y = pSrc->boundingbox.Extents.y;
	pVB->BB.extents.z = pSrc->boundingbox.Extents.z;
	if (pSrc->nNumTriangles > 0) {
		long nVI = 0;
		LONG nNumVertices = pSrc->vTriangles.size();
		pVB->pVertices = new CCUDAVertex[nNumVertices];
		for (auto j : pSrc->vTriangles) {
			pVB->pVertices[nVI].pos.x = j.pos.x;
			pVB->pVertices[nVI].pos.y = j.pos.y;
			pVB->pVertices[nVI].pos.z = j.pos.z;
			pVB->pVertices[nVI].normal.x = j.normal.x;
			pVB->pVertices[nVI].normal.y = j.normal.y;
			pVB->pVertices[nVI].normal.z = j.normal.z;
			pVB->pVertices[nVI].tex.x = j.tex.x;
			pVB->pVertices[nVI].tex.y = j.tex.y;
			nVI++;
		}
	}
}
//-----------------------------------------------------------------------//
void CRayTracerView::UpdateProgress(long nProgress, long nTotal)
{
	::SendMessage(gGlobalData->m_hSideWnd, WM_PROGRESS_UPDATE, nProgress, nTotal);
}
//-----------------------------------------------------------------------//
void CRayTracerView::CalcRayCUDA(long nNumSamples, bool bUseTextures)
{
	if (nNumSamples < 1) nNumSamples = 1;
	if (nNumSamples > 10000) nNumSamples = 10000;

	float nDiv =1.0f;
	long nImageWidth = m_nClientWidth / nDiv;
	long nImageHeight = m_nClientHeight / nDiv;
	CCUDAPathTracer PT;
	float3* pOutput = new float3[nImageWidth * nImageHeight];
	//PT.AddMesh();

	m_Camera.UpdateViewMatrix();
	m_bRayValid = true;
	DirectX::XMMATRIX CT = m_Camera.Proj();
	XMFLOAT4X4 P;
	XMStoreFloat4x4(&P, CT);
	// Transform ray to local space of Mesh.
	DirectX::XMMATRIX V = m_Camera.View();
	DirectX::XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(V), V);
	DirectX::XMMATRIX W = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(W), W);
	DirectX::XMMATRIX toLocal2 = XMMatrixMultiply(invView, invWorld);
	DirectX::XMFLOAT4X4 toLocal;
	XMStoreFloat4x4(&toLocal, toLocal2);
	float3 sunPos = { m_Sun.pos.x, m_Sun.pos.y,m_Sun.pos.z };
	float3 sunDir = { m_Sun.direction.x, m_Sun.direction.y, m_Sun.direction.z};
	float sunIntensity = m_Sun.nIntensity;

	//////////////////////////////////////////////////////////////////////////////////////////
	//Copy materials
	long nNumMaterials = gGlobalData->m_vMaterials.size();
	CUDAMaterial* pMaterials = new CUDAMaterial[nNumMaterials];
	long nMatIndex = 0; 	
	for (auto& m = gGlobalData->m_vMaterials.begin(); m != gGlobalData->m_vMaterials.end(); ++m)
	{
	 
		if (m->nWidth != 0 && m->nHeight != 0) {
			pMaterials[nMatIndex].pTexData = new float3[m->nWidth * m->nHeight];// i->m_pTexData
			for (int h = 0; h < m->nWidth; h++)
			{
				for (int j = 0; j < m->nHeight; j++)
				{
					pMaterials[nMatIndex].pTexData[h * m->nWidth + j].x = (float)LOBYTE(LOWORD(m->m_pTexData[j * m->nWidth + h]));
					pMaterials[nMatIndex].pTexData[h * m->nWidth + j].y = (float)HIBYTE(LOWORD(m->m_pTexData[j * m->nWidth + h]));
					pMaterials[nMatIndex].pTexData[h * m->nWidth + j].z = (float)LOBYTE(HIWORD(m->m_pTexData[j * m->nWidth + h]));
				}
			}
			pMaterials[nMatIndex].nWidth = m->nWidth;
			pMaterials[nMatIndex].nHeight = m->nHeight;
			m->pCUDAMaterial = &(pMaterials[nMatIndex]);
		}
		else
		{
			pMaterials[nMatIndex].pTexData = 0;
			pMaterials[nMatIndex].nWidth = 0;
			pMaterials[nMatIndex].nHeight = 0;
		}
		nMatIndex++;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//Copy meshes
	long nNumMeshs = gGlobalData->m_vMeshes.size();
	CUDAMesh* pVB = new CUDAMesh[nNumMeshs];	
	int nMeshIndex = 0;
	for (auto& mesh = gGlobalData->m_vMeshes.begin(); mesh != gGlobalData->m_vMeshes.end(); ++mesh)
	{ 
		pVB[nMeshIndex].pMaterial = 0;
		CopyMesh(&pVB[nMeshIndex], &(*mesh));

		for (auto mt : gGlobalData->m_vMaterials) {
			if (mt.strName == mesh->strMaterial) {
				pVB[nMeshIndex].pMaterial = mt.pCUDAMaterial;
				break;
			}
		}
		nMeshIndex++;
	}

 

	//send a messaahe to the side bar to that it can disable controls unmtil the render is finished.
	::SendMessage(gGlobalData->m_hSideWnd, WM_RENDER_START, 0, 0);

	DWORD dwStart = GetTickCount();

	//////////////////////////////////////////////////////////////////////////////////////////
	//Execute raytracing
	PT.CalcRays(this, pOutput, m_nClientWidth, m_nClientHeight, nNumSamples, nDiv, P(0, 0), P(1, 1), toLocal.m, sunPos, sunDir, sunIntensity, m_bGlobalIllumination, bUseTextures, pVB, nNumMeshs, pMaterials, nNumMaterials);
	DWORD dwEnd = GetTickCount();
	
	//////////////////////////////////////////////////////////////////////////////////////////
	//Free memory
	for (int h = 0; h < nNumMaterials; h++)
	{ 
		if (pMaterials[h].pTexData)  {
			delete[] pMaterials[h].pTexData;
		}
	}
	delete[] pMaterials;
	for (int h = 0; h < nNumMeshs; h++)
	{
		FreeMesh(&pVB[h]);
	}
	delete[] pVB;
 
	pVB = 0;
	//////////////////////////////////////////////////////////////////////////////////////////
	//Create Image of raytracing results
	CImage* pImage = new CImage();
	pImage->Create(nImageWidth, nImageHeight, 24, 0);
	BYTE* pArray = (BYTE*)pImage->GetBits();
	int nPitch = pImage->GetPitch();
	int nBitCount = pImage->GetBPP() / 8;
	int height = pImage->GetHeight();
	int width = pImage->GetWidth();
	for (int k = 0; k < width; k++) {
		for (int j = 0; j < height; j++) {
			*(pArray + nPitch * j + k * nBitCount + 0) = pOutput[j * nImageWidth + k].x;//blue
			*(pArray + nPitch * j + k * nBitCount + 1) = pOutput[j * nImageWidth + k].y;//green
			*(pArray + nPitch * j + k * nBitCount + 2) = pOutput[j * nImageWidth + k].z;//red
		}
	}
	::SendMessage(gGlobalData->m_hSideWnd, WM_RENDER_END, 0, 0);
	//save to disk
	pImage->Save(L"_ImageCUDA.jpg");
	
	delete[] pOutput;
	//////////////////////////////////////////////////////////////////////////////////////////
	//Display image in popup window
	CImageDlg dlg;
	dlg.SetImage(pImage, (dwEnd - dwStart) / 1000);
	dlg.DoModal();
	//send a messaahe to the side bar to that it can re-enable controls
	::SendMessage(gGlobalData->m_hSideWnd, WM_RENDER_END, 0, 0);

	delete pImage;
}
//-----------------------------------------------------------------------//
void CRayTracerView::CalcRayCPU(long nNumSamples, bool bUseTextures)
{
	if (nNumSamples < 1) nNumSamples = 1;
	if (nNumSamples > 10000) nNumSamples = 10000;
	m_nNumSamples = nNumSamples;
	float nDiv = 4.0f;
	long nImageWidth = m_nClientWidth / nDiv;
	long nImageHeight = m_nClientHeight / nDiv;
	m_Camera.UpdateViewMatrix();
	m_bRayValid = true;
	DirectX::XMMATRIX CT = m_Camera.Proj();
	XMFLOAT4X4 P;
	XMStoreFloat4x4(&P, CT);
	// Transform ray to local space of Mesh.
	DirectX::XMMATRIX V = m_Camera.View();
	DirectX::XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(V), V);
	DirectX::XMMATRIX W = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(W), W);
	DirectX::XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);
	DirectX::XMFLOAT3 r[2];
  
	DWORD* pImageData = new DWORD[nImageWidth * nImageHeight];
	CImage* pImage = new CImage();
	pImage->Create(nImageWidth, nImageHeight, 24, 0);
	BYTE* pArray = (BYTE*)pImage->GetBits();
	//#pragma omp parallel for schedule(dynamic, 1) private(r)       // OpenMP
	DWORD dwStart = GetTickCount();
 
	::SendMessage(gGlobalData->m_hSideWnd, WM_RENDER_START, 0, 0);
	CCPUPathTracer PT;
	PT.CalcRays(this, pImageData, m_nClientWidth, m_nClientHeight, nDiv, nNumSamples, P(0, 0), P(1, 1), toLocal, m_Sun, m_bGlobalIllumination, bUseTextures);

  
	/*if (h == 0 && j == 0)
	{
		//render debug ray
		m_rayDir = rayDir;
		m_rayStart = rayOrigin; 
		DirectX::XMFLOAT3 dir;
		DirectX::XMFLOAT3 origin;
		DirectX::XMStoreFloat3(&dir, rayDir);
		DirectX::XMStoreFloat3(&origin, rayOrigin);
		DirectX::XMFLOAT3 pos = m_Camera.GetPosition();
		r[0].x = origin.x;
		r[0].y = origin.y;
		r[0].z = origin.z;
		r[1].x = origin.x + dir.x * 2000;
		r[1].y = origin.y + dir.y * 2000;
		r[1].z = origin.z + dir.z * 2000;
	}*/
 

	DWORD dwEnd = GetTickCount(); 
	if (mRayVB) {
		mRayVB->Release();
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::VertexPos) * 2;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &r[0];
	HR(m_pD3DDevice->CreateBuffer(&vbd, &vinitData, &mRayVB));

	int nPitch = pImage->GetPitch();
	int nBitCount = pImage->GetBPP() / 8;
	int height = pImage->GetHeight();
	int width = pImage->GetWidth();
	for (int k = 0; k < width; k++) {
		for (int j = 0; j < height; j++) {
			//DWORD test = HIBYTE(HIWORD(pImageData[j * nClientWidth + k]));
			*(pArray + nPitch * j + k * nBitCount + 0) = LOBYTE(LOWORD(pImageData[j * nImageWidth + k]));
			*(pArray + nPitch * j + k * nBitCount + 1) = HIBYTE(LOWORD(pImageData[j * nImageWidth + k]));
			*(pArray + nPitch * j + k * nBitCount + 2) = LOBYTE(HIWORD(pImageData[j * nImageWidth + k]));
		}
	}
	delete[] pImageData; 

	//save result to disk
	pImage->Save(L"_ImageCPU.jpg");
	::SendMessage(gGlobalData->m_hSideWnd, WM_RENDER_END, 0, 0);

	//display resulting path-traced image
	CImageDlg dlg;
	dlg.SetImage(pImage, (dwEnd - dwStart) / 1000);
	dlg.DoModal();
	
	delete pImage; 
} 
//-----------------------------------------------------------------------//
void CRayTracerView::SetSunPos(float nAngle)
{
	//0 = sunrise, PI / 2 = noon, PI = sunset
	float nDist = 2000;
	DirectX::XMFLOAT3 sunpos(0, nDist * sin(nAngle), nDist * cos(nAngle));
 
	if (mSunVB) mSunVB->Release();
	if (mSunIB) mSunIB->Release();
	m_Sun.pos = sunpos;
	XMStoreFloat3(&m_Sun.direction, XMVector3Normalize(XMLoadFloat3(&sunpos)));
	//m_Sun.direction = sunpos;

	CMesh mesh;
	GeometryGenerator G;
	GeometryGenerator::MeshData meshData;
	G.CreateCylinder(200, 200, 50, 18, 1, meshData);
	long nNumVertices = meshData.Vertices.size();
	long nNumTris = meshData.Indices.size() / 3;

	int vertexSize = 2 * sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
	UINT bufferSize = nNumTris * vertexSize * 3;
	VBVertex* pVBData = new VBVertex[nNumTris * 3];
	int* pIndex = new int[nNumTris * 3];
	Vertex::PosNormal* pV = new Vertex::PosNormal[nNumVertices];
	for (int h = 0; h < nNumVertices; h++) {
		pV[h].Normal = meshData.Vertices[h].Normal;
		pV[h].Pos = meshData.Vertices[h].Position;
	}

	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(sunpos.x, sunpos.y, sunpos.z);
	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationX(M_PI / 2 - nAngle);
	DirectX::XMVector3TransformCoordStream((DirectX::XMFLOAT3*)pV, sizeof(Vertex::PosNormal), (DirectX::XMFLOAT3*)pV, sizeof(Vertex::PosNormal), nNumVertices, rotation * translation);
	for (int h = 0; h < nNumTris; h++)
	{
		DirectX::XMFLOAT3 pos1 = pV[meshData.Indices[3 * h + 0]].Pos;
		DirectX::XMFLOAT3 pos2 = pV[meshData.Indices[3 * h + 1]].Pos;
		DirectX::XMFLOAT3 pos3 = pV[meshData.Indices[3 * h + 2]].Pos;

		if (h == 0) {
			mesh.bbmin = mesh.bbmax = pos1;
		}

		if (pos1.x < mesh.bbmin.x) mesh.bbmin.x = pos1.x; else if (pos1.x > mesh.bbmax.x) mesh.bbmax.x = pos1.x;
		if (pos1.y < mesh.bbmin.y) mesh.bbmin.y = pos1.y; else if (pos1.y > mesh.bbmax.y) mesh.bbmax.y = pos1.y;
		if (pos1.z < mesh.bbmin.z) mesh.bbmin.z = pos1.z; else if (pos1.z > mesh.bbmax.z) mesh.bbmax.z = pos1.z;

		if (pos2.x < mesh.bbmin.x) mesh.bbmin.x = pos2.x; else if (pos2.x > mesh.bbmax.x) mesh.bbmax.x = pos2.x;
		if (pos2.y < mesh.bbmin.y) mesh.bbmin.y = pos2.y; else if (pos2.y > mesh.bbmax.y) mesh.bbmax.y = pos2.y;
		if (pos2.z < mesh.bbmin.z) mesh.bbmin.z = pos2.z; else if (pos2.z > mesh.bbmax.z) mesh.bbmax.z = pos2.z;

		if (pos3.x < mesh.bbmin.x) mesh.bbmin.x = pos3.x; else if (pos3.x > mesh.bbmax.x) mesh.bbmax.x = pos3.x;
		if (pos3.y < mesh.bbmin.y) mesh.bbmin.y = pos3.y; else if (pos3.y > mesh.bbmax.y) mesh.bbmax.y = pos3.y;
		if (pos3.z < mesh.bbmin.z) mesh.bbmin.z = pos3.z; else if (pos3.z > mesh.bbmax.z) mesh.bbmax.z = pos3.z;

		VBVertex v1; v1.pos = pos1;
		VBVertex v2; v2.pos = pos2;
		VBVertex v3; v3.pos = pos3;
		v1.normal.x = m_Sun.direction.x; v1.normal.y = m_Sun.direction.y; v1.normal.z = m_Sun.direction.z;
		v2.normal.x = m_Sun.direction.x; v2.normal.y = m_Sun.direction.y; v2.normal.z = m_Sun.direction.z;
		v3.normal.x = m_Sun.direction.x; v3.normal.y = m_Sun.direction.y; v3.normal.z = m_Sun.direction.z;
		v1.tex.x = 0; v1.tex.y = 0;
		v2.tex.x = 0; v2.tex.y = 0;
		v3.tex.x = 0; v3.tex.y = 0;

		pVBData[3 * h + 0] = v1;
		pVBData[3 * h + 1] = v2;
		pVBData[3 * h + 2] = v3;

		pIndex[3 * h + 0] = 3 * h + 0;
		pIndex[3 * h + 1] = 3 * h + 1;
		pIndex[3 * h + 2] = 3 * h + 2;

		mesh.vTriangles.push_back(v1);
		mesh.vTriangles.push_back(v2);
		mesh.vTriangles.push_back(v3);
	}

	mesh.nNumTriangles = nNumTris;
	{
		mesh.boundingbox.Center.x = (mesh.bbmin.x + mesh.bbmax.x) / 2;
		mesh.boundingbox.Center.y = (mesh.bbmin.y + mesh.bbmax.y) / 2;
		mesh.boundingbox.Center.z = (mesh.bbmin.z + mesh.bbmax.z) / 2;

		mesh.boundingbox.Extents.x = (mesh.bbmax.x - mesh.bbmin.x) / 2;
		mesh.boundingbox.Extents.y = (mesh.bbmax.y - mesh.bbmin.y) / 2;
		mesh.boundingbox.Extents.z = (mesh.bbmax.z - mesh.bbmin.z) / 2;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::PosNormal) * meshData.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = pV;
	HRESULT hr = m_pD3DDevice->CreateBuffer(&vbd, &vinitData, &mSunVB);

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * meshData.Indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &meshData.Indices[0];
	HR(m_pD3DDevice->CreateBuffer(&ibd, &iinitData, &mSunIB));
	m_nSunIndexCount = meshData.Indices.size();

	mesh.CreateBuffers(m_pD3DDevice, nNumTris, (BYTE*)pVBData, pIndex);
	mesh.bLight = true;

	for (auto i = gGlobalData->m_vMeshes.begin(); i != gGlobalData->m_vMeshes.end(); ++i)
	{
		if (i->bLight) {
			gGlobalData->m_vMeshes.erase(i);
			break;
		}
	}
	gGlobalData->m_vMeshes.push_back(mesh);
	m_SunMesh = mesh;
	//mDirLights[0].Direction = pos;// DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);
	//mDirLights[1].Direction = pos;// DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);
	//mDirLights[2].Direction = pos;// DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	delete[] pVBData;
	delete[] pIndex;
	delete[] pV;
}
//-----------------------------------------------------------------------//
void CRayTracerView::OnMouseMove(UINT nFlags, CPoint point)
{
	if ((nFlags & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(point.x - mLastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(point.y - mLastMousePos.y));

		m_Camera.Pitch(dy);
		m_Camera.RotateY(dx);
	}

	mLastMousePos.x = point.x;
	mLastMousePos.y = point.y;

	CView::OnMouseMove(nFlags, point);
}
//-----------------------------------------------------------------------//
void CRayTracerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CView::OnLButtonDown(nFlags, point);
}
//-----------------------------------------------------------------------//
void CRayTracerView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CView::OnLButtonUp(nFlags, point);
}
//-----------------------------------------------------------------------//