// Tovar etiketkasi (rasta uchun narx yorlig'i) — saytdagi ombor/etiketka.html va
// config/etiketka_print.ps1 bilan bir xil: nomi, narxi, Code 128 shtrix va kod.
// Chop etish shu kompyuterning Windows printeriga (GDI), qog'oz o'lchami — yorliq o'lchami.
#include "ilova.h"

#include <winspool.h>

#include <algorithm>
#include <cmath>

// ---------- Code 128-C (ombor/shtrix.py) ----------

static const char* NAQSHLAR[107] = {
    "212222", "222122", "222221", "121223", "121322", "131222", "122213", "122312", "132212", "221213",
    "221312", "231212", "112232", "122132", "122231", "113222", "123122", "123221", "223211", "221132",
    "221231", "213212", "223112", "312131", "311222", "321122", "321221", "312212", "322112", "322211",
    "212123", "212321", "232121", "111323", "131123", "131321", "112313", "132113", "132311", "211313",
    "231113", "231311", "112133", "112331", "132131", "113123", "113321", "133121", "313121", "211331",
    "231131", "213113", "213311", "213131", "311123", "311321", "331121", "312113", "312311", "332111",
    "314111", "221411", "431111", "111224", "111422", "121124", "121421", "141122", "141221", "112214",
    "112412", "122114", "122411", "142112", "142211", "241211", "221114", "413111", "241112", "134111",
    "111242", "121142", "121241", "114212", "124112", "124211", "411212", "421112", "421211", "212141",
    "214121", "412121", "111143", "111341", "131141", "114113", "114311", "411113", "411311", "113141",
    "114131", "311141", "411131", "211412", "211214", "211232", "2331112",
};
static const int JIM_ZONA = 10;

struct Shtrix {
    std::vector<std::pair<int, int>> chiziqlar;  // (boshlanishi, eni) modulda
    int eni = 0;                                 // jim zonalar bilan
};

static Shtrix shtrix_yasa(const wstring& raqamlar) {
    Shtrix r;
    if (raqamlar.empty() || raqamlar.size() % 2 || !raqammi(raqamlar)) return r;
    std::vector<int> q = {105};
    for (size_t i = 0; i < raqamlar.size(); i += 2) q.push_back((raqamlar[i] - L'0') * 10 + raqamlar[i + 1] - L'0');
    int nazorat = q[0];
    for (size_t i = 1; i < q.size(); ++i) nazorat += q[i] * (int)i;
    q.push_back(nazorat % 103);
    q.push_back(106);
    int x = JIM_ZONA;
    for (int b : q) {
        const char* n = NAQSHLAR[b];
        for (int i = 0; n[i]; ++i) {
            int en = n[i] - '0';
            if (i % 2 == 0) r.chiziqlar.push_back({x, en});
            x += en;
        }
    }
    r.eni = x + JIM_ZONA;
    return r;
}

// Tovar kodi shtrixda 6 raqam (models.etiketka_kodi)
static wstring etiketka_kodi(const Tovar& t) {
    wstring k = t.kod;
    while (k.size() < 6) k = L"0" + k;
    return k;
}

int etiketka_soni(const Tovar& t, double miqdor, const wstring& birlik) {
    bool sanaladi = birlik != L"kg" && birlik != L"metr" && birlik != L"litr" && birlik != L"tonna";
    if (sanaladi && miqdor == std::floor(miqdor) && miqdor > 0 && miqdor <= ETIKETKA_KOPI) return (int)miqdor;
    return 1;
}

// ---------- Yorliq tuzilishi (etiketka_print.ps1: 56 x 40 mm asosda, keyin cho'ziladi) ----------

struct Olcham {
    const wchar_t* nom;
    const wchar_t* kalit;
    float w, h;
};
static const Olcham OLCHAMLAR[] = {
    {L"40 × 30 mm", L"40x30", 40, 30}, {L"58 × 40 mm", L"58x40", 58, 40}, {L"56 × 40 mm · tarozi", L"56x40", 56, 40},
    {L"50 × 30 mm", L"50x30", 50, 30}, {L"30 × 20 mm", L"30x20", 30, 20},
};

