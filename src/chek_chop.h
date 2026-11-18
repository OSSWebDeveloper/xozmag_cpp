// Chek matni va Windows printeri orqali chop etish (58 / 80 mm chek printerlari).
#pragma once
#include "db.h"

wstring chek_matni(const Chek& c);
bool chek_chop(const Chek& c, wstring& xato);
std::vector<wstring> printerlar();
// Sozlamada chek printeri tanlangan bo'lsa yakunlangan chekni chop etadi
void chekni_avtomatik_chop(long long sotuv_id);
wstring ini_qiymat(const wchar_t* kalit, const wchar_t* standart = L"");
void ini_qiymat_yoz(const wchar_t* kalit, const wstring& v);
