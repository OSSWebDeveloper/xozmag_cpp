// Dastur bo'ylab umumiy narsalar: sahifalar, o'tish, joriy foydalanuvchi.
#pragma once
#include "db.h"
#include "ui.h"
#include "chek_chop.h"  // ini_qiymat
#include <ctime>

// Saytdagi manzillar (urls.py) bilan mos sahifalar
enum Joy {
    J_KIRISH, J_BOSH, J_SOTUV, J_SOTUVLAR, J_HISOBOT,
    J_QARZDORLAR, J_QARZDOR_ROYXAT, J_HUDUDLAR, J_HUDUD, J_KARTA, J_YANGI_QARZDOR, J_QIDIRISH,
    J_OMBOR, J_MAHSULOT, J_QURILMALAR, J_KASSALAR, J_AKKAUNT, J_QARZ_YOZ,
    J_EXCEL, J_YANGILANISHLAR, J_YANGILANISH, J_QATOR,
    J_ULANISH, J_SERVER, J_PRINTER, J_ETIKETKA,
    J_SONI
};

void ot(Joy j, long long param = 0);  // sahifaga o'tish
Joy joriy_joy();
wstring versiya();

extern Foydalanuvchi foydalanuvchi;
extern bool sensor;  // sensorli rejim: raqamlar klaviaturasi ko'rinadi
void chiqish();

// sozlama.ini (shu kompyuterning o'zi: server manzili, sessiya, mavzu, printer)
void ini_saqla(const wchar_t* kalit, const wstring& v);
// Serverga ulangach: saqlangan sessiya bo'lsa ichkariga (uzilgan sahifaga), aks holda Kirish
void ulanganda();
void ulanish_ochildi(const wstring& xato);
void ulanish_chiz(R r);
void server_chiz(R r);
void kirish_ochildi();
void xavfsizlik_devori();
bool administratormi();
extern int server_porti;
// Printerlar sahifasi (printer.cpp): har bir printerga «chek / etiketka / ishlatilmaydi»
bool printer_soralsinmi();          // ochilganda so'rash kerakmi (yangi yoki yo'qolgan printer)
void printer_ochildi(bool boshida);
void printer_chiz(R r);
wstring printer_qisqacha();
void printerlar_tayyor();           // main.cpp: «Davom etish» bosilganda
wstring malumot_papka();            // sozlama.ini turgan papka (hisobot, drayverlar)

// Etiketka (etiketka.cpp): rasta uchun narx yorlig'i
constexpr int ETIKETKA_KOPI = 500;  // bir yo'la chop etiladigan yorliqlar chegarasi
// Nechta yorliq kerak: sanaladigan tovarning har biriga bittadan (kg, metr, litr — bitta)
int etiketka_soni(const Tovar& t, double miqdor, const wstring& birlik);
void etiketka_och(long long tovar_id, int soni);
void etiketka_ochildi(long long id);
void etiketka_chiz(R r);
// Rongta tarozisi paneli (tarozi.cpp) — Qurilmalar sahifasida
void tarozi_ochildi();
float tarozi_panel(float x, float y, float w);  // chizilgan balandlik

// Sahifa funksiyalari (har biri o'z .cpp faylida)
void kirish_chiz(R r);
void bosh_chiz(R r);
void sotuv_ochildi(long long p);
void sotuv_chiz(R r);
void qarz_yozish_ochildi(long long qarz_id);  // sotuv.cpp — o'sha kassa ekrani
void hisobot_ochildi(long long p);
void hisobot_chiz(R r);
void qarzdorlar_ochildi(long long p);
void qarzdorlar_chiz(R r);
void qarzdor_royxat_ochildi(long long p);
void qarzdor_royxat_chiz(R r);
void hududlar_ochildi(long long p);
void hududlar_chiz(R r);
void karta_ochildi(long long p);
void karta_chiz(R r);
void yangi_qarzdor_ochildi(long long p);
void yangi_qarzdor_chiz(R r);
void qidirish_ochildi(long long p);
void qidirish_chiz(R r);
void ombor_ochildi(long long p);
void ombor_chiz(R r);
void mahsulot_ochildi(long long p);
void mahsulot_chiz(R r);
void sotuvlar_ochildi(long long p);
void sotuvlar_chiz(R r);
void excel_yuklash_ochildi(long long p);
void excel_yuklash_chiz(R r);
void yangilanishlar_ochildi(long long p);
void yangilanishlar_chiz(R r);
void yangilanish_ochildi(long long p);
void yangilanish_chiz(R r);
void qator_ochildi(long long p);
void qator_chiz(R r);
void ombor_shablon();
void qurilmalar_ochildi(long long p);
void qurilmalar_chiz(R r);
void akkaunt_ochildi(long long p);
void akkaunt_chiz(R r);
void tayyorlanmoqda_chiz(R r, const wstring& nom, const char* ikon);
void ombor_eskirdi();  // tovarlar o'zgardi — kassa qidiruvi qayta o'qisin

// Umumiy bezaklar (sahifalar.cpp)
void sarlavha(R& r, const wstring& matn, const wstring& kichik = L"");  // .sarlavha, r ni pastga suradi
void quti(R r);                                // .quti foni
void bosh_holat(R r, const char* ikon, const wstring& b, const wstring& matn);
// .kiritish-maydon / .tolov-maydon: chapda nomi, o'ngda qiymat (EDIT) va birlik
HWND tolov_maydon(const string& id, R r, const wstring& nomi, const wstring& birlik,
                  float olcham, Rang chegara = {}, Rang nom_rang = {}, bool faol = false);
// Raqamlar klaviaturasi (.raqamlar): bosilgan belgini nishon maydoniga yozadi
void raqamlar(const string& id, R r, const string& nishon, bool vergul = true);
// Matnni songa: bo'sh — 0
bool sonmi(const wstring& s, double& v);
// .oyna-fon + .oyna + .oyna-bosh: modal oyna; tana (body) to'rtburchagini qaytaradi.
// yopildi — «Yopish (Esc)» bosildi yoki Esc.
R oyna(const string& id, const wstring& sarlavha, float max_en, float boy, bool& yopildi);
// «pul-maydon»: yozilgan zahoti mingliklar guruhlanadi (200000 -> 200 000)
void pul_guruhla(const string& id);
// Ishchi paneldagi kichik yorliq (F2, Enter)
void yorliq_belgi(float x, float cy, const wstring& t, Rang rang);
// .korsatkich: tur 0 oddiy, 1 diqqat (ko'k), 2 ogoh (sariq)
void korsatkich(R r, const wstring& nomi, const wstring& qiymat, const wstring& dollar = L"", int tur = 0);
// Jadval ustunlari: en > 0 — piksel, en < 0 — qolgan joydan ulush
struct Ustun {
    wstring nom;
    float en;
    bool son = false;
};
std::vector<R> ustunlar(R satr, const std::vector<Ustun>& u);
void jadval_sarlavha(R r, const std::vector<Ustun>& u);
// .tanlov (oddiy sahifalardagi katta tugma): tur 0 oddiy, 2 qarz (ko'k), 3 yangi (yashil)
bool tanlov_karta(const string& id, R r, const wstring& nom, const wstring& izoh, const char* ikon, int tur);
