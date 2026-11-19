// v2: saytdagi (Django) mantiqning C++ dagi nusxasi — kirish, kassa cheki, yakunlash.
#include <windows.h>
#include <bcrypt.h>
#include <shlobj.h>

#include <algorithm>

#include "db.h"
#include "aloqa.h"

// ======================= Parollar (Django pbkdf2_sha256) =======================

static string base64(const unsigned char* d, size_t n) {
    static const char* A = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    string r;
    for (size_t i = 0; i < n; i += 3) {
        unsigned v = d[i] << 16 | (i + 1 < n ? d[i + 1] << 8 : 0) | (i + 2 < n ? d[i + 2] : 0);
        r += A[v >> 18 & 63];
        r += A[v >> 12 & 63];
        r += i + 1 < n ? A[v >> 6 & 63] : '=';
        r += i + 2 < n ? A[v & 63] : '=';
    }
    return r;
}

static string pbkdf2(const string& parol, const string& tuz, unsigned takror) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    unsigned char chiqish[32] = {};
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr,
                                    BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0)
        return {};
    BCryptDeriveKeyPBKDF2(alg, (PUCHAR)parol.data(), (ULONG)parol.size(), (PUCHAR)tuz.data(),
                          (ULONG)tuz.size(), takror, chiqish, 32, 0);
    BCryptCloseAlgorithmProvider(alg, 0);
    return base64(chiqish, 32);
}

