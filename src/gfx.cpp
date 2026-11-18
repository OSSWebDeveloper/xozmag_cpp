#include "gfx.h"

#include <shlwapi.h>

#include <cstdio>

Gfx gfx;

Rang hex(const char* s, float a) {
    unsigned v = 0;
    sscanf(s + 1, "%x", &v);
    return {((v >> 16) & 255) / 255.f, ((v >> 8) & 255) / 255.f, (v & 255) / 255.f, a};
}

string Rang::hex() const {
    char s[8];
    snprintf(s, 8, "#%02x%02x%02x", int(r * 255 + .5f), int(g * 255 + .5f), int(b * 255 + .5f));
    return s;
}

// ---- uslub.css :root (kechki) ----
Mavzu KECHKI = {
    true,
    hex("#232323"), hex("#2d2d2d"), hex("#404040"), hex("#4d4d4d"), hex("#404040"), hex("#595959"),
    hex("#ffffff"), hex("#d7d7d7"), hex("#adadad"),
    hex("#468ff3"), hex("#2f7ae0"), hex("#468ff3", .15f), hex("#468ff3", .4f), hex("#8fbdf8"),
    hex("#ffb020"), hex("#ffb020", .12f), hex("#ffb020", .35f),
    hex("#5ad39e"), hex("#45b986"), hex("#5ad39e", .14f), hex("#5ad39e", .35f),
    hex("#f87171"), hex("#f87171", .14f), hex("#f87171", .35f),
    hex("#57c2f5"), hex("#1d99dd", .16f), hex("#1d99dd", .35f),
    hex("#ffffff"), hex("#0a2617"),
    hex("#4d4d4d"), hex("#6e6e6e"),
};

// ---- body.kunduzgi ----
Mavzu KUNDUZGI = {
    false,
    hex("#f2f4f7"), hex("#ffffff"), hex("#f8fafc"), hex("#e2ecf7"), hex("#d5dbe3"), hex("#b6c0cd"),
    hex("#202020"), hex("#60656b"), hex("#6c727b"),
    hex("#0068e0"), hex("#0153b2"), hex("#0068e0", .1f), hex("#0068e0", .35f), hex("#0068e0"),
    hex("#8a5a00"), hex("#e29100", .15f), hex("#e29100", .45f),
    hex("#0b7d45"), hex("#096637"), hex("#0b7d45", .12f), hex("#0b7d45", .35f),
    hex("#d93025"), hex("#d93025", .1f), hex("#d93025", .35f),
    hex("#006397"), hex("#0074b7", .12f), hex("#006397", .35f),
    hex("#ffffff"), hex("#ffffff"),
    hex("#e2ecf7"), hex("#b6c0cd"),
};

const Mavzu* M = &KECHKI;

string resurs_matni(int id) {
    HRSRC r = FindResourceW(nullptr, MAKEINTRESOURCEW(id), (LPCWSTR)RT_RCDATA);
    if (!r) return {};
    HGLOBAL g = LoadResource(nullptr, r);
    return string((const char*)LockResource(g), SizeofResource(nullptr, r));
}

template <class T> static void tashla(T*& p) {
    if (p) p->Release();
    p = nullptr;
}

bool Gfx::boshla(HWND h) {
    h_ = h;
    HDC dc = GetDC(nullptr);
    ms_ = GetDeviceCaps(dc, LOGPIXELSY) / 96.f * 0.9f;  // CSS: zoom .9
    ReleaseDC(nullptr, dc);
    // ID2D1Factory6 — SVG (DeviceContext5) uchun; bo'lmasa oddiy fabrika
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory6),
                                 nullptr, (void**)&fabrika_)) &&
        FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory),
                                 nullptr, (void**)&fabrika_)))
        return false;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                   (IUnknown**)&dw_)))
        return false;
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                     IID_IWICImagingFactory, (void**)&wic_);
    ikonlarni_yukla();
    return true;
}

void Gfx::qurilma() {
    if (rt_) return;
    RECT rc;
    GetClientRect(h_, &rc);
    D2D1_RENDER_TARGET_PROPERTIES p = D2D1::RenderTargetProperties();
    p.dpiX = p.dpiY = 96.f * ms_;
    fabrika_->CreateHwndRenderTarget(
        p, D2D1::HwndRenderTargetProperties(h_, D2D1::SizeU(rc.right, rc.bottom)), &rt_);
    if (!rt_) return;
    rt_->SetDpi(96.f * ms_, 96.f * ms_);
    rt_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    rt_->CreateSolidColorBrush(D2D1::ColorF(0), &mo_);
    rt_->QueryInterface(__uuidof(ID2D1DeviceContext5), (void**)&dc5_);
}