static wstring narx_matni(const Tovar& t) {
    if (t.narx <= 0) return L"Narx belgilanmagan";
    return t.dollarmi() ? son(t.narx) + L" $" : pul(t.narx) + L" so'm";
}

// ---------- Printerga chiqarish ----------

static bool printer_ulanganmi(const wstring& nom) {
    HANDLE h = nullptr;
    if (!OpenPrinterW((LPWSTR)nom.c_str(), &h, nullptr)) return false;
    DWORD kerak = 0;
    GetPrinterW(h, 2, nullptr, 0, &kerak);
    bool ok = true;
    if (kerak) {
        std::vector<BYTE> b(kerak);
        if (GetPrinterW(h, 2, b.data(), kerak, &kerak)) {
            auto* p = (PRINTER_INFO_2W*)b.data();
            ok = !(p->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE);
        }
    }
    ClosePrinter(h);
    return ok;
}

static void gdi_matn(HDC dc, const wstring& s, float bal_mm, bool qalin, float y, float boy, bool kop_qator) {
    // Birlik: 0.01 mm (asos 56 x 40)
    HFONT f = CreateFontW(-(int)std::lround(bal_mm * 100), 0, 0, 0, qalin ? FW_BOLD : FW_NORMAL, 0, 0, 0,
                          DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, L"Arial");
    HGDIOBJ eski = SelectObject(dc, f);
    RECT r = {200, (LONG)std::lround(y * 100), 5400, (LONG)std::lround((y + boy) * 100)};
    UINT bayroq = DT_CENTER | DT_NOPREFIX | DT_END_ELLIPSIS;
    if (kop_qator) {
        RECT o = r;
        DrawTextW(dc, s.c_str(), (int)s.size(), &o, bayroq | DT_WORDBREAK | DT_CALCRECT);
        int h = std::min<int>(o.bottom - o.top, r.bottom - r.top);
        r.top += (r.bottom - r.top - h) / 2;
        r.bottom = r.top + h;
        DrawTextW(dc, s.c_str(), (int)s.size(), &r, bayroq | DT_WORDBREAK | DT_EDITCONTROL);
    } else {
        DrawTextW(dc, s.c_str(), (int)s.size(), &r, bayroq | DT_SINGLELINE | DT_VCENTER);
    }
    SelectObject(dc, eski);
    DeleteObject(f);
}

