
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
class CRayTracerView;

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame() noexcept;
	DECLARE_DYNCREATE(CMainFrame)
	inline double erand48(unsigned short seed[3]) {
		return (double)rand() / (double)RAND_MAX;// *** Added for VS2012
	}

	bool DoesDirExist(CString str);

	//dxgi
	IDXGIFactory* m_pDXGIFactory = 0;

	//d3d
	ID3D11Debug* m_pDebug = 0;  
	ID3D11Device* m_pD3DDevice = 0;
	ID3D11DeviceContext* m_pD3DImmediateContext = 0;
	ID3D11Texture2D* m_pDepthStencilBuffer = 0;
	ID3D11RenderTargetView* m_pRenderTargetView = 0;
	ID3D11DepthStencilView* m_pDepthStencilView = 0;
	//d2d
	ID2D1Factory* m_pD2DFactory = 0;
	IDWriteFactory* m_pWriteFactory = 0;
	IDWriteTextFormat* m_pTextFormat = 0;
	D3D11_VIEWPORT m_ScreenViewport;
	UINT      m_4xMsaaQuality = 0;
	void Render();
	// Derived class should set these in derived constructor to customize starting values.
	std::wstring m_MainWndCaption;
	bool m_Enable4xMsaa = false;
	void OnSize(UINT nType, int cx, int cy);
	D3D_DRIVER_TYPE m_D3DDriverType = D3D_DRIVER_TYPE_HARDWARE;
	CRayTracerView* m_pView = 0;
	bool Initialize();
	bool InitD2D();
	bool InitD3D();

// Attributes
protected:
	CSplitterWnd m_wndSplitter;
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	float m_fps = 0;
	float m_mspf = 0;

// Implementation
public:
	virtual ~CMainFrame();
	CRayTracerView* GetRightPane();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CToolBar          m_wndToolBar;
	CStatusBar        m_wndStatusBar;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct); 
	DECLARE_MESSAGE_MAP()

};


