#pragma once

struct Vec {        // Usage: time ./smallpt 5000 && xv image.ppm 
    double x, y, z;                  // position, also color (r,g,b) 
    Vec(double x_ = 0, double y_ = 0, double z_ = 0) { x = x_; y = y_; z = z_; }
    Vec operator+(const Vec& b) const { return Vec(x + b.x, y + b.y, z + b.z); }
    Vec operator-(const Vec& b) const { return Vec(x - b.x, y - b.y, z - b.z); }
    Vec operator*(double b) const { return Vec(x * b, y * b, z * b); }
    Vec mult(const Vec& b) const { return Vec(x * b.x, y * b.y, z * b.z); }
    Vec& norm() { return *this = *this * (1 / sqrt(x * x + y * y + z * z)); }
    double dot(const Vec& b) const { return x * b.x + y * b.y + z * b.z; } // cross: 
    Vec operator%(Vec& b) { return Vec(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x); }
};
struct Ray 
{ 
    Ray(Vec o_, Vec d_)
    {
        o = o_;//origin
        d = d_;//direction
    }
    Vec o;
    Vec d;     
};
enum Refl_t { DIFF, SPEC, REFR };  // material types, used in radiance() 
struct Sphere {
    double rad;       // radius 
    Vec p, e, c;      // position, emission, color 
    Refl_t refl;      // reflection type (DIFFuse, SPECular, REFRactive) 
    Sphere(double rad_, Vec p_, Vec e_, Vec c_, Refl_t refl_) :
        rad(rad_), p(p_), e(e_), c(c_), refl(refl_) {}
    double intersect(const Ray& r) const { // returns distance, 0 if nohit 
        Vec op = p - r.o; // Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0 
        double t, eps = 1e-4, b = op.dot(r.d), det = b * b - op.dot(op) + rad * rad;
        if (det < 0) return 0; else det = sqrt(det);
        return (t = b - det) > eps ? t : ((t = b + det) > eps ? t : 0);
    }
};

class CRayTracerDoc;
// CSideView form view

class CSideView : public CFormView
{
	DECLARE_DYNCREATE(CSideView)
    CProgressCtrl m_Progress;
protected:
	CSideView();           // protected constructor used by dynamic creation
	virtual ~CSideView();
    CSliderCtrl m_SunPos;
public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SIDEVIEW };
#endif
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
 
    Vec radiance(const Ray& r, int depth, unsigned short* Xi); 
    inline double clamp(double x) { return x < 0 ? 0 : x>1 ? 1 : x; }
    inline int toInt(double x) { return int(pow(clamp(x), 1 / 2.2) * 255 + .5); }
    bool DoPump();
    LRESULT OnProgressUpdate(WPARAM wp, LPARAM lp);
    LRESULT OnRenderStart(WPARAM wp, LPARAM lp);
    LRESULT OnRenderEnd(WPARAM wp, LPARAM lp);
	DECLARE_MESSAGE_MAP()
public:
	virtual void OnInitialUpdate();
    void SetSunPos(DirectX::XMFLOAT3& pos);
	afx_msg void OnBnClickedRender();
    afx_msg void OnBnClickedCalcRay(); 
    afx_msg void OnBnClickedToggleMesh();
    afx_msg void OnNMCustomdrawSlider1(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnBnClickedWireframe();
    afx_msg void OnBnClickedGi();
};