static bool etiketka_chop(const Tovar& t, const wstring& printer, const Olcham& o, int soni, wstring& xato) {
    if (!printer_ulanganmi(printer)) {
        xato = L"«" + printer + L"» ulanmagan yoki topilmadi.";
        return false;
    }
    // Qog'oz o'lchami — yorliqniki (aks holda printer A4 deb o'ylab o'rtaga qo'yadi yoki buradi)
    HANDLE hp = nullptr;
    std::vector<BYTE> dm_b;
    DEVMODEW* dm = nullptr;
    if (OpenPrinterW((LPWSTR)printer.c_str(), &hp, nullptr)) {
        LONG n = DocumentPropertiesW(nullptr, hp, (LPWSTR)printer.c_str(), nullptr, nullptr, 0);
        if (n > 0) {
            dm_b.resize(n);
            dm = (DEVMODEW*)dm_b.data();
            if (DocumentPropertiesW(nullptr, hp, (LPWSTR)printer.c_str(), dm, nullptr, DM_OUT_BUFFER) == IDOK) {
                dm->dmFields |= DM_PAPERSIZE | DM_PAPERWIDTH | DM_PAPERLENGTH | DM_ORIENTATION;
                dm->dmPaperSize = DMPAPER_USER;
                dm->dmPaperWidth = (short)std::lround(o.w * 10);
                dm->dmPaperLength = (short)std::lround(o.h * 10);
                dm->dmOrientation = DMORIENT_PORTRAIT;
                DocumentPropertiesW(nullptr, hp, (LPWSTR)printer.c_str(), dm, dm, DM_IN_BUFFER | DM_OUT_BUFFER);
            } else
                dm = nullptr;
        }
        ClosePrinter(hp);
    }
    HDC dc = CreateDCW(L"WINSPOOL", printer.c_str(), nullptr, dm);
    if (!dc) {
        xato = L"«" + printer + L"» printeri ochilmadi.";
        return false;
    }
    DOCINFOW di = {sizeof di};
    wstring nom = L"Xozmag yorliq " + t.kod;
    di.lpszDocName = nom.c_str();
    if (StartDocW(dc, &di) <= 0) {
        DeleteDC(dc);
        xato = L"Chop etish boshlanmadi (printer ulanganmi?).";
        return false;
    }
    float dpx = GetDeviceCaps(dc, LOGPIXELSX) / 25.4f, dpy = GetDeviceCaps(dc, LOGPIXELSY) / 25.4f;
    float offx = (float)GetDeviceCaps(dc, PHYSICALOFFSETX), offy = (float)GetDeviceCaps(dc, PHYSICALOFFSETY);
    Shtrix sh = shtrix_yasa(etiketka_kodi(t));
    for (int k = 0; k < soni; ++k) {
        StartPage(dc);
        SetGraphicsMode(dc, GM_ADVANCED);
        // 0.01 mm asos birligi -> qurilma nuqtasi; 56x40 asos yorliq o'lchamiga cho'ziladi
        XFORM x = {dpx * o.w / 56 / 100, 0, 0, dpy * o.h / 40 / 100, -offx, -offy};
        SetWorldTransform(dc, &x);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(0, 0, 0));
        gdi_matn(dc, t.nom, 3.0f, true, 1.5f, 10, true);
        gdi_matn(dc, narx_matni(t), 4.5f, true, 12.5f, 6, false);
        gdi_matn(dc, L"1 " + t.birlik + L" uchun", 2.0f, false, 18.5f, 3, false);
        if (sh.eni) {
            float modul = std::min(0.5f, 50.0f / sh.eni);
            float chap = (56 - sh.eni * modul) / 2;
            HBRUSH qora = (HBRUSH)GetStockObject(BLACK_BRUSH);
            for (auto [bx, bw] : sh.chiziqlar) {
                RECT r = {(LONG)std::lround((chap + bx * modul) * 100), 2300,
                          (LONG)std::lround((chap + (bx + bw) * modul) * 100), 3200};
                FillRect(dc, &r, qora);
            }
        }
        gdi_matn(dc, L"KOD " + t.kod, 3.5f, true, 33, 5, false);
        ModifyWorldTransform(dc, nullptr, MWT_IDENTITY);
        if (EndPage(dc) <= 0) {
            AbortDoc(dc);
            DeleteDC(dc);
            xato = L"Printer yorliqni qabul qilmadi.";
            return false;
        }
    }
    EndDoc(dc);
    DeleteDC(dc);
    return true;
}

// ---------- Sahifa ----------

static Tovar e_t;
static int e_olcham = 0, e_printer = 0, e_ochiq_soni = 1;
static std::vector<wstring> e_printerlar, e_olcham_nomlari;
static Shtrix e_sh;

void etiketka_och(long long tovar_id, int soni) {
    e_ochiq_soni = std::clamp(soni, 1, ETIKETKA_KOPI);
    ot(J_ETIKETKA, tovar_id);
}

static bool etiketka_printeri_nomi(const wstring& n) {
    wstring k = kichik(n);
    for (const wchar_t* s : {L"label", L"barcode", L"shtrix", L"etiket", L"zebra", L"tsc", L"godex", L"xp-3", L"xp-4",
                             L"xp3", L"xp4", L"365"})
        if (k.find(s) != wstring::npos) return true;
    return false;
}

