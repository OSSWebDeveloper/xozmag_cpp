// .xlsx o'qish va yozish: zip (deflate ochish, "stored" yozish) + oddiy XML.
// Tashqi kutubxonasiz — Excel fayli aslida XML fayllar solingan zip arxiv.
#pragma once
#include <map>
#include <string>
#include <vector>

#include "util.h"

struct Katak {
    enum Tur { BOSH, SON, MATN, MANTIQ } tur = BOSH;
    wstring matn;   // SON uchun ham <v> dagi asl yozuv ("2.65")
    double son = 0;
    bool boshmi() const { return tur == BOSH || (tur == MATN && tozala(matn).empty()); }
};

// Birinchi varaqning qatorlari: qator raqami (1 dan) -> kataklar (0 dan, ustun bo'yicha)
struct Varaq {
    std::map<int, std::vector<Katak>> qatorlar;
};
bool xlsx_oqi(const wstring& fayl, Varaq& v, wstring& xato);

// ---- yozish ----
struct YKatak {
    enum Tur { BOSH, SON, MATN } tur = BOSH;
    wstring matn;
    double son = 0;
    int uslub = 0;  // styles.xml dagi cellXfs indeksi
};
struct YVaraq {
    wstring nom;
    std::vector<std::vector<YKatak>> qatorlar;
    std::vector<double> enlar;          // ustun enlari
    float sarlavha_boyi = 0;            // 1-qator balandligi
    wstring muzlatish;                  // "C2"
    wstring avtofiltr;                  // "A1:H10"
    struct Tekshiruv { wstring royxat, oraliq; };
    std::vector<Tekshiruv> tekshiruvlar;  // ochiluvchi ro'yxatlar
};
// Uslublar: 0 oddiy, 1 sarlavha (oq qalin, ko'k fon), 2 matn (@), 3 #,##0.###,
// 4 #,##0.####, 5 qalin, 6 o'raladigan matn, 7 qalin 14
bool xlsx_yoz(const wstring& fayl, const std::vector<YVaraq>& varaqlar, wstring& xato);
