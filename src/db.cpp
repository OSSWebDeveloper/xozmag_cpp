#include "db.h"
#include "aloqa.h"

#include <algorithm>
#include <set>

Baza baza;

const std::vector<wstring> BIRLIKLAR = {L"dona", L"kg",    L"metr",   L"litr", L"qop",
                                        L"quti", L"rulon", L"o'ram",  L"pachka", L"list",
                                        L"tonna", L"kvm",  L"blok"};

// ======================= SQLite o'rami =======================
// Masofaviy rejimda (aloqa.h) so'rov birinchi qator()/bajar() da serverga ketadi.

Sorov::Sorov(Baza& b, const string& sql) : b_(b) {
    if (b.uzoqmi()) {
        sql_ = sql;
        return;
    }
    if (sqlite3_prepare_v2(b.h(), sql.c_str(), -1, &st_, nullptr) != SQLITE_OK)
        throw BazaXato(string(sqlite3_errmsg(b.h())) + "\nSQL: " + sql);
}
Sorov::~Sorov() { if (st_) sqlite3_finalize(st_); }

void Sorov::qoy(int i, Qiymat q) {
    if (i < 1) return;
    if ((int)par_.size() < i) par_.resize(i);
    par_[i - 1] = std::move(q);
}
Sorov& Sorov::bog(int i, const string& v) {
    if (b_.uzoqmi()) { Qiymat q; q.tur = 3; q.s = v; qoy(i, std::move(q)); return *this; }
    sqlite3_bind_text(st_, i, v.c_str(), (int)v.size(), SQLITE_TRANSIENT);
    return *this;
}
Sorov& Sorov::bog(int i, double v) {
    if (b_.uzoqmi()) { Qiymat q; q.tur = 2; q.d = v; qoy(i, q); return *this; }
    sqlite3_bind_double(st_, i, v);
    return *this;
}
Sorov& Sorov::bog(int i, long long v) {
    if (b_.uzoqmi()) { Qiymat q; q.tur = 1; q.i = v; qoy(i, q); return *this; }
    sqlite3_bind_int64(st_, i, v);
    return *this;
}
Sorov& Sorov::bosh(int i) {
    if (b_.uzoqmi()) { qoy(i, Qiymat{}); return *this; }
    sqlite3_bind_null(st_, i);
    return *this;
}
bool Sorov::qator() {
    if (b_.uzoqmi()) {
        if (!bajarildi_) {
            b_.yubor();  // avvalgi yozuvlar natijaga ta'sir qiladi
            uzoq_sql(sql_, par_, nat_, ustun_, b_.uzoq_ozgarish, b_.uzoq_id);
            qatorlar_ = ustun_ ? nat_.size() / ustun_ : 0;
            joriy_ = 0;
            bajarildi_ = true;
            return qatorlar_ > 0;
        }
        if (joriy_ + 1 < qatorlar_) { ++joriy_; return true; }
        joriy_ = qatorlar_;
        return false;
    }
    int r = sqlite3_step(st_);
    if (r == SQLITE_ROW) return true;
    if (r == SQLITE_DONE) return false;
    throw BazaXato(sqlite3_errmsg(b_.h()));
}
void Sorov::bajar() {
    if (b_.uzoqmi()) {
        if (bajarildi_) return;
        if (b_.toplash()) {  // tranzaksiya ichida: navbatga, COMMIT bilan birga ketadi
            b_.navbatga(sql_, par_);
            bajarildi_ = true;
            ustun_ = qatorlar_ = joriy_ = 0;
            return;
        }
        qator();
        return;
    }
    while (qator()) {}
}
void Sorov::qayta() {
    if (b_.uzoqmi()) {
        bajarildi_ = false;
        nat_.clear();
        qatorlar_ = joriy_ = 0;
        return;
    }
    sqlite3_reset(st_);
}

const Qiymat& Sorov::q(int i) {
    static const Qiymat bosh_q;
    if (!bajarildi_ || joriy_ >= qatorlar_ || i < 0 || (size_t)i >= ustun_) return bosh_q;
    return nat_[joriy_ * ustun_ + i];
}
string Sorov::matnU(int i) {
    if (b_.uzoqmi()) {
        const Qiymat& v = q(i);
        if (v.tur == 3) return v.s;
        if (v.tur == 1) return std::to_string(v.i);
        if (v.tur == 2) {  // SQLite'dagidek: "%!.15g" — butun bo'lsa ham ".0"
            char b[40];
            snprintf(b, sizeof b, "%.15g", v.d);
            string r = b;
            if (r.find_first_of(".eEn") == string::npos) r += ".0";
            return r;
        }
        return {};
    }
    const unsigned char* p = sqlite3_column_text(st_, i);
    return p ? string((const char*)p) : string();
}
double Sorov::son(int i) {
    if (b_.uzoqmi()) {
        const Qiymat& v = q(i);
        return v.tur == 2 ? v.d : v.tur == 1 ? (double)v.i : v.tur == 3 ? strtod(v.s.c_str(), nullptr) : 0;
    }
    return sqlite3_column_double(st_, i);
}
long long Sorov::butun(int i) {
    if (b_.uzoqmi()) {
        const Qiymat& v = q(i);
        if (v.tur == 1) return v.i;
        if (v.tur == 2) return (long long)v.d;
        if (v.tur == 3) {  // SQLite: matn son bo'lsa (masalan "12.0") o'shani oladi
            char* e = nullptr;
            long long n = strtoll(v.s.c_str(), &e, 10);
            if (e && (*e == '.' || *e == 'e' || *e == 'E')) return (long long)strtod(v.s.c_str(), nullptr);
            return n;
        }
        return 0;
    }
    return sqlite3_column_int64(st_, i);
}
bool Sorov::bormi(int i) {
    if (b_.uzoqmi()) return q(i).tur != 0;
    return sqlite3_column_type(st_, i) != SQLITE_NULL;
}