void etiketka_ochildi(long long id) {
    e_t = {};
    tovar_ol(id, e_t);
    e_sh = shtrix_yasa(etiketka_kodi(e_t));
    ui.qiymat_qoy("et/soni", std::to_wstring(e_ochiq_soni));
    e_ochiq_soni = 1;
    wstring ol = ini_qiymat(L"etiketka_olcham", L"40x30");
    e_olcham = 0;
    e_olcham_nomlari.clear();
    for (int i = 0; i < (int)std::size(OLCHAMLAR); ++i) {
        e_olcham_nomlari.push_back(OLCHAMLAR[i].nom);
        if (ol == OLCHAMLAR[i].kalit) e_olcham = i;
    }
    // Printer: sozlamadagi etiketka printeri, bo'lmasa nomidan etiketka printeri, bo'lmasa standart
    e_printerlar = printerlar();
    wstring tanlangan = ini_qiymat(L"etiketka_printer");
    if (tanlangan.empty())
        for (auto& p : e_printerlar)
            if (etiketka_printeri_nomi(p)) { tanlangan = p; break; }
    if (tanlangan.empty()) {
        wchar_t b[512];
        DWORD n = 512;
        if (GetDefaultPrinterW(b, &n)) tanlangan = b;
    }
    e_printer = 0;
    for (size_t i = 0; i < e_printerlar.size(); ++i)
        if (e_printerlar[i] == tanlangan) e_printer = (int)i;
    ui.scroll_tepaga("et/s");
}

// Ekrandagi namuna: chop etiladiganining aynan o'zi (1 mm = k piksel)
static void yorliq_chiz(R r, const Olcham& o) {
    gfx.quti({r.x, r.y + 2, r.w, r.h}, {0, 0, 0, .18f}, 3);
    gfx.quti(r, hex("#ffffff"), 3, hex("#b6c0cd"));
    float sx = r.w / 56, sy = r.h / 40, k = std::min(sx, sy);
    Rang qora = hex("#000000");
    auto matn = [&](const wstring& s, float bal, int og, float y, float boy, bool kop) {
        gfx.matn({r.x + 2 * sx, r.y + y * sy, 52 * sx, boy * sy}, s, bal * k * 0.95f, og, qora, MARKAZ, 1, kop);
    };
    matn(e_t.nom, 3.0f, 700, 1.5f, 10, true);
    matn(narx_matni(e_t), 4.5f, 700, 12.5f, 6, false);
    matn(L"1 " + e_t.birlik + L" uchun", 2.0f, 400, 18.5f, 3, false);
    if (e_sh.eni) {
        float modul = std::min(0.5f, 50.0f / e_sh.eni);
        float chap = (56 - e_sh.eni * modul) / 2;
        for (auto [bx, bw] : e_sh.chiziqlar)
            gfx.quti({r.x + (chap + bx * modul) * sx, r.y + 23 * sy, bw * modul * sx, 9 * sy}, qora);
    }
    matn(L"KOD " + e_t.kod, 3.5f, 700, 33, 5, false);
}

