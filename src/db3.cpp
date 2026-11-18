#include "db3.h"

#include <algorithm>
#include <map>

// ======================= Qarzdorlar =======================

static const char* QQ_SQL =
    "SELECT d.id, COALESCE((SELECT SUM(jami - qaytarilgan_summa) FROM qarz_qarz WHERE qarzdor_id=d.id),0),"
    " COALESCE((SELECT SUM(jami_dollar - qaytarilgan_summa_dollar) FROM qarz_qarz WHERE qarzdor_id=d.id),0),"
    " COALESCE((SELECT SUM(summa) FROM qarz_tolov WHERE qarzdor_id=d.id AND valyuta='som'),0),"
    " COALESCE((SELECT SUM(summa) FROM qarz_tolov WHERE qarzdor_id=d.id AND valyuta='dollar'),0)"
    " FROM qarz_qarzdor d ";

QarzdorQ qarzdor_hisobi(long long id) {
    QarzdorQ r;
    qarzdor_ol(id, r.q);
    Sorov s = baza.sor(string(QQ_SQL) + "WHERE d.id=?");
    s.bog(1, id);
    if (s.qator()) {
        r.olgan = s.son(1);
        r.olgan_d = s.son(2);
        r.tolagan = s.son(3);
        r.tolagan_d = s.son(4);
    }
    return r;
}

std::vector<QarzdorQ> qarzdorlar_royxati(long long hudud_id) {
    std::map<long long, Qarzdor> qm;
    for (auto& q : qarzdorlar(L"")) qm[q.id] = q;
    std::vector<QarzdorQ> r;
    Sorov s = baza.sor(string(QQ_SQL) + (hudud_id ? "WHERE d.hudud_id=?" : ""));
    if (hudud_id) s.bog(1, hudud_id);
    while (s.qator()) {
        QarzdorQ x;
        x.q = qm[s.butun(0)];
        x.olgan = s.son(1);
        x.olgan_d = s.son(2);
        x.tolagan = s.son(3);
        x.tolagan_d = s.son(4);
        r.push_back(x);
    }
    std::sort(r.begin(), r.end(), [](const QarzdorQ& a, const QarzdorQ& b) {
        if (a.q.balans != b.q.balans) return a.q.balans > b.q.balans;
        if (a.q.balans_d != b.q.balans_d) return a.q.balans_d > b.q.balans_d;
        if (a.q.familiya != b.q.familiya) return a.q.familiya < b.q.familiya;
        return a.q.ism < b.q.ism;
    });
    return r;
}

std::vector<HududKarta> hudud_kartalari() {
    std::vector<HududKarta> r;
    std::map<long long, size_t> joy;
    for (auto& h : hududlar()) {
        joy[h.id] = r.size();
        r.push_back({h, 0, 0, 0});
    }
    for (auto& q : qarzdorlar(L"")) {
        auto it = joy.find(q.hudud_id);
        if (it == joy.end()) continue;
        auto& k = r[it->second];
        k.soni++;
        k.balans += q.balans;
        k.balans_d += q.balans_d;
    }
    return r;
}

bool hudud_ol(long long id, Hudud& h) {
    Sorov s = baza.sor("SELECT id, nom FROM qarz_hudud WHERE id=?");
    s.bog(1, id);
    if (!s.qator()) return false;
    h = {s.butun(0), s.matn(1)};
    return true;
}

static std::vector<KartaQator> qatorlar_ol(const char* sql, long long id) {
    std::vector<KartaQator> r;
    Sorov s = baza.sor(sql);
    s.bog(1, id);
    while (s.qator())
        r.push_back({s.matn(0), s.matn(1), s.son(2) - s.son(3), s.son(3), s.matn(4) == L"dollar"});
    return r;
}