string parol_hash(const wstring& parol) {
    static const char* B = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    unsigned char t[22];
    BCryptGenRandom(nullptr, t, sizeof t, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    string tuz;
    for (unsigned char c : t) tuz += B[c % 62];
    const unsigned TAKROR = 1500000;  // saytdagi Django bilan bir xil
    return "pbkdf2_sha256$" + std::to_string(TAKROR) + "$" + tuz + "$" + pbkdf2(U(parol), tuz, TAKROR);
}

bool parol_mos(const wstring& parol, const string& hash) {
    // pbkdf2_sha256$<takror>$<tuz>$<hash>
    size_t a = hash.find('$'), b = hash.find('$', a + 1), c = hash.find('$', b + 1);
    if (a == string::npos || b == string::npos || c == string::npos) return false;
    if (hash.substr(0, a) != "pbkdf2_sha256") return false;
    unsigned takror = (unsigned)strtoul(hash.substr(a + 1, b - a - 1).c_str(), nullptr, 10);
    string tuz = hash.substr(b + 1, c - b - 1), kutilgan = hash.substr(c + 1);
    string hisob = pbkdf2(U(parol), tuz, takror);
    if (hisob.size() != kutilgan.size()) return false;
    unsigned char farq = 0;
    for (size_t i = 0; i < hisob.size(); ++i) farq |= hisob[i] ^ kutilgan[i];
    return farq == 0;
}

bool django_bazasimi() {
    Sorov s = baza.sor("SELECT count(*) FROM sqlite_master WHERE name='django_migrations'");
    return s.qator() && s.butun(0) > 0;
}

long long foydalanuvchilar_soni() {
    if (baza.uzoqmi()) return uzoq_foydalanuvchilar_soni();
    Sorov s = baza.sor("SELECT count(*) FROM auth_user WHERE is_active=1");
    return s.qator() ? s.butun(0) : 0;
}

bool kirish_tekshir(const wstring& login, const wstring& parol, Foydalanuvchi& f, wstring& xato,
                    bool sessiya) {
    if (baza.uzoqmi()) return uzoq_kirish(login, parol, f, xato, sessiya);
    Sorov s = baza.sor("SELECT id, username, password, is_superuser, is_active FROM auth_user"
                       " WHERE username=?");
    s.bog(1, tozala(login));
    // Django xabari bilan bir xil: qaysi biri xato ekani aytilmaydi
    xato = L"Login yoki parol noto'g'ri.";
    if (!s.qator() || !s.butun(4)) return false;
    if (!parol_mos(parol, s.matnU(2))) return false;
    f.id = s.butun(0);
    f.login = s.matn(1);
    f.superuser = s.butun(3) != 0;
    baza.sor("UPDATE auth_user SET last_login=? WHERE id=?").bog(1, hozir_utc()).bog(2, f.id).bajar();
    xato.clear();
    return true;
}

bool foydalanuvchi_ol(long long id, Foydalanuvchi& f) {
    Sorov s = baza.sor("SELECT id, username, is_superuser FROM auth_user WHERE id=? AND is_active=1");
    s.bog(1, id);
    if (!s.qator()) return false;
    f.id = s.butun(0);
    f.login = s.matn(1);
    f.superuser = s.butun(2) != 0;
    return true;
}

bool foydalanuvchi_yarat(const wstring& login, const wstring& parol, bool superuser, wstring& xato) {
    if (tozala(login).empty() || parol.size() < 4) {
        xato = L"Login yozing, parol kamida 4 belgi bo'lsin.";
        return false;
    }
    // Serverda hali hech kim yo'q: birinchi foydalanuvchini server o'zi yaratadi
    if (baza.uzoqmi() && !uzoq_kirgan()) return uzoq_birinchi(login, parol, xato);
    try {
        baza.sor("INSERT INTO auth_user (password, last_login, is_superuser, username, last_name,"
                 " email, is_staff, is_active, date_joined, first_name)"
                 " VALUES (?, NULL, ?, ?, '', '', ?, 1, ?, '')")
            .bog(1, parol_hash(parol)).bog(2, superuser ? 1 : 0).bog(3, tozala(login))
            .bog(4, superuser ? 1 : 0).bog(5, hozir_utc()).bajar();
        return true;
    } catch (const BazaXato&) {
        xato = L"Bunday login bor.";
        return false;
    }
}

bool parol_almashtir(long long id, const wstring& eski, const wstring& yangi, wstring& xato) {
    Sorov s = baza.sor("SELECT password FROM auth_user WHERE id=?");
    s.bog(1, id);
    if (!s.qator() || !parol_mos(eski, s.matnU(0))) {
        xato = L"Hozirgi parol noto'g'ri.";
        return false;
    }
    if (yangi.size() < 4) {
        xato = L"Yangi parol kamida 4 belgi bo'lsin.";
        return false;
    }
    baza.sor("UPDATE auth_user SET password=? WHERE id=?").bog(1, parol_hash(yangi)).bog(2, id).bajar();
    return true;
}

bool login_almashtir(long long id, const wstring& yangi, wstring& xato) {
    if (tozala(yangi).empty()) {
        xato = L"Login bo'sh bo'lmasin.";
        return false;
    }
    try {
        baza.sor("UPDATE auth_user SET username=? WHERE id=?").bog(1, tozala(yangi)).bog(2, id).bajar();
        return true;
    } catch (const BazaXato&) {
        xato = L"Bunday login band.";
        return false;
    }
}

// ======================= Kassa cheki =======================

long long ochiq_sotuv(long long kassir_id) {
    Sorov s = baza.sor("SELECT id FROM sotuv_sotuv WHERE kassir_id=? AND yakunlangan=0 AND"
                       " bekor_qilingan=0 ORDER BY id LIMIT 1");
    s.bog(1, kassir_id);
    if (s.qator()) return s.butun(0);
    baza.sor("INSERT INTO sotuv_sotuv (sana, izoh, yakunlangan, jami, jami_dollar, kurs,"
             " bekor_qilingan, qaytarilgan_summa, qaytarilgan_summa_dollar, kassir_id)"
             " VALUES (?, '', 0, 0, 0, 0, 0, 0, 0, ?)")
        .bog(1, hozir_utc()).bog(2, kassir_id).bajar();
    return baza.oxirgi_id();
}

string sotuv_sanasi(long long sotuv_id) {
    Sorov s = baza.sor("SELECT sana FROM sotuv_sotuv WHERE id=?");
    s.bog(1, sotuv_id);
    return s.qator() ? s.matnU(0) : string();
}

static const char* QJ(HujjatTur t) { return t == H_SOTUV ? "sotuv_sotuvqator" : "qarz_qarzqator"; }
static const char* QF(HujjatTur t) { return t == H_SOTUV ? "sotuv_id" : "qarz_id"; }
static const char* HJ(HujjatTur t) { return t == H_SOTUV ? "sotuv_sotuv" : "qarz_qarz"; }

std::vector<Qator> sotuv_qatorlari(long long sotuv_id) { return hujjat_qatorlari(H_SOTUV, sotuv_id); }

std::vector<Qator> hujjat_qatorlari(HujjatTur tur, long long hujjat_id) {
    std::vector<Qator> r;
    Sorov s = baza.sor(string("SELECT q.id, q.mahsulot_id, COALESCE(m.kod,''), q.mahsulot_nomi, q.birlik,"
                       " q.valyuta, q.miqdor, q.narx, COALESCE(m.qoldiq,0) FROM ") + QJ(tur) + " q"
                       " LEFT JOIN ombor_mahsulot m ON m.id=q.mahsulot_id WHERE q." + QF(tur) + "=?"
                       " ORDER BY q.id");
    s.bog(1, hujjat_id);
    while (s.qator()) {
        Qator q;
        q.id = s.butun(0);
        q.mahsulot_id = s.butun(1);
        q.kod = s.matn(2);
        q.nom = s.matn(3);
        q.birlik = s.matn(4);
        q.valyuta = s.matn(5);
        q.miqdor = s.son(6);
        q.narx_bor = s.bormi(7);
        q.narx = s.son(7);
        q.qoldiq = s.son(8);
        r.push_back(q);
    }
    return r;
}

static bool hujjat_ochiqmi(HujjatTur tur, long long id) {
    Sorov s = baza.sor(tur == H_SOTUV ? "SELECT yakunlangan, bekor_qilingan FROM sotuv_sotuv WHERE id=?"
                                      : "SELECT yakunlangan, 0 FROM qarz_qarz WHERE id=?");
    s.bog(1, id);
    return s.qator() && !s.butun(0) && !s.butun(1);
}
static bool sotuv_ochiqmi(long long id) { return hujjat_ochiqmi(H_SOTUV, id); }

long long sotuv_qator_qosh(long long sotuv_id, long long mahsulot_id, double miqdor, wstring& xato) {
    return hujjat_qator_qosh(H_SOTUV, sotuv_id, mahsulot_id, miqdor, xato);
}

long long hujjat_qator_qosh(HujjatTur tur, long long sotuv_id, long long mahsulot_id, double miqdor,
                            wstring& xato) {
    try {
        Tranzaksiya tr;
        if (!hujjat_ochiqmi(tur, sotuv_id)) {
            xato = L"Hujjat allaqachon yopilgan.";
            return 0;
        }
        Tovar t;
        if (!tovar_ol(mahsulot_id, t)) {
            xato = L"Tovar topilmadi.";
            return 0;
        }
        Sorov q = baza.sor(string("SELECT id, miqdor FROM ") + QJ(tur) + " WHERE " + QF(tur) +
                           "=? AND mahsulot_id=? ORDER BY id LIMIT 1");
        q.bog(1, sotuv_id).bog(2, mahsulot_id);
        long long qid = 0;
        double bor = 0;
        if (q.qator()) {
            qid = q.butun(0);
            bor = q.son(1);
        }
        double yangi = yaxlit(bor + miqdor, 3);
        if (yangi > t.qoldiq + 1e-9) {
            xato = L"Omborda yetarli emas. " + t.nom + L": " + son(t.qoldiq) + L" " + t.birlik + L" qoldi.";
            return 0;
        }
        if (qid) {
            baza.sor(string("UPDATE ") + QJ(tur) + " SET miqdor=? WHERE id=?").bog(1, yangi).bog(2, qid).bajar();
        } else {
            Sorov i = baza.sor(string("INSERT INTO ") + QJ(tur) + " (mahsulot_nomi, birlik, miqdor, mahsulot_id, " +
                               QF(tur) + ", qaytarilgan, narx, valyuta) VALUES (?,?,?,?,?,0,?,?)");
            i.bog(1, t.nom).bog(2, t.birlik).bog(3, yangi).bog(4, t.id).bog(5, sotuv_id);
            if (t.narx > 0) i.bog(6, t.narx); else i.bosh(6);
            i.bog(7, t.valyuta).bajar();
            qid = baza.oxirgi_id();
        }
        tr.tasdiq();
        return qid;
    } catch (const BazaXato& e) {
        xato = W(e.what());
        return 0;
    }
}

bool sotuv_qator_miqdor(long long qator_id, double yangi, wstring& xato) {
    return hujjat_qator_miqdor(H_SOTUV, qator_id, yangi, xato);
}

bool hujjat_qator_miqdor(HujjatTur tur, long long qator_id, double yangi, wstring& xato) {
    Sorov s = baza.sor(string("SELECT q.") + QF(tur) + ", m.qoldiq, m.nom FROM " + QJ(tur) +
                       " q JOIN ombor_mahsulot m ON m.id=q.mahsulot_id WHERE q.id=?");
    s.bog(1, qator_id);
    if (!s.qator()) return false;
    if (!hujjat_ochiqmi(tur, s.butun(0))) {
        xato = L"Hujjat allaqachon yopilgan.";
        return false;
    }
    if (yangi <= 0) {
        xato = L"Miqdor noldan katta bo'lsin.";
        return false;
    }
    if (yangi > s.son(1) + 1e-9) {
        xato = L"Omborda yetarli emas: " + s.matn(2) + L".";
        return false;
    }
    baza.sor(string("UPDATE ") + QJ(tur) + " SET miqdor=? WHERE id=?").bog(1, yaxlit(yangi, 3)).bog(2, qator_id).bajar();
    return true;
}

void sotuv_qator_ochir(long long qator_id) { hujjat_qator_ochir(H_SOTUV, qator_id); }

void hujjat_qator_ochir(HujjatTur tur, long long qator_id) {
    baza.sor(string("DELETE FROM ") + QJ(tur) + " WHERE id=? AND " + QF(tur) + " IN (SELECT id FROM " +
             HJ(tur) + " WHERE yakunlangan=0)").bog(1, qator_id).bajar();
}

// ---------- qarz hujjati (qarz/views.py: qarz_boshlash, qarz_yakunlash) ----------

long long ochiq_qarz(long long qarzdor_id, long long kassir_id) {
    Sorov s = baza.sor("SELECT id FROM qarz_qarz WHERE qarzdor_id=? AND kassir_id=? AND yakunlangan=0"
                       " ORDER BY id LIMIT 1");
    s.bog(1, qarzdor_id).bog(2, kassir_id);
    if (s.qator()) return s.butun(0);
    baza.sor("INSERT INTO qarz_qarz (sana, izoh, yakunlangan, qarzdor_id, jami, jami_dollar, kurs,"
             " qaytarilgan_summa, qaytarilgan_summa_dollar, sotuv_id, kassir_id)"
             " VALUES (?, '', 0, ?, 0, 0, 0, 0, 0, NULL, ?)")
        .bog(1, hozir_utc()).bog(2, qarzdor_id).bog(3, kassir_id).bajar();
    return baza.oxirgi_id();
}

long long qarz_qarzdori(long long qarz_id) {
    Sorov s = baza.sor("SELECT qarzdor_id FROM qarz_qarz WHERE id=?");
    s.bog(1, qarz_id);
    return s.qator() ? s.butun(0) : 0;
}

string qarz_sanasi(long long qarz_id) {
    Sorov s = baza.sor("SELECT sana FROM qarz_qarz WHERE id=?");
    s.bog(1, qarz_id);
    return s.qator() ? s.matnU(0) : string();
}

bool qarz_bosh_bolsa_ochir(long long qarz_id) {
    Sorov s = baza.sor("SELECT count(*) FROM qarz_qarzqator WHERE qarz_id=?");
    s.bog(1, qarz_id);
    if (s.qator() && s.butun(0) == 0) {
        baza.sor("DELETE FROM qarz_qarz WHERE id=? AND yakunlangan=0").bog(1, qarz_id).bajar();
        return true;
    }
    return false;
}

bool qarz_yakunla(long long qarz_id, double jami, double jami_d, double kurs, double oldindan,
                  double oldindan_d, wstring& xato, wstring& tolandi) {
    jami = yaxlit(jami, 2);
    jami_d = yaxlit(jami_d, 2);
    oldindan = yaxlit(oldindan, 2);
    oldindan_d = yaxlit(oldindan_d, 2);
    if (jami <= 0 && jami_d <= 0) {
        xato = L"Qarz summasini yozing — so'mda yoki dollarda.";
        return false;
    }
    if (jami_d > 0 && kurs <= 0) {
        xato = L"Dollar summasi yozildi — o'sha kungi kursni ham yozing.";
        return false;
    }
    if (oldindan < 0 || oldindan_d < 0) {
        xato = L"Oldindan to'lov manfiy bo'lmaydi.";
        return false;
    }
    if (oldindan > jami) {
        xato = L"Oldindan to'langan pul qarz summasidan ko'p: " + pul(oldindan) + L" > " + pul(jami) + L" so'm.";
        return false;
    }
    if (oldindan_d > jami_d) {
        xato = L"Oldindan to'langan dollar qarz summasidan ko'p: " + son(oldindan_d) + L" > " + son(jami_d) + L" $.";
        return false;
    }
    try {
        Tranzaksiya tr;
        if (!hujjat_ochiqmi(H_QARZ, qarz_id)) {
            xato = L"Hujjat allaqachon yopilgan.";
            return false;
        }
        auto qs = hujjat_qatorlari(H_QARZ, qarz_id);
        string vaqt = hozir_utc();
        wstring izoh = L"Qarz #" + std::to_wstring(qarz_id) + L": yakunlandi";
        std::sort(qs.begin(), qs.end(), [](auto& a, auto& b) { return a.mahsulot_id < b.mahsulot_id; });
        for (auto& q : qs) {
            Tovar t;
            if (!tovar_ol(q.mahsulot_id, t) || q.miqdor > t.qoldiq + 1e-9) {
                xato = L"Omborda yetarli emas. " + q.nom + L": " + son(t.qoldiq) + L" " + q.birlik +
                       L" qoldi. Hujjat yakunlanmadi.";
                return false;
            }
            baza.sor("UPDATE ombor_mahsulot SET qoldiq = qoldiq - ? WHERE id=?").bog(1, q.miqdor)
                .bog(2, q.mahsulot_id).bajar();
            baza.sor("INSERT INTO ombor_omborharakati (tur, miqdor, izoh, sana, mahsulot_id,"
                     " kiritilgan_birlik, kiritilgan_miqdor) VALUES ('chiqim',?,?,?,?,'',NULL)")
                .bog(1, q.miqdor).bog(2, izoh).bog(3, vaqt).bog(4, q.mahsulot_id).bajar();
        }
        baza.sor("UPDATE qarz_qarz SET jami=?, jami_dollar=?, kurs=?, yakunlangan=1 WHERE id=?")
            .bog(1, jami).bog(2, jami_d).bog(3, kurs).bog(4, qarz_id).bajar();
        long long qarzdor = qarz_qarzdori(qarz_id);
        auto tolov = [&](double summa, const char* val) {
            baza.sor("INSERT INTO qarz_tolov (summa, sana, izoh, qarzdor_id, valyuta, qarz_id)"
                     " VALUES (?,?,'',?,?,?)")
                .bog(1, summa).bog(2, vaqt).bog(3, qarzdor).bog(4, val).bog(5, qarz_id).bajar();
        };
        tolandi.clear();
        if (oldindan > 0) {
            tolov(oldindan, "som");
            tolandi = pul(oldindan) + L" so'm";
        }
        if (oldindan_d > 0) {
            tolov(oldindan_d, "dollar");
            tolandi += (tolandi.empty() ? L"" : L" va ") + son(oldindan_d) + L" $";
        }
        tr.tasdiq();
        return true;
    } catch (const BazaXato& e) {
        xato = L"Baza xatosi: " + W(e.what());
        return false;
    }
}


bool sotuv_bekor(long long sotuv_id) {
    Sorov s = baza.sor("SELECT count(*) FROM sotuv_sotuvqator WHERE sotuv_id=?");
    s.bog(1, sotuv_id);
    if (s.qator() && s.butun(0) == 0) {
        baza.sor("DELETE FROM sotuv_sotuv WHERE id=? AND yakunlangan=0").bog(1, sotuv_id).bajar();
        return true;
    }
    baza.sor("UPDATE sotuv_sotuv SET bekor_qilingan=1, sana=? WHERE id=? AND yakunlangan=0")
        .bog(1, hozir_utc()).bog(2, sotuv_id).bajar();
    return false;
}

AvtoHisob avto_hisob(const std::vector<Qator>& qs) {
    AvtoHisob h;
    for (auto& q : qs) {
        if (!q.narx_bor || (q.valyuta != L"som" && q.valyuta != L"dollar")) {
            ++h.narxsiz;
            continue;
        }
        (q.dollarmi() ? h.dollar : h.som) += q.narx * q.miqdor;
    }
    h.som = std::floor(h.som + 0.5);
    h.dollar = yaxlit(h.dollar, 2);
    h.toliq = !qs.empty() && h.narxsiz == 0;
    return h;
}

bool sotuv_yakunla_v2(long long sotuv_id, long long kassir_id, double jami, double jami_d,
                      double kurs, long long qarzdor_id, double tolandi, double tolandi_d,
                      ChekNatija& n, wstring& xato) {
    (void)kassir_id;
    jami = yaxlit(jami, 2);
    jami_d = yaxlit(jami_d, 2);
    if (jami < 0 || jami_d < 0 || kurs < 0) {
        xato = L"Summalar manfiy bo'lmaydi.";
        return false;
    }
    if (jami <= 0 && jami_d <= 0) {
        xato = L"Jami summasini yozing — so'mda yoki dollarda.";
        return false;
    }
    if (jami_d > 0 && kurs <= 0) {
        xato = L"Dollar summasi yozildi — o'sha kungi kursni ham yozing.";
        return false;
    }
    double qarz = 0, qarz_d = 0;
    if (qarzdor_id) {
        tolandi = yaxlit(tolandi, 2);
        tolandi_d = yaxlit(tolandi_d, 2);
        if (tolandi > jami) {
            xato = L"Mijoz bergan pul chek summasidan ko'p: " + pul(tolandi) + L" > " + pul(jami) + L" so'm.";
            return false;
        }
        if (tolandi_d > jami_d) {
            xato = L"Mijoz bergan dollar chek summasidan ko'p: " + son(tolandi_d) + L" > " + son(jami_d) + L" $.";
            return false;
        }
        qarz = jami - tolandi;
        qarz_d = jami_d - tolandi_d;
        if (qarz <= 0 && qarz_d <= 0) {
            xato = L"Chek to'liq to'langan — qarzga hech narsa qolmadi. «Qarzga» dagi tanlovni olib tashlang.";
            return false;
        }
    }
    double naqd = jami - qarz, naqd_d = jami_d - qarz_d;
    try {
        Tranzaksiya tr;
        if (!sotuv_ochiqmi(sotuv_id)) {
            xato = L"Chek #" + std::to_wstring(sotuv_id) + L" allaqachon yopilgan.";
            return false;
        }
        auto qs = sotuv_qatorlari(sotuv_id);
        if (qs.empty()) {
            xato = L"Chek bo'sh — avval tovar qo'shing.";
            return false;
        }
        std::sort(qs.begin(), qs.end(), [](auto& a, auto& b) { return a.mahsulot_id < b.mahsulot_id; });
        string vaqt = hozir_utc();
        wstring izoh = L"Sotuv #" + std::to_wstring(sotuv_id) + L": yakunlandi";
        for (auto& q : qs) {
            Tovar t;
            if (!tovar_ol(q.mahsulot_id, t) || q.miqdor > t.qoldiq + 1e-9) {
                xato = L"Omborda yetarli emas. " + q.nom + L": " + son(t.qoldiq) + L" " + q.birlik +
                       L" qoldi. Savdo yakunlanmadi.";
                return false;
            }
            baza.sor("UPDATE ombor_mahsulot SET qoldiq = qoldiq - ? WHERE id=?").bog(1, q.miqdor)
                .bog(2, q.mahsulot_id).bajar();
            baza.sor("INSERT INTO ombor_omborharakati (tur, miqdor, izoh, sana, mahsulot_id,"
                     " kiritilgan_birlik, kiritilgan_miqdor) VALUES ('chiqim',?,?,?,?,'',NULL)")
                .bog(1, q.miqdor).bog(2, izoh).bog(3, vaqt).bog(4, q.mahsulot_id).bajar();
        }
        baza.sor("UPDATE sotuv_sotuv SET jami=?, jami_dollar=?, kurs=?, yakunlangan=1, sana=? WHERE id=?")
            .bog(1, naqd).bog(2, naqd_d).bog(3, kurs).bog(4, vaqt).bog(5, sotuv_id).bajar();
        n.sotuv_id = sotuv_id;
        n.qarz_id = 0;
        if (qarzdor_id) {
            Sorov z = baza.sor("INSERT INTO qarz_qarz (sana, izoh, yakunlangan, qarzdor_id, jami, jami_dollar,"
                               " kurs, qaytarilgan_summa, qaytarilgan_summa_dollar, sotuv_id, kassir_id)"
                               " VALUES (?,?,1,?,?,?,?,0,0,?,?)");
            z.bog(1, vaqt).bog(2, L"Chek #" + std::to_wstring(sotuv_id) + L" dan qolgan qarz")
                .bog(3, qarzdor_id).bog(4, qarz).bog(5, qarz_d).bog(6, kurs).bog(7, sotuv_id);
            if (kassir_id) z.bog(8, kassir_id); else z.bosh(8);
            z.bajar();
            n.qarz_id = baza.oxirgi_id();
        }
        tr.tasdiq();
        return true;
    } catch (const BazaXato& e) {
        xato = L"Baza xatosi: " + W(e.what());
        return false;
    }
}

std::vector<Tovar> oyna_tovarlari() {
    std::vector<Tovar> r;
    Sorov s = baza.sor(
        "SELECT m.id FROM ombor_mahsulot m WHERE m.faol=1 ORDER BY"
        " ((SELECT count(DISTINCT sotuv_id) FROM sotuv_sotuvqator WHERE mahsulot_id=m.id)"
        " + (SELECT count(DISTINCT qarz_id) FROM qarz_qarzqator WHERE mahsulot_id=m.id)) DESC, m.nom"
        " LIMIT 60");
    std::vector<long long> idlar;
    while (s.qator()) idlar.push_back(s.butun(0));
    for (auto id : idlar) {
        Tovar t;
        if (tovar_ol(id, t)) r.push_back(t);
    }
    return r;
}

double oxirgi_kurs() {
    // Bugun bankdan olingan kurs, bo'lmasa hujjatlarga oxirgi yozilgan kurs
    Sorov b = baza.sor("SELECT kurs FROM ombor_dollarkursi WHERE sana=? AND kurs>0");
    b.bog(1, bugun_mahalliy());
    if (b.qator()) return b.son(0);
    Sorov s = baza.sor("SELECT kurs FROM (SELECT kurs, sana FROM qarz_qarz WHERE kurs>0 UNION ALL"
                       " SELECT kurs, sana FROM sotuv_sotuv WHERE kurs>0) ORDER BY sana DESC LIMIT 1");
    if (s.qator()) return s.son(0);
    return dollar_kursi();
}

// ======================= Baza fayli =======================

static wstring papka(REFKNOWNFOLDERID id) {
    PWSTR p = nullptr;
    wstring r;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p))) r = p;
    CoTaskMemFree(p);
    return r;
}