// Django jadvallari — o'zgartirmasdan (eski baza bilan bir xil bo'lishi shart).
static const char* SXEMA = R"SQL(
CREATE TABLE IF NOT EXISTS "ombor_mahsulot" ("id" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "nom" varchar(120) NOT NULL UNIQUE, "birlik" varchar(10) NOT NULL, "qoldiq" decimal NOT NULL, "faol" bool NOT NULL, "yaratilgan" datetime NOT NULL, "olish_birligi" varchar(10) NOT NULL, "olish_miqdori" decimal NOT NULL, "valyuta" varchar(10) NOT NULL, "narx" decimal NOT NULL, "kod" varchar(4) NOT NULL UNIQUE, "kelish_narxi" decimal NULL, "kelish_valyutasi" varchar(10) NOT NULL, "manba" text NOT NULL CHECK ((JSON_VALID("manba") OR "manba" IS NULL)), "tarozi_kodi" varchar(6) NULL UNIQUE, "shtrix_kod" varchar(14) NULL UNIQUE);
CREATE TABLE IF NOT EXISTS "ombor_omborharakati" ("id" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "tur" varchar(10) NOT NULL, "miqdor" decimal NOT NULL, "izoh" varchar(200) NOT NULL, "sana" datetime NOT NULL, "mahsulot_id" bigint NOT NULL REFERENCES "ombor_mahsulot" ("id") DEFERRABLE INITIALLY DEFERRED, "kiritilgan_birlik" varchar(10) NOT NULL, "kiritilgan_miqdor" decimal NULL);
CREATE TABLE IF NOT EXISTS "ombor_dollarkursi" ("id" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "sana" date NOT NULL UNIQUE, "kurs" decimal NOT NULL, "urinish" datetime NOT NULL);
CREATE TABLE IF NOT EXISTS "qarz_hudud" ("id" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "nom" varchar(80) NOT NULL UNIQUE, "tartib" smallint unsigned NOT NULL CHECK ("tartib" >= 0));
CREATE TABLE IF NOT EXISTS "qarz_qarzdor" ("id" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "ism" varchar(60) NOT NULL, "familiya" varchar(60) NOT NULL, "telefon" varchar(25) NOT NULL, "izoh" varchar(200) NOT NULL, "yaratilgan" datetime NOT NULL, "hudud_id" bigint NOT NULL REFERENCES "qarz_hudud" ("id") DEFERRABLE INITIALLY DEFERRED);
CREATE TABLE IF NOT EXISTS "sotuv_sotuv" ("id" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "sana" datetime NOT NULL, "izoh" varchar(200) NOT NULL, "yakunlangan" bool NOT NULL, "jami" decimal NOT NULL, "jami_dollar" decimal NOT NULL, "kurs" decimal NOT NULL, "bekor_qilingan" bool NOT NULL, "qaytarilgan_summa" decimal NOT NULL, "qaytarilgan_summa_dollar" decimal NOT NULL, "kassir_id" integer NULL REFERENCES "auth_user" ("id") DEFERRABLE INITIALLY DEFERRED);
CREATE TABLE IF NOT EXISTS "qarz_qarz" ("id" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "sana" datetime NOT NULL, "izoh" varchar(200) NOT NULL, "yakunlangan" bool NOT NULL, "qarzdor_id" bigint NOT NULL REFERENCES "qarz_qarzdor" ("id") DEFERRABLE INITIALLY DEFERRED, "jami" decimal NOT NULL, "jami_dollar" decimal NOT NULL, "kurs" decimal NOT NULL, "qaytarilgan_summa" decimal NOT NULL, "qaytarilgan_summa_dollar" decimal NOT NULL, "sotuv_id" bigint NULL UNIQUE REFERENCES "sotuv_sotuv" ("id") DEFERRABLE INITIALLY DEFERRED, "kassir_id" integer NULL REFERENCES "auth_user" ("id") DEFERRABLE INITIALLY DEFERRED);
CREATE TABLE IF NOT EXISTS "qarz_qarzqator" ("id" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "mahsulot_nomi" varchar(120) NOT NULL, "birlik" varchar(10) NOT NULL, "miqdor" decimal NOT NULL, "mahsulot_id" bigint NOT NULL REFERENCES "ombor_mahsulot" ("id") DEFERRABLE INITIALLY DEFERRED, "qarz_id" bigint NOT NULL REFERENCES "qarz_qarz" ("id") DEFERRABLE INITIALLY DEFERRED, "qaytarilgan" decimal NOT NULL, "narx" decimal NULL, "valyuta" varchar(10) NOT NULL);
CREATE TABLE IF NOT EXISTS "qarz_tolov" ("id" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "summa" decimal NOT NULL, "sana" datetime NOT NULL, "izoh" varchar(200) NOT NULL, "qarzdor_id" bigint NOT NULL REFERENCES "qarz_qarzdor" ("id") DEFERRABLE INITIALLY DEFERRED, "valyuta" varchar(10) NOT NULL, "qarz_id" bigint NULL REFERENCES "qarz_qarz" ("id") DEFERRABLE INITIALLY DEFERRED);
CREATE TABLE IF NOT EXISTS "sotuv_sotuvqator" ("id" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "mahsulot_nomi" varchar(120) NOT NULL, "birlik" varchar(10) NOT NULL, "miqdor" decimal NOT NULL, "mahsulot_id" bigint NOT NULL REFERENCES "ombor_mahsulot" ("id") DEFERRABLE INITIALLY DEFERRED, "sotuv_id" bigint NOT NULL REFERENCES "sotuv_sotuv" ("id") DEFERRABLE INITIALLY DEFERRED, "qaytarilgan" decimal NOT NULL, "narx" decimal NULL, "valyuta" varchar(10) NOT NULL);
CREATE TABLE IF NOT EXISTS "auth_user" ("id" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "password" varchar(128) NOT NULL, "last_login" datetime NULL, "is_superuser" bool NOT NULL, "username" varchar(150) NOT NULL UNIQUE, "last_name" varchar(150) NOT NULL, "email" varchar(254) NOT NULL, "is_staff" bool NOT NULL, "is_active" bool NOT NULL, "date_joined" datetime NOT NULL, "first_name" varchar(150) NOT NULL);
CREATE TABLE IF NOT EXISTS "xozmag_sozlama" ("kalit" text NOT NULL PRIMARY KEY, "qiymat" text NOT NULL);
CREATE TABLE IF NOT EXISTS "xozmag_sessiya" ("token" text NOT NULL PRIMARY KEY, "user_id" integer NOT NULL, "yaratilgan" integer NOT NULL, "oxirgi" integer NOT NULL, "kompyuter" text NOT NULL);
CREATE INDEX IF NOT EXISTS "x_harakat_mahsulot" ON "ombor_omborharakati" ("mahsulot_id");
CREATE INDEX IF NOT EXISTS "x_qarzdor_hudud" ON "qarz_qarzdor" ("hudud_id");
CREATE INDEX IF NOT EXISTS "x_qarz_qarzdor" ON "qarz_qarz" ("qarzdor_id");
CREATE INDEX IF NOT EXISTS "x_qarz_sana" ON "qarz_qarz" ("sana");
CREATE INDEX IF NOT EXISTS "x_qarzqator_qarz" ON "qarz_qarzqator" ("qarz_id");
CREATE INDEX IF NOT EXISTS "x_tolov_qarzdor" ON "qarz_tolov" ("qarzdor_id");
CREATE INDEX IF NOT EXISTS "x_tolov_sana" ON "qarz_tolov" ("sana");
CREATE INDEX IF NOT EXISTS "x_sotuv_sana" ON "sotuv_sotuv" ("sana");
CREATE INDEX IF NOT EXISTS "x_sotuvqator_sotuv" ON "sotuv_sotuvqator" ("sotuv_id");
)SQL";

Baza::~Baza() { yop(); }

void Baza::och(const wstring& fayl) {
    yop();
    sqlite3_initialize();  // Python'dagi sqlite3.dll o'zi ishga tushmaydi
    if (sqlite3_open_v2(U(fayl).c_str(), &h_,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI,
                        nullptr) != SQLITE_OK) {
        string x = h_ ? sqlite3_errmsg(h_) : "baza ochilmadi";
        yop();
        throw BazaXato(x);
    }
    fayl_ = fayl;
    sqlite3_busy_timeout(h_, 30000);
    // Django (sayt) bazasi bo'lsa uning sozlamalariga tegilmaydi: sayt ham shu
    // faylni ishlatishi mumkin.
    bool django = false;
    {
        Sorov d = sor("SELECT count(*) FROM sqlite_master WHERE name='django_migrations'");
        django = d.qator() && d.butun(0) > 0;
    }
    if (!django) bajar("PRAGMA journal_mode=WAL");
    bajar(SXEMA);
    Sorov s = sor("SELECT count(*) FROM qarz_hudud");
    if (!django && s.qator() && s.butun(0) == 0) {
        // Tumanda 13 ta hudud bor; haqiqiy nomlari keyin o'zgartiriladi.
        for (int i = 1; i <= 13; ++i)
            sor("INSERT INTO qarz_hudud (nom, tartib) VALUES (?, ?)")
                .bog(1, "Hudud " + std::to_string(i)).bog(2, i).bajar();
    }
}

