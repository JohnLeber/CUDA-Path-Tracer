
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
#include <omp.h> 
#include "GeometryGenerator.h"

#include "vec.h"

#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "PathTracer.h"
#include "ImageDlg.h"

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
	if (mSunVB)
	{
		mSunVB->Release();
	}
	if (mSunIB)
	{
		mSunIB->Release();
	}
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
//-----------------------------------------------------------------------//
bool CRayTracerView::RayTriangleIntersect(
	const DirectX::XMFLOAT3& v0, DirectX::XMFLOAT3& v1, DirectX::XMFLOAT3& v2,
	const DirectX::XMFLOAT3& orig, const DirectX::XMFLOAT3& dir,
	float& tnear, float& u, float& v)
{
	DirectX::XMFLOAT3 edge1 = { 0,0,0 };
	DirectX::XMFLOAT3 edge2 = { 0,0,0 };
	DirectX::XMFLOAT3 pvec = { 0,0,0 }; //= crossProduct(dir, edge2);
	DirectX::XMFLOAT3 det2 = { 0,0,0 };
	DirectX::XMFLOAT3 ut = { 0,0,0 };
	DirectX::XMFLOAT3 vt = { 0,0,0 };
	DirectX::XMFLOAT3 tnear2 = { 0,0,0 };
	float det = 0;
	XMStoreFloat3(&edge1, XMVector3Length(XMVectorSubtract(XMLoadFloat3(&v1), XMLoadFloat3(&v0))));
	XMStoreFloat3(&edge2, XMVector3Length(XMVectorSubtract(XMLoadFloat3(&v2), XMLoadFloat3(&v0))));
	XMStoreFloat3(&pvec, DirectX::XMVector3Cross(XMLoadFloat3(&dir), XMLoadFloat3(&edge2)));
	XMStoreFloat3(&det2, DirectX::XMVector3Dot(XMLoadFloat3(&edge1), XMLoadFloat3(&pvec)));
	//float det = dotProduct(edge1, pvec);
	det = det2.x;
	if (det == 0 || det < 0) return false;

	DirectX::XMFLOAT3 tvec = { 0, 0, 0 };// orig - v0;
	XMStoreFloat3(&tvec, XMVector3Length(XMVectorSubtract(XMLoadFloat3(&orig), XMLoadFloat3(&v0))));
	//u = dotProduct(tvec, pvec);

	XMStoreFloat3(&ut, DirectX::XMVector3Dot(XMLoadFloat3(&tvec), XMLoadFloat3(&pvec)));
	u = ut.x;
	if (u < 0 || u > det) return false;

	DirectX::XMFLOAT3 qvec = { 0,0,0 };// crossProduct(tvec, edge1);
	XMStoreFloat3(&qvec, DirectX::XMVector3Cross(XMLoadFloat3(&tvec), XMLoadFloat3(&edge1)));
	//v = dotProduct(dir, qvec);
	XMStoreFloat3(&vt, DirectX::XMVector3Dot(XMLoadFloat3(&dir), XMLoadFloat3(&qvec)));
	v = vt.x;
	if (v < 0 || u + v > det) return false;

	float invDet = 1 / det;

	//tnear = dotProduct(edge2, qvec) * invDet;
	XMStoreFloat3(&tnear2, DirectX::XMVector3Dot(XMLoadFloat3(&edge2), XMLoadFloat3(&qvec)));
	tnear = tnear2.x * invDet;

	u *= invDet;
	v *= invDet;

	return true;
} 
//--------------------------------------------------------------------//
bool CMesh::DEBUGIntersectRayAxisAlignedBox(DirectX::XMVECTOR& rayOrigin2, DirectX::XMVECTOR& rayDir2, DirectX::XMFLOAT3& bbmax, DirectX::XMFLOAT3& bbmin)
{
	rayDir2 = XMVector3Normalize(rayDir2);

	float nScale = 1;
	DirectX::XMFLOAT3 f1;
	DirectX::XMFLOAT3 f2;
	DirectX::XMStoreFloat3(&f1, rayOrigin2);
	DirectX::XMStoreFloat3(&f2, rayDir2);
	float3 rayOrigin = { f1.x, f1.y, f1.z };
	float3 rayDir = { f2.x * nScale, f2.y * nScale, f2.z * nScale };
 
	//https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms/18459#18459
	float t = 0;
	float3 dirfrac;
	dirfrac.x = 1.0f / rayDir.x;
	dirfrac.y = 1.0f / rayDir.y;
	dirfrac.z = 1.0f / rayDir.z;
	// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
	// r.org is origin of ray
	float t1 = (bbmin.x - rayOrigin.x) * dirfrac.x;
	float t2 = (bbmax.x - rayOrigin.x) * dirfrac.x;
	float t3 = (bbmin.y - rayOrigin.y) * dirfrac.y;
	float t4 = (bbmax.y - rayOrigin.y) * dirfrac.y;
	float t5 = (bbmin.z - rayOrigin.z) * dirfrac.z;
	float t6 = (bbmax.z - rayOrigin.z) * dirfrac.z;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
	if (tmax < 0)
	{
		t = tmax;
		return false;
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		t = tmax;
		return false;
	}

	t = tmin;
	return true; 
}
//--------------------------------------------------------------------//
float3 DEBUGVec3Subtract(float3& in1, float3& in2)
{
	float3 out;
	out.x = in1.x - in2.x;
	out.y = in1.y - in2.y;
	out.z = in1.z - in2.z;
	return out;
}
//--------------------------------------------------------------------//
float DEBUGVec3Dot(float3& v1, float3& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
//--------------------------------------------------------------------//
float3 DEBUGVec3Cross(float3& v1, float3& v2)
{
	float3 out;
	out.x = v1.y * v2.z - v1.z * v2.y;
	out.y = -v1.x * v2.z + v1.z * v2.x;
	out.z = v1.x * v2.y - v1.y * v2.x;
	return out;
}
//--------------------------------------------------------------------//
bool CMesh::DEBUGIntersectRayTriangle(DirectX::XMVECTOR rayOrigin2, DirectX::XMVECTOR rayDir2, DirectX::XMVECTOR v0B, DirectX::XMVECTOR v1B, DirectX::XMVECTOR v2B, float* pDist, float* pU, float* pV)
{ 
	DirectX::XMFLOAT3 f1;
	DirectX::XMFLOAT3 f2;
	DirectX::XMStoreFloat3(&f1, rayOrigin2);
	DirectX::XMStoreFloat3(&f2, rayDir2);

	DirectX::XMFLOAT3 vv0;
	DirectX::XMFLOAT3 vv1;
	DirectX::XMFLOAT3 vv2;
	DirectX::XMStoreFloat3(&vv0, v0B);
	DirectX::XMStoreFloat3(&vv1, v1B);
	DirectX::XMStoreFloat3(&vv2, v2B);
	float3 rayOrigin = { f1.x, f1.y, f1.z };
	float3 rayDir = { f2.x, f2.y, f2.z };
	float3 v0 = { vv0.x, vv0.y, vv0.z };
	float3 v1 = { vv1.x, vv1.y, vv1.z };
	float3 v2 = { vv2.x, vv2.y, vv2.z };

	//https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
	float kEpsilon = 0.00001f;
	float3 e1 = DEBUGVec3Subtract(v1, v0);
	float3 e2 = DEBUGVec3Subtract(v2, v0);
	float3 p = DEBUGVec3Cross(rayDir, e2);
	float det = DEBUGVec3Dot(e1, p);
	 
	if (det >= kEpsilon) {
		 
		float3 s = DEBUGVec3Subtract(rayOrigin, v0);

		*pU = DEBUGVec3Dot(s, p);
		if (*pU < 0.0 || *pU > det) return false;

		float3 q = DEBUGVec3Cross(s, e1);
		*pV = DEBUGVec3Dot(rayDir, q);
		if (*pV < 0.0 || *pU + *pV > det) return false;
		*pDist = DEBUGVec3Dot(e2, q);
		if (*pDist < 0) return false;
	}
	else if (det <= -kEpsilon) {
		float3 s = DEBUGVec3Subtract(rayOrigin, v0);

		*pU = DEBUGVec3Dot(s, p);
		if (*pU > 0.0 || *pU < det) return false;

		float3 q = DEBUGVec3Cross(s, e1);
		*pV = DEBUGVec3Dot(rayDir, q);
		if (*pV > 0.0 || *pU + *pV < det) return false;
		*pDist = DEBUGVec3Dot(e2, q);
		if (*pDist > 0) return false;
	}
	else
	{
		return false;
	}
	float invDet = 1.0f / det;
	*pU = *pU * invDet;
	*pV = *pV * invDet;
	*pDist = *pDist * invDet;
	return true;
}
//-----------------------------------------------------------------------//
bool CMesh::Intersect(DirectX::XMVECTOR& rayOrigin, DirectX::XMVECTOR& rayDir, float& u, float& v, float& dist, DirectX::XMFLOAT3& hitpoint, DirectX::XMFLOAT3& nml)
{
	float dist2 = 0;
	float3 rayOrigin2 = {};
	 
	if (!DEBUGIntersectRayAxisAlignedBox(rayOrigin, rayDir, bbmax, bbmin))
	{
		return false;
	}

	//if (!XNA::IntersectRayAxisAlignedBox(rayOrigin, rayDir, &boundingbox, &dist2))
	//{
	//	return false;
	//}
	if (m_pMesh)
	{
		bool bHit = false;
		for (int h = 0; h < 8; h++) {
			if (m_pMesh[h].Intersect(rayOrigin, rayDir, u, v, dist, hitpoint, nml)) {
				//return true;
				bHit = true;
			}
		}
		return bHit;
	}
	//if (!DoesRayIntersectBox(rayOrigin, rayDir, bbmin, bbmax))//!XNA::IntersectRayAxisAlignedBox(rayOrigin, rayDir, &boundingbox, &dist))
 
	bool bHit = false;
	//check if ray intersects mesh, if not skip...

	//DirectX::XMVECTOR origin = XMLoadFloat3(&rayOrigin);
	//DirectX::XMVECTOR dir	= XMLoadFloat3(&rayDir);
//	float dist2 = MathHelper::Infinity;
	for (int k = 0; k < nNumTriangles; k++)
	{
		float t = 0.0f;
		FLOAT ua = 0;
		FLOAT va = 0;
		// We have to iterate over all the triangles in order to find the nearest intersection.
		DirectX::XMVECTOR v1 = XMLoadFloat3(&vTriangles[3 * k + 0].pos);
		DirectX::XMVECTOR v2 = XMLoadFloat3(&vTriangles[3 * k + 1].pos);
		DirectX::XMVECTOR v3 = XMLoadFloat3(&vTriangles[3 * k + 2].pos);
		if (DEBUGIntersectRayTriangle(rayOrigin, rayDir, v1, v2, v3, &t, &ua, &va))
		//if (XNA::IntersectRayTriangle(rayOrigin, rayDir, v1, v2, v3, &t, &ua, &va))
		{
			if (t < dist) {
				dist = t;
				u = (1 - ua - va) * vTriangles[3 * k + 0].tex.x + ua * vTriangles[3 * k + 1].tex.x + va * vTriangles[3 * k + 2].tex.x;
				v = (1 - ua - va) * vTriangles[3 * k + 0].tex.y + ua * vTriangles[3 * k + 1].tex.y + va * vTriangles[3 * k + 2].tex.y;

				hitpoint.x = (1 - ua - va) * vTriangles[3 * k + 0].pos.x + ua * vTriangles[3 * k + 1].pos.x + va * vTriangles[3 * k + 2].pos.x;
				hitpoint.y = (1 - ua - va) * vTriangles[3 * k + 0].pos.y + ua * vTriangles[3 * k + 1].pos.y + va * vTriangles[3 * k + 2].pos.y;
				hitpoint.z = (1 - ua - va) * vTriangles[3 * k + 0].pos.z + ua * vTriangles[3 * k + 1].pos.z + va * vTriangles[3 * k + 2].pos.z;

				nml.x = (vTriangles[3 * k + 0].normal.x + vTriangles[3 * k + 1].normal.x + vTriangles[3 * k + 2].normal.x) / 3;
				nml.y = (vTriangles[3 * k + 0].normal.y + vTriangles[3 * k + 1].normal.y + vTriangles[3 * k + 2].normal.y) / 3;
				nml.z = (vTriangles[3 * k + 0].normal.z + vTriangles[3 * k + 1].normal.z + vTriangles[3 * k + 2].normal.z) / 3;
				DirectX::XMStoreFloat3(&nml, XMVector3Normalize(XMLoadFloat3(&nml)) );
				bHit = true;
			}
		}
	}
	return bHit;
}
//-----------------------------------------------------------------------//
bool CRayTracerView::Trace(DirectX::XMVECTOR& rayOrigin, DirectX::XMVECTOR& rayDir, bool bHitOnly, DirectX::XMFLOAT3& hitpoint, DirectX::XMFLOAT3& nml, DirectX::XMFLOAT3& rgb, float& nHitDist)
{ 
	float tmin = MathHelper::Infinity;
	float u = 0;
	float v = 0;
	long nTexWidth  = 0;
	long nTexHeight  = 0;
	DWORD* pTexData = 0;
	bool bHit = false;
	float dist = MathHelper::Infinity;
	for (auto& m : gGlobalData->m_vMeshes)
	{
		if (m.bLight) continue;
		float ua = 0;
		float va = 0;
		if (!m.Intersect(rayOrigin, rayDir, ua, va, dist, hitpoint, nml)) {
			continue;
		}
		bHit = true;
		u = ua;
		v = va;
		//meshhit = m;
		nTexWidth = m.nWidth;
		nTexHeight = m.nHeight;
		pTexData = m.m_pData;
	}
	if (bHit)
	{
		if (bHitOnly) return true;
		//Get surface properties (texture and uv...).
		/*if (v >= 1) { v = fmod(v, 1); }
		if (u >= 1) { u = fmod(u, 1); }*/
		while (u > 1) { u = u - 1; }
		while (v > 1) { v = v - 1; }
		while (u < 0) { u = u + 1; }
		while (v < 0) { v = v + 1; }
		long j = nTexWidth * v;
		long h = nTexHeight * u;
		if (nTexWidth > 0) {
			DWORD dwPixel = pTexData[j * nTexWidth + h];
			rgb.x = 0.5f;//(float)(LOBYTE(LOWORD(dwPixel))) / 255.0f;
			rgb.y = 0.5f;//(float)(HIBYTE(LOWORD(dwPixel))) / 255.0f;
			rgb.z = 0.5f;//(float)(LOBYTE(HIWORD(dwPixel))) / 255.0f;
		}
	}

	return bHit;
}
std::default_random_engine generator;
std::uniform_real_distribution<float> distribution(0, 1);
//-----------------------------------------------------------------------//
DirectX::XMFLOAT3 CRayTracerView::Radiance(DirectX::XMVECTOR& rayOrigin, DirectX::XMVECTOR& rayDir, const int& depth, unsigned short* Xi)
{ 
	//CMesh meshhit;
	DirectX::XMFLOAT3 rgb = { 0, 0, 0 };
	DirectX::XMFLOAT3 nml = {0, 0, 0};
	DirectX::XMFLOAT3 hitpoint = { 0, 0, 0 };
	if (depth > 2) return rgb;
	bool bHit = false;
	float tmin = MathHelper::Infinity;
	float dist = MathHelper::Infinity;
	float nHitDist = 0;
	DirectX::XMFLOAT3 directLighting = { 0, 0, 0 };
	DirectX::XMFLOAT3 indirectLighting = { 0, 0, 0 };
	//for (auto& m : gGlobalData->m_vMeshes)
	//{
	//	float ua = 0;
	//	float va = 0;
	//	if (!m.Intersect(rayOrigin, rayDir, ua, va, dist, nml)) {
	//		continue;
	//	}
	//	bHit = true;
	//	u = ua;
	//	v = va;
	//	//meshhit = m;
	//	nTexWidth = m.nWidth;
	//	nTexHeight = m.nHeight;
	//	pTexData = m.m_pData;
	//}
	bHit = Trace(rayOrigin, rayDir, false, hitpoint, nml, rgb, nHitDist);
	if (bHit)
	{ 
		//assume diffuse material
		//Direct light - cast shadow ray towards sun
		DirectX::XMVECTOR hitPoint = XMLoadFloat3(&hitpoint);
		DirectX::XMVECTOR hitNml = XMLoadFloat3(&nml);
		DirectX::XMVECTOR sunPos = XMLoadFloat3(&m_Sun.pos);
		DirectX::XMVECTOR sunDir = sunPos - hitPoint;
		sunDir = XMVector3Normalize(sunDir);

		float bias = 0.01f;
		DirectX::XMFLOAT3 hitsunnml = { 0, 0, 0 };
		DirectX::XMFLOAT3 sunhitpoint = { 0, 0, 0 };
		DirectX::XMFLOAT3 hitrgb = { 0, 0, 0 };
		bHit = Trace(hitPoint + bias * hitNml, sunDir, true, sunhitpoint, hitsunnml, hitrgb, nHitDist);
		if (!bHit)
		{ 
			DirectX::XMFLOAT3 dp = { 0,0,0 };
			//the sun is a distance light, so lets not divide by sqrt(r*r) and 4/pi/r2 etc
			XMStoreFloat3(&dp, DirectX::XMVector3Dot(hitNml, sunDir));
			directLighting.x = m_Sun.nIntensity * max(0.0f, dp.x);
			directLighting.y = m_Sun.nIntensity * max(0.0f, dp.y);
			directLighting.z = m_Sun.nIntensity * max(0.0f, dp.z);
			
		}		
		//https://www.scratchapixel.com/code.php?id=34&origin=/lessons/3d-basic-rendering/global-illumination-path-tracing&src=0
		if (m_bGlobalIllumination)
		{
			uint32_t N = m_nNumSamples;// / (depth + 1); 
			DirectX::XMFLOAT3 Nt;
			DirectX::XMFLOAT3 Nb;
			DirectX::XMFLOAT3 hitNormal = { 0, 0, 0 };
			XMStoreFloat3(&hitNormal, hitNml);
			CreateCoordinateSystem(hitNml, Nt, Nb);
			
			float pdf = 1 / (2 * M_PI);
			for (uint32_t n = 0; n < N; ++n) {
				float r1 = distribution(generator);
				float r2 = distribution(generator);
				DirectX::XMFLOAT3 sample = uniformSampleHemisphere(r1, r2);
				DirectX::XMFLOAT3 sampleWorldF(
					sample.x * Nb.x + sample.y * hitNormal.x + sample.z * Nt.x,
					sample.x * Nb.y + sample.y * hitNormal.y + sample.z * Nt.y,
					sample.x * Nb.z + sample.y * hitNormal.z + sample.z * Nt.z);
				DirectX::XMVECTOR sampleWorld = DirectX::XMLoadFloat3(&sampleWorldF);
				DirectX::XMFLOAT3 rd = Radiance(hitPoint + bias * sampleWorld, sampleWorld, depth + 1, Xi);
				indirectLighting.x = indirectLighting.x + r1 * rd.x / pdf;
				indirectLighting.y = indirectLighting.y + r1 * rd.y / pdf;
				indirectLighting.z = indirectLighting.z + r1 * rd.z / pdf;
				if (indirectLighting.x > 0.001)
				{
					long nStop = 0;
				}
				//indirectLighting += r1 * castRay(hitPoint + sampleWorld * options.bias,
			//		sampleWorld, objects, lights, options, depth + 1) / pdf;
			}
			indirectLighting.x = indirectLighting.x	/ N;
			indirectLighting.y = indirectLighting.y	/ N;
			indirectLighting.z = indirectLighting.z	/ N;
		}	
		rgb.x = (directLighting.x / M_PI + 2 * indirectLighting.x) * rgb.x;// *isect.hitObject->albedo;
		rgb.y = (directLighting.y / M_PI + 2 * indirectLighting.y) * rgb.y;// *isect.hitObject->albedo;
		rgb.z = (directLighting.z / M_PI + 2 * indirectLighting.z) * rgb.z;// *isect.hitObject->albedo;
	}
	return rgb;
}
//-----------------------------------------------------------------------//
DirectX::XMFLOAT3 CRayTracerView::uniformSampleHemisphere(const float& r1, const float& r2)
{
	//https://www.scratchapixel.com/code.php?id=34&origin=/lessons/3d-basic-rendering/global-illumination-path-tracing
	float sinTheta = sqrtf(1 - r1 * r1);
	float phi = 2 * M_PI * r2;
	float x = sinTheta * cosf(phi);
	float z = sinTheta * sinf(phi);
	return DirectX::XMFLOAT3(x, r1, z);
}
//-----------------------------------------------------------------------//
void CRayTracerView::CreateCoordinateSystem(const DirectX::XMVECTOR& N2, DirectX::XMFLOAT3& NtF, DirectX::XMFLOAT3& Nb)
{
	//https://www.scratchapixel.com/code.php?id=34&origin=/lessons/3d-basic-rendering/global-illumination-path-tracing
	DirectX::XMFLOAT3 N = { 0,0,0 };
	DirectX::XMVECTOR Nt;
	XMStoreFloat3(&N, N2);
	if (std::fabs(N.x) > std::fabs(N.y)) {
		Nt  = DirectX::XMLoadFloat3(&DirectX::XMFLOAT3(N.z, 0, -N.x)) / _hypot(N.x, N.z);// (N.x * N.x + N.z * N.z);
	}
	else {
		Nt = DirectX::XMLoadFloat3(&DirectX::XMFLOAT3(0, -N.z, N.y)) / _hypot(N.y, N.z);//sqrtf(N.y * N.y + N.z * N.z);
	}
	XMStoreFloat3(&NtF, Nt); 
	XMStoreFloat3(&Nb, DirectX::XMVector3Cross(XMLoadFloat3(&N), Nt));
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
void CRayTracerView::CUDAProgress(long nProgress, long nTotal)
{
	::SendMessage(gGlobalData->m_hSideWnd, WM_PROGRESS_UPDATE, nProgress, nTotal);
}
//-----------------------------------------------------------------------//
void CRayTracerView::CalcRayCUDA(long nNumSamples)
{
	if (nNumSamples < 1) nNumSamples = 1;
	if (nNumSamples > 10000) nNumSamples = 10000;

	float nDiv = 4.0f;
	long nImageWidth = m_nClientWidth / nDiv;
	long nImageHeight = m_nClientHeight / nDiv;
	CPathTracer PT;
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

	long nNumMeshs = gGlobalData->m_vMeshes.size();
	CUDAMesh* pVB = new CUDAMesh[nNumMeshs];	
	int nMeshIndex = 0;
	for (auto& i = gGlobalData->m_vMeshes.begin(); i != gGlobalData->m_vMeshes.end(); ++i)
	{
		CopyMesh(&pVB[nMeshIndex++], &(*i));
		//if (nMeshIndex > 20) break;
	}

	::SendMessage(gGlobalData->m_hSideWnd, WM_RENDER_START, 0, 0);

	DWORD dwStart = GetTickCount();
	PT.CalcRays(this, pOutput, m_nClientWidth, m_nClientHeight, nNumSamples, nDiv, P(0, 0), P(1, 1), toLocal.m, sunPos, sunDir, sunIntensity, m_bGlobalIllumination, pVB, nNumMeshs);
	DWORD dwEnd = GetTickCount();
	CString strTime;
	strTime.Format(L"CUDA %d", dwEnd - dwStart);
	AfxMessageBox(strTime);
	delete[] pVB;
	pVB = 0;

	CImage* pImage = new CImage();
	pImage->Create(nImageWidth, nImageHeight, 24, 0);
	BYTE* pArray = (BYTE*)pImage->GetBits();
	int nPitch = pImage->GetPitch();
	int nBitCount = pImage->GetBPP() / 8;
	int height = pImage->GetHeight();
	int width = pImage->GetWidth();
	for (int k = 0; k < width; k++) {
		for (int j = 0; j < height; j++) {
			if (pOutput[j * nImageWidth + k].x > 10)
			{
				long nStop = 0;
			}
			*(pArray + nPitch * j + k * nBitCount + 0) = pOutput[j * nImageWidth + k].x;//blue
			*(pArray + nPitch * j + k * nBitCount + 1) = pOutput[j * nImageWidth + k].y;//green
			*(pArray + nPitch * j + k * nBitCount + 2) = pOutput[j * nImageWidth + k].z;//red
		}
	}
	::SendMessage(gGlobalData->m_hSideWnd, WM_RENDER_END, 0, 0);
	//save to disk
	pImage->Save(L"_ImageCUDA.jpg");

	CImageDlg dlg;
	dlg.SetImage(pImage, (dwEnd - dwStart) / 1000);
	dlg.DoModal();

	::SendMessage(gGlobalData->m_hSideWnd, WM_RENDER_END, 0, 0);

	delete pImage;
}
//-----------------------------------------------------------------------//
void CRayTracerView::CalcRayCPU(long nNumSamples)
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
 
	//DirectX::XMFLOAT3 pos = m_Camera.GetPosition();
	//float imageAspectRatio = GetAspectRatio();
	//float scale = tan(deg2rad(45 * 0.5));
	DWORD* pImageData = new DWORD[nImageWidth * nImageHeight];
	DirectX::XMFLOAT3* pRGB = new DirectX::XMFLOAT3[nImageWidth * nImageHeight];
	CImage* pImage = new CImage();
	pImage->Create(nImageWidth, nImageHeight, 24, 0);
	BYTE* pArray = (BYTE*)pImage->GetBits();
	//#pragma omp parallel for schedule(dynamic, 1) private(r)       // OpenMP
	DWORD dwStart = GetTickCount();
	long nProgress = 0;//used to update progressbar
	::SendMessage(gGlobalData->m_hSideWnd, WM_PROGRESS_UPDATE, 0, nImageWidth * nImageHeight);

	::SendMessage(gGlobalData->m_hSideWnd, WM_RENDER_START, 0, 0);
 

omp_set_num_threads(16);
#pragma omp parallel for
	for (int j = 0; j < nImageWidth; ++j) {
		unsigned short Xi[3] = { 0 ,0, j * j * j }; // *** Moved outside for VS2012
		for (int h = 0; h < nImageHeight; ++h) {
			DWORD dwPixel = MAKELONG(MAKEWORD(0, 0), MAKEWORD(0, 255));
			// generate primary ray direction
			float vx = (+2.0f * (float)nDiv * j / (float)m_nClientWidth - 1.0f) / P(0, 0);
			float vy = (-2.0f * (float)nDiv * h / (float)m_nClientHeight + 1.0f) / P(1, 1);
			 
			//Ray definition in view space.
			DirectX::XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			DirectX::XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);
			rayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
			rayDir = XMVector3TransformNormal(rayDir, toLocal);
			rayDir = XMVector3Normalize(rayDir);
			//transform to ray origin to local space should equal the camera position (eyepos).

			DirectX::XMFLOAT3 dir;
			DirectX::XMFLOAT3 origin;
			DirectX::XMStoreFloat3(&dir, rayDir);
			DirectX::XMStoreFloat3(&origin, rayOrigin);
			 
			int depth = 0;
			DirectX::XMFLOAT3 rgb = Radiance(rayOrigin, rayDir, depth, Xi);
			 
			::SendMessage(gGlobalData->m_hSideWnd, WM_PROGRESS_UPDATE, nProgress++ , nImageWidth * nImageHeight);

			pImageData[h * nImageWidth + j] = MAKELONG(MAKEWORD(rgb.x * 255, rgb.y * 255), MAKEWORD(rgb.z * 255, 255));
			pRGB[h * nImageWidth + j].x = rgb.x;
			pRGB[h * nImageWidth + j].y = rgb.y;
			pRGB[h * nImageWidth + j].z = rgb.z;
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
		}
	}

	DWORD dwEnd = GetTickCount();
	CString strTime;
	strTime.Format(L"%d", dwEnd - dwStart);
	//AfxMessageBox(strTime);

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
	delete[] pRGB;

	//save to disk
	pImage->Save(L"_ImageCPU.jpg");
	::SendMessage(gGlobalData->m_hSideWnd, WM_RENDER_END, 0, 0);
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