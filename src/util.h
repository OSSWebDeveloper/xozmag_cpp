// Yordamchi funksiyalar: matn kodlash, son va pul ko'rinishi, vaqt.
#pragma once
#include <windows.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using std::string;
using std::wstring;

// ---------- UTF-8 <-> UTF-16 ----------
// Baza UTF-8 da, Windows oynalari UTF-16 (wchar_t) bilan ishlaydi.

inline wstring W(const string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    wstring r(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), r.data(), n);
    return r;
}

inline string U(const wstring& s) {
    if (s.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    string r(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), r.data(), n, nullptr, nullptr);
    return r;
}

inline wstring tozala(const wstring& s) {
    size_t a = s.find_first_not_of(L" \t\r\n");
    if (a == wstring::npos) return {};
    size_t b = s.find_last_not_of(L" \t\r\n");
    return s.substr(a, b - a + 1);
}

inline bool raqammi(const wstring& s) {
    if (s.empty()) return false;
    for (wchar_t c : s)
        if (c < L'0' || c > L'9') return false;
    return true;
}

// ---------- Sonlar ----------

// Mingliklarni bo'shliq bilan ajratadi: 1234567 -> "1 234 567".
inline wstring guruhla(long long butun) {
    wstring raqam = std::to_wstring(butun < 0 ? -butun : butun);
    wstring r;
    int sanoq = 0;
    for (int i = (int)raqam.size() - 1; i >= 0; --i) {
        r.insert(r.begin(), raqam[i]);
        if (++sanoq % 3 == 0 && i > 0) r.insert(r.begin(), L' ');
    }
    if (butun < 0) r.insert(r.begin(), L'-');
    return r;
}

// Pul: 2 xonagacha kasr, butun bo'lsa kasrsiz. 198000 -> "198 000", 12.5 -> "12,50".
inline wstring pul(double v) {
    long long tiyin = std::llround(v * 100.0);
    long long butun = tiyin / 100;
    int kasr = (int)std::llabs(tiyin % 100);
    wstring r = guruhla(butun);
    if (tiyin < 0 && butun == 0) r = L"-" + r;
    if (kasr) {
        wchar_t b[8];
        swprintf(b, 8, L",%02d", kasr);
        r += b;
    }
    return r;
}

// Miqdor: 3 xonagacha kasr, ortiqcha nollarsiz. 2.500 -> "2,5".
inline wstring son(double v) {
    wchar_t b[64];
    swprintf(b, 64, L"%.3f", v);
    wstring r = b;
    while (!r.empty() && r.back() == L'0') r.pop_back();
    if (!r.empty() && r.back() == L'.') r.pop_back();
    for (auto& c : r)
        if (c == L'.') c = L',';
    if (r == L"-0") r = L"0";
    return r;
}

// Bazaga yoziladigan son (nuqta bilan, ortiqcha nolsiz).
inline string sonU(double v) {
    char b[64];
    snprintf(b, 64, "%.4f", v);
    string r = b;
    while (!r.empty() && r.back() == '0') r.pop_back();
    if (!r.empty() && r.back() == '.') r.pop_back();
    return r;
}

// Kiritilgan matnni songa aylantiradi: "1 250,5" -> 1250.5. Bo'sh matn — 0.
inline bool songa(const wstring& kirish, double& natija) {
    wstring s;
    for (wchar_t c : kirish) {
        if (c == L' ' || c == 0xA0 || c == 0x202F || c == L'\t') continue;
        s += (c == L',') ? L'.' : c;
    }
    if (s.empty()) {
        natija = 0;
        return true;
    }
    wchar_t* oxir = nullptr;
    natija = wcstod(s.c_str(), &oxir);
    return oxir && *oxir == 0 && std::isfinite(natija);
}

inline double yaxlit(double v, int xona) {
    double k = std::pow(10.0, xona);
    return std::round(v * k) / k;
}

// ---------- Vaqt ----------
// Django bazasi vaqtni UTC da "YYYY-MM-DD HH:MM:SS.ffffff" ko'rinishida saqlaydi.
// Dastur ham shunday yozadi, ko'rsatishda kompyuterning mahalliy vaqtiga o'tkazadi.

inline string vaqt_matni(const SYSTEMTIME& t) {
    char b[40];
    snprintf(b, sizeof b, "%04d-%02d-%02d %02d:%02d:%02d.%06d", t.wYear, t.wMonth, t.wDay, t.wHour,
             t.wMinute, t.wSecond, t.wMilliseconds * 1000);
    return b;
}

inline string hozir_utc() {
    SYSTEMTIME t;
    GetSystemTime(&t);
    return vaqt_matni(t);
}

inline bool vaqtni_oqi(const string& s, SYSTEMTIME& t) {
    ZeroMemory(&t, sizeof t);
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, se = 0;
    int n = sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &se);
    if (n < 3) return false;
    t.wYear = (WORD)y; t.wMonth = (WORD)mo; t.wDay = (WORD)d;
    t.wHour = (WORD)h; t.wMinute = (WORD)mi; t.wSecond = (WORD)se;
    return true;
}

// UTC matnidan mahalliy "18.11.2026 14:05".
inline wstring mahalliy_vaqt(const string& utc, bool soat = true) {
    SYSTEMTIME u, m;
    if (!vaqtni_oqi(utc, u)) return W(utc);
    if (!SystemTimeToTzSpecificLocalTime(nullptr, &u, &m)) m = u;
    wchar_t b[40];
    if (soat)
        swprintf(b, 40, L"%02d.%02d.%04d %02d:%02d", m.wDay, m.wMonth, m.wYear, m.wHour, m.wMinute);
    else
        swprintf(b, 40, L"%02d.%02d.%04d", m.wDay, m.wMonth, m.wYear);
    return b;
}

// Mahalliy kunning boshlanishi va oxiri UTC matnida — kunlik hisobot uchun.
inline void kun_oraligi(const SYSTEMTIME& kun, string& bosh, string& oxir) {
    SYSTEMTIME m = {}, u;
    m.wYear = kun.wYear; m.wMonth = kun.wMonth; m.wDay = kun.wDay;
    TzSpecificLocalTimeToSystemTime(nullptr, &m, &u);
    bosh = vaqt_matni(u);
    FILETIME f;
    SystemTimeToFileTime(&m, &f);
    ULARGE_INTEGER x;
    x.LowPart = f.dwLowDateTime; x.HighPart = f.dwHighDateTime;
    x.QuadPart += 864000000000ULL;  // +1 kun
    f.dwLowDateTime = x.LowPart; f.dwHighDateTime = x.HighPart;
    FileTimeToSystemTime(&f, &m);
    TzSpecificLocalTimeToSystemTime(nullptr, &m, &u);
    oxir = vaqt_matni(u);
}

inline string bugun_mahalliy() {
    SYSTEMTIME t;
    GetLocalTime(&t);
    char b[16];
    snprintf(b, sizeof b, "%04d-%02d-%02d", t.wYear, t.wMonth, t.wDay);
    return b;
}

inline wstring fayl_vaqti() {
    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t b[32];
    swprintf(b, 32, L"%04d%02d%02d_%02d%02d%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute,
             t.wSecond);
    return b;
}