void Baza::yop() {
    if (uzoq_) return;
    if (h_) sqlite3_close_v2(h_);
    h_ = nullptr;
}

void Baza::navbatga(const string& sql, const std::vector<Qiymat>& par) {
    navbat_.push_back({sql, par, false});
    if (navbat_.size() >= 3000) yubor();  // kadr juda katta bo'lmasin
}

void Baza::yubor() {
    if (navbat_.empty()) return;
    std::vector<Navbat> n;
    n.swap(navbat_);
    uzoq_toplam(n, uzoq_ozgarish, uzoq_id);
}

void Baza::bajar(const string& sql) {
    if (uzoq_) {
        size_t a = sql.find_first_not_of(" \t\r\n");
        string k = a == string::npos ? "" : sql.substr(a, 8);
        for (char& c : k) c = (char)toupper((unsigned char)c);
        if (k.rfind("ROLLBACK", 0) == 0) {  // navbat bekor — baribir qaytariladi
            navbat_.clear();
            toplash_ = false;
            uzoq_exec(sql, uzoq_ozgarish);
            return;
        }
        if (k.rfind("COMMIT", 0) == 0 || k.rfind("END", 0) == 0) {
            toplash_ = false;
            navbat_.push_back({sql, {}, true});
            yubor();
            return;
        }
        yubor();
        uzoq_exec(sql, uzoq_ozgarish);
        if (k.rfind("BEGIN", 0) == 0) toplash_ = true;
        return;
    }
    char* xato = nullptr;
    if (sqlite3_exec(h_, sql.c_str(), nullptr, nullptr, &xato) != SQLITE_OK) {
        string m = xato ? xato : "noma'lum xato";
        sqlite3_free(xato);
        throw BazaXato(m);
    }
}

long long Baza::oxirgi_id() {
    if (!uzoq_) return sqlite3_last_insert_rowid(h_);
    yubor();
    return uzoq_id;
}
long long Baza::ozgarish() {
    if (!uzoq_) return sqlite3_changes(h_);
    yubor();
    return uzoq_ozgarish;
}

// ======================= Qidiruv yordamchilari =======================

wstring kichik(const wstring& s) {
    wstring r = s;
    if (!r.empty()) CharLowerBuffW(r.data(), (DWORD)r.size());
    for (auto& c : r)
        if (c == 0x2018 || c == 0x2019 || c == 0x02BB || c == 0x02BC || c == L'`') c = L'\'';
    return r;
}

std::vector<wstring> bolakla(const wstring& qidiruv) {
    std::vector<wstring> r;
    wstring s = kichik(qidiruv), b;
    for (wchar_t c : s + L" ") {
        if (c == L' ') {
            if (!b.empty()) r.push_back(b);
            b.clear();
        } else
            b += c;
    }
    return r;
}

bool mos_keladi(const wstring& izlash, const std::vector<wstring>& bolaklar) {
    for (auto& b : bolaklar)
        if (izlash.find(b) == wstring::npos) return false;
    return true;
}

// ======================= Tovarlar =======================

wstring kod_yasa(long long raqam) {
    wchar_t b[16];
    swprintf(b, 16, L"%04lld", raqam);
    return b;
}

wstring keyingi_kod() {
    std::set<long long> band;
    Sorov s = baza.sor("SELECT kod FROM ombor_mahsulot");
    while (s.qator()) {
        wstring k = s.matn(0);
        if (raqammi(k) && k.size() < 10) band.insert(std::stoll(k));
    }
    long long eng = band.empty() ? 0 : *band.rbegin();
    if (eng < KOD_MAX) return kod_yasa(eng + 1);
    for (long long r = 1; r <= KOD_MAX; ++r)
        if (!band.count(r)) return kod_yasa(r);
    return L"";
}

static const char* TOVAR_USTUN =
    "id, kod, nom, birlik, valyuta, olish_birligi, olish_miqdori, narx, qoldiq, "
    "kelish_narxi, kelish_valyutasi, faol, COALESCE(tarozi_kodi, '') FROM ombor_mahsulot ";

static Tovar tovar_qatori(Sorov& s) {
    Tovar t;
    t.id = s.butun(0);
    t.kod = s.matn(1);
    t.nom = s.matn(2);
    t.birlik = s.matn(3);
    t.valyuta = s.matn(4);
    t.olish_birligi = s.matn(5);
    t.olish_miqdori = s.son(6);
    t.narx = s.son(7);
    t.qoldiq = s.son(8);
    t.kelish_bor = s.bormi(9);
    t.kelish_narxi = s.son(9);
    t.kelish_valyutasi = s.matn(10);
    t.faol = s.butun(11) != 0;
    t.tarozi_kodi = s.matn(12);
    t.izlash = kichik(t.kod + L" " + t.nom);
    return t;
}

std::vector<Tovar> tovarlarni_ol(bool yashirinlar_ham) {
    std::vector<Tovar> r;
    Sorov s = baza.sor(string("SELECT ") + TOVAR_USTUN +
                       (yashirinlar_ham ? "" : "WHERE faol=1 ") + "ORDER BY kod");
    while (s.qator()) r.push_back(tovar_qatori(s));
    return r;
}

bool tovar_ol(long long id, Tovar& t) {
    Sorov s = baza.sor(string("SELECT ") + TOVAR_USTUN + "WHERE id=?");
    s.bog(1, id);
    if (!s.qator()) return false;
    t = tovar_qatori(s);
    return true;
}

// EAN-13 nazorat raqami (12 raqam bo'yicha).
static int nazorat_raqami(const wstring& s) {
    int yig = 0;
    for (size_t i = 0; i < s.size(); ++i) yig += (s[i] - L'0') * (i % 2 ? 3 : 1);
    return (10 - yig % 10) % 10;
}

static bool kod_raqami_bilan(long long raqam, Tovar& t) {
    if (raqam < 1 || raqam > KOD_MAX) return false;
    wchar_t besh[16];
    swprintf(besh, 16, L"%05lld", raqam);
    Sorov s = baza.sor(string("SELECT ") + TOVAR_USTUN + "WHERE kod IN (?, ?) LIMIT 1");
    s.bog(1, kod_yasa(raqam)).bog(2, wstring(besh));
    if (!s.qator()) return false;
    t = tovar_qatori(s);
    return true;
}