std::vector<KartaQarz> karta_qarzlari(long long qarzdor_id) {
    std::vector<KartaQarz> r;
    Sorov s = baza.sor(
        "SELECT q.id, q.sana, q.yakunlangan, COALESCE(q.sotuv_id,0), q.jami - q.qaytarilgan_summa,"
        " q.jami_dollar - q.qaytarilgan_summa_dollar, q.qaytarilgan_summa, q.qaytarilgan_summa_dollar,"
        " COALESCE((SELECT SUM(summa) FROM qarz_tolov WHERE qarz_id=q.id AND valyuta='som'),0),"
        " COALESCE((SELECT SUM(summa) FROM qarz_tolov WHERE qarz_id=q.id AND valyuta='dollar'),0)"
        " FROM qarz_qarz q WHERE q.qarzdor_id=? ORDER BY q.sana DESC, q.id DESC");
    s.bog(1, qarzdor_id);
    while (s.qator()) {
        KartaQarz k;
        k.id = s.butun(0);
        k.sana = s.matnU(1);
        k.yakunlangan = s.butun(2) != 0;
        k.sotuv_id = s.butun(3);
        k.sof = s.son(4);
        k.sof_d = s.son(5);
        k.qayt = s.son(6);
        k.qayt_d = s.son(7);
        k.old = s.son(8);
        k.old_d = s.son(9);
        r.push_back(k);
    }
    for (auto& k : r) {
        k.q = qatorlar_ol("SELECT q.mahsulot_nomi, q.birlik, q.miqdor, q.qaytarilgan, COALESCE(m.valyuta,'')"
                          " FROM qarz_qarzqator q LEFT JOIN ombor_mahsulot m ON m.id=q.mahsulot_id"
                          " WHERE q.qarz_id=? ORDER BY q.id", k.id);
        if (k.sotuv_id) {
            auto ch = qatorlar_ol("SELECT q.mahsulot_nomi, q.birlik, q.miqdor, q.qaytarilgan,"
                                  " COALESCE(m.valyuta,'') FROM sotuv_sotuvqator q LEFT JOIN ombor_mahsulot m"
                                  " ON m.id=q.mahsulot_id WHERE q.sotuv_id=? ORDER BY q.id", k.sotuv_id);
            k.q.insert(k.q.end(), ch.begin(), ch.end());
        }
    }
    return r;
}

std::vector<KartaTolov> karta_tolovlari(long long qarzdor_id) {
    std::vector<KartaTolov> r;
    Sorov s = baza.sor("SELECT id, sana, summa, valyuta FROM qarz_tolov WHERE qarzdor_id=?"
                       " ORDER BY sana DESC, id DESC");
    s.bog(1, qarzdor_id);
    while (s.qator()) r.push_back({s.butun(0), s.matnU(1), s.son(2), s.matn(3) == L"dollar"});
    return r;
}

void tolov_ochir(long long id) { baza.sor("DELETE FROM qarz_tolov WHERE id=?").bog(1, id).bajar(); }

// ======================= Hisobot =======================

