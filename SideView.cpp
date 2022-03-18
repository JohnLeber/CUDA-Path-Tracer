// SideView.cpp : implementation file
//

#include "pch.h"
#include "RayTracer.h"
#include "SideView.h"
#include "RayTracerView.h"
#include "MainFrm.h"
#include <omp.h>
Sphere spheres[] = {//Scene: radius, position, emission, color, material 
      Sphere(1e5, Vec(1e5 + 1,40.8,81.6), Vec(),Vec(.75,.25,.25),DIFF),//Left 
      Sphere(1e5, Vec(-1e5 + 99,40.8,81.6),Vec(),Vec(.25,.25,.75),DIFF),//Rght 
      Sphere(1e5, Vec(50,40.8, 1e5),     Vec(),Vec(.75,.75,.75),DIFF),//Back 
      Sphere(1e5, Vec(50,40.8,-1e5 + 170), Vec(),Vec(),           DIFF),//Frnt 
      Sphere(1e5, Vec(50, 1e5, 81.6),    Vec(),Vec(.75,.75,.75),DIFF),//Botm 
      Sphere(1e5, Vec(50,-1e5 + 81.6,81.6),Vec(),Vec(.75,.75,.75),DIFF),//Top 
      Sphere(16.5,Vec(27,16.5,47),       Vec(),Vec(1,1,1) * .999, SPEC),//Mirr 
      Sphere(16.5,Vec(73,16.5,78),       Vec(),Vec(1,1,1) * .999, REFR),//Glas 
      Sphere(600, Vec(50,681.6 - .27,81.6),Vec(12,12,12),  Vec(), DIFF) //Lite 
};
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
// CSideView message handlers
inline bool intersect(const Ray& r, double& t, int& id) {
    double n = sizeof(spheres) / sizeof(Sphere), d, inf = t = 1e20;
    for (int i = int(n); i--;) if ((d = spheres[i].intersect(r)) && d < t) { t = d; id = i; }
    return t < inf;
}
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
#define M_PI 3.1415926535897932384626433832795// *** Added for VS2012
double erand48(unsigned short seed[3]) { 
   return (double)rand() / (double)RAND_MAX;// *** Added for VS2012
}
//-----------------------------------------------------------------------//
Vec CSideView::radiance(const Ray& r, int depth, unsigned short* Xi) {
    double t;                               // distance to intersection 
    int id = 0;                               // id of intersected object 
    if (!intersect(r, t, id)) return Vec(); // if miss, return black 
   
    const Sphere& obj = spheres[id];        // the hit object 
    Vec x = r.o + r.d * t;
    Vec n = (x - obj.p).norm();
    Vec nl = n.dot(r.d) < 0 ? n : n * -1;
    Vec f = obj.c;// Vec p, e, c;      // position, emission, color 
    double p = f.x > f.y && f.x > f.z ? f.x : f.y > f.z ? f.y : f.z; // max refl 
    if (++depth > 5) {
        if (erand48(Xi) < p) {
            f = f * (1 / p);
        }
        else return obj.e; //R.R. 
    }
    if (depth > 6) return obj.e; //Bound to avoid stack overflow at -O0.
    if (obj.refl == DIFF) {                  // Ideal DIFFUSE reflection 
        double r1 = 2 * M_PI * erand48(Xi), r2 = erand48(Xi), r2s = sqrt(r2);
        Vec w = nl, u = ((fabs(w.x) > .1 ? Vec(0, 1) : Vec(1)) % w).norm(), v = w % u;
        Vec d = (u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1 - r2)).norm();
        return obj.e + f.mult(radiance(Ray(x, d), depth, Xi));
    }
    else if (obj.refl == SPEC)            // Ideal SPECULAR reflection 
        return obj.e + f.mult(radiance(Ray(x, r.d - n * 2 * n.dot(r.d)), depth, Xi));
    Ray reflRay(x, r.d - n * 2 * n.dot(r.d));     // Ideal dielectric REFRACTION 
    bool into = n.dot(nl) > 0;                // Ray from outside going in? 
    double nc = 1, nt = 1.5, nnt = into ? nc / nt : nt / nc, ddn = r.d.dot(nl), cos2t;
    if ((cos2t = 1 - nnt * nnt * (1 - ddn * ddn)) < 0)    // Total internal reflection 
        return obj.e + f.mult(radiance(reflRay, depth, Xi));
    Vec tdir = (r.d * nnt - n * ((into ? 1 : -1) * (ddn * nnt + sqrt(cos2t)))).norm();
    double a = nt - nc, b = nt + nc, R0 = a * a / (b * b), c = 1 - (into ? -ddn : tdir.dot(n));
    double Re = R0 + (1 - R0) * c * c * c * c * c, Tr = 1 - Re, P = .25 + .5 * Re, RP = Re / P, TP = Tr / (1 - P);
    return obj.e + f.mult(depth > 2 ? (erand48(Xi) < P ?   // Russian roulette 
        radiance(reflRay, depth, Xi) * RP : radiance(Ray(x, tdir), depth, Xi) * TP) :
        radiance(reflRay, depth, Xi) * Re + radiance(Ray(x, tdir), depth, Xi) * Tr);
}
//-----------------------------------------------------------------------//
void CSideView::OnBnClickedRender()
{ 
    int w = 1024, h = 768, samps = 40;// argc == 2 ? atoi(argv[1]) / 4 : 1; // # samples 
    Ray cam(Vec(50, 52, 295.6), Vec(0, -0.042612, -1).norm()); // cam pos, dir
    Vec cx = Vec(w * 0.5135f / h), cy = (cx % cam.d).norm() * 0.5135f, r, * c = new Vec[w * h];

  //  omp_set_num_threads(16);
//#pragma omp parallel for schedule(dynamic, 1) private(r)       // OpenMP
    for (int y = 0; y < h; y++) {                       // Loop over image rows
      // *** Commented out for Visual Studio, fprintf is not thread-safe
      //fprintf(stderr,"\rRendering (%d spp) %5.2f%%",samps*4,100.*y/(h-1));
        unsigned short Xi[3] = { 0 ,0, y * y * y }; // *** Moved outside for VS2012
        for (unsigned short x = 0; x < w; x++)   // Loop cols
        {
            if (0)
            {
                for (int sy = 0; sy < 2; sy++)     // 2x2 subpixel rows
                {
                    int i = (h - y - 1) * w + x;
                    for (int sx = 0; sx < 2; sx++)
                    {
                        r = Vec();
                        // 2x2 subpixel cols
                        for (int s = 0; s < samps; s++)
                        {
                            double r1 = 2 * erand48(Xi);
                            double dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                            double r2 = 2 * erand48(Xi);
                            double dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);

                            Vec d = cx * (((sx + 0.5f + dx) / 2 + x) / w - 0.5f) + cy * (((sy + 0.5f + dy) / 2 + y) / h - 0.5f) + cam.d;
                            r = r + radiance(Ray(cam.o + d * 140.0f, d.norm()), 0, Xi) * (1.0f / samps);
                        } // Camera rays are pushed ^^^^^ forward to start in interior
                        c[i] = c[i] + Vec(clamp(r.x), clamp(r.y), clamp(r.z));
                    }
                }
            }
            else
            {
                int i = (h - y - 1) * w + x;
                r = Vec();
                for (int s = 0; s < samps; s++)
                {
                    double r1 = 2 * erand48(Xi);
                    double dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                    double r2 = 2 * erand48(Xi);
                    double dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);
                    Vec d = cx * (((0.5f + dx) / 2 + x) / w - 0.5f) + cy * (((0.5f + dy) / 2 + y) / h - 0.5f) + cam.d;
                    r = r + radiance(Ray(cam.o + d * 140.0f, d.norm()), 0, Xi) * (1.0f / samps);
                } // Camera rays are pushed ^^^^^ forward to start in interior
                c[i] = c[i] + Vec(clamp(r.x), clamp(r.y), clamp(r.z)) * 0.25f;
            } 
        }
    } 
    
  /*  for (int i = 0; i < w * h; i++)
        fprintf(f, "%d %d %d ", toInt(c[i].x), toInt(c[i].y), toInt(c[i].z));*/

    CImage* pImage = new CImage();
    pImage->Create(w, h, 24, 0);
    BYTE* pArray = (BYTE*)pImage->GetBits();

   
    int nPitch = pImage->GetPitch();
    int nBitCount = pImage->GetBPP() / 8;
    int height = h;
    int width = w; 
    for (int k = 0; k < width; k++) {
        for (int j = 0; j < height; j++) { 
            *(pArray + nPitch * j + k * nBitCount + 0) = toInt(c[j * width + k].x);
            *(pArray + nPitch * j + k * nBitCount + 1) = toInt(c[j * width + k].y);
            *(pArray + nPitch * j + k * nBitCount + 2) = toInt(c[j * width + k].z);
        }
    }
    pImage->Save(L"Image.jpg");
    delete pImage; 
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
