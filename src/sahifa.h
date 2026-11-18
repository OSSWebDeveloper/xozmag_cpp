// Asosiy oynadagi bo'limlar (sahifalar) uchun umumiy asos.
#pragma once
#include "db.h"
#include "ui.h"

struct Sahifa {
    HWND h = nullptr;
    virtual ~Sahifa() {}
    virtual void yarat() = 0;                 // h ichida elementlarni yaratadi
    virtual void joylash(int w, int hgt) = 0;  // o'lcham o'zgardi
    virtual void korsatildi() {}              // bo'limga o'tildi — ma'lumotni yangilash
    virtual bool kalit(MSG&) { return false; }  // klaviatura (true — ishlandi)
    virtual void buyruq(int id, int kod, HWND el) {}
    virtual LRESULT xabarnoma(NMHDR*) { return 0; }
    virtual HBRUSH rang(HDC, HWND) { return nullptr; }  // WM_CTLCOLORSTATIC
};

void sahifa_sinfi();
HWND sahifa_yarat(Sahifa* s, HWND ota);

enum BolimRaqami { B_KASSA = 0, B_QARZ, B_OMBOR, B_HISOBOT, B_SOZLAMA, B_SONI };
void bolimga_ot(int i);
wstring versiya();
wstring malumot_papkasi();

Sahifa* kassa_sahifa();
Sahifa* qarz_sahifa();
Sahifa* ombor_sahifa();
Sahifa* hisobot_sahifa();
Sahifa* sozlama_sahifa();

void kassa_qarzdor_qoy(long long qarzdor_id);  // qarz daftaridan «Qarz yozish»
bool kassa_savat_bormi();                     // kassada yakunlanmagan chek bormi
void ombor_eskirdi();                         // tovarlar o'zgardi — keyingi safar qayta o'qilsin

// Dialoglar (dialoglar.cpp)
long long tovar_tanla(HWND ota);                     // 0 — tanlanmadi
long long qarzdor_tanla(HWND ota);                   // 0 — tanlanmadi
long long qarzdor_formasi(HWND ota, long long id);   // yangi (id=0) yoki tahrir; 0 — bekor
bool tovar_formasi(HWND ota, long long id);          // yangi (id=0) yoki tahrir
void hududlar_oynasi(HWND ota);
void chek_oynasi(HWND ota, long long sotuv_id);

// Chek (chek.cpp)
wstring chek_matni(const Chek& c);
bool chek_chop(const Chek& c, wstring& xato);
std::vector<wstring> printerlar();
