// Chizish qatlami: Direct2D + DirectWrite + WIC.
// Sayt CSS'idagi ranglar (uslub.css :root va body.kunduzgi) aynan shu yerga ko'chirilgan.
#pragma once
#include <windows.h>
#include <d2d1_3.h>
#include <dwrite.h>
#include <wincodec.h>

#include <map>
#include <string>

#include "util.h"

struct Rang {
    float r = 0, g = 0, b = 0, a = 0;
    D2D1_COLOR_F d2d() const { return {r, g, b, a}; }
    bool bormi() const { return a > 0; }
    Rang shaffof(float k) const { return {r, g, b, a * k}; }
    COLORREF gdi() const { return RGB(int(r * 255 + .5f), int(g * 255 + .5f), int(b * 255 + .5f)); }
    string hex() const;
};
Rang hex(const char* s, float a = 1);  // "#468ff3"

// CSS o'zgaruvchilari
struct Mavzu {
    bool kechki;
    Rang fon, panel, panel2, panel3, chegara, chegara2;
    Rang matn, matn2, matn3;
    Rang urgu, urgu_quyuq, urgu_fon, urgu_chek, urgu_matn;
    Rang ogoh, ogoh_fon, ogoh_chek;
    Rang tasdiq, tasdiq_quyuq, tasdiq_fon, tasdiq_chek;
    Rang xato, xato_fon, xato_chek;
    Rang kok, kok_fon, kok_chek;
    Rang asosiy_matn, tasdiq_matn;
    Rang tugma_hover, tugma_hover_chegara;  // .tugma:hover
};
extern Mavzu KECHKI, KUNDUZGI;
extern const Mavzu* M;  // joriy mavzu

struct R {
    float x = 0, y = 0, w = 0, h = 0;
    float o() const { return x + w; }
    float p() const { return y + h; }
    R ichki(float d) const { return {x + d, y + d, w - 2 * d, h - 2 * d}; }
    R ichki(float dx, float dy) const { return {x + dx, y + dy, w - 2 * dx, h - 2 * dy}; }
    bool ichida(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
    D2D1_RECT_F d2d() const { return {x, y, x + w, y + h}; }
};

enum Tekis { CHAP = 0, MARKAZ = 1, ONG = 2 };

class Gfx {
public:
    bool boshla(HWND h);
    void olcham(UINT w, UINT h);
    bool chizish_boshla();  // false — qurilma yo'qoldi, qayta yaratish kerak
    void chizish_tugat();
    float masshtab() const { return ms_; }  // 1 CSS px necha fizik piksel

    // Shakllar
    void quti(R r, Rang fon, float radius = 0, Rang chegara = {}, float qalinlik = 1);
    void gradient_quti(R r, Rang a, Rang b, float radius, bool diagonal = true);
    void chiziq(float x1, float y1, float x2, float y2, Rang r, float q = 1);
    void doira(float cx, float cy, float rad, Rang fon);

    // Matn (o'lcham CSS px, og'irlik 400/500/600/700)
    void matn(R r, const wstring& s, float olcham, int ogirlik, Rang rang, Tekis gor = CHAP,
              int vert = 1, bool kop_qator = false);
    float matn_eni(const wstring& s, float olcham, int ogirlik);
    float matn_boyi(const wstring& s, float olcham, int ogirlik, float en);

    // SVG ikonka (saytdagi ikonlar.html dan) va rasmlar
    void ikon(const string& nom, R r, Rang rang, float qalinlik = 0);
    void svg_rasm(const string& kalit, const string& svg, R r);
    void rasm(const string& kalit, int resurs, R r, bool cover = true);

    // Kesish va siljitish
    void kes(R r);
    void kesishni_ol();
    void siljit(float dx, float dy);
    float dx = 0, dy = 0;

    ID2D1DeviceContext5* dc5() { return dc5_; }

private:
    HWND h_ = nullptr;
    float ms_ = 1;
    ID2D1Factory* fabrika_ = nullptr;
    ID2D1HwndRenderTarget* rt_ = nullptr;
    ID2D1DeviceContext5* dc5_ = nullptr;
    ID2D1SolidColorBrush* mo_ = nullptr;
    IDWriteFactory* dw_ = nullptr;
    IWICImagingFactory* wic_ = nullptr;
    std::map<int, IDWriteTextFormat*> formatlar_;
    std::map<string, ID2D1SvgDocument*> svglar_;
    std::map<string, ID2D1Bitmap*> rasmlar_;
    std::map<string, string> ikon_manba_;
    IDWriteTextFormat* format(float olcham, int ogirlik);
    void qurilma();
    void qurilmani_tashla();
    ID2D1SvgDocument* svg_hujjat(const string& kalit, const string& matn, float w, float h);
    void ikonlarni_yukla();
};

extern Gfx gfx;
string resurs_matni(int id);
