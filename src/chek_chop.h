// Chek matni va Windows printeri orqali chop etish (58 / 80 mm chek printerlari).
#pragma once
#include "db.h"

wstring chek_matni(const Chek& c);
// printer bo'sh bo'lsa — sozlamadagi chek printeri (u ham bo'lmasa Windows standarti)
bool chek_chop(const Chek& c, wstring& xato, const wstring& printer = L"");
std::vector<wstring> printerlar();
// Code 128-C shtrix (ombor/shtrix.py): chiziqlar (boshlanishi, eni) modulda, jim zonalar bilan
struct Shtrix {
    std::vector<std::pair<int, int>> chiziqlar;
    int eni = 0;
};
Shtrix shtrix_yasa(const wstring& juft_raqamlar);
// Savdo chekining qaytarish uchun kodi: 88 + chek raqami (9) + nazorat (sotuv/chek.py: kod)
wstring savdo_chek_kodi(long long sotuv_id);
int ean_nazorat(const wstring& raqamlar);
// Sozlamada chek printeri tanlangan bo'lsa yakunlangan chekni chop etadi
void chekni_avtomatik_chop(long long sotuv_id);
wstring ini_qiymat(const wchar_t* kalit, const wchar_t* standart = L"");
void ini_qiymat_yoz(const wchar_t* kalit, const wstring& v);
