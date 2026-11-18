// Oyna yordamchilari: shriftlar, tugmalar, ro'yxatlar, modal oynalar va forma.
#pragma once
#include <windows.h>
#include <commctrl.h>

#include <functional>
#include <string>
#include <vector>

#include "util.h"

extern HINSTANCE hIlova;
extern HWND hAsosiy;
extern HFONT fOddiy, fQalin, fKatta, fTugma, fJuda, fMono;
extern HBRUSH oqFon;

// Ranglar (dizayn ataylab oddiy: to'rtburchak, katta tugmalar)
constexpr COLORREF RANG_FON = RGB(255, 255, 255);
constexpr COLORREF RANG_YON = RGB(38, 50, 56);
constexpr COLORREF RANG_KOK = RGB(21, 101, 192);
constexpr COLORREF RANG_YASHIL = RGB(46, 125, 50);
constexpr COLORREF RANG_QIZIL = RGB(198, 40, 40);
constexpr COLORREF RANG_KULRANG = RGB(230, 232, 235);

enum TugmaRang { T_ODDIY = 0, T_YASHIL, T_KOK, T_YON, T_YON_FAOL, T_QIZIL };

int dp(int px);  // DPI bo'yicha o'lcham
void ui_boshla();

HWND yorliq(HWND ota, const wstring& matn, int id = 0, HFONT f = nullptr, DWORD stil = 0);
HWND maydon(HWND ota, int id, HFONT f = nullptr, DWORD stil = 0, const wchar_t* ishora = nullptr);
HWND tugma(HWND ota, const wstring& matn, int id, TugmaRang rang = T_ODDIY, HFONT f = nullptr);
HWND belgi(HWND ota, const wstring& matn, int id);
HWND tanlov(HWND ota, int id, const std::vector<wstring>& variantlar, int tanlangan = 0);
HWND royxat(HWND ota, int id, bool virtual_ = false);
void ustun(HWND lv, int i, const wchar_t* nom, int en, bool ong = false);
int lv_qosh(HWND lv, const std::vector<wstring>& qiymatlar, LPARAM param = 0);
LPARAM lv_param(HWND lv, int i);
int lv_tanlangan(HWND lv);
void lv_tanla(HWND lv, int i);

void tugma_rangi(HWND h, TugmaRang r);
bool tugma_chiz(LPARAM lp);  // WM_DRAWITEM uchun

wstring matn_ol(HWND h);
void matn_qoy(HWND h, const wstring& s);
void joy(HWND h, int x, int y, int w, int hgt);
bool belgilanganmi(HWND h);
void belgila(HWND h, bool b);

void xabar(HWND ota, const wstring& matn, bool xato = false);
bool sorash(HWND ota, const wstring& matn);

// ---------- Modal oyna ----------

struct Oyna {
    HWND h = nullptr;
    bool tugadi = false;
    int natija = IDCANCEL;
    // Xabarlarni qayta ishlash; true qaytarsa — ishlandi (natija `qaytar` da)
    std::function<bool(Oyna&, UINT, WPARAM, LPARAM, LRESULT&)> ishlov;
    void yop(int n) { natija = n; tugadi = true; }
};
// mijoz o'lchami w x h (dp birligida emas — piksel)
void oyna_yarat(Oyna& o, HWND ota, const wstring& sarlavha, int w, int h);
int oyna_modal(Oyna& o, HWND ota);
HWND faol_oyna();  // hozirgi modal yoki asosiy oyna

// ---------- Forma: yorliq + maydon qatorlari, OK / Bekor ----------

struct Maydon {
    enum Tur { MATN, SON, TANLOV, BELGI, MALUMOT } tur = MATN;
    wstring yorliq;
    wstring qiymat;                    // MATN, SON, MALUMOT
    std::vector<wstring> variantlar;   // TANLOV
    int tanlangan = 0;                 // TANLOV
    bool belgilangan = false;          // BELGI
    HWND h = nullptr;

    static Maydon matn(const wstring& y, const wstring& q = L"") { Maydon m; m.tur = MATN; m.yorliq = y; m.qiymat = q; return m; }
    static Maydon son(const wstring& y, const wstring& q = L"") { Maydon m; m.tur = SON; m.yorliq = y; m.qiymat = q; return m; }
    static Maydon tanla(const wstring& y, std::vector<wstring> v, int t = 0) { Maydon m; m.tur = TANLOV; m.yorliq = y; m.variantlar = std::move(v); m.tanlangan = t; return m; }
    static Maydon belgi(const wstring& y, bool b) { Maydon m; m.tur = BELGI; m.yorliq = y; m.belgilangan = b; return m; }
    static Maydon malumot(const wstring& q) { Maydon m; m.tur = MALUMOT; m.qiymat = q; return m; }
};

// tekshir: bo'sh matn qaytarsa forma yopiladi, aks holda xato ko'rsatiladi.
bool forma(HWND ota, const wstring& sarlavha, std::vector<Maydon>& maydonlar,
           std::function<wstring(std::vector<Maydon>&)> tekshir = {});

// ---------- Ro'yxatli oyna: qidiruv + jadval + tugmalar ----------

struct RoyxatSozlama {
    wstring sarlavha;
    bool qidiruv = true;
    wstring qidiruv_ishora;
    std::vector<std::pair<wstring, int>> ustunlar;  // nom, en (dp); manfiy en — o'ngga tekislangan
    // qidiruv matniga qarab qatorlarni to'ldiradi
    std::function<void(HWND lv, const wstring& qidiruv)> toldir;
    std::vector<wstring> tugmalar;  // 0-tugma — asosiy (Enter / ikki marta bosish)
    // tugma bosildi: indeks, tanlangan param (yo'q bo'lsa -1) -> true — oyna yopilsin
    std::function<bool(int tugma, LPARAM param, HWND lv)> bosildi;
    int en = 900, boy = 560;
    wstring pastki;  // pastdagi izoh
};
// Yopilganda oxirgi bosilgan tugma indeksi (-1 — bekor).
int royxat_oyna(HWND ota, RoyxatSozlama& s);

// Faqat o'qiladigan matn oynasi (chek ko'rinishi va h.k.). tugma bo'sh bo'lmasa
// pastda qo'shimcha tugma chiqadi va bosilganda amal() chaqiriladi.
void matn_oyna(HWND ota, const wstring& sarlavha, const wstring& matn, const wstring& tugma = L"",
               std::function<void()> amal = {});