void etiketka_chiz(R r) {
    if (!e_t.id) {
        sarlavha(r, L"Etiketka");
        gfx.matn(r, L"Tovar topilmadi.", 15, 500, M->matn3);
        return;
    }
    sarlavha(r, L"Rasta uchun narx yorlig'i", e_t.nom + L" · kod " + e_t.kod);
    // Boshqaruv qutisi
    R q = {r.x, r.y, r.w, 14 + 22 + 50 + 12 + 18 + 14};
    quti(q);
    float x = q.x + 14, y = q.y + 14;
    gfx.matn({x, y, 150, 17}, L"Nechta", 12, 600, M->matn2);
    gfx.matn({x + 164, y, 220, 17}, L"Etiketka o'lchami", 12, 600, M->matn2);
    float pw = std::max(220.f, q.o() - 14 - 200 - 12 - 190 - 12 - (x + 164 + 220 + 14));
    gfx.matn({x + 164 + 234, y, pw, 17}, L"Printer", 12, 600, M->matn2);
    y += 22;
    MaydonSoz ms;
    ms.ong = true;
    ui.maydon("et/soni", {x, y, 150, 50}, ms);
    if (ui.tanlov("et/olcham", {x + 164, y, 220, 50}, e_olcham_nomlari, e_olcham))
        ini_qiymat_yoz(L"etiketka_olcham", OLCHAMLAR[e_olcham].kalit);
    std::vector<wstring> pv = e_printerlar.empty() ? std::vector<wstring>{L"— printer topilmadi —"} : e_printerlar;
    ui.tanlov("et/printer", {x + 398, y, pw, 50}, pv, e_printer, 15);
    float bx = x + 398 + pw + 12;
    bool chop = ui.tugma("et/chop", {bx, y, 190, 50}, L"Chop etish", YASHIL, "i-printer") || ui.enter("et/soni");
    if (ui.tugma("et/qayt", {bx + 202, y, 200, 50}, L"Omborga qaytish", TINCH, "i-chap")) {
        ot(J_OMBOR);
        return;
    }
    y += 50 + 12;
    gfx.matn({x, y, q.w - 28, 18},
             L"Nom, sotish narxi, kod va shtrix kod chiqariladi. O'lcham va printer shu kompyuterda eslab qolinadi.",
             12, 400, M->matn3);

    double sd = 0;
    int soni = 1;
    if (songa(ui.qiymat("et/soni"), sd)) soni = (int)std::clamp(sd, 1.0, (double)ETIKETKA_KOPI);
    if (chop) {
        if (!songa(ui.qiymat("et/soni"), sd) || sd < 1 || sd > ETIKETKA_KOPI || sd != std::floor(sd))
            ui.xabar(L"Nechta: 1 dan " + std::to_wstring(ETIKETKA_KOPI) + L" gacha butun son yozing.", X_XATO);
        else if (e_printerlar.empty())
            ui.xabar(L"Windows'da printer topilmadi.", X_XATO);
        else {
            wstring p = e_printerlar[e_printer], xato;
            ini_qiymat_yoz(L"etiketka_printer", p);
            HCURSOR k = SetCursor(LoadCursor(nullptr, IDC_WAIT));
            bool ok = etiketka_chop(e_t, p, OLCHAMLAR[e_olcham], soni, xato);
            SetCursor(k);
            if (ok) ui.xabar(std::to_wstring(soni) + L" ta yorliq «" + p + L"» ga yuborildi. Qog'oz chiqqanini tekshiring.");
            else ui.xabar(xato, X_XATO);
        }
    }

    // Ko'rinish: har bir nusxa (ko'p bo'lsa birinchilari)
    const Olcham& o = OLCHAMLAR[e_olcham];
    const float MM = 6;  // ekranda 1 mm
    float ew = o.w * MM, eh = o.h * MM, ora = 16;
    R tana = {r.x, q.p() + 14, r.w, r.p() - q.p() - 14};
    int ustun = std::max(1, (int)((tana.w - 10 + ora) / (ew + ora)));
    int korsat = std::min(soni, 60);
    int qatorlar = (korsat + ustun - 1) / ustun;
    float kontent = qatorlar * (eh + ora) + (soni > korsat ? 30 : 0) + 4;
    ui.scroll("et/s", tana, kontent);
    for (int i = 0; i < korsat; ++i) {
        float ex = tana.x + (i % ustun) * (ew + ora), ey = tana.y + 2 + (i / ustun) * (eh + ora);
        if (ey > tana.p() + 1000) break;
        yorliq_chiz({ex, ey, ew, eh}, o);
    }
    if (soni > korsat)
        gfx.matn({tana.x, tana.y + qatorlar * (eh + ora), tana.w, 24},
                 L"… va yana " + std::to_wstring(soni - korsat) + L" ta xuddi shunday yorliq", 13, 500, M->matn3);
    ui.scroll_tugat();
}
