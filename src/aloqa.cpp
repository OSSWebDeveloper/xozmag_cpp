// Kassa <-> server aloqasi. Kadr: 4 bayt uzunlik + ma'lumot (birinchi bayt — amal).
#include <winsock2.h>
#include <ws2tcpip.h>
#include <bcrypt.h>

#include "aloqa.h"

#include <ctime>

#include "excel.h"

wstring versiya();  // main.cpp

// SQLite'dan qo'shimcha (sqlite_min.h dagi ro'yxat qisqa)
extern "C" {
int sqlite3_set_authorizer(sqlite3*, int (*)(void*, int, const char*, const char*, const char*, const char*),
                           void*);
int sqlite3_get_autocommit(sqlite3*);
}

namespace {

constexpr unsigned PROTOKOL = 1;
constexpr unsigned MAKS_KADR = 512u << 20;

enum Amal : unsigned char {
    A_SALOM = 1, A_SONI, A_KIRISH, A_TOKEN, A_BIRINCHI, A_SQL, A_EXEC, A_PING, A_CHIQISH, A_TOPLAM, A_AMAL, A_TOQNASH
};
enum Javob : unsigned char { J_OK = 0, J_XATO = 1, J_RUXSAT = 2 };

// ---------- kodlash ----------
struct Yoz {
    string b;
    Yoz& u8(unsigned char v) { b += (char)v; return *this; }
    Yoz& u32(unsigned v) { b.append((const char*)&v, 4); return *this; }
    Yoz& i64(long long v) { b.append((const char*)&v, 8); return *this; }
    Yoz& f64(double v) { b.append((const char*)&v, 8); return *this; }
    Yoz& s(const string& v) { u32((unsigned)v.size()); b += v; return *this; }
    Yoz& q(const Qiymat& v) {
        u8(v.tur);
        if (v.tur == 1) i64(v.i);
        else if (v.tur == 2) f64(v.d);
        else if (v.tur == 3) s(v.s);
        return *this;
    }
};

struct BuzuqKadr {};
struct Oqi {
    const string& b;
    size_t p = 0;
    explicit Oqi(const string& x) : b(x) {}
    void kerak(size_t n) { if (b.size() - p < n || p > b.size()) throw BuzuqKadr{}; }
    unsigned char u8() { kerak(1); return (unsigned char)b[p++]; }
    unsigned u32() { kerak(4); unsigned v; memcpy(&v, b.data() + p, 4); p += 4; return v; }
    long long i64() { kerak(8); long long v; memcpy(&v, b.data() + p, 8); p += 8; return v; }
    double f64() { kerak(8); double v; memcpy(&v, b.data() + p, 8); p += 8; return v; }
    string s() { unsigned n = u32(); kerak(n); string r = b.substr(p, n); p += n; return r; }
    Qiymat q() {
        Qiymat v;
        v.tur = (char)u8();
        if (v.tur == 1) v.i = i64();
        else if (v.tur == 2) v.d = f64();
        else if (v.tur == 3) v.s = s();
        else if (v.tur != 0) throw BuzuqKadr{};
        return v;
    }
};

bool hammasi_yubor(SOCKET s, const char* p, size_t n) {
    while (n) {
        int k = send(s, p, (int)std::min<size_t>(n, 1 << 20), 0);
        if (k <= 0) return false;
        p += k;
        n -= k;
    }
    return true;
}
bool hammasi_ol(SOCKET s, char* p, size_t n) {
    while (n) {
        int k = recv(s, p, (int)std::min<size_t>(n, 1 << 20), 0);
        if (k <= 0) return false;
        p += k;
        n -= k;
    }
    return true;
}
bool kadr_yubor(SOCKET s, const string& m) {
    string b(4, '\0');
    unsigned n = (unsigned)m.size();
    memcpy(b.data(), &n, 4);
    b += m;
    return hammasi_yubor(s, b.data(), b.size());
}
bool kadr_ol(SOCKET s, string& m) {
    unsigned n = 0;
    if (!hammasi_ol(s, (char*)&n, 4) || n > MAKS_KADR) return false;
    m.resize(n);
    return hammasi_ol(s, m.data(), n);
}

void kutish_qoy(SOCKET s, int ms) {
    DWORD v = ms;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&v, sizeof v);
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&v, sizeof v);
}
void tez_qoy(SOCKET s) {
    BOOL ha = TRUE;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&ha, sizeof ha);
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (const char*)&ha, sizeof ha);
}