bool kod_bilan_top(const wstring& kirish, Tovar& t, double& miqdor, wstring& xato) {
    miqdor = -1;
    wstring s;
    for (wchar_t c : kirish)
        if (c != L' ' && c != L'-' && c != L'\t' && c != L'\r' && c != L'\n') s += c;
    if (s.empty()) {
        xato = L"Kod yozilmadi.";
        return false;
    }
    bool topildi = false;
    // Zavod/maxsus shtrix (bazada yozilgan bo'lsa)
    {
        Sorov q = baza.sor(string("SELECT ") + TOVAR_USTUN + "WHERE shtrix_kod=? LIMIT 1");
        q.bog(1, s);
        if (q.qator()) {
            t = tovar_qatori(q);
            topildi = true;
        }
    }
    if (!topildi && !raqammi(s)) {
        xato = L"«" + s + L"» — notanish kod.";
        return false;
    }
    // Tarozi etiketkasi: 2 | PLU 6 | gramm 5 | nazorat
    if (!topildi && s.size() == 13 && s[0] == L'2' &&
        nazorat_raqami(s.substr(0, 12)) == s[12] - L'0') {
        wstring plu = s.substr(1, 6);
        int gramm = std::stoi(s.substr(7, 5));
        Sorov q = baza.sor(string("SELECT ") + TOVAR_USTUN + "WHERE tarozi_kodi=? LIMIT 1");
        q.bog(1, plu);
        if (q.qator()) {
            t = tovar_qatori(q);
            topildi = true;
        } else
            topildi = kod_raqami_bilan(std::stoll(plu), t);
        if (!topildi) {
            xato = L"Tarozi etiketkasidagi " + plu + L" tovar topilmadi.";
            return false;
        }
        if (gramm > 0) miqdor = gramm / 1000.0;
    }
    if (!topildi) {
        if (s.size() > 10) {
            xato = L"«" + s + L"» — notanish kod. Do'kon etiketkasini skanerlang.";
            return false;
        }
        long long raqam = std::stoll(s);
        topildi = kod_raqami_bilan(raqam, t);
        if (!topildi) {
            xato = (raqam >= 1 && raqam <= KOD_MAX ? kod_yasa(raqam) : s) +
                   L" kodli tovar topilmadi.";
            return false;
        }
    }
    if (!t.faol) {
        xato = t.kod + L" " + t.nom + L" — tovar yashirilgan (Ombor bo'limida oching).";
        return false;
    }
    return true;
}

static wstring unikal_xato(const BazaXato& e) {
    string m = e.what();
    if (m.find("ombor_mahsulot.nom") != string::npos) return L"Bunday nomli tovar allaqachon bor.";
    if (m.find("ombor_mahsulot.kod") != string::npos) return L"Bu kod boshqa tovarda band.";
    if (m.find("qarz_hudud.nom") != string::npos) return L"Bunday hudud allaqachon bor.";
    return W(m);
}

long long tovar_saqla(const Tovar& kirish, double boshlangich, wstring& xato) {
    Tovar t = kirish;
    t.nom = tozala(t.nom);
    t.kod = tozala(t.kod);
    if (t.nom.empty()) {
        xato = L"Tovar nomini yozing.";
        return 0;
    }
    if (t.kod.empty()) {
        t.kod = keyingi_kod();
        if (t.kod.empty()) {
            xato = L"Bo'sh kod qolmadi (0001…9999 hammasi band).";
            return 0;
        }
    } else {
        if (!raqammi(t.kod) || t.kod.size() > 5 || std::stoll(t.kod) < 1 ||
            std::stoll(t.kod) > KOD_MAX) {
            xato = L"Kod 1 dan 9999 gacha raqam bo'lsin (turgan joyi).";
            return 0;
        }
        t.kod = kod_yasa(std::stoll(t.kod));
    }
    if (t.olish_birligi == t.birlik) t.olish_birligi.clear();
    if (t.olish_birligi.empty())
        t.olish_miqdori = 1;
    else if (t.olish_miqdori <= 0) {
        xato = L"«Bittasida» noldan katta bo'lsin: 1 " + t.olish_birligi + L" da nechta " +
               t.birlik + L" bor.";
        return 0;
    }
    if (t.narx < 0 || boshlangich < 0) {
        xato = L"Narx va qoldiq manfiy bo'lmaydi.";
        return 0;
    }
    try {
        Tranzaksiya tr;
        long long id = t.id;
        if (id == 0) {
            Sorov s = baza.sor(
                "INSERT INTO ombor_mahsulot (nom, birlik, qoldiq, faol, yaratilgan, olish_birligi,"
                " olish_miqdori, valyuta, narx, kod, kelish_narxi, kelish_valyutasi, manba)"
                " VALUES (?,?,?,1,?,?,?,?,?,?,?,?,'{}')");
            s.bog(1, t.nom).bog(2, t.birlik).bog(3, boshlangich).bog(4, hozir_utc());
            s.bog(5, t.olish_birligi).bog(6, t.olish_miqdori).bog(7, t.valyuta).bog(8, t.narx);
            s.bog(9, t.kod);
            if (t.kelish_bor) s.bog(10, t.kelish_narxi); else s.bosh(10);
            s.bog(11, t.kelish_bor ? t.kelish_valyutasi : L"");
            s.bajar();
            id = baza.oxirgi_id();
            if (boshlangich > 0)
                baza.sor("INSERT INTO ombor_omborharakati (tur, miqdor, izoh, sana, mahsulot_id,"
                         " kiritilgan_birlik, kiritilgan_miqdor) VALUES ('kirim',?,?,?,?,'',NULL)")
                    .bog(1, boshlangich).bog(2, "Boshlang'ich qoldiq").bog(3, hozir_utc())
                    .bog(4, id).bajar();
        } else {
            Sorov s = baza.sor(
                "UPDATE ombor_mahsulot SET nom=?, birlik=?, olish_birligi=?, olish_miqdori=?,"
                " valyuta=?, narx=?, kod=?, kelish_narxi=?, kelish_valyutasi=? WHERE id=?");
            s.bog(1, t.nom).bog(2, t.birlik).bog(3, t.olish_birligi).bog(4, t.olish_miqdori);
            s.bog(5, t.valyuta).bog(6, t.narx).bog(7, t.kod);
            if (t.kelish_bor) s.bog(8, t.kelish_narxi); else s.bosh(8);
            s.bog(9, t.kelish_bor ? t.kelish_valyutasi : L"").bog(10, id);
            s.bajar();
        }
        tr.tasdiq();
        return id;
    } catch (const BazaXato& e) {
        xato = unikal_xato(e);
        return 0;
    }
}

void tovar_faol(long long id, bool faol) {
    baza.sor("UPDATE ombor_mahsulot SET faol=? WHERE id=?").bog(1, faol ? 1 : 0).bog(2, id).bajar();
}

void kirim_qil(long long id, double miqdor, double kiritilgan, const wstring& kiritilgan_birlik,
               const wstring& izoh) {
    Tranzaksiya tr;
    baza.sor("UPDATE ombor_mahsulot SET qoldiq = qoldiq + ? WHERE id=?").bog(1, miqdor).bog(2, id)
        .bajar();
    Sorov s = baza.sor("INSERT INTO ombor_omborharakati (tur, miqdor, izoh, sana, mahsulot_id,"
                       " kiritilgan_birlik, kiritilgan_miqdor) VALUES ('kirim',?,?,?,?,?,?)");
    s.bog(1, miqdor).bog(2, izoh).bog(3, hozir_utc()).bog(4, id).bog(5, kiritilgan_birlik);
    if (kiritilgan_birlik.empty()) s.bosh(6); else s.bog(6, kiritilgan);
    s.bajar();
    tr.tasdiq();
}

