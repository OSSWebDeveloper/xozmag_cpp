// Omborni Excel orqali yangilash (ombor/excel.py ning C++ dagi nusxasi):
// fayl -> qoralama -> qo'llash -> kerak bo'lsa bekor qilish.
#pragma once
#include <map>
#include <set>

#include "db.h"

// Tovar holati (JSON'dagi kabi): kod, nom, birlik, qoldiq, narx, valyuta, faol, kelish_narxi, kelish_valyutasi.
// "faol" qiymati L"true"/L"false".
using Holat = std::map<string, wstring>;
extern const std::vector<string> HOLAT_MAYDONLARI;

string json_yoz(const Holat& h);
Holat json_oqi(const string& s);
Holat mahsulot_holati(long long id);

struct Yangilanish {
    long long id = 0;
    wstring holat, fayl_nomi, izoh;
    bool yoqlarini_yashir = false;
    long long jami_qator = 0, ozgarmagan = 0;
    string yaratilgan, qollangan, bekor_qilingan;
    long long n_yangi = 0, n_ozgardi = 0, n_yashirildi = 0, n_xato = 0, n_chiqarilgan = 0;
    wstring nomi() const { return !fayl_nomi.empty() ? fayl_nomi : !izoh.empty() ? izoh : L"Qo'lda"; }
};
std::vector<Yangilanish> yangilanishlar_royxati(const wstring& holat);
bool yangilanish_ol(long long id, Yangilanish& y);
long long qoralamalar_soni();

struct YQator {
    long long id = 0, excel_qator = 0, mahsulot_id = 0;
    wstring amal, kod, xato, ogoh;
    Holat eski, yangi;
    bool eski_bor = false, yangi_bor = false, chiqarilgan = false;
};
// turi: "" hammasi, "yangi"/"ozgardi"/"yashirildi"/"xato" (chiqarilmaganlar), "chiqarilgan"
std::vector<YQator> yangilanish_qatorlari(long long id, const string& turi, const wstring& matn);
bool yqator_ol(long long qid, YQator& q, long long& yangilanish_id);

// Fayldan qoralama. Xato bo'lsa 0 va sabablar.
long long fayldan_qoralama(const wstring& fayl, bool yoqlarini_yashir, const wstring& izoh,
                           std::vector<wstring>& xatolar);
bool qollash(long long id, std::vector<wstring>& xatolar);
struct Toqnash {
    wstring kod, sabab;
    long long keyingi = 0;
};
std::vector<Toqnash> bekor_toqnashuvlari(long long id);
bool bekor_qil(long long id, std::vector<wstring>& xatolar);
bool qoralamani_ochir(long long id, wstring& xato);
long long xatolarni_chiqar(long long id);
bool qator_chiqar(long long qid, wstring& xato);  // almashtiradi (chiqarish/qaytarish)

// Qatorni tuzatish (QatorForm): maydonlar matn ko'rinishida
Holat qator_formasi_uchun(long long qid);
// kirish — formadagi qiymatlar (kod, nom, birlik, qoldiq, narx, valyuta, kelish_narxi, kelish_valyutasi)
wstring qatorni_saqla(long long qid, const Holat& kirish);

// Yuklab olish: filtr bo'yicha (bo'sh ro'yxat — shablon)
bool excel_eksport(const wstring& fayl, const std::vector<long long>& tovar_idlari, wstring& xato);