Hisobot hisobot_ol(const SYSTEMTIME& dan, const SYSTEMTIME& gacha, long long kassir_id) {
    string b1, o1, b2, o2;
    kun_oraligi(dan, b1, o1);
    kun_oraligi(gacha, b2, o2);
    if (b2 < b1) std::swap(b1, b2), std::swap(o1, o2);
    Hisobot h;
    string shart = " FROM sotuv_sotuv s LEFT JOIN auth_user u ON u.id=s.kassir_id WHERE s.yakunlangan=1"
                   " AND s.bekor_qilingan=0 AND s.sana>=? AND s.sana<?" +
                   string(kassir_id ? " AND s.kassir_id=?" : "");
    auto bog = [&](Sorov& q) {
        q.bog(1, b1).bog(2, o2);
        if (kassir_id) q.bog(3, kassir_id);
    };
    {
        Sorov q = baza.sor("SELECT count(*), COALESCE(SUM(s.jami - s.qaytarilgan_summa),0),"
                           " COALESCE(SUM(s.jami_dollar - s.qaytarilgan_summa_dollar),0)" + shart);
        bog(q);
        if (q.qator()) {
            h.soni = (int)q.butun(0);
            h.som = q.son(1);
            h.dollar = q.son(2);
        }
    }
    {
        Sorov q = baza.sor("SELECT COALESCE(u.username, ''), count(*), SUM(s.jami - s.qaytarilgan_summa),"
                           " SUM(s.jami_dollar - s.qaytarilgan_summa_dollar)" + shart +
                           " GROUP BY s.kassir_id ORDER BY u.username");
        bog(q);
        while (q.qator()) {
            wstring k = q.matn(0);
            h.kassalar.push_back({k.empty() ? L"Noma'lum" : k, (int)q.butun(1), q.son(2), q.son(3)});
        }
    }
    {
        Sorov q = baza.sor("SELECT s.id, s.sana, COALESCE(u.username,''), s.jami - s.qaytarilgan_summa,"
                           " s.jami_dollar - s.qaytarilgan_summa_dollar" + shart +
                           " ORDER BY s.sana DESC, s.id DESC LIMIT 500");
        bog(q);
        while (q.qator()) {
            wstring k = q.matn(2);
            h.cheklar.push_back({q.butun(0), q.matnU(1), k.empty() ? L"Noma'lum" : k, q.son(3), q.son(4)});
        }
    }
    return h;
}

std::vector<std::pair<long long, wstring>> kassirlar() {
    std::vector<std::pair<long long, wstring>> r;
    Sorov s = baza.sor("SELECT id, username FROM auth_user WHERE id IN (SELECT kassir_id FROM sotuv_sotuv"
                       " WHERE yakunlangan=1) ORDER BY username");
    while (s.qator()) r.push_back({s.butun(0), s.matn(1)});
    return r;
}

// ======================= Kunlik sotuvlar =======================

KunSotuvlari kun_sotuvlari(const SYSTEMTIME& kun) {
    string b, o;
    kun_oraligi(kun, b, o);
    KunSotuvlari k;
    Sorov s = baza.sor(
        "SELECT s.id, s.sana, s.bekor_qilingan, s.jami - s.qaytarilgan_summa,"
        " s.jami_dollar - s.qaytarilgan_summa_dollar, s.qaytarilgan_summa, s.qaytarilgan_summa_dollar,"
        " COALESCE(q.id,0), COALESCE(q.qarzdor_id,0), COALESCE(d.familiya || ' ' || d.ism,''),"
        " COALESCE(q.jami - q.qaytarilgan_summa,0), COALESCE(q.jami_dollar - q.qaytarilgan_summa_dollar,0),"
        " COALESCE(q.qaytarilgan_summa,0), COALESCE(q.qaytarilgan_summa_dollar,0),"
        " (SELECT count(*) FROM sotuv_sotuvqator x WHERE x.sotuv_id=s.id AND x.qaytarilgan>0)"
        " FROM sotuv_sotuv s LEFT JOIN qarz_qarz q ON q.sotuv_id=s.id LEFT JOIN qarz_qarzdor d"
        " ON d.id=q.qarzdor_id WHERE s.sana>=? AND s.sana<? AND (s.yakunlangan=1 OR s.bekor_qilingan=1)"
        " ORDER BY s.sana DESC, s.id DESC");
    s.bog(1, b).bog(2, o);
    while (s.qator()) {
        SotuvKun x;
        x.id = s.butun(0);
        x.sana = s.matnU(1);
        x.bekor = s.butun(2) != 0;
        x.sof = s.son(3);
        x.sof_d = s.son(4);
        x.qayt = s.son(5);
        x.qayt_d = s.son(6);
        x.qarz_bor = s.butun(7) != 0;
        x.qarzdor_id = s.butun(8);
        x.qarzdor = s.matn(9);
        x.qarz_sof = s.son(10);
        x.qarz_sof_d = s.son(11);
        x.qarz_qayt = s.son(12);
        x.qarz_qayt_d = s.son(13);
        x.qaytarilgan = x.qayt > 0 || x.qayt_d > 0 || x.qarz_qayt > 0 || x.qarz_qayt_d > 0 || s.butun(14) > 0;
        if (!x.bekor) {
            k.soni++;
            k.jami += x.sof;
            k.jami_d += x.sof_d;
            k.qarzga += x.qarz_sof;
            k.qarzga_d += x.qarz_sof_d;
        }
        k.s.push_back(x);
    }
    for (auto& x : k.s) {
        Chek c;
        if (chek_ol(x.id, c)) x.q = c.qatorlar;
    }
    return k;
}