void qoldiq_tuzat(long long id, double yangi, const wstring& izoh) {
    Tranzaksiya tr;
    Tovar t;
    if (!tovar_ol(id, t)) return;
    double farq = yaxlit(yangi - t.qoldiq, 3);
    if (farq == 0) return;
    baza.sor("UPDATE ombor_mahsulot SET qoldiq=? WHERE id=?").bog(1, yangi).bog(2, id).bajar();
    baza.sor("INSERT INTO ombor_omborharakati (tur, miqdor, izoh, sana, mahsulot_id,"
             " kiritilgan_birlik, kiritilgan_miqdor) VALUES ('tuzatish',?,?,?,?,'',NULL)")
        .bog(1, farq).bog(2, izoh.empty() ? L"Qoldiq qo'lda tuzatildi" : izoh).bog(3, hozir_utc())
        .bog(4, id).bajar();
    tr.tasdiq();
}

std::vector<Harakat> harakatlar(long long tovar_id) {
    std::vector<Harakat> r;
    Tovar t;
    tovar_ol(tovar_id, t);
    Sorov s = baza.sor("SELECT sana, tur, miqdor, kiritilgan_miqdor, kiritilgan_birlik, izoh"
                       " FROM ombor_omborharakati WHERE mahsulot_id=? ORDER BY sana DESC, id DESC"
                       " LIMIT 1000");
    s.bog(1, tovar_id);
    while (s.qator()) {
        Harakat h;
        h.sana = mahalliy_vaqt(s.matnU(0));
        wstring tur = s.matn(1);
        h.tur = tur == L"kirim" ? L"Kirim" : tur == L"chiqim" ? L"Chiqim"
              : tur == L"qaytarish" ? L"Qaytarish" : tur == L"tuzatish" ? L"Tuzatish" : tur;
        h.miqdor = son(s.son(2)) + L" " + t.birlik;
        wstring kb = s.matn(4);
        if (!kb.empty() && s.bormi(3) && kb != t.birlik)
            h.miqdor = son(s.son(3)) + L" " + kb + L" = " + h.miqdor;
        h.izoh = s.matn(5);
        r.push_back(h);
    }
    return r;
}

// ======================= Qarz daftari =======================

std::vector<Hudud> hududlar() {
    std::vector<Hudud> r;
    Sorov s = baza.sor("SELECT id, nom FROM qarz_hudud ORDER BY tartib, nom");
    while (s.qator()) r.push_back({s.butun(0), s.matn(1)});
    return r;
}

long long hudud_qosh(const wstring& nom, wstring& xato) {
    if (tozala(nom).empty()) {
        xato = L"Hudud nomini yozing.";
        return 0;
    }
    try {
        baza.sor("INSERT INTO qarz_hudud (nom, tartib) VALUES (?,"
                 " (SELECT COALESCE(MAX(tartib),0)+1 FROM qarz_hudud))")
            .bog(1, tozala(nom)).bajar();
        return baza.oxirgi_id();
    } catch (const BazaXato& e) {
        xato = unikal_xato(e);
        return 0;
    }
}

bool hudud_nomla(long long id, const wstring& nom, wstring& xato) {
    if (tozala(nom).empty()) {
        xato = L"Hudud nomini yozing.";
        return false;
    }
    try {
        baza.sor("UPDATE qarz_hudud SET nom=? WHERE id=?").bog(1, tozala(nom)).bog(2, id).bajar();
        return true;
    } catch (const BazaXato& e) {
        xato = unikal_xato(e);
        return false;
    }
}

static const char* QARZDOR_SQL =
    "SELECT d.id, d.ism, d.familiya, d.telefon, d.izoh, d.hudud_id, COALESCE(h.nom,''),"
    " COALESCE((SELECT SUM(jami - qaytarilgan_summa) FROM qarz_qarz WHERE qarzdor_id=d.id),0)"
    " - COALESCE((SELECT SUM(summa) FROM qarz_tolov WHERE qarzdor_id=d.id AND valyuta='som'),0),"
    " COALESCE((SELECT SUM(jami_dollar - qaytarilgan_summa_dollar) FROM qarz_qarz"
    "  WHERE qarzdor_id=d.id),0)"
    " - COALESCE((SELECT SUM(summa) FROM qarz_tolov WHERE qarzdor_id=d.id AND valyuta='dollar'),0)"
    " FROM qarz_qarzdor d LEFT JOIN qarz_hudud h ON h.id=d.hudud_id ";

static Qarzdor qarzdor_qatori(Sorov& s) {
    Qarzdor q;
    q.id = s.butun(0);
    q.ism = s.matn(1);
    q.familiya = s.matn(2);
    q.telefon = s.matn(3);
    q.izoh = s.matn(4);
    q.hudud_id = s.butun(5);
    q.hudud = s.matn(6);
    q.balans = yaxlit(s.son(7), 2);
    q.balans_d = yaxlit(s.son(8), 2);
    return q;
}

std::vector<Qarzdor> qarzdorlar(const wstring& qidiruv) {
    auto b = bolakla(qidiruv);
    std::vector<Qarzdor> r;
    Sorov s = baza.sor(string(QARZDOR_SQL) + "ORDER BY d.familiya, d.ism");
    while (s.qator()) {
        Qarzdor q = qarzdor_qatori(s);
        if (mos_keladi(kichik(q.familiya + L" " + q.ism + L" " + q.telefon + L" " + q.hudud), b))
            r.push_back(q);
    }
    return r;
}

bool qarzdor_ol(long long id, Qarzdor& q) {
    Sorov s = baza.sor(string(QARZDOR_SQL) + "WHERE d.id=?");
    s.bog(1, id);
    if (!s.qator()) return false;
    q = qarzdor_qatori(s);
    return true;
}

long long qarzdor_saqla(const Qarzdor& q, wstring& xato) {
    if (tozala(q.ism).empty() || tozala(q.familiya).empty()) {
        xato = L"Ism va familiyani yozing.";
        return 0;
    }
    if (q.hudud_id == 0) {
        xato = L"Hududni tanlang.";
        return 0;
    }
    if (q.id == 0) {
        baza.sor("INSERT INTO qarz_qarzdor (ism, familiya, telefon, izoh, yaratilgan, hudud_id)"
                 " VALUES (?,?,?,?,?,?)")
            .bog(1, tozala(q.ism)).bog(2, tozala(q.familiya)).bog(3, tozala(q.telefon))
            .bog(4, tozala(q.izoh)).bog(5, hozir_utc()).bog(6, q.hudud_id).bajar();
        return baza.oxirgi_id();
    }
    baza.sor("UPDATE qarz_qarzdor SET ism=?, familiya=?, telefon=?, izoh=?, hudud_id=? WHERE id=?")
        .bog(1, tozala(q.ism)).bog(2, tozala(q.familiya)).bog(3, tozala(q.telefon))
        .bog(4, tozala(q.izoh)).bog(5, q.hudud_id).bog(6, q.id).bajar();
    return q.id;
}

