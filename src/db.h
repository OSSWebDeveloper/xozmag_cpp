// Baza qatlami: SQLite o'rami va Xozmag'ning barcha ma'lumot amallari.
//
// Jadvallar Django'dagi Xozmag bilan AYNAN BIR XIL (ombor_mahsulot, qarz_qarz,
// sotuv_sotuv ...). Shuning uchun eski db.sqlite3 dan ma'lumot to'g'ridan-to'g'ri
// ko'chiriladi va qoidalar ham o'sha: so'm va dollar hech qayerda qo'shilmaydi,
// ombor faqat chek yakunlanganda kamayadi, chek summasi savdolashib o'zgaradi.
#pragma once
#include <stdexcept>
#include <string>
#include <vector>

#include "sqlite_min.h"
#include "util.h"

struct BazaXato : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Server bilan almashinadigan qiymat (masofaviy rejimda bog'lash va natija)
struct Qiymat {
    char tur = 0;  // 0 NULL, 1 butun, 2 son, 3 matn
    long long i = 0;
    double d = 0;
    string s;
};

class Baza;

// Tranzaksiya ichida natijasi kerak bo'lmagan so'rovlar navbatga yig'iladi va serverga
// bir yo'la ketadi (Wi-Fi'da har bir so'rov uchun alohida kutmaslik uchun).
struct Navbat {
    string sql;
    std::vector<Qiymat> par;
    bool exec = false;  // ko'p iborali (sqlite3_exec)
};

// So'rov. Mahalliy rejimda — sqlite3_stmt. Masofaviy rejimda (kassa serverga IP orqali
// ulangan) SQL va qiymatlar serverga yuboriladi, natija qatorlari bir yo'la keladi.
class Sorov {
public:
    Sorov(Baza& b, const string& sql);
    ~Sorov();
    Sorov(const Sorov&) = delete;
    Sorov& operator=(const Sorov&) = delete;

    Sorov& bog(int i, const string& v);
    Sorov& bog(int i, const wstring& v) { return bog(i, U(v)); }
    Sorov& bog(int i, const char* v) { return bog(i, string(v)); }
    Sorov& bog(int i, double v);
    Sorov& bog(int i, long long v);
    Sorov& bog(int i, int v) { return bog(i, (long long)v); }
    Sorov& bosh(int i);

    bool qator();     // keyingi qator bo'lsa true
    void bajar();     // natijasiz so'rov
    void qayta();     // qayta ishlatish uchun

    string matnU(int i);
    wstring matn(int i) { return W(matnU(i)); }
    double son(int i);
    long long butun(int i);
    bool bormi(int i);   // NULL emasmi

private:
    Baza& b_;
    sqlite3_stmt* st_ = nullptr;
    // masofaviy
    string sql_;
    std::vector<Qiymat> par_, nat_;
    size_t ustun_ = 0, qatorlar_ = 0, joriy_ = 0;
    bool bajarildi_ = false;
    void qoy(int i, Qiymat q);
    const Qiymat& q(int i);
};

class Baza {
public:
    ~Baza();
    void och(const wstring& fayl);
    void yop();
    void bajar(const string& sql);
    Sorov sor(const string& sql) { return Sorov(*this, sql); }
    long long oxirgi_id();
    long long ozgarish();  // oxirgi so'rov o'zgartirgan qatorlar soni
    sqlite3* h() { return h_; }
    wstring fayl() const { return fayl_; }
    // Masofaviy rejim: barcha so'rovlar serverga (aloqa.h)
    void uzoq_qoy(bool ha) { uzoq_ = ha; }
    bool uzoqmi() const { return uzoq_; }
    long long uzoq_id = 0, uzoq_ozgarish = 0;
    bool toplash() const { return toplash_; }
    void navbatga(const string& sql, const std::vector<Qiymat>& par);
    void yubor();  // navbatdagilarni serverga

private:
    sqlite3* h_ = nullptr;
    wstring fayl_;
    bool uzoq_ = false, toplash_ = false;
    std::vector<Navbat> navbat_;
};

extern Baza baza;

// Tranzaksiya: obyekt yo'q bo'lganda tasdiqlanmagan bo'lsa bekor qilinadi.
class Tranzaksiya {
public:
    Tranzaksiya() { baza.bajar("BEGIN IMMEDIATE"); }
    ~Tranzaksiya() {
        if (!tayyor_) try { baza.bajar("ROLLBACK"); } catch (...) {}
    }
    void tasdiq() { baza.bajar("COMMIT"); tayyor_ = true; }

private:
    bool tayyor_ = false;
};

// ---------- Tovarlar ----------

struct Tovar {
    long long id = 0;
    wstring kod, nom, birlik = L"dona", valyuta = L"som";
    wstring olish_birligi;
    double olish_miqdori = 1;
    double narx = 0, qoldiq = 0;
    bool kelish_bor = false;
    double kelish_narxi = 0;
    wstring kelish_valyutasi;
    bool faol = true;
    wstring tarozi_kodi;
    wstring izlash;  // kichik harflardagi "kod nom" — tez qidiruv uchun

    bool dollarmi() const { return valyuta == L"dollar"; }
    wstring belgi() const { return dollarmi() ? L"$" : L"so'm"; }
};

extern const std::vector<wstring> BIRLIKLAR;
constexpr int KOD_MAX = 9999;

wstring kod_yasa(long long raqam);       // 7 -> "0007"
wstring keyingi_kod();
std::vector<Tovar> tovarlarni_ol(bool yashirinlar_ham);
bool tovar_ol(long long id, Tovar& t);
// Kassadagi yagona qidiruv: kod (27 = 0027), eski 5/6/10 xonali etiketka,
// tarozi shtrixi (13 raqam, "2" bilan; ichida og'irlik). miqdor < 0 — kod miqdor aytmadi.
bool kod_bilan_top(const wstring& kirish, Tovar& t, double& miqdor, wstring& xato);
long long tovar_saqla(const Tovar& t, double boshlangich_qoldiq, wstring& xato);
void tovar_faol(long long id, bool faol);
void kirim_qil(long long id, double miqdor, double kiritilgan, const wstring& kiritilgan_birlik,
               const wstring& izoh);
void qoldiq_tuzat(long long id, double yangi, const wstring& izoh);

struct Harakat {
    wstring sana, tur, miqdor, izoh;
};
std::vector<Harakat> harakatlar(long long tovar_id);

wstring kichik(const wstring& s);
bool mos_keladi(const wstring& izlash, const std::vector<wstring>& bolaklar);
std::vector<wstring> bolakla(const wstring& qidiruv);

// ---------- Qarz daftari ----------

struct Hudud {
    long long id;
    wstring nom;
};
std::vector<Hudud> hududlar();
long long hudud_qosh(const wstring& nom, wstring& xato);
bool hudud_nomla(long long id, const wstring& nom, wstring& xato);

struct Qarzdor {
    long long id = 0;
    wstring ism, familiya, telefon, izoh;
    long long hudud_id = 0;
    wstring hudud;
    double balans = 0, balans_d = 0;
    wstring toliq() const { return familiya + L" " + ism; }
};
std::vector<Qarzdor> qarzdorlar(const wstring& qidiruv);
bool qarzdor_ol(long long id, Qarzdor& q);
long long qarzdor_saqla(const Qarzdor& q, wstring& xato);

struct TarixQator {
    string sana_utc;
    wstring hujjat, tafsilot;
    double som = 0, dollar = 0;  // qarz musbat, to'lov manfiy
};
std::vector<TarixQator> qarzdor_tarixi(long long qarzdor_id);
void tolov_qosh(long long qarzdor_id, double summa, const wstring& valyuta, const wstring& izoh);

// ---------- Sotuv (kassa) ----------

struct SavatQator {
    Tovar t;
    double miqdor = 0;
    double summa() const { return t.narx * miqdor; }
};

struct ChekNatija {
    long long sotuv_id = 0;
    long long qarz_id = 0;
};

// Chekni yopadi va omborni bitta tranzaksiyada ayiradi. Mijozda pul yetmasa
// qolgani qarzdorning daftariga yoziladi (qarz_qarz.sotuv_id = chek).
bool sotuv_yakunla(const std::vector<SavatQator>& savat, double jami, double jami_d,
                   double tolandi, double tolandi_d, double kurs, long long qarzdor_id,
                   ChekNatija& natija, wstring& xato);

struct ChekQator {
    wstring nom, birlik, valyuta;
    double miqdor = 0, narx = 0;  // miqdor — qaytarilganidan keyin qolgani
    bool narx_bor = false;
    long long id = 0;             // sotuv_sotuvqator.id
    double qaytarilgan = 0;
    wstring kod;                  // tovar kodi
};
struct Chek {
    long long id = 0;
    string sana_utc;
    std::vector<ChekQator> qatorlar;
    double naqd = 0, naqd_d = 0, qarz = 0, qarz_d = 0;
    wstring qarzdor;
    bool bekor = false;
};
bool chek_ol(long long sotuv_id, Chek& c);

// ---------- Hisobot ----------

struct KunHisobi {
    int cheklar = 0;
    double naqd = 0, naqd_d = 0;
    double qarz = 0, qarz_d = 0;
    double tolov = 0, tolov_d = 0;
};
struct ChekQisqa {
    long long id;
    string sana_utc;
    int qatorlar;
    double naqd, naqd_d, qarz, qarz_d;
    wstring qarzdor;
};
KunHisobi kun_hisobi(const SYSTEMTIME& kun, std::vector<ChekQisqa>& cheklar);

// ---------- Sozlamalar, kurs, import ----------

wstring sozlama(const string& kalit, const wstring& standart = L"");
void sozlama_yoz(const string& kalit, const wstring& qiymat);
double dollar_kursi();
void kurs_saqla(double kurs);

wstring zaxira_ol(const wstring& papka);  // fayl yo'lini qaytaradi
bool eski_bazani_import(const wstring& fayl, wstring& hisobot);

struct Sanoq {
    long long tovar = 0, qarzdor = 0, sotuv = 0, qarz = 0;
};
Sanoq sanoq();

// ======================= v2: saytdagi mantiq =======================

// ---- Foydalanuvchilar (Django auth_user, parol pbkdf2_sha256) ----
struct Foydalanuvchi {
    long long id = 0;
    wstring login;
    bool superuser = false;
};
long long foydalanuvchilar_soni();
// sessiya=false — faqat parolni tekshirish (masofaviy rejimda yangi sessiya ochilmaydi)
bool kirish_tekshir(const wstring& login, const wstring& parol, Foydalanuvchi& f, wstring& xato,
                    bool sessiya = true);
bool parol_mos(const wstring& parol, const string& hash);
bool foydalanuvchi_ol(long long id, Foydalanuvchi& f);
bool foydalanuvchi_yarat(const wstring& login, const wstring& parol, bool superuser, wstring& xato);
bool parol_almashtir(long long id, const wstring& eski, const wstring& yangi, wstring& xato);
bool login_almashtir(long long id, const wstring& yangi, wstring& xato);
string parol_hash(const wstring& parol);
bool django_bazasimi();

// ---- Kassa: chek bazada turadi (sotuv_sotuv yakunlangan=0) ----
struct Qator {
    long long id = 0, mahsulot_id = 0;
    wstring kod, nom, birlik, valyuta;
    double miqdor = 0, narx = 0, qoldiq = 0;
    bool narx_bor = false;
    bool dollarmi() const { return valyuta == L"dollar"; }
};
long long ochiq_sotuv(long long kassir_id);         // bor bo'lsa o'sha, yo'q bo'lsa yangi
string sotuv_sanasi(long long sotuv_id);
std::vector<Qator> sotuv_qatorlari(long long sotuv_id);
// Bir xil tovar bitta qatorga yig'iladi; qoldiq yetmasa xato
long long sotuv_qator_qosh(long long sotuv_id, long long mahsulot_id, double miqdor, wstring& xato);
bool sotuv_qator_miqdor(long long qator_id, double yangi, wstring& xato);
void sotuv_qator_ochir(long long qator_id);
// Bo'sh chek o'chiriladi (true), aks holda bekor qilingan deb belgilanadi
bool sotuv_bekor(long long sotuv_id);
struct AvtoHisob {
    double som = 0, dollar = 0;
    bool toliq = false;
    int narxsiz = 0;
};
AvtoHisob avto_hisob(const std::vector<Qator>& q);
// Yakunlash: jami — chekning to'liq summasi; qarzdor bo'lsa tolandi qismi naqd, qolgani qarz
bool sotuv_yakunla_v2(long long sotuv_id, long long kassir_id, double jami, double jami_d,
                      double kurs, long long qarzdor_id, double tolandi, double tolandi_d,
                      ChekNatija& n, wstring& xato);
std::vector<Tovar> oyna_tovarlari();  // eng ko'p sotiladigan 60 ta
double oxirgi_kurs();                 // bugungi bank kursi yoki oxirgi yozilgani

// ---- Baza fayli ----
wstring baza_yolini_aniqla();   // saqlangan yoki BizaroPOS bazasi yoki o'z bazasi
void baza_yolini_saqla(const wstring& yol);
wstring oz_baza_yoli();
wstring bizaropos_baza_yoli();

// ---- Qarz yozish (qarz_qarz + qarz_qarzqator) — kassa bilan bir xil ekran ----
enum HujjatTur { H_SOTUV, H_QARZ };
std::vector<Qator> hujjat_qatorlari(HujjatTur t, long long id);
long long hujjat_qator_qosh(HujjatTur t, long long id, long long mahsulot_id, double miqdor, wstring& xato);
bool hujjat_qator_miqdor(HujjatTur t, long long qator_id, double yangi, wstring& xato);
void hujjat_qator_ochir(HujjatTur t, long long qator_id);
long long ochiq_qarz(long long qarzdor_id, long long kassir_id);
long long qarz_qarzdori(long long qarz_id);
string qarz_sanasi(long long qarz_id);
// Bo'sh bo'lsa hujjat o'chiriladi (true)
bool qarz_bosh_bolsa_ochir(long long qarz_id);
bool qarz_yakunla(long long qarz_id, double jami, double jami_d, double kurs, double oldindan,
                  double oldindan_d, wstring& xato, wstring& tolandi);
