// Sahifalar uchun so'rovlar: qarzdorlar, hudud, karta, hisobot, sotuvlar, ombor.
#pragma once
#include "db.h"

struct QarzdorQ {
    Qarzdor q;
    double olgan = 0, olgan_d = 0, tolagan = 0, tolagan_d = 0;
    bool qarzli() const { return q.balans > 0 || q.balans_d > 0; }
};
// hudud_id = 0 — hammasi. Eng ko'p qarzdorlar tepada.
std::vector<QarzdorQ> qarzdorlar_royxati(long long hudud_id);

struct HududKarta {
    Hudud h;
    int soni = 0;
    double balans = 0, balans_d = 0;
};
std::vector<HududKarta> hudud_kartalari();
bool hudud_ol(long long id, Hudud& h);

struct KartaQator {
    wstring nom, birlik;
    double qolgan = 0, qaytarilgan = 0;
    bool dollar = false;
};
struct KartaQarz {
    long long id = 0, sotuv_id = 0;
    string sana;
    bool yakunlangan = true;
    double sof = 0, sof_d = 0, qayt = 0, qayt_d = 0, old = 0, old_d = 0;
    std::vector<KartaQator> q;
};
std::vector<KartaQarz> karta_qarzlari(long long qarzdor_id);
struct KartaTolov {
    long long id;
    string sana;
    double summa;
    bool dollar;
};
std::vector<KartaTolov> karta_tolovlari(long long qarzdor_id);
void tolov_ochir(long long id);
QarzdorQ qarzdor_hisobi(long long id);

// Hisobot — kassa savdolari (sotuv/hisobot.py)
struct HisobotKassa { wstring kassir; int soni; double som, dollar; };
struct HisobotChek { long long id; string sana; wstring kassir; double som, dollar; };
struct Hisobot {
    int soni = 0;
    double som = 0, dollar = 0;
    std::vector<HisobotKassa> kassalar;
    std::vector<HisobotChek> cheklar;
};
Hisobot hisobot_ol(const SYSTEMTIME& dan, const SYSTEMTIME& gacha, long long kassir_id);
std::vector<std::pair<long long, wstring>> kassirlar();

// Kunlik sotuvlar (sotuv/views.py: royxat)
struct SotuvKun {
    long long id = 0;
    string sana;
    bool bekor = false;
    double sof = 0, sof_d = 0, qayt = 0, qayt_d = 0;
    long long qarzdor_id = 0;
    wstring qarzdor;
    bool qarz_bor = false;
    double qarz_sof = 0, qarz_sof_d = 0, qarz_qayt = 0, qarz_qayt_d = 0;
    bool qaytarilgan = false;
    std::vector<ChekQator> q;
};
struct KunSotuvlari {
    int soni = 0;
    double jami = 0, jami_d = 0, qarzga = 0, qarzga_d = 0;
    std::vector<SotuvKun> s;
};
KunSotuvlari kun_sotuvlari(const SYSTEMTIME& kun);

// Ombor ro'yxati (ombor/views.py: royxat, 50 tadan)
struct OmborTovar {
    Tovar t;
    wstring shtrix;
};
struct OmborFiltr {
    wstring matn;
    int valyuta = 0;  // 0 hammasi, 1 so'm, 2 dollar
    bool kam = false, tugagan = false, yashirilgan = false;
};
void ombor_keshini_tashla();
std::vector<const OmborTovar*> ombor_filtrla(const OmborFiltr& f);
long long tugagan_soni();
bool shtrix_saqla(long long id, const wstring& kod, wstring& xato);

// Sana: "18.11.2026" <-> SYSTEMTIME
bool sana_oqi(const wstring& s, SYSTEMTIME& t);
wstring sana_yoz(const SYSTEMTIME& t);