static wstring qatorlar_matni(const string& sql, long long id) {
    wstring r;
    Sorov s = baza.sor(sql);
    s.bog(1, id);
    while (s.qator()) {
        if (!r.empty()) r += L"; ";
        r += s.matn(0) + L" " + son(s.son(1)) + L" " + s.matn(2);
        double qayt = s.son(3);
        if (qayt > 0) r += L" (qaytdi " + son(qayt) + L")";
    }
    return r;
}

std::vector<TarixQator> qarzdor_tarixi(long long qarzdor_id) {
    std::vector<TarixQator> r;
    {
        Sorov s = baza.sor("SELECT id, sana, jami - qaytarilgan_summa,"
                           " jami_dollar - qaytarilgan_summa_dollar, sotuv_id, izoh"
                           " FROM qarz_qarz WHERE qarzdor_id=?");
        s.bog(1, qarzdor_id);
        while (s.qator()) {
            TarixQator t;
            long long id = s.butun(0);
            t.sana_utc = s.matnU(1);
            t.som = s.son(2);
            t.dollar = s.son(3);
            if (s.bormi(4)) {
                long long sid = s.butun(4);
                t.hujjat = L"Qarz #" + std::to_wstring(id) + L" (chek #" + std::to_wstring(sid) + L")";
                t.tafsilot = qatorlar_matni("SELECT mahsulot_nomi, miqdor, birlik, qaytarilgan FROM"
                                            " sotuv_sotuvqator WHERE sotuv_id=? ORDER BY id", sid);
            } else {
                t.hujjat = L"Qarz #" + std::to_wstring(id);
                t.tafsilot = qatorlar_matni("SELECT mahsulot_nomi, miqdor, birlik, qaytarilgan FROM"
                                            " qarz_qarzqator WHERE qarz_id=? ORDER BY id", id);
            }
            wstring izoh = s.matn(5);
            if (t.tafsilot.empty()) t.tafsilot = izoh;
            r.push_back(t);
        }
    }
    {
        Sorov s = baza.sor("SELECT sana, summa, valyuta, izoh, qarz_id FROM qarz_tolov"
                           " WHERE qarzdor_id=?");
        s.bog(1, qarzdor_id);
        while (s.qator()) {
            TarixQator t;
            t.sana_utc = s.matnU(0);
            (s.matn(2) == L"dollar" ? t.dollar : t.som) = -s.son(1);
            t.hujjat = s.bormi(4) ? L"To'lov (qarz #" + std::to_wstring(s.butun(4)) + L" ga)"
                                  : L"To'lov";
            t.tafsilot = s.matn(3);
            r.push_back(t);
        }
    }
    std::sort(r.begin(), r.end(),
              [](const TarixQator& a, const TarixQator& b) { return a.sana_utc > b.sana_utc; });
    return r;
}

void tolov_qosh(long long qarzdor_id, double summa, const wstring& valyuta, const wstring& izoh) {
    baza.sor("INSERT INTO qarz_tolov (summa, sana, izoh, qarzdor_id, valyuta, qarz_id)"
             " VALUES (?,?,?,?,?,NULL)")
        .bog(1, summa).bog(2, hozir_utc()).bog(3, izoh).bog(4, qarzdor_id).bog(5, valyuta).bajar();
}

// ======================= Sotuv =======================

bool sotuv_yakunla(const std::vector<SavatQator>& savat, double jami, double jami_d,
                   double tolandi, double tolandi_d, double kurs, long long qarzdor_id,
                   ChekNatija& natija, wstring& xato) {
    if (savat.empty()) {
        xato = L"Chek bo'sh — avval tovar qo'shing.";
        return false;
    }
    jami = yaxlit(jami, 2);
    jami_d = yaxlit(jami_d, 2);
    tolandi = yaxlit(tolandi, 2);
    tolandi_d = yaxlit(tolandi_d, 2);
    if (jami < 0 || jami_d < 0 || tolandi < 0 || tolandi_d < 0) {
        xato = L"Summalar manfiy bo'lmaydi.";
        return false;
    }
    if (tolandi > jami) {
        xato = L"Mijoz bergan pul chek summasidan ko'p: " + pul(tolandi) + L" > " + pul(jami) +
               L" so'm.";
        return false;
    }
    if (tolandi_d > jami_d) {
        xato = L"Mijoz bergan dollar chek summasidan ko'p: " + pul(tolandi_d) + L" > " +
               pul(jami_d) + L" $.";
        return false;
    }
    double qarz = yaxlit(jami - tolandi, 2), qarz_d = yaxlit(jami_d - tolandi_d, 2);
    if ((qarz > 0 || qarz_d > 0) && qarzdor_id == 0) {
        xato = L"Pul yetmayapti: " + pul(qarz) + L" so'm" +
               (qarz_d > 0 ? L", " + pul(qarz_d) + L" $" : L"") +
               L" qoladi. Qarzdorni tanlang yoki to'langan summani tuzating.";
        return false;
    }
    try {
        Tranzaksiya tr;
        for (auto& q : savat) {
            Tovar t;
            if (!tovar_ol(q.t.id, t)) {
                xato = q.t.nom + L" — tovar bazada topilmadi.";
                return false;
            }
            if (q.miqdor > t.qoldiq + 1e-9) {
                xato = L"Omborda yetarli emas. " + t.nom + L": " + son(t.qoldiq) + L" " + t.birlik +
                       L" qoldi. Savdo yakunlanmadi.";
                return false;
            }
        }
        string vaqt = hozir_utc();
        baza.sor("INSERT INTO sotuv_sotuv (sana, izoh, yakunlangan, jami, jami_dollar, kurs,"
                 " bekor_qilingan, qaytarilgan_summa, qaytarilgan_summa_dollar, kassir_id)"
                 " VALUES (?, '', 1, ?, ?, ?, 0, 0, 0, NULL)")
            .bog(1, vaqt).bog(2, tolandi).bog(3, tolandi_d).bog(4, kurs).bajar();
        long long sid = baza.oxirgi_id();
        wstring izoh = L"Sotuv #" + std::to_wstring(sid) + L": yakunlandi";
        Sorov qator = baza.sor("INSERT INTO sotuv_sotuvqator (mahsulot_nomi, birlik, miqdor,"
                               " mahsulot_id, sotuv_id, qaytarilgan, narx, valyuta)"
                               " VALUES (?,?,?,?,?,0,?,?)");
        Sorov ayir = baza.sor("UPDATE ombor_mahsulot SET qoldiq = qoldiq - ? WHERE id=?");
        Sorov harakat = baza.sor("INSERT INTO ombor_omborharakati (tur, miqdor, izoh, sana,"
                                 " mahsulot_id, kiritilgan_birlik, kiritilgan_miqdor)"
                                 " VALUES ('chiqim',?,?,?,?,'',NULL)");
        for (auto& q : savat) {
            qator.qayta();
            qator.bog(1, q.t.nom).bog(2, q.t.birlik).bog(3, q.miqdor).bog(4, q.t.id).bog(5, sid);
            if (q.t.narx > 0) qator.bog(6, q.t.narx); else qator.bosh(6);
            qator.bog(7, q.t.valyuta).bajar();
            ayir.qayta();
            ayir.bog(1, q.miqdor).bog(2, q.t.id).bajar();
            harakat.qayta();
            harakat.bog(1, q.miqdor).bog(2, izoh).bog(3, vaqt).bog(4, q.t.id).bajar();
        }
        natija.sotuv_id = sid;
        natija.qarz_id = 0;
        if (qarzdor_id && (qarz > 0 || qarz_d > 0)) {
            baza.sor("INSERT INTO qarz_qarz (sana, izoh, yakunlangan, qarzdor_id, jami,"
                     " jami_dollar, kurs, qaytarilgan_summa, qaytarilgan_summa_dollar, sotuv_id,"
                     " kassir_id) VALUES (?,?,1,?,?,?,?,0,0,?,NULL)")
                .bog(1, vaqt).bog(2, L"Chek #" + std::to_wstring(sid) + L" dan qolgan qarz")
                .bog(3, qarzdor_id).bog(4, qarz).bog(5, qarz_d).bog(6, kurs).bog(7, sid).bajar();
            natija.qarz_id = baza.oxirgi_id();
        }
        tr.tasdiq();
        return true;
    } catch (const BazaXato& e) {
        xato = L"Baza xatosi: " + W(e.what());
        return false;
    }
}