wstring oz_baza_yoli() { return papka(FOLDERID_LocalAppData) + L"\\Xozmag\\xozmag.sqlite3"; }
wstring bizaropos_baza_yoli() { return papka(FOLDERID_LocalAppData) + L"\\BizaroPOS-Server\\db.sqlite3"; }

static wstring yol_fayli() { return papka(FOLDERID_LocalAppData) + L"\\Xozmag\\baza_yoli.txt"; }

wstring baza_yolini_aniqla() {
    FILE* f = _wfopen(yol_fayli().c_str(), L"rb");
    if (f) {
        char b[2048] = {};
        size_t n = fread(b, 1, sizeof b - 1, f);
        fclose(f);
        wstring y = tozala(W(string(b, n)));
        if (!y.empty() && GetFileAttributesW(y.c_str()) != INVALID_FILE_ATTRIBUTES) return y;
    }
    if (GetFileAttributesW(bizaropos_baza_yoli().c_str()) != INVALID_FILE_ATTRIBUTES)
        return bizaropos_baza_yoli();
    return oz_baza_yoli();
}

void baza_yolini_saqla(const wstring& yol) {
    SHCreateDirectoryExW(nullptr, (papka(FOLDERID_LocalAppData) + L"\\Xozmag").c_str(), nullptr);
    FILE* f = _wfopen(yol_fayli().c_str(), L"wb");
    if (!f) return;
    string u = U(yol);
    fwrite(u.data(), 1, u.size(), f);
    fclose(f);
}