bool wsa_tayyor() {
    static bool tayyor = [] {
        WSADATA w;
        return WSAStartup(MAKEWORD(2, 2), &w) == 0;
    }();
    return tayyor;
}

// ======================= Mijoz =======================

SOCKET mijoz = INVALID_SOCKET;
bool uzildi = false, kirgan = false;
DWORD oxirgi_aloqa = 0;  // oxirgi muvaffaqiyatli javob vaqti
wstring manzil_joriy;
string token;

[[noreturn]] void aloqa_yiqildi() {
    if (mijoz != INVALID_SOCKET) closesocket(mijoz);
    mijoz = INVALID_SOCKET;
    uzildi = true;
    throw AloqaXato("Server bilan aloqa uzildi");
}

// So'rov yuborib javob oladi; tarmoq xatosida — AloqaXato
// Sinov uchun: XOZMAG_JURNAL=<fayl> bo'lsa har bir so'rov yoziladi
void jurnal(const string& sorov) {
    static const char* f = getenv("XOZMAG_JURNAL");
    if (!f || sorov.empty()) return;
    FILE* j = fopen(f, "ab");
    if (!j) return;
    string m = sorov.size() > 5 && (sorov[0] == A_SQL || sorov[0] == A_EXEC) ? sorov.substr(5, 70)
               : sorov[0] == A_TOPLAM && sorov.size() > 5 ? std::to_string(*(const unsigned*)(sorov.data() + 1)) + " ta"
                                                          : "";
    for (char& c : m) if (c == '\n' || c == '\r') c = ' ';
    fprintf(j, "%lu %d %s\n", GetTickCount(), (int)sorov[0], m.c_str());
    fclose(j);
}

string sora(const string& sorov, int kutish_ms = 15000) {
    jurnal(sorov);
    if (mijoz == INVALID_SOCKET) {
        uzildi = true;
        throw AloqaXato("Server bilan aloqa yo'q");
    }
    kutish_qoy(mijoz, kutish_ms);
    string javob;
    if (!kadr_yubor(mijoz, sorov) || !kadr_ol(mijoz, javob) || javob.empty()) aloqa_yiqildi();
    oxirgi_aloqa = GetTickCount();
    return javob;
}

}  // namespace

bool ulan(const wstring& manzil_, wstring& xato) {
    uzil();
    uzildi = false;
    if (!wsa_tayyor()) { xato = L"Tarmoq ishga tushmadi."; return false; }
    wstring manzil = tozala(manzil_), host = manzil;
    int port = STANDART_PORT;
    size_t ik = manzil.rfind(L':');
    if (ik != wstring::npos && manzil.find(L':') == ik) {
        host = manzil.substr(0, ik);
        port = _wtoi(manzil.c_str() + ik + 1);
        if (port <= 0 || port > 65535) { xato = L"Port noto'g'ri."; return false; }
    }
    if (host.empty()) { xato = L"IP manzilni yozing."; return false; }
    ADDRINFOW mas = {}, *nat = nullptr;
    mas.ai_family = AF_INET;
    mas.ai_socktype = SOCK_STREAM;
    mas.ai_protocol = IPPROTO_TCP;
    if (GetAddrInfoW(host.c_str(), std::to_wstring(port).c_str(), &mas, &nat) != 0 || !nat) {
        xato = L"Bunday manzil topilmadi.";
        return false;
    }
    SOCKET s = socket(nat->ai_family, SOCK_STREAM, IPPROTO_TCP);
    u_long bloksiz = 1;
    ioctlsocket(s, FIONBIO, &bloksiz);
    connect(s, nat->ai_addr, (int)nat->ai_addrlen);
    FreeAddrInfoW(nat);
    fd_set yoz, xat;
    FD_ZERO(&yoz); FD_SET(s, &yoz);
    FD_ZERO(&xat); FD_SET(s, &xat);
    timeval tv = {3, 0};
    int r = select(0, nullptr, &yoz, &xat, &tv);
    if (r <= 0 || FD_ISSET(s, &xat)) {
        closesocket(s);
        xato = r == 0 ? L"Server javob bermadi (3 soniya)." : L"Server ulanishni qabul qilmadi.";
        return false;
    }
    bloksiz = 0;
    ioctlsocket(s, FIONBIO, &bloksiz);
    tez_qoy(s);
    mijoz = s;
    try {
        Yoz y;
        y.u8(A_SALOM).s("XOZMAG").u32(PROTOKOL).s(U(versiya()));
        string j = sora(y.b, 5000);
        Oqi o(j);
        if (o.u8() != J_OK) {
            xato = W(o.s());
            uzil();
            return false;
        }
    } catch (const AloqaXato&) {
        xato = L"Bu manzilda Xozmag Server yo'q.";
        uzil();
        uzildi = false;
        return false;
    } catch (const BuzuqKadr&) {
        xato = L"Bu manzilda Xozmag Server yo'q.";
        uzil();
        return false;
    }
    manzil_joriy = manzil;
    uzildi = false;
    return true;
}

