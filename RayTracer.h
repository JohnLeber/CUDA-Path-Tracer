
// RayTracer.h : main header file for the RayTracer application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "GameTimer.h"

// CRayTracerApp:
// See RayTracer.cpp for the implementation of this class
//

class CRayTracerApp : public CWinApp
{
public:
	CRayTracerApp() noexcept;

	GameTimer m_Timer;
// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	BOOL OnIdle(LONG lCount);
// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CRayTracerApp theApp;
