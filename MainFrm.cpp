
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "pch.h"
#include "framework.h"
#include "RayTracer.h"

#include "MainFrm.h"
#include "SideView.h"
#include "RayTracerView.h"
#include "D3DObjMesh.h"
#include "WICTextureLoader11.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE() 
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

//-------------------------------------------------------------//
CGlobalData* gGlobalData = 0;

// CMainFrame construction/destruction

CMainFrame::CMainFrame() noexcept
{
	gGlobalData = new CGlobalData();
}
//-------------------------------------------------------------//
CMainFrame::~CMainFrame()
{
	//D3D
	if (m_pD3DImmediateContext)
	{
		m_pD3DImmediateContext->ClearState();
		m_pD3DImmediateContext->Release();
	}

	if (m_pD3DDevice)
	{
		m_pD3DDevice->Release();
	}

	//D2D
	if (m_pTextFormat)
	{
		m_pTextFormat->Release();
	}
	if (m_pWriteFactory)
	{
		m_pWriteFactory->Release();
	}
	if (m_pD2DFactory)
	{
		m_pD2DFactory->Release();
	}

	//DXGI

	if (m_pDXGIFactory) {
		m_pDXGIFactory->Release();
	}

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
			if (i.m_pData)
			{
				delete[] i.m_pData;
				i.m_pData = 0;
			}
		}
		delete gGlobalData;
		gGlobalData = 0;
	}
}
//-------------------------------------------------------------//
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	//if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
	//	!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	//{
	//	TRACE0("Failed to create toolbar\n");
	//	return -1;      // fail to create
	//}

	//if (!m_wndStatusBar.Create(this))
	//{
	//	TRACE0("Failed to create status bar\n");
	//	return -1;      // fail to create
	//}
	//m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	//// TODO: Delete these three lines if you don't want the toolbar to be dockable
	//m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	//EnableDocking(CBRS_ALIGN_ANY);
	//DockControlBar(&m_wndToolBar);
	 
	return 0;
}
//-------------------------------------------------------------//
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	// create splitter window
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
		return FALSE;

	if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CSideView), CSize(500, 100), pContext) ||
		!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CRayTracerView), CSize(500, 100), pContext))
	{
		m_wndSplitter.DestroyWindow();
		return FALSE;
	}
	m_pView = (CRayTracerView*)m_wndSplitter.GetPane(0, 1);
	return TRUE;
}
//-------------------------------------------------------------//
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.style &= ~FWS_ADDTOTITLE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}
//-------------------------------------------------------------//
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}
//-------------------------------------------------------------//
void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers
//-------------------------------------------------------------//
bool CMainFrame::Initialize()
{
	if (!InitD2D())
	{
		ATLASSERT(0);
		return false;
	}
	if (!InitD3D())
	{
		ATLASSERT(0);
		return false;
	}

	//first we need to find the Meshess folder//
	//This will be at a higher level. Recursively search until we find it
	CString strMeshPath;
	TCHAR szFilePath[MAX_PATH + _ATL_QUOTES_SPACE];
	DWORD dwFLen = ::GetModuleFileName(NULL, szFilePath + 0, MAX_PATH);
	strMeshPath = CString(szFilePath);

	long nRight = strMeshPath.ReverseFind(_T('\\'));//remove exe name
	strMeshPath = strMeshPath.Left(nRight);//this will ne the same folder as the EXe
	bool bFoundMeshFolder = false;
	int nNumAttempts = 0;
	while (!bFoundMeshFolder && nNumAttempts < 3)
	{
		nRight = strMeshPath.ReverseFind(_T('\\'));//move one folder up
		strMeshPath = strMeshPath.Left(nRight);//this will ne the same folder as 
		nNumAttempts++;
		if (DoesDirExist(strMeshPath + L"\\Meshes")) {
			bFoundMeshFolder = true;
			strMeshPath.Append(L"\\Meshes\\");
		}

	}

	CString strPath = strMeshPath + L"Sponza\\sponza.mesh";
	TObjMesh objMesh;
	LoadObj(strPath, &objMesh);
	 
	for (auto& m : objMesh.materials)//could use a map here, but...
	{   
		CString strMapKa = CA2T(m->map_Ka);
		CString strMapKd = CA2T(m->map_Kd);
		
		CMat mat;
		mat.strPath = strMapKd;
		if (strMapKd != L"")
		{
			ID3D11Resource* texResource = nullptr;
			HRESULT hr = DirectX::CreateWICTextureFromFile(m_pD3DDevice, strMapKd, &texResource, &mat.mDiffuseMapSRV);
			if (SUCCEEDED(hr)) { 
				CImage image;
				if (SUCCEEDED(image.Load(strMapKd)))
				{
					mat.nWidth = image.GetWidth();
					mat.nHeight = image.GetHeight();
					BYTE* pArray = (BYTE*)image.GetBits();
					int nPitch = image.GetPitch();
					int nBitCount = image.GetBPP() / 8;
					mat.m_pData = new DWORD[mat.nWidth * mat.nHeight];
					for (int h = 0; h < mat.nWidth; h++) {
						for (int j = 0; j < mat.nHeight; j++) {
							mat.m_pData[j * mat.nWidth + h] = MAKELONG(
								MAKEWORD( *(pArray + nPitch * j + h * nBitCount + 0), *(pArray + nPitch * j + h * nBitCount + 1) ),
								MAKEWORD( *(pArray + nPitch * j + h * nBitCount + 2), 255)
							);
						}
					} 
				}
				mat.strName = CA2T(m->name);
				gGlobalData->m_vMaterials.push_back(mat);
			}
		}
		/*
		mesh.strMapKs = CA2T(m->map_Ks);
		mesh.strMapNs = CA2T(m->map_Ns);
		mesh.strMapTr = CA2T(m->map_Tr);
		mesh.strMapDisp = CA2T(m->map_Disp);
		mesh.strMapBump = CA2T(m->map_Bump);
		mesh.strMapRefl = CA2T(m->map_Refl);
		*/ 
	}
  
	long nLastfaceIndex = 0;
	for(auto& i : objMesh.matGroups)
	{
		CMesh mesh;
		i.firstFace;
		i.numFaces;
		mesh.strMaterial = i.name;
		bool bFoundMat = false;
		for (auto& m : objMesh.materials)//could use a map here, but...
		{
			if (strcmp(m->name, i.name) == 0)
			{
				bFoundMat = true;
				for (int j = 0; j < 3; j++) {
					mesh.Ka[j] = m->Ka[j];
					mesh.Kd[j] = m->Kd[j];
					mesh.Ks[j] = m->Ks[j];
					mesh.Tf[j] = m->Tf[j];
				}
				mesh.Tr = m->Tr;
				mesh.Ns = m->Ns;
				mesh.Ni = m->Ni;
				mesh.illum = m->illum;
				mesh.strMapKa = CA2T(m->map_Ka);
				mesh.strMapKd = CA2T(m->map_Kd);
				mesh.strMapKs = CA2T(m->map_Ks);
				mesh.strMapNs = CA2T(m->map_Ns);
				mesh.strMapTr = CA2T(m->map_Tr);
				mesh.strMapDisp = CA2T(m->map_Disp);
				mesh.strMapBump = CA2T(m->map_Bump);
				mesh.strMapRefl = CA2T(m->map_Refl); 
				break;
			}
		}	
		if (!bFoundMat)
		{
			long nStop = 0;
			ATLASSERT(0);
		}

		int vertexSize = 2 * sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
		UINT bufferSize = i.numFaces * vertexSize * 3;
		BYTE* pVBData = new BYTE[bufferSize];
		int* pIndex = new int[i.numFaces * 3];
		BYTE* pVBData0 = pVBData;
		DirectX::XMFLOAT3* pPoints = new DirectX::XMFLOAT3[i.numFaces * 3];
		std::vector<DirectX::XMFLOAT3> vTest;
		for (int h = i.firstFace; h < i.firstFace + i.numFaces; h++)
		{
			VBVertex* pVBVertex = (VBVertex*)pVBData;
			DirectX::XMFLOAT3 pos1 = objMesh.vertices[objMesh.faceVertices[objMesh.faces[h].firstVertex]];
			DirectX::XMFLOAT3 normal1 = objMesh.normals[objMesh.faceNormals[objMesh.faces[h].firstNormal]];
			DirectX::XMFLOAT2 tex1 = objMesh.texCoords[objMesh.faceTexCoords[objMesh.faces[h].firstTexCoord]];
			if (h == i.firstFace) {
				mesh.bbmin = mesh.bbmax = pos1;
			}
			 
			pVBVertex->pos = pos1;
			pVBVertex->normal = normal1;
			pVBVertex->tex = tex1;
			pVBData += vertexSize;
			
			pVBVertex = (VBVertex*)pVBData;
			DirectX::XMFLOAT3 pos2 = objMesh.vertices[objMesh.faceVertices[objMesh.faces[h].firstVertex + 2]];
			DirectX::XMFLOAT3 normal2 = objMesh.normals[objMesh.faceNormals[objMesh.faces[h].firstNormal + 2]];
			DirectX::XMFLOAT2 tex2 = objMesh.texCoords[objMesh.faceTexCoords[objMesh.faces[h].firstTexCoord + 2]];
			pVBVertex->pos = pos2;
			pVBVertex->normal = normal2;
			pVBVertex->tex = tex2;
			pVBData += vertexSize;

			pVBVertex = (VBVertex*)pVBData;
			DirectX::XMFLOAT3 pos3 = objMesh.vertices[objMesh.faceVertices[objMesh.faces[h].firstVertex + 1]];
			DirectX::XMFLOAT3 normal3 = objMesh.normals[objMesh.faceNormals[objMesh.faces[h].firstNormal + 1]];
			DirectX::XMFLOAT2 tex3 = objMesh.texCoords[objMesh.faceTexCoords[objMesh.faces[h].firstTexCoord + 1]];
			pVBVertex->pos = pos3;
			pVBVertex->normal = normal3;
			pVBVertex->tex = tex3;
			 
			if (pos1.x < mesh.bbmin.x) mesh.bbmin.x = pos1.x; else if (pos1.x > mesh.bbmax.x) mesh.bbmax.x = pos1.x;
			if (pos1.y < mesh.bbmin.y) mesh.bbmin.y = pos1.y; else if (pos1.y > mesh.bbmax.y) mesh.bbmax.y = pos1.y;
			if (pos1.z < mesh.bbmin.z) mesh.bbmin.z = pos1.z; else if (pos1.z > mesh.bbmax.z) mesh.bbmax.z = pos1.z;

			if (pos2.x < mesh.bbmin.x) mesh.bbmin.x = pos2.x; else if (pos2.x > mesh.bbmax.x) mesh.bbmax.x = pos2.x;
			if (pos2.y < mesh.bbmin.y) mesh.bbmin.y = pos2.y; else if (pos2.y > mesh.bbmax.y) mesh.bbmax.y = pos2.y;
			if (pos2.z < mesh.bbmin.z) mesh.bbmin.z = pos2.z; else if (pos2.z > mesh.bbmax.z) mesh.bbmax.z = pos2.z;

			if (pos3.x < mesh.bbmin.x) mesh.bbmin.x = pos3.x; else if (pos3.x > mesh.bbmax.x) mesh.bbmax.x = pos3.x;
			if (pos3.y < mesh.bbmin.y) mesh.bbmin.y = pos3.y; else if (pos3.y > mesh.bbmax.y) mesh.bbmax.y = pos3.y;
			if (pos3.z < mesh.bbmin.z) mesh.bbmin.z = pos3.z; else if (pos3.z > mesh.bbmax.z) mesh.bbmax.z = pos3.z;

			pVBData += vertexSize;
			
			pIndex[3 * (h - i.firstFace) + 0] = 3 * (h - i.firstFace) + 0;
			pIndex[3 * (h - i.firstFace) + 1] = 3 * (h - i.firstFace) + 1;
			pIndex[3 * (h - i.firstFace) + 2] = 3 * (h - i.firstFace) + 2;

			VBVertex t1; t1.pos = pos1;	t1.normal = normal1; t1.tex = tex1;
			VBVertex t2; t2.pos = pos2;	t2.normal = normal2; t2.tex = tex2;
			VBVertex t3; t3.pos = pos3;	t3.normal = normal3; t3.tex = tex3;
			mesh.vTriangles.push_back( t1 );
			mesh.vTriangles.push_back( t2 );
			mesh.vTriangles.push_back( t3 );
			mesh.vTrianglesEx.push_back(XMLoadFloat3(&pos1));
			mesh.vTrianglesEx.push_back(XMLoadFloat3(&pos2));
			mesh.vTrianglesEx.push_back(XMLoadFloat3(&pos3));

			pPoints[3 * (h - i.firstFace) + 0] = pos1;
			pPoints[3 * (h - i.firstFace) + 1] = pos2;
			pPoints[3 * (h - i.firstFace) + 2] = pos3;

			vTest.push_back(pos1);
			vTest.push_back(pos2);
			vTest.push_back(pos3);
		}

		mesh.nNumTriangles = i.numFaces;
		bool bFoundmat = false;
		for (auto mt : gGlobalData->m_vMaterials) {
			if (mt.strName == mesh.strMaterial) {
				mesh.m_pData = mt.m_pData;
				mesh.nWidth = mt.nWidth;
				mesh.nHeight = mt.nHeight;
				bFoundmat = true;
				break;
			}
		}
		if (!bFoundmat)
		{
			long nStop = 0;
		}
		{
			mesh.boundingbox.Center.x = (mesh.bbmin.x + mesh.bbmax.x) / 2;
			mesh.boundingbox.Center.y = (mesh.bbmin.y + mesh.bbmax.y) / 2;
			mesh.boundingbox.Center.z = (mesh.bbmin.z + mesh.bbmax.z) / 2;

			mesh.boundingbox.Extents.x = (mesh.bbmax.x - mesh.bbmin.x) / 2;
			mesh.boundingbox.Extents.y = (mesh.bbmax.y - mesh.bbmin.y) / 2;
			mesh.boundingbox.Extents.z = (mesh.bbmax.z - mesh.bbmin.z) / 2;
		}
		if (mesh.nNumTriangles >= 200)
		{
			long nDepth = 0;
			mesh.ComputeSubmeshes(nDepth);
		}
		//ComputeBoundingAxisAlignedBoxFromPoints(&mesh.boundingbox, i.numFaces * 3, (DirectX::XMFLOAT3*)pVBData0, sizeof(Vertex::Basic32));
		delete[] pPoints;
		pPoints = 0;

		mesh.CreateBuffers(m_pD3DDevice, i.numFaces, (BYTE*)pVBData0, pIndex);

	 

		gGlobalData->m_vMeshes.push_back(mesh);

		delete[] pVBData0;
		delete[] pIndex;
	}


	sort(gGlobalData->m_vMeshes.begin(), gGlobalData->m_vMeshes.end(), [](const CMesh& a, const CMesh& b) -> bool
		{
			return a.nNumTriangles > b.nNumTriangles;
		});

	long nTotalInternalTris = 0;
	long nTotalTris = 0;
	long nMaxDepth = 0;
	for (auto& i : gGlobalData->m_vMeshes)
	{
		long nDepth = 0;
		i.Count(nTotalInternalTris, nDepth);
		if (nDepth > nMaxDepth) {
			nMaxDepth = nDepth;
		}
		i.nMaxDepth = nDepth;
	}
	for (auto& i : gGlobalData->m_vMeshes)
	{
		//ATLTRACE2(L" NumTris = %d\n", i.nNumTriangles);
		nTotalTris = nTotalTris + i.nNumTriangles;
	}
	return true;
} 
//-------------------------------------------------------------------------------------//
bool CMainFrame::DoesDirExist(CString str)
{
	DWORD dwRetVal = GetFileAttributes(str);
	if (dwRetVal == 0xFFFFFFFF)
	{
		return false;
	}
	else if (dwRetVal & FILE_ATTRIBUTE_DIRECTORY)
	{
		return true;
	}
	return false;
}
//-------------------------------------------------------------//
bool CMainFrame::InitD2D()
{
	//initialize Direct2D and DirectWrite
	if (!m_pD2DFactory)
	{
		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
		if (FAILED(hr))
		{
			ATLASSERT(0);
		}
	}
	if (!m_pWriteFactory)
	{
		float nTextHeight = 18;

		HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pWriteFactory), reinterpret_cast<IUnknown**>(&m_pWriteFactory));

		DWRITE_FONT_WEIGHT w = DWRITE_FONT_WEIGHT_NORMAL;
		DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;
		hr = m_pWriteFactory->CreateTextFormat(L"Arial",
			NULL,
			w,
			style,
			DWRITE_FONT_STRETCH_NORMAL,
			nTextHeight,
			L"",
			&m_pTextFormat
		);

	}

	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	return SUCCEEDED(hr);
}
//-------------------------------------------------------------//
// CMainFrame message handlers
bool CMainFrame::InitD3D()
{
	//initialize DirectX 11
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)  
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(
		0,                 // default adapter
		m_D3DDriverType,
		0,                 // no software device
		createDeviceFlags,
		0, 0,              // default feature level array
		D3D11_SDK_VERSION,
		&m_pD3DDevice,
		&featureLevel,
		&m_pD3DImmediateContext);


	if (FAILED(hr))
	{
		AfxMessageBox(L"D3D11CreateDevice Failed.");
		return false;
	}

	if (featureLevel != D3D_FEATURE_LEVEL_11_0)
	{
		AfxMessageBox(L"Direct3D Feature Level 11 unsupported.");
		return false;
	}

	hr = m_pD3DDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m_4xMsaaQuality);
	ATLASSERT(m_4xMsaaQuality > 0);

	// Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.
	CRect quadrantRect;
	GetClientRect(&quadrantRect);
	float w = (float)quadrantRect.Width() / 2;
	float h = (float)quadrantRect.Height() / 2;

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = w;
	sd.BufferDesc.Height = h;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Use 4X MSAA? 
	if (m_Enable4xMsaa)
	{
		sd.SampleDesc.Count = 4;
		sd.SampleDesc.Quality = m_4xMsaaQuality - 1;
	}
	// No MSAA
	else
	{
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
	}

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = GetSafeHwnd();
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;
	 
	m_pView->Init(m_pTextFormat, m_pD2DFactory, sd, m_Enable4xMsaa, m_4xMsaaQuality, m_pD3DDevice, m_pD3DImmediateContext);

	DragAcceptFiles(TRUE);
	return true;
}
//-------------------------------------------------------------//
CRayTracerView* CMainFrame::GetRightPane()
{
	return m_pView;
}
//-------------------------------------------------------------//
void CMainFrame::Render()
{
	CRayTracerApp* pWinApp = (CRayTracerApp*)AfxGetApp();
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;
	frameCnt++;

	////////////////////////////////////////////////////////////////////////
	// Compute averages over one second period.
	if ((pWinApp->m_Timer.TotalTime() - timeElapsed) >= 1.0f)
	{
		m_fps = (float)frameCnt; // fps = frameCnt / 1
		m_mspf = 1000.0f / m_fps;
		 
		frameCnt = 0;
		timeElapsed += 1.0f;
	}

	float nTimer = pWinApp->m_Timer.DeltaTime();
	if (m_pView) m_pView->UpdateScene(nTimer);
	if (m_pView) m_pView->Render(m_fps, m_mspf);
}