void uzil() {
    if (mijoz != INVALID_SOCKET) closesocket(mijoz);
    mijoz = INVALID_SOCKET;
    kirgan = false;
}
bool ulangan() { return mijoz != INVALID_SOCKET; }
bool aloqa_uzildi() { return uzildi; }
wstring ulangan_manzil() { return manzil_joriy; }
string& sessiya_tokeni() { return token; }
bool uzoq_kirgan() { return kirgan; }

bool ping() {
    if (mijoz == INVALID_SOCKET) return false;
    try {
        sora(string(1, (char)A_PING), 5000);
        return true;
    } catch (const AloqaXato&) {
        return false;
    }
}

bool tirikmi() {
    if (mijoz == INVALID_SOCKET || uzildi) return true;
    if (GetTickCount() - oxirgi_aloqa < 5000) return true;
    return ping();
}

void uzoq_sql(const string& sql, const std::vector<Qiymat>& par, std::vector<Qiymat>& nat,
              size_t& ustun, long long& ozgarish, long long& oxirgi_id) {
    Yoz y;
    y.u8(A_SQL).s(sql).u32((unsigned)par.size());
    for (auto& q : par) y.q(q);
    string j = sora(y.b);
    try {
        Oqi o(j);
        unsigned char h = o.u8();
        if (h == J_RUXSAT) throw BazaXato("Serverda kirilmagan (sessiya tugagan)");
        if (h != J_OK) throw BazaXato(o.s());
        ozgarish = o.i64();
        oxirgi_id = o.i64();
        ustun = o.u32();
        unsigned qatorlar = o.u32();
        nat.clear();
        nat.reserve((size_t)ustun * qatorlar);
        for (size_t i = 0; i < (size_t)ustun * qatorlar; ++i) nat.push_back(o.q());
    } catch (const BuzuqKadr&) {
        aloqa_yiqildi();
    }
}

void uzoq_exec(const string& sql, long long& ozgarish) {
    Yoz y;
    y.u8(A_EXEC).s(sql);
    string j = sora(y.b);
    try {
        Oqi o(j);
        unsigned char h = o.u8();
        if (h == J_RUXSAT) throw BazaXato("Serverda kirilmagan (sessiya tugagan)");
        if (h != J_OK) throw BazaXato(o.s());
        ozgarish = o.i64();
    } catch (const BuzuqKadr&) {
        aloqa_yiqildi();
    }
}

void uzoq_toplam(const std::vector<Navbat>& n, long long& ozgarish, long long& oxirgi_id) {
    Yoz y;
    y.u8(A_TOPLAM).u32((unsigned)n.size());
    for (auto& x : n) {
        y.u8(x.exec ? 1 : 0).s(x.sql).u32((unsigned)x.par.size());
        for (auto& q : x.par) y.q(q);
    }
    string j = sora(y.b, 120000);
    try {
        Oqi o(j);
        unsigned char h = o.u8();
        if (h == J_RUXSAT) throw BazaXato("Serverda kirilmagan (sessiya tugagan)");
        if (h != J_OK) {
            o.u32();  // qaysi so'rovda to'xtagani
            throw BazaXato(o.s());
        }
        ozgarish = o.i64();
        oxirgi_id = o.i64();
    } catch (const BuzuqKadr&) {
        aloqa_yiqildi();
    }
}