// ======================= Ombor =======================

static std::vector<OmborTovar> kesh;
static bool kesh_bor = false;

void ombor_keshini_tashla() { kesh_bor = false; }

static void kesh_yukla() {
    if (kesh_bor) return;
    kesh.clear();
    std::map<long long, wstring> sh;
    Sorov s = baza.sor("SELECT id, COALESCE(shtrix_kod,'') FROM ombor_mahsulot");
    while (s.qator()) sh[s.butun(0)] = s.matn(1);
    for (auto& t : tovarlarni_ol(true)) {
        OmborTovar o{t, sh[t.id]};
        o.t.izlash = kichik(t.kod + L" " + t.nom + L" " + o.shtrix);
        kesh.push_back(o);
    }
    kesh_bor = true;
}

std::vector<const OmborTovar*> ombor_filtrla(const OmborFiltr& f) {
    kesh_yukla();
    auto b = bolakla(f.matn);
    std::vector<const OmborTovar*> r;
    for (auto& o : kesh) {
        if (!f.yashirilgan && !o.t.faol) continue;
        if (f.valyuta == 1 && o.t.dollarmi()) continue;
        if (f.valyuta == 2 && !o.t.dollarmi()) continue;
        if (f.tugagan && o.t.qoldiq > 0) continue;
        if (!mos_keladi(o.t.izlash, b)) continue;
        r.push_back(&o);
    }
    std::stable_sort(r.begin(), r.end(), [&](const OmborTovar* a, const OmborTovar* b2) {
        if (f.kam && a->t.qoldiq != b2->t.qoldiq) return a->t.qoldiq < b2->t.qoldiq;
        return a->t.kod < b2->t.kod;
    });
    return r;
}

long long tugagan_soni() {
    Sorov s = baza.sor("SELECT count(*) FROM ombor_mahsulot WHERE faol=1 AND qoldiq<=0");
    return s.qator() ? s.butun(0) : 0;
}

bool shtrix_saqla(long long id, const wstring& kod, wstring& xato) {
    wstring k = tozala(kod);
    if (!k.empty() && (!raqammi(k) || k.size() > 14)) {
        xato = L"Shtrix kod faqat raqamlardan iborat bo'lsin (14 tagacha).";
        return false;
    }
    try {
        Sorov s = baza.sor("UPDATE ombor_mahsulot SET shtrix_kod=? WHERE id=?");
        if (k.empty()) s.bosh(1); else s.bog(1, k);
        s.bog(2, id).bajar();
        ombor_keshini_tashla();
        return true;
    } catch (const BazaXato&) {
        xato = L"Bu shtrix kod boshqa tovarda bor.";
        return false;
    }
}

// ======================= Sana =======================

bool sana_oqi(const wstring& s, SYSTEMTIME& t) {
    int d = 0, m = 0, y = 0;
    if (swscanf(s.c_str(), L"%d.%d.%d", &d, &m, &y) != 3 || d < 1 || d > 31 || m < 1 || m > 12 || y < 2000)
        return false;
    ZeroMemory(&t, sizeof t);
    t.wDay = (WORD)d;
    t.wMonth = (WORD)m;
    t.wYear = (WORD)y;
    return true;
}

wstring sana_yoz(const SYSTEMTIME& t) {
    wchar_t b[16];
    swprintf(b, 16, L"%02d.%02d.%04d", t.wDay, t.wMonth, t.wYear);
    return b;
}