bool chek_ol(long long sotuv_id, Chek& c) {
    Sorov s = baza.sor("SELECT sana, jami - qaytarilgan_summa, jami_dollar -"
                       " qaytarilgan_summa_dollar, bekor_qilingan FROM sotuv_sotuv WHERE id=?");
    s.bog(1, sotuv_id);
    if (!s.qator()) return false;
    c = Chek();
    c.id = sotuv_id;
    c.sana_utc = s.matnU(0);
    c.naqd = s.son(1);
    c.naqd_d = s.son(2);
    c.bekor = s.butun(3) != 0;
    Sorov q = baza.sor("SELECT mahsulot_nomi, birlik, miqdor - qaytarilgan, narx, valyuta FROM"
                       " sotuv_sotuvqator WHERE sotuv_id=? ORDER BY id");
    q.bog(1, sotuv_id);
    while (q.qator()) {
        ChekQator x;
        x.nom = q.matn(0);
        x.birlik = q.matn(1);
        x.miqdor = q.son(2);
        x.narx_bor = q.bormi(3);
        x.narx = q.son(3);
        x.valyuta = q.matn(4);
        c.qatorlar.push_back(x);
    }
    Sorov z = baza.sor("SELECT q.jami - q.qaytarilgan_summa, q.jami_dollar -"
                       " q.qaytarilgan_summa_dollar, d.familiya || ' ' || d.ism FROM qarz_qarz q"
                       " JOIN qarz_qarzdor d ON d.id=q.qarzdor_id WHERE q.sotuv_id=?");
    z.bog(1, sotuv_id);
    if (z.qator()) {
        c.qarz = z.son(0);
        c.qarz_d = z.son(1);
        c.qarzdor = z.matn(2);
    }
    return true;
}

// ======================= Hisobot =======================

KunHisobi kun_hisobi(const SYSTEMTIME& kun, std::vector<ChekQisqa>& cheklar) {
    string bosh, oxir;
    kun_oraligi(kun, bosh, oxir);
    KunHisobi h;
    cheklar.clear();
    Sorov s = baza.sor(
        "SELECT s.id, s.sana, (SELECT count(*) FROM sotuv_sotuvqator x WHERE x.sotuv_id=s.id),"
        " s.jami - s.qaytarilgan_summa, s.jami_dollar - s.qaytarilgan_summa_dollar,"
        " COALESCE(q.jami - q.qaytarilgan_summa, 0),"
        " COALESCE(q.jami_dollar - q.qaytarilgan_summa_dollar, 0),"
        " COALESCE(d.familiya || ' ' || d.ism, '')"
        " FROM sotuv_sotuv s LEFT JOIN qarz_qarz q ON q.sotuv_id=s.id"
        " LEFT JOIN qarz_qarzdor d ON d.id=q.qarzdor_id"
        " WHERE s.yakunlangan=1 AND s.bekor_qilingan=0 AND s.sana>=? AND s.sana<?"
        " ORDER BY s.sana DESC");
    s.bog(1, bosh).bog(2, oxir);
    while (s.qator()) {
        ChekQisqa c{s.butun(0), s.matnU(1), (int)s.butun(2), s.son(3),
                    s.son(4),   s.son(5),   s.son(6),        s.matn(7)};
        cheklar.push_back(c);
        h.cheklar++;
        h.naqd += c.naqd;
        h.naqd_d += c.naqd_d;
    }
    Sorov q = baza.sor("SELECT COALESCE(SUM(jami - qaytarilgan_summa),0),"
                       " COALESCE(SUM(jami_dollar - qaytarilgan_summa_dollar),0)"
                       " FROM qarz_qarz WHERE sana>=? AND sana<?");
    q.bog(1, bosh).bog(2, oxir);
    if (q.qator()) {
        h.qarz = q.son(0);
        h.qarz_d = q.son(1);
    }
    Sorov t = baza.sor("SELECT COALESCE(SUM(CASE WHEN valyuta='som' THEN summa END),0),"
                       " COALESCE(SUM(CASE WHEN valyuta='dollar' THEN summa END),0)"
                       " FROM qarz_tolov WHERE sana>=? AND sana<?");
    t.bog(1, bosh).bog(2, oxir);
    if (t.qator()) {
        h.tolov = t.son(0);
        h.tolov_d = t.son(1);
    }
    return h;
}

// ======================= Sozlamalar va kurs =======================

wstring sozlama(const string& kalit, const wstring& standart) {
    Sorov s = baza.sor("SELECT qiymat FROM xozmag_sozlama WHERE kalit=?");
    s.bog(1, kalit);
    return s.qator() ? s.matn(0) : standart;
}

void sozlama_yoz(const string& kalit, const wstring& qiymat) {
    baza.sor("INSERT INTO xozmag_sozlama (kalit, qiymat) VALUES (?,?)"
             " ON CONFLICT(kalit) DO UPDATE SET qiymat=excluded.qiymat")
        .bog(1, kalit).bog(2, qiymat).bajar();
}

double dollar_kursi() {
    Sorov s = baza.sor("SELECT kurs FROM ombor_dollarkursi WHERE kurs>0 ORDER BY sana DESC LIMIT 1");
    return s.qator() ? s.son(0) : 0;
}

void kurs_saqla(double kurs) {
    baza.sor("INSERT INTO ombor_dollarkursi (sana, kurs, urinish) VALUES (?,?,?)"
             " ON CONFLICT(sana) DO UPDATE SET kurs=excluded.kurs, urinish=excluded.urinish")
        .bog(1, bugun_mahalliy()).bog(2, kurs).bog(3, hozir_utc()).bajar();
}

// ======================= Zaxira va import =======================

static string sql_matn(const string& s) {
    string r = "'";
    for (char c : s) {
        r += c;
        if (c == '\'') r += '\'';
    }
    return r + "'";
}

wstring zaxira_ol(const wstring& papka) {
    CreateDirectoryW(papka.c_str(), nullptr);
    wstring fayl = papka + L"\\xozmag_" + fayl_vaqti() + L".sqlite3";
    baza.bajar("VACUUM INTO " + sql_matn(U(fayl)));
    return fayl;
}