bool uzoq_amal(ServerAmal amal, long long id, std::vector<wstring>& xatolar) {
    Yoz y;
    y.u8(A_AMAL).u8((unsigned char)amal).i64(id);
    string j = sora(y.b, 300000);  // katta fayl: server bir necha soniya ishlaydi
    try {
        Oqi o(j);
        unsigned char h = o.u8();
        if (h == J_RUXSAT) throw BazaXato("Serverda kirilmagan (sessiya tugagan)");
        if (h != J_OK) throw BazaXato(o.s());
        bool ok = o.u8() != 0;
        xatolar.clear();
        for (unsigned n = o.u32(); n; --n) xatolar.push_back(W(o.s()));
        return ok;
    } catch (const BuzuqKadr&) {
        aloqa_yiqildi();
    }
}

std::vector<Toqnash> uzoq_toqnashuvlar(long long id) {
    Yoz y;
    y.u8(A_TOQNASH).i64(id);
    string j = sora(y.b, 120000);
    try {
        Oqi o(j);
        unsigned char h = o.u8();
        if (h == J_RUXSAT) throw BazaXato("Serverda kirilmagan (sessiya tugagan)");
        if (h != J_OK) throw BazaXato(o.s());
        std::vector<Toqnash> r;
        for (unsigned n = o.u32(); n; --n) {
            Toqnash t;
            t.kod = W(o.s());
            t.sabab = W(o.s());
            t.keyingi = o.i64();
            r.push_back(t);
        }
        return r;
    } catch (const BuzuqKadr&) {
        aloqa_yiqildi();
    }
}

long long uzoq_foydalanuvchilar_soni() {
    string j = sora(string(1, (char)A_SONI));
    try {
        Oqi o(j);
        if (o.u8() != J_OK) return 0;
        return o.i64();
    } catch (const BuzuqKadr&) {
        aloqa_yiqildi();
    }
}

static bool foydalanuvchini_oqi(Oqi& o, Foydalanuvchi& f) {
    f.id = o.i64();
    f.login = W(o.s());
    f.superuser = o.u8() != 0;
    return f.id != 0;
}