void Gfx::qurilmani_tashla() {
    for (auto& [k, v] : svglar_) v->Release();
    svglar_.clear();
    for (auto& [k, v] : rasmlar_) if (v) v->Release();
    rasmlar_.clear();
    tashla(mo_);
    tashla(dc5_);
    tashla(rt_);
}

void Gfx::olcham(UINT w, UINT h) {
    if (rt_) rt_->Resize(D2D1::SizeU(w, h));
}

bool Gfx::chizish_boshla() {
    qurilma();
    if (!rt_) return false;
    rt_->BeginDraw();
    rt_->SetTransform(D2D1::Matrix3x2F::Identity());
    dx = dy = 0;
    return true;
}

void Gfx::chizish_tugat() {
    if (rt_ && rt_->EndDraw() == (HRESULT)D2DERR_RECREATE_TARGET) qurilmani_tashla();
}

// ---------- Shakllar ----------

void Gfx::quti(R r, Rang fon, float radius, Rang chegara, float q) {
    if (!rt_) return;
    D2D1_ROUNDED_RECT rr = {r.d2d(), radius, radius};
    if (fon.bormi()) {
        mo_->SetColor(fon.d2d());
        if (radius > 0) rt_->FillRoundedRectangle(rr, mo_);
        else rt_->FillRectangle(r.d2d(), mo_);
    }
    if (chegara.bormi()) {
        mo_->SetColor(chegara.d2d());
        R i = r.ichki(q / 2);
        D2D1_ROUNDED_RECT ri = {i.d2d(), radius - q / 2, radius - q / 2};
        if (radius > 0) rt_->DrawRoundedRectangle(ri, mo_, q);
        else rt_->DrawRectangle(i.d2d(), mo_, q);
    }
}

void Gfx::gradient_quti(R r, Rang a, Rang b, float radius, bool diagonal) {
    if (!rt_) return;
    D2D1_GRADIENT_STOP st[2] = {{0, a.d2d()}, {1, b.d2d()}};
    ID2D1GradientStopCollection* c = nullptr;
    rt_->CreateGradientStopCollection(st, 2, &c);
    ID2D1LinearGradientBrush* g = nullptr;
    // linear-gradient(150deg, ...) — yuqoridan pastga, biroz chapdan o'ngga
    D2D1_POINT_2F p1 = {r.x + (diagonal ? r.w * .25f : 0), r.y};
    D2D1_POINT_2F p2 = {r.x + (diagonal ? r.w * .75f : 0), r.p()};
    rt_->CreateLinearGradientBrush({p1, p2}, c, &g);
    if (g) rt_->FillRoundedRectangle({r.d2d(), radius, radius}, g);
    tashla(g);
    tashla(c);
}

void Gfx::chiziq(float x1, float y1, float x2, float y2, Rang r, float q) {
    if (!rt_) return;
    mo_->SetColor(r.d2d());
    rt_->DrawLine({x1, y1}, {x2, y2}, mo_, q);
}

void Gfx::doira(float cx, float cy, float rad, Rang fon) {
    if (!rt_) return;
    mo_->SetColor(fon.d2d());
    rt_->FillEllipse({{cx, cy}, rad, rad}, mo_);
}

// ---------- Matn ----------