// Fayl yo'lidan SQLite URI: file:///D:/papka/db.sqlite3?mode=ro
static string uri_yasa(const wstring& fayl) {
    string u = U(fayl), r = "file:///";
    static const char* hex = "0123456789ABCDEF";
    for (unsigned char c : u) {
        if (c == '\\') c = '/';
        if (isalnum(c) || strchr("/-_.~:", c))
            r += (char)c;
        else {
            r += '%';
            r += hex[c >> 4];
            r += hex[c & 15];
        }
    }
    return r + "?mode=ro";
}

struct Ustun {
    string nom, tur;
    bool majburiy;
};

static std::vector<Ustun> ustunlar(const string& sxema, const string& jadval) {
    std::vector<Ustun> r;
    Sorov s = baza.sor("PRAGMA " + sxema + ".table_info(\"" + jadval + "\")");
    while (s.qator()) r.push_back({s.matnU(1), s.matnU(2), s.butun(3) != 0});
    return r;
}

// Eski bazada bo'lmagan majburiy ustun uchun qiymat.
static string standart_qiymat(const string& jadval, const Ustun& u) {
    if (jadval == "ombor_mahsulot") {
        if (u.nom == "kod") return "printf('%04d', id)";
        if (u.nom == "birlik") return "'dona'";
        if (u.nom == "valyuta") return "'som'";
        if (u.nom == "olish_miqdori") return "1";
        if (u.nom == "manba") return "'{}'";
        if (u.nom == "faol") return "1";
    }
    if (u.nom == "valyuta") return "'som'";
    if (u.nom == "yakunlangan") return "1";
    string t = u.tur;
    for (auto& c : t) c = (char)tolower(c);
    if (t.find("char") != string::npos || t.find("text") != string::npos) return "''";
    if (t.find("date") != string::npos) return sql_matn(hozir_utc());
    return "0";
}

bool eski_bazani_import(const wstring& fayl, wstring& hisobot) {
    static const char* JADVALLAR[] = {"auth_user", "ombor_mahsulot", "ombor_omborharakati", "ombor_dollarkursi",
                                      "qarz_hudud",     "qarz_qarzdor",        "sotuv_sotuv",
                                      "qarz_qarz",      "qarz_qarzqator",      "qarz_tolov",
                                      "sotuv_sotuvqator"};
    static const wchar_t* NOMLAR[] = {L"Foydalanuvchilar", L"Tovarlar",        L"Ombor harakatlari", L"Dollar kurslari",
                                      L"Hududlar",        L"Qarzdorlar",        L"Cheklar (sotuv)",
                                      L"Qarz hujjatlari", L"Qarz qatorlari",    L"To'lovlar",
                                      L"Chek qatorlari"};
    const int N = sizeof(JADVALLAR) / sizeof(*JADVALLAR);

    if (_wcsicmp(fayl.c_str(), baza.fayl().c_str()) == 0) {
        hisobot = L"Bu dasturning o'z bazasi — boshqa faylni tanlang.";
        return false;
    }
    try {
        baza.bajar("ATTACH DATABASE " + sql_matn(uri_yasa(fayl)) + " AS eski");
    } catch (const BazaXato& e) {
        hisobot = L"Faylni ochib bo'lmadi: " + W(e.what());
        return false;
    }
    struct Ajrat {
        ~Ajrat() { try { baza.bajar("DETACH DATABASE eski"); } catch (...) {} }
    } ajrat;

    try {
        Sorov s = baza.sor("SELECT count(*) FROM eski.sqlite_master WHERE type='table' AND"
                           " name IN ('ombor_mahsulot','qarz_qarzdor')");
        if (!s.qator() || s.butun(0) < 2) {
            hisobot = L"Bu fayl Xozmag bazasi emas (ombor_mahsulot / qarz_qarzdor jadvallari yo'q).";
            return false;
        }
    } catch (const BazaXato& e) {
        hisobot = L"Fayl SQLite bazasi emas: " + W(e.what());
        return false;
    }

    wstring zaxira;
    try {
        Sanoq sq = sanoq();
        if (sq.tovar || sq.qarzdor || sq.sotuv) {
            wchar_t yol[MAX_PATH];
            wcscpy_s(yol, baza.fayl().c_str());
            wchar_t* p = wcsrchr(yol, L'\\');
            if (p) *p = 0;
            zaxira = zaxira_ol(wstring(yol) + L"\\zaxira");
        }
    } catch (const BazaXato& e) {
        hisobot = L"Import qilishdan oldin zaxira olinmadi: " + W(e.what());
        return false;
    }

    std::vector<long long> soni(N, 0);
    try {
        Tranzaksiya tr;
        for (int i = N - 1; i >= 0; --i) baza.bajar(string("DELETE FROM main.\"") + JADVALLAR[i] + "\"");
        for (int i = 0; i < N; ++i) {
            auto eski = ustunlar("eski", JADVALLAR[i]);
            if (eski.empty()) continue;  // eski versiyada bu jadval yo'q
            std::set<string> bor;
            for (auto& u : eski) bor.insert(u.nom);
            string kim, nima;
            for (auto& u : ustunlar("main", JADVALLAR[i])) {
                string qiymat;
                if (bor.count(u.nom))
                    qiymat = "\"" + u.nom + "\"";
                else if (u.majburiy)
                    qiymat = standart_qiymat(JADVALLAR[i], u);
                else
                    continue;
                if (!kim.empty()) { kim += ", "; nima += ", "; }
                kim += "\"" + u.nom + "\"";
                nima += qiymat;
            }
            baza.bajar(string("INSERT INTO main.\"") + JADVALLAR[i] + "\" (" + kim + ") SELECT " +
                       nima + " FROM eski.\"" + JADVALLAR[i] + "\"");
            soni[i] = baza.ozgarish();
        }
        // Eski 5 xonali kodlar 4 xonaga (0027); band bo'lsa o'zgarmaydi.
        baza.bajar("UPDATE OR IGNORE ombor_mahsulot SET kod = printf('%04d', CAST(kod AS INTEGER))"
                   " WHERE kod GLOB '[0-9]*' AND length(kod) <> 4"
                   " AND CAST(kod AS INTEGER) BETWEEN 1 AND 9999");
        tr.tasdiq();
    } catch (const BazaXato& e) {
        hisobot = L"Import to'xtadi, hech narsa o'zgarmadi.\n\n" + W(e.what());
        return false;
    }
    hisobot = L"Import tugadi:\n\n";
    for (int i = 0; i < N; ++i)
        hisobot += L"  " + wstring(NOMLAR[i]) + L": " + guruhla(soni[i]) + L"\n";
    if (!zaxira.empty()) hisobot += L"\nOldingi ma'lumotlar zaxirasi:\n" + zaxira;
    return true;
}

Sanoq sanoq() {
    Sanoq r;
    Sorov s = baza.sor("SELECT (SELECT count(*) FROM ombor_mahsulot),"
                       " (SELECT count(*) FROM qarz_qarzdor), (SELECT count(*) FROM sotuv_sotuv),"
                       " (SELECT count(*) FROM qarz_qarz)");
    if (s.qator()) {
        r.tovar = s.butun(0);
        r.qarzdor = s.butun(1);
        r.sotuv = s.butun(2);
        r.qarz = s.butun(3);
    }
    return r;
}