bool uzoq_kirish(const wstring& login, const wstring& parol, Foydalanuvchi& f, wstring& xato,
                 bool sessiya) {
    Yoz y;
    wchar_t komp[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD n = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameW(komp, &n);
    y.u8(A_KIRISH).s(U(login)).s(U(parol)).u8(sessiya ? 1 : 0).s(U(komp));
    string j = sora(y.b, 30000);  // parol xeshi sekin hisoblanadi
    try {
        Oqi o(j);
        if (o.u8() != J_OK) {
            xato = W(o.s());
            return false;
        }
        foydalanuvchini_oqi(o, f);
        string t = o.s();
        if (sessiya) {
            token = t;
            kirgan = true;
        }
        xato.clear();
        return true;
    } catch (const BuzuqKadr&) {
        aloqa_yiqildi();
    }
}

bool uzoq_birinchi(const wstring& login, const wstring& parol, wstring& xato) {
    Yoz y;
    y.u8(A_BIRINCHI).s(U(login)).s(U(parol));
    string j = sora(y.b, 30000);
    try {
        Oqi o(j);
        if (o.u8() != J_OK) {
            xato = W(o.s());
            return false;
        }
        return true;
    } catch (const BuzuqKadr&) {
        aloqa_yiqildi();
    }
}

bool uzoq_sessiya(Foydalanuvchi& f) {
    if (token.empty()) return false;
    Yoz y;
    y.u8(A_TOKEN).s(token);
    string j = sora(y.b);
    try {
        Oqi o(j);
        if (o.u8() != J_OK) {
            token.clear();
            return false;
        }
        foydalanuvchini_oqi(o, f);
        kirgan = true;
        return true;
    } catch (const BuzuqKadr&) {
        aloqa_yiqildi();
    }
}

void uzoq_chiqish() {
    if (mijoz != INVALID_SOCKET) {
        try { sora(string(1, (char)A_CHIQISH), 5000); } catch (const AloqaXato&) {}
    }
    token.clear();
    kirgan = false;
}

// ======================= Server =======================

namespace {

SRWLOCK qulf = SRWLOCK_INIT;
// Serverning asosiy (global) bazasida bajariladigan og'ir amallar bittadan
SRWLOCK amal_qulf = SRWLOCK_INIT;
struct AmalQulf {
    AmalQulf() { AcquireSRWLockExclusive(&amal_qulf); }
    ~AmalQulf() { ReleaseSRWLockExclusive(&amal_qulf); }
};
struct Qulf {
    Qulf() { AcquireSRWLockExclusive(&qulf); }
    ~Qulf() { ReleaseSRWLockExclusive(&qulf); }
};

struct Ulanish {
    SOCKET s;
    Mijoz m;
};
std::vector<Ulanish*> ulanishlar;
wstring server_fayl;
HWND kuzatuvchi = nullptr;
long long jami_sorov = 0;
SOCKET tinglovchi = INVALID_SOCKET;

void xabar_ber() {
    if (kuzatuvchi) InvalidateRect(kuzatuvchi, nullptr, FALSE);
}

// Kassadan kelgan SQL faqat shu bazada ishlaydi: boshqa faylni ulab olish taqiqlanadi.
int ruxsat(void*, int amal, const char*, const char*, const char*, const char*) {
    const int SQLITE_ATTACH = 24, SQLITE_DETACH = 25, SQLITE_DENY = 1;
    return amal == SQLITE_ATTACH || amal == SQLITE_DETACH ? SQLITE_DENY : 0;
}

string yangi_token() {
    unsigned char b[24];
    BCryptGenRandom(nullptr, b, sizeof b, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    static const char* H = "0123456789abcdef";
    string r;
    for (unsigned char c : b) { r += H[c >> 4]; r += H[c & 15]; }
    return r;
}

constexpr long long SESSIYA_MUDDAT = 14 * 86400;  // saytdagi sessiya kabi

// Bitta SQL: natija qatorlari `qatorlar` ga (kerak bo'lsa). Xato bo'lsa matni qaytadi.
bool sql_bajar(sqlite3* db, const string& sql, const std::vector<Qiymat>& par, Yoz* qatorlar,
               unsigned& soni, int& ustun, string& xato) {
    sqlite3_stmt* st = nullptr;
    soni = 0;
    ustun = 0;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
        xato = string(sqlite3_errmsg(db)) + "\nSQL: " + sql;
        return false;
    }
    for (size_t i = 0; i < par.size(); ++i) {
        const Qiymat& q = par[i];
        int k = (int)i + 1;
        if (q.tur == 1) sqlite3_bind_int64(st, k, q.i);
        else if (q.tur == 2) sqlite3_bind_double(st, k, q.d);
        else if (q.tur == 3) sqlite3_bind_text(st, k, q.s.c_str(), (int)q.s.size(), SQLITE_TRANSIENT);
        else sqlite3_bind_null(st, k);
    }
    ustun = sqlite3_column_count(st);
    int r;
    while ((r = sqlite3_step(st)) == SQLITE_ROW) {
        ++soni;
        if (!qatorlar) continue;
        for (int c = 0; c < ustun; ++c) {
            int t = sqlite3_column_type(st, c);
            Qiymat v;
            if (t == 1) { v.tur = 1; v.i = sqlite3_column_int64(st, c); }
            else if (t == 2) { v.tur = 2; v.d = sqlite3_column_double(st, c); }
            else if (t == SQLITE_NULL) v.tur = 0;
            else {
                v.tur = 3;
                const unsigned char* p = sqlite3_column_text(st, c);
                if (p) v.s = (const char*)p;
            }
            qatorlar->q(v);
        }
    }
    if (r != SQLITE_DONE) {
        xato = sqlite3_errmsg(db);
        sqlite3_finalize(st);
        return false;
    }
    sqlite3_finalize(st);
    return true;
}

// Server ulanishidagi tranzaksiya (global `baza` emas)
struct Tranzaksiya_b {
    Baza& b;
    bool tayyor = false;
    explicit Tranzaksiya_b(Baza& x) : b(x) { b.bajar("BEGIN IMMEDIATE"); }
    ~Tranzaksiya_b() {
        if (!tayyor) try { b.bajar("ROLLBACK"); } catch (...) {}
    }
    void tasdiq() { b.bajar("COMMIT"); tayyor = true; }
};

string xato_javob(const string& m) {
    Yoz y;
    y.u8(J_XATO).s(m);
    return y.b;
}

void foydalanuvchi_yoz(Yoz& y, long long id, const string& login, bool su) {
    y.i64(id).s(login).u8(su ? 1 : 0);
}

// Bitta so'rovni bajaradi. f — shu ulanishda kirgan foydalanuvchi (0 — kirmagan).
string bajar(Baza& b, Ulanish& u, long long& f, string& tok, const string& kadr) {
    Oqi o(kadr);
    unsigned char amal = o.u8();
    switch (amal) {
        case A_SALOM: {
            string sehr = o.s();
            unsigned p = o.u32();
            string v = o.s();
            if (sehr != "XOZMAG") return xato_javob("Bu Xozmag emas.");
            if (p != PROTOKOL)
                return xato_javob("Server versiyasi boshqa (v" + U(versiya()) + ", kassada v" + v +
                                  "). Ikkala kompyuterga bir xil versiyani o'rnating.");
            Yoz y;
            y.u8(J_OK).s(U(versiya()));
            return y.b;
        }
        case A_PING: return string(1, (char)J_OK);
        case A_SONI: {
            Sorov s = b.sor("SELECT count(*) FROM auth_user WHERE is_active=1");
            Yoz y;
            y.u8(J_OK).i64(s.qator() ? s.butun(0) : 0);
            return y.b;
        }
        case A_KIRISH: {
            wstring login = tozala(W(o.s()));
            wstring parol = W(o.s());
            bool sessiya = o.u8() != 0;
            string komp = o.s();
            Sorov s = b.sor("SELECT id, username, password, is_superuser, is_active FROM auth_user WHERE username=?");
            s.bog(1, login);
            if (!s.qator() || !s.butun(4) || !parol_mos(parol, s.matnU(2)))
                return xato_javob("Login yoki parol noto'g'ri.");
            long long id = s.butun(0);
            string nom = s.matnU(1);
            bool su = s.butun(3) != 0;
            // O'qish yopilmasa yozishda SQLite kutmay «database is locked» beradi (boshqa kassa yozayotgan bo'lsa)
            s.qayta();
            string t;
            if (sessiya) {
                t = yangi_token();
                long long hozir = time(nullptr);
                Tranzaksiya_b tr(b);
                b.sor("UPDATE auth_user SET last_login=? WHERE id=?").bog(1, hozir_utc()).bog(2, id).bajar();
                b.sor("DELETE FROM xozmag_sessiya WHERE oxirgi < ?").bog(1, hozir - SESSIYA_MUDDAT).bajar();
                b.sor("INSERT INTO xozmag_sessiya (token, user_id, yaratilgan, oxirgi, kompyuter) VALUES (?, ?, ?, ?, ?)")
                    .bog(1, t).bog(2, id).bog(3, hozir).bog(4, hozir).bog(5, komp).bajar();
                tr.tasdiq();
                f = id;
                tok = t;
                Qulf q;
                u.m.login = W(nom);
            }
            Yoz y;
            y.u8(J_OK);
            foydalanuvchi_yoz(y, id, nom, su);
            y.s(t);
            xabar_ber();
            return y.b;
        }
        case A_TOKEN: {
            string t = o.s();
            long long hozir = time(nullptr);
            Sorov s = b.sor("SELECT u.id, u.username, u.is_superuser FROM xozmag_sessiya s"
                            " JOIN auth_user u ON u.id = s.user_id"
                            " WHERE s.token=? AND u.is_active=1 AND s.oxirgi >= ?");
            s.bog(1, t).bog(2, hozir - SESSIYA_MUDDAT);
            if (!s.qator()) return xato_javob("Sessiya tugagan.");
            f = s.butun(0);
            tok = t;
            string nom = s.matnU(1);
            bool su = s.butun(2) != 0;
            s.qayta();
            b.sor("UPDATE xozmag_sessiya SET oxirgi=? WHERE token=?").bog(1, hozir).bog(2, t).bajar();
            {
                Qulf q;
                u.m.login = W(nom);
            }
            Yoz y;
            y.u8(J_OK);
            foydalanuvchi_yoz(y, f, nom, su);
            xabar_ber();
            return y.b;
        }
        case A_CHIQISH: {
            if (!tok.empty()) b.sor("DELETE FROM xozmag_sessiya WHERE token=?").bog(1, tok).bajar();
            tok.clear();
            f = 0;
            {
                Qulf q;
                u.m.login.clear();
            }
            xabar_ber();
            return string(1, (char)J_OK);
        }
        case A_BIRINCHI: {
            wstring login = tozala(W(o.s())), parol = W(o.s());
            Sorov s = b.sor("SELECT count(*) FROM auth_user WHERE is_active=1");
            if (s.qator() && s.butun(0) > 0) return xato_javob("Foydalanuvchi allaqachon bor.");
            s.qayta();
            if (login.empty() || parol.size() < 4) return xato_javob("Login yozing, parol kamida 4 belgi bo'lsin.");
            try {
                b.sor("INSERT INTO auth_user (password, last_login, is_superuser, username, last_name,"
                      " email, is_staff, is_active, date_joined, first_name)"
                      " VALUES (?, NULL, 1, ?, '', '', 1, 1, ?, '')")
                    .bog(1, parol_hash(parol)).bog(2, login).bog(3, hozir_utc()).bajar();
            } catch (const BazaXato&) {
                return xato_javob("Bunday login bor.");
            }
            return string(1, (char)J_OK);
        }
        case A_SQL: {
            if (!f) return string(1, (char)J_RUXSAT);
            string sql = o.s();
            unsigned n = o.u32();
            std::vector<Qiymat> par;
            for (unsigned i = 0; i < n; ++i) par.push_back(o.q());
            Yoz qatorlar;
            unsigned soni;
            int ustun;
            string xato;
            if (!sql_bajar(b.h(), sql, par, &qatorlar, soni, ustun, xato)) return xato_javob(xato);
            Yoz y;
            y.u8(J_OK).i64(sqlite3_changes(b.h())).i64(sqlite3_last_insert_rowid(b.h())).u32(ustun).u32(soni);
            y.b += qatorlar.b;
            return y.b;
        }
        case A_AMAL: {
            if (!f) return string(1, (char)J_RUXSAT);
            unsigned char amal = o.u8();
            long long id = o.i64();
            std::vector<wstring> xatolar;
            bool ok = false;
            {
                AmalQulf q;
                if (amal == SA_QOLLASH) ok = qollash(id, xatolar);
                else if (amal == SA_BEKOR) ok = bekor_qil(id, xatolar);
                else return xato_javob("Noma'lum amal.");
            }
            Yoz y;
            y.u8(J_OK).u8(ok ? 1 : 0).u32((unsigned)xatolar.size());
            for (auto& x : xatolar) y.s(U(x));
            return y.b;
        }
        case A_TOQNASH: {
            if (!f) return string(1, (char)J_RUXSAT);
            long long id = o.i64();
            std::vector<Toqnash> t;
            {
                AmalQulf q;
                t = bekor_toqnashuvlari(id);
            }
            Yoz y;
            y.u8(J_OK).u32((unsigned)t.size());
            for (auto& x : t) y.s(U(x.kod)).s(U(x.sabab)).i64(x.keyingi);
            return y.b;
        }
        case A_TOPLAM: {
            if (!f) return string(1, (char)J_RUXSAT);
            unsigned n = o.u32();
            for (unsigned i = 0; i < n; ++i) {
                bool exec = o.u8() != 0;
                string sql = o.s();
                unsigned k = o.u32();
                std::vector<Qiymat> par;
                for (unsigned j = 0; j < k; ++j) par.push_back(o.q());
                string xato;
                if (exec) {
                    try { b.bajar(sql); } catch (const BazaXato& e) { xato = e.what(); }
                } else {
                    unsigned soni;
                    int ustun;
                    sql_bajar(b.h(), sql, par, nullptr, soni, ustun, xato);
                }
                if (!xato.empty()) {
                    Yoz y;
                    y.u8(J_XATO).u32(i).s(xato);
                    return y.b;
                }
            }
            Yoz y;
            y.u8(J_OK).i64(sqlite3_changes(b.h())).i64(sqlite3_last_insert_rowid(b.h()));
            return y.b;
        }
        case A_EXEC: {
            if (!f) return string(1, (char)J_RUXSAT);
            string sql = o.s();
            try {
                b.bajar(sql);
            } catch (const BazaXato& e) {
                return xato_javob(e.what());
            }
            Yoz y;
            y.u8(J_OK).i64(b.ozgarish());
            return y.b;
        }
    }
    throw BuzuqKadr{};
}

DWORD WINAPI mijoz_ishi(void* p) {
    Ulanish* u = (Ulanish*)p;
    wstring fayl;
    {
        Qulf q;
        fayl = server_fayl;
    }
    Baza b;
    long long f = 0;
    string tok;
    try {
        b.och(fayl);
        sqlite3_set_authorizer(b.h(), ruxsat, nullptr);
        // Kassa jim qolsa (o'chdi, tarmoq uzildi) ochiq tranzaksiya bazani qulflab qolmasin:
        // kassa har 5 soniyada «ping» yuboradi, 45 soniya jimlik — ulanish yopiladi.
        kutish_qoy(u->s, 45000);
        string kadr;
        while (kadr_ol(u->s, kadr) && !kadr.empty()) {
            string javob;
            try {
                javob = bajar(b, *u, f, tok, kadr);
            } catch (const BazaXato& e) {
                javob = xato_javob(e.what());
            }
            {
                Qulf q;
                u->m.oxirgi = time(nullptr);
                ++u->m.sorovlar;
                ++jami_sorov;
            }
            if (!kadr_yubor(u->s, javob)) break;
        }
    } catch (...) {
    }
    b.yop();  // ochiq tranzaksiya bo'lsa bekor bo'ladi
    {
        Qulf q;
        for (size_t i = 0; i < ulanishlar.size(); ++i)
            if (ulanishlar[i] == u) { ulanishlar.erase(ulanishlar.begin() + i); break; }
    }
    closesocket(u->s);
    delete u;
    xabar_ber();
    return 0;
}

DWORD WINAPI tinglash(void*) {
    for (;;) {
        sockaddr_in a = {};
        int n = sizeof a;
        SOCKET s = accept(tinglovchi, (sockaddr*)&a, &n);
        if (s == INVALID_SOCKET) {
            Sleep(200);
            continue;
        }
        tez_qoy(s);
        Ulanish* u = new Ulanish{s, {}};
        wchar_t ip[64] = {};
        InetNtopW(AF_INET, &a.sin_addr, ip, 64);
        u->m.ip = ip;
        u->m.ulangan = u->m.oxirgi = time(nullptr);
        {
            Qulf q;
            ulanishlar.push_back(u);
        }
        HANDLE t = CreateThread(nullptr, 0, mijoz_ishi, u, 0, nullptr);
        if (t) CloseHandle(t);
        else {
            Qulf q;
            ulanishlar.pop_back();
            closesocket(s);
            delete u;
        }
        xabar_ber();
    }
}

}  // namespace

bool server_boshla(int port, const wstring& baza_fayl, wstring& xato) {
    if (!wsa_tayyor()) { xato = L"Tarmoq ishga tushmadi."; return false; }
    server_fayl = baza_fayl;
    tinglovchi = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    BOOL ha = TRUE;
    setsockopt(tinglovchi, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&ha, sizeof ha);
    sockaddr_in a = {};
    a.sin_family = AF_INET;
    a.sin_port = htons((u_short)port);
    a.sin_addr.s_addr = INADDR_ANY;
    if (bind(tinglovchi, (sockaddr*)&a, sizeof a) != 0 || listen(tinglovchi, 32) != 0) {
        int k = WSAGetLastError();
        closesocket(tinglovchi);
        tinglovchi = INVALID_SOCKET;
        xato = k == WSAEADDRINUSE ? L"Port " + std::to_wstring(port) + L" band: server allaqachon ochiq emasmi?"
                                  : L"Portni ochib bo'lmadi (" + std::to_wstring(k) + L").";
        return false;
    }
    HANDLE t = CreateThread(nullptr, 0, tinglash, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
    return true;
}

bool server_baza_almashtir(const wstring& baza_fayl, wstring& xato) {
    AmalQulf aq;
    wstring eski = baza.fayl();
    try {
        baza.och(baza_fayl);
    } catch (const std::exception& e) {
        xato = W(e.what());
        try { baza.och(eski); } catch (...) {}
        return false;
    }
    Qulf q;
    server_fayl = baza_fayl;
    for (auto* u : ulanishlar) shutdown(u->s, SD_BOTH);  // kassalar yangi bazaga qayta ulanadi
    return true;
}

std::vector<Mijoz> server_mijozlari() {
    Qulf q;
    std::vector<Mijoz> r;
    for (auto* u : ulanishlar) r.push_back(u->m);
    return r;
}

long long server_sorovlari() {
    Qulf q;
    return jami_sorov;
}

void server_kuzatuvchi(HWND oyna) { kuzatuvchi = oyna; }
