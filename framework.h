#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcview.h>


#include <afxdisp.h>        // MFC Automation classes



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars




#include <math.h>   // smallpt, a Path Tracer by Kevin Beason, 2008 
#include <stdlib.h> // Make : g++ -O3 -fopenmp smallpt.cpp -o smallpt 
#include <stdio.h>  //        Remove "-fopenmp" for g++ version < 4.2 

#include <random>
#include <iostream>




#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h> 
#include "DirectxMath.h"
#include <DirectXPackedVector.h>
 

#include "..\MRIServer\MRIServer_i.h"

//#include "ObjLoader.h"
#include "xnacollision.h"

#include <string>
#include <vector>
#include <map>


const UINT WM_PROGRESS_UPDATE = WM_USER + 1000;
const UINT WM_RENDER_START = WM_USER + 1001;
const UINT WM_RENDER_END = WM_USER + 1002;
//---------------------------------------------------------------//
struct VBVertex
{
	DirectX::XMFLOAT3 pos, normal;
	DirectX::XMFLOAT2 tex;
};
//---------------------------------------------------------------//
struct CMat
{
	CString strName;
	CString strPath;
	DWORD* m_pData = 0;//pointer to RGBA bytes
	LONG nWidth = 0;
	LONG nHeight = 0;
	ID3D11ShaderResourceView* mDiffuseMapSRV = 0;
}; 
//-------------------------------------------------------------------------------//
struct CMesh
{
	//for sun
	bool bLight = false;
	float nIntensity = 1.0f; 

	long nMaxDepth = 0;//number of octree submeshes
	void ComputeSubmeshes(long& nDepth);
	void RecomputeBB();
	void Count(long& nTris, long& nMaxDepth);
	void Clear();
	 
	void CreateBuffers(ID3D11Device* pD3DDevice, long nNumFaces, BYTE* pVBData0, int* pIndex);
	CMesh* m_pMesh = 0;
	std::vector<DirectX::XMVECTOR> vTrianglesEx;
	std::vector<VBVertex> vTriangles;
	long nNumTriangles = 0;

 
	bool DEBUGIntersectRayAxisAlignedBox(DirectX::XMVECTOR& rayOrigin, DirectX::XMVECTOR& rayDir, DirectX::XMFLOAT3& bbmax, DirectX::XMFLOAT3& bbmin);
	bool DEBUGIntersectRayTriangle(DirectX::XMVECTOR rayOrigin, DirectX::XMVECTOR rayDir, DirectX::XMVECTOR v0, DirectX::XMVECTOR v1, DirectX::XMVECTOR v2, float* pDist, float* pU, float* pV);

	bool Intersect(DirectX::XMVECTOR& origin, DirectX::XMVECTOR& dir, float& u, float& v, float& dist, DirectX::XMFLOAT3& hitpoint, DirectX::XMFLOAT3& nml);
	
	void AddTriangle(VBVertex& v1, VBVertex& v2, VBVertex& v3);
	//std::vector<VBVertex> vTriangles;
	ID3D11Buffer* pVB = 0;
	ID3D11Buffer* pIB = 0;
	//texture
	DWORD* m_pData = 0;//pointer to RGBA bytes
	LONG nWidth = 0;
	LONG nHeight = 0;

	DirectX::XMFLOAT3 bbmin, bbmax; // bounding box.
	XNA::AxisAlignedBox boundingbox;
	 
 
	CString strMaterial;
	float Ka[3] = { 0,0,0 };	// Ambient.
	float Kd[3] = { 0,0,0 };	// Diffuse (color).
	float Ks[3] = { 0,0,0 };	// Specular color.
	float Tf[3] = { 0,0,0 };	// Transmission filter.
	float Tr = 0;		// Transparency (also called d).
	float Ns = 0;		// Specular power (shininess).
	float Ni = 0;		// Index of refraction (optical density).
	int illum = 0;		// Illumination model (See remark at bottom of file).
	CString strMapKa; // Ambient texture file name.
	CString strMapKd; // Diffuse texture file name.
	CString strMapKs; // Specular color texture file name.
	CString strMapNs; // Specular power texture file name.
	CString strMapTr; // Transparency texture file name.
	CString strMapDisp; // Displacement map.
	CString strMapBump; // Bump map.
	CString strMapRefl; // Reflection map.
};
//-------------------------------------------------------------------------------//
struct CGlobalData
{
	~CGlobalData()
	{

	}
	std::vector<CMesh> m_vMeshes;
	std::vector<CMat> m_vMaterials;	 
	HWND m_hSideWnd = 0;//send progressm messages to this window
};
//-------------------------------------------------------------------------------//
extern CGlobalData* gGlobalData;
//-------------------------------------------------------------------------------//



#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


