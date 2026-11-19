// Kassa <-> server aloqasi (TCP, ichki tarmoq).
//
// Do'kondagi bitta kompyuterda «Xozmag Server» (Xozmag.exe --server) bazani ochib turadi.
// Kassalardagi Xozmag.exe unga IP manzil orqali ulanadi: har bir SQL so'rov serverga
// boradi, natija qaytadi. Kirish (login/parol) server tomonida tekshiriladi va sessiya
// beriladi: sessiyasiz so'rov bajarilmaydi.
#pragma once
#include <stdexcept>

#include "db.h"

constexpr int STANDART_PORT = 5577;

// Tarmoq uzildi (server o'chdi, kabel/Wi-Fi). BazaXato emas — sahifalar uni «saqlanmadi»
// deb yutib yubormasin; dastur ulanish oynasiga qaytadi.
struct AloqaXato : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// ---------- Kassa (mijoz) ----------
bool ulan(const wstring& manzil, wstring& xato);  // "192.168.1.5" yoki "192.168.1.5:5577"
void uzil();
bool ulangan();
bool aloqa_uzildi();      // ulangan edi, keyin uzildi
wstring ulangan_manzil();
bool ping();              // jim tekshiruv; uzilgan bo'lsa false
bool tirikmi();           // 5 soniyadan beri javob bo'lmasa ping qiladi

void uzoq_sql(const string& sql, const std::vector<Qiymat>& par, std::vector<Qiymat>& nat,
              size_t& ustun, long long& ozgarish, long long& oxirgi_id);
void uzoq_exec(const string& sql, long long& ozgarish);
void uzoq_toplam(const std::vector<Navbat>& n, long long& ozgarish, long long& oxirgi_id);

// Og'ir amallar (minglab qator) serverning o'zida bajariladi: tarmoqdan har qator uchun
// alohida so'rov ketmaydi. amal: 1 qo'llash, 2 bekor qilish.
enum ServerAmal { SA_QOLLASH = 1, SA_BEKOR = 2 };
bool uzoq_amal(ServerAmal amal, long long id, std::vector<wstring>& xatolar);
struct Toqnash;
std::vector<Toqnash> uzoq_toqnashuvlar(long long id);

long long uzoq_foydalanuvchilar_soni();
bool uzoq_kirish(const wstring& login, const wstring& parol, Foydalanuvchi& f, wstring& xato,
                 bool sessiya);
bool uzoq_birinchi(const wstring& login, const wstring& parol, wstring& xato);
bool uzoq_sessiya(Foydalanuvchi& f);  // saqlangan token bilan kirish
void uzoq_chiqish();
bool uzoq_kirgan();
string& sessiya_tokeni();

// ---------- Server ----------
struct Mijoz {
    wstring ip, login;
    long long ulangan = 0, oxirgi = 0, sorovlar = 0;
};
bool server_boshla(int port, const wstring& baza_fayl, wstring& xato);
bool server_baza_almashtir(const wstring& baza_fayl, wstring& xato);  // hamma kassalar qayta ulanadi
std::vector<Mijoz> server_mijozlari();
long long server_sorovlari();
void server_kuzatuvchi(HWND oyna);  // mijozlar o'zgarsa shu oyna qayta chiziladi