IDWriteTextFormat* Gfx::format(float olcham, int ogirlik) {
    int kalit = int(olcham * 4) * 1000 + ogirlik;
    auto it = formatlar_.find(kalit);
    if (it != formatlar_.end()) return it->second;
    DWRITE_FONT_WEIGHT w = ogirlik >= 700 ? DWRITE_FONT_WEIGHT_BOLD
                         : ogirlik >= 600 ? DWRITE_FONT_WEIGHT_SEMI_BOLD
                         : ogirlik >= 500 ? DWRITE_FONT_WEIGHT_MEDIUM : DWRITE_FONT_WEIGHT_NORMAL;
    IDWriteTextFormat* f = nullptr;
    dw_->CreateTextFormat(L"Segoe UI", nullptr, w, DWRITE_FONT_STYLE_NORMAL,
                          DWRITE_FONT_STRETCH_NORMAL, olcham, L"uz", &f);
    if (f) {
        IDWriteInlineObject* uch = nullptr;
        dw_->CreateEllipsisTrimmingSign(f, &uch);
        DWRITE_TRIMMING t = {DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
        f->SetTrimming(&t, uch);
        if (uch) uch->Release();
    }
    formatlar_[kalit] = f;
    return f;
}

void Gfx::matn(R r, const wstring& s, float olcham, int ogirlik, Rang rang, Tekis gor, int vert,
               bool kop_qator) {
    if (!rt_ || s.empty()) return;
    IDWriteTextFormat* f = format(olcham, ogirlik);
    if (!f) return;
    f->SetTextAlignment(gor == CHAP ? DWRITE_TEXT_ALIGNMENT_LEADING
                        : gor == MARKAZ ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_TRAILING);
    f->SetParagraphAlignment(vert == 0 ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR
                             : vert == 1 ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER : DWRITE_PARAGRAPH_ALIGNMENT_FAR);
    f->SetWordWrapping(kop_qator ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
    mo_->SetColor(rang.d2d());
    rt_->DrawText(s.c_str(), (UINT32)s.size(), f, r.d2d(), mo_, D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

float Gfx::matn_eni(const wstring& s, float olcham, int ogirlik) {
    IDWriteTextFormat* f = format(olcham, ogirlik);
    if (!f || s.empty()) return 0;
    f->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    IDWriteTextLayout* l = nullptr;
    dw_->CreateTextLayout(s.c_str(), (UINT32)s.size(), f, 10000, 1000, &l);
    if (!l) return 0;
    DWRITE_TEXT_METRICS m;
    l->GetMetrics(&m);
    l->Release();
    return m.widthIncludingTrailingWhitespace;
}

float Gfx::matn_boyi(const wstring& s, float olcham, int ogirlik, float en) {
    IDWriteTextFormat* f = format(olcham, ogirlik);
    if (!f || s.empty()) return 0;
    f->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    IDWriteTextLayout* l = nullptr;
    dw_->CreateTextLayout(s.c_str(), (UINT32)s.size(), f, en, 10000, &l);
    if (!l) return 0;
    DWRITE_TEXT_METRICS m;
    l->GetMetrics(&m);
    l->Release();
    return m.height;
}

// ---------- SVG va rasmlar ----------

// ikonlar.html dagi har bir <symbol id="i-..." ...>...</symbol> alohida SVG bo'ladi.
void Gfx::ikonlarni_yukla() {
    string s = resurs_matni(201);
    size_t p = 0;
    while ((p = s.find("<symbol", p)) != string::npos) {
        size_t yop = s.find('>', p), oxir = s.find("</symbol>", p);
        if (yop == string::npos || oxir == string::npos) break;
        string attr = s.substr(p + 7, yop - p - 7), ichi = s.substr(yop + 1, oxir - yop - 1);
        size_t i = attr.find("id=\"");
        string nom = attr.substr(i + 4, attr.find('"', i + 4) - i - 4);
        attr.erase(i, nom.size() + 5);
        ikon_manba_[nom] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\"" +
                           attr + ">" + ichi + "</svg>";
        p = oxir;
    }
    // Yon menyudagi «Qurilmalar» ikonkasi base.html ichida yozilgan
    ikon_manba_["i-monitor"] =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" "
        "fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\"><rect x=\"3\" y=\"4\" "
        "width=\"18\" height=\"12\" rx=\"2\"/><path d=\"M8 21h8M12 16v5\"/></svg>";
}

static string almashtir(string s, const string& a, const string& b) {
    for (size_t p = 0; (p = s.find(a, p)) != string::npos; p += b.size()) s.replace(p, a.size(), b);
    return s;
}

ID2D1SvgDocument* Gfx::svg_hujjat(const string& kalit, const string& matn, float w, float h) {
    auto it = svglar_.find(kalit);
    if (it != svglar_.end()) return it->second;
    ID2D1SvgDocument* d = nullptr;
    if (dc5_) {
        IStream* st = SHCreateMemStream((const BYTE*)matn.data(), (UINT)matn.size());
        if (st) {
            dc5_->CreateSvgDocument(st, {w, h}, &d);
            st->Release();
        }
    }
    svglar_[kalit] = d;
    return d;
}

void Gfx::ikon(const string& nom, R r, Rang rang, float qalinlik) {
    if (!dc5_) return;
    auto it = ikon_manba_.find(nom);
    if (it == ikon_manba_.end()) return;
    char ah[16];
    snprintf(ah, 16, "#%02x%02x%02x", int(rang.r * 255 + .5f), int(rang.g * 255 + .5f),
             int(rang.b * 255 + .5f));
    string kalit = nom + ah + std::to_string(int(rang.a * 100)) + std::to_string(qalinlik);
    string m = almashtir(it->second, "currentColor", ah);
    if (rang.a < 1) {
        char op[48];
        snprintf(op, 48, "<svg opacity=\"%.2f\" ", rang.a);
        m = almashtir(m, "<svg ", op);
    }
    if (qalinlik > 0) {
        size_t p = m.find("stroke-width=\"");
        if (p != string::npos) m.replace(p + 14, m.find('"', p + 14) - p - 14, std::to_string(qalinlik));
    }
    ID2D1SvgDocument* d = svg_hujjat(kalit, m, 24, 24);
    if (!d) return;
    D2D1_MATRIX_3X2_F eski;
    dc5_->GetTransform(&eski);
    dc5_->SetTransform(D2D1::Matrix3x2F::Scale(r.w / 24, r.h / 24) *
                       D2D1::Matrix3x2F::Translation(r.x, r.y) * eski);
    dc5_->DrawSvgDocument(d);
    dc5_->SetTransform(eski);
}

void Gfx::svg_rasm(const string& kalit, const string& svg, R r) {
    if (!dc5_) return;
    // 1920x1080 hujjat «cover» bo'yicha joylanadi
    float k = std::max(r.w / 1920, r.h / 1080);
    ID2D1SvgDocument* d = svg_hujjat(kalit, svg, 1920, 1080);
    if (!d) return;
    D2D1_MATRIX_3X2_F eski;
    dc5_->GetTransform(&eski);
    dc5_->SetTransform(D2D1::Matrix3x2F::Scale(k, k) *
                       D2D1::Matrix3x2F::Translation(r.x + (r.w - 1920 * k) / 2, r.y + (r.h - 1080 * k) / 2) * eski);
    dc5_->DrawSvgDocument(d);
    dc5_->SetTransform(eski);
}

void Gfx::rasm(const string& kalit, int resurs, R r, bool cover) {
    if (!rt_ || !wic_) return;
    ID2D1Bitmap* b = nullptr;
    auto it = rasmlar_.find(kalit);
    if (it != rasmlar_.end())
        b = it->second;
    else {
        string m = resurs_matni(resurs);
        IWICStream* st = nullptr;
        IWICBitmapDecoder* dek = nullptr;
        IWICBitmapFrameDecode* kadr = nullptr;
        IWICBitmapScaler* sk = nullptr;
        IWICFormatConverter* kon = nullptr;
        if (!m.empty() && SUCCEEDED(wic_->CreateStream(&st)) &&
            SUCCEEDED(st->InitializeFromMemory((BYTE*)m.data(), (DWORD)m.size())) &&
            SUCCEEDED(wic_->CreateDecoderFromStream(st, nullptr, WICDecodeMetadataCacheOnLoad, &dek)) &&
            SUCCEEDED(dek->GetFrame(0, &kadr))) {
            UINT w, h;
            kadr->GetSize(&w, &h);
            IWICBitmapSource* manba = kadr;
            if (w > 1920 && SUCCEEDED(wic_->CreateBitmapScaler(&sk))) {
                sk->Initialize(kadr, 1920, UINT(h * 1920.0 / w), WICBitmapInterpolationModeFant);
                manba = sk;
            }
            if (SUCCEEDED(wic_->CreateFormatConverter(&kon)) &&
                SUCCEEDED(kon->Initialize(manba, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
                                          nullptr, 0, WICBitmapPaletteTypeCustom)))
                rt_->CreateBitmapFromWicBitmap(kon, nullptr, &b);
        }
        tashla(kon); tashla(sk); tashla(kadr); tashla(dek); tashla(st);
        rasmlar_[kalit] = b;
    }
    if (!b) return;
    D2D1_SIZE_F o = b->GetSize();
    float k = cover ? std::max(r.w / o.width, r.h / o.height) : std::min(r.w / o.width, r.h / o.height);
    float w = o.width * k, h = o.height * k;
    D2D1_RECT_F manzil = {r.x + (r.w - w) / 2, r.y + (r.h - h) / 2, r.x + (r.w + w) / 2, r.y + (r.h + h) / 2};
    rt_->PushAxisAlignedClip(r.d2d(), D2D1_ANTIALIAS_MODE_ALIASED);
    rt_->DrawBitmap(b, manzil, 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    rt_->PopAxisAlignedClip();
}

// ---------- Kesish ----------

void Gfx::kes(R r) {
    if (rt_) rt_->PushAxisAlignedClip(r.d2d(), D2D1_ANTIALIAS_MODE_ALIASED);
}
void Gfx::kesishni_ol() {
    if (rt_) rt_->PopAxisAlignedClip();
}
void Gfx::siljit(float x, float y) {
    dx += x;
    dy += y;
    if (rt_) rt_->SetTransform(D2D1::Matrix3x2F::Translation(dx, dy));
}
