// Tovar qaytarish (vozvrat) — saytdagi sotuv/views.qaytarish, qarz/views.qator_qaytarish,
// sotuv/chek_views.qaytarish_cheki va templates/qaytarish.html bilan bir xil.
// Qaytarish qisman bo'lishi mumkin (20 qopdan 5 tasi), pul qo'lda yoziladi.
#include "chek_chop.h"
#include "db3.h"
#include "ilova.h"

#include <algorithm>
#include <cmath>

// ======================= Baza =======================

namespace {

struct QQator {
    long long id = 0, hujjat = 0, mahsulot = 0, qarz_id = 0, qarzdor_id = 0;
    bool sotuvdan = true, yakunlangan = false, bekor = false, dollar = false;
    wstring nom, birlik, qarzdor;
    double miqdor = 0, qaytarilgan = 0;
    string sana;
    double jami = 0, jami_d = 0;            // hujjatda qolgani (sof)
    double qarz_sof = 0, qarz_sof_d = 0;    // chekning qarzga yozilgan qismi
    double balans = 0, balans_d = 0;        // qarzdorning hozirgi qarzi
    double qolgan() const { return yaxlit(miqdor - qaytarilgan, 3); }
};

const char* BALANS_SQL =
    "COALESCE((SELECT SUM(jami - qaytarilgan_summa) FROM qarz_qarz WHERE qarzdor_id=d.id),0)"
    " - COALESCE((SELECT SUM(summa) FROM qarz_tolov WHERE qarzdor_id=d.id AND valyuta='som'),0),"
    " COALESCE((SELECT SUM(jami_dollar - qaytarilgan_summa_dollar) FROM qarz_qarz WHERE qarzdor_id=d.id),0)"
    " - COALESCE((SELECT SUM(summa) FROM qarz_tolov WHERE qarzdor_id=d.id AND valyuta='dollar'),0)";

bool qator_ol(bool sotuvdan, long long id, QQator& q) {
    q = {};
    q.sotuvdan = sotuvdan;
    if (sotuvdan) {
        Sorov s = baza.sor(
            "SELECT x.id, x.sotuv_id, x.mahsulot_id, x.mahsulot_nomi, x.birlik, x.miqdor, x.qaytarilgan,"
            " COALESCE(m.valyuta,''), s.sana, s.yakunlangan, s.bekor_qilingan,"
            " s.jami - s.qaytarilgan_summa, s.jami_dollar - s.qaytarilgan_summa_dollar"
            " FROM sotuv_sotuvqator x JOIN sotuv_sotuv s ON s.id=x.sotuv_id"
            " LEFT JOIN ombor_mahsulot m ON m.id=x.mahsulot_id WHERE x.id=?");
        s.bog(1, id);
        if (!s.qator()) return false;
        q.id = s.butun(0);
        q.hujjat = s.butun(1);
        q.mahsulot = s.butun(2);
        q.nom = s.matn(3);
        q.birlik = s.matn(4);
        q.miqdor = s.son(5);
        q.qaytarilgan = s.son(6);
        q.dollar = s.matn(7) == L"dollar";
        q.sana = s.matnU(8);
        q.yakunlangan = s.butun(9) != 0;
        q.bekor = s.butun(10) != 0;
        q.jami = s.son(11);
        q.jami_d = s.son(12);
        // Chekning bir qismi qarzga yozilganmi (sotuv.qarz_hujjati)
        Sorov z = baza.sor(string("SELECT q.id, q.jami - q.qaytarilgan_summa, q.jami_dollar - q.qaytarilgan_summa_dollar,"
                                  " d.id, d.familiya || ' ' || d.ism, ") + BALANS_SQL +
                           " FROM qarz_qarz q JOIN qarz_qarzdor d ON d.id=q.qarzdor_id WHERE q.sotuv_id=?");
        z.bog(1, q.hujjat);
        if (z.qator()) {
            q.qarz_id = z.butun(0);
            q.qarz_sof = z.son(1);
            q.qarz_sof_d = z.son(2);
            q.qarzdor_id = z.butun(3);
            q.qarzdor = z.matn(4);
            q.balans = z.son(5);
            q.balans_d = z.son(6);
        }
    } else {
        Sorov s = baza.sor(
            "SELECT x.id, x.qarz_id, x.mahsulot_id, x.mahsulot_nomi, x.birlik, x.miqdor, x.qaytarilgan,"
            " COALESCE(m.valyuta,''), q.sana, q.yakunlangan, q.jami - q.qaytarilgan_summa,"
            " q.jami_dollar - q.qaytarilgan_summa_dollar, d.id, d.familiya || ' ' || d.ism"
            " FROM qarz_qarzqator x JOIN qarz_qarz q ON q.id=x.qarz_id JOIN qarz_qarzdor d ON d.id=q.qarzdor_id"
            " LEFT JOIN ombor_mahsulot m ON m.id=x.mahsulot_id WHERE x.id=?");
        s.bog(1, id);
        if (!s.qator()) return false;
        q.id = s.butun(0);
        q.hujjat = s.butun(1);
        q.mahsulot = s.butun(2);
        q.nom = s.matn(3);
        q.birlik = s.matn(4);
        q.miqdor = s.son(5);
        q.qaytarilgan = s.son(6);
        q.dollar = s.matn(7) == L"dollar";
        q.sana = s.matnU(8);
        q.yakunlangan = s.butun(9) != 0;
        q.jami = s.son(10);
        q.jami_d = s.son(11);
        q.qarzdor_id = s.butun(12);
        q.qarzdor = s.matn(13);
    }
    return true;
}

wstring qarz_matn(double s, double d) {
    wstring r;
    if (s > 0) r = pul(s) + L" so'm";
    if (d > 0) r += (r.empty() ? L"" : L" va ") + son(d) + L" $";
    return r;
}

// Saytdagi qaytarishni_oqi + qaytarishni_taqsimla + yozish. xabar — muvaffaqiyat matni.
bool qaytarish_yoz(bool sotuvdan, long long id, const wstring& miqdor_m, const wstring& summa_m,
                   const wstring& dollar_m, wstring& xabar, wstring& xato) {
    Tranzaksiya tr;
    QQator q;
    if (!qator_ol(sotuvdan, id, q)) {
        xato = L"Qator topilmadi.";
        return false;
    }
    if (sotuvdan && (!q.yakunlangan || q.bekor)) {
        xato = L"Faqat yakunlangan chekdan tovar qaytarish mumkin.";
        return false;
    }
    if (!sotuvdan && !q.yakunlangan) {
        xato = L"Faqat yakunlangan hujjatdan tovar qaytariladi. Ochiq savatdagi qatorni o'chiring.";
        return false;
    }
    double miqdor = 0, summa = 0, dollar = 0;
    if (!songa(miqdor_m, miqdor)) {
        xato = L"Qaytarilgan miqdor xato kiritildi.";
        return false;
    }
    if (miqdor <= 0) {
        xato = L"Qaytarilgan miqdor noldan katta bo'lishi kerak.";
        return false;
    }
    miqdor = yaxlit(miqdor, 3);
    if (miqdor > q.qolgan() + 1e-9) {
        xato = L"Bunchasi sotilmagan: ko'pi bilan " + son(q.qolgan()) + L" " + q.birlik + L" qaytarish mumkin.";
        return false;
    }
    if (!tozala(summa_m).empty() && !songa(summa_m, summa)) {
        xato = L"Qaytarilgan pul (so'm) xato kiritildi.";
        return false;
    }
    if (!tozala(dollar_m).empty() && !songa(dollar_m, dollar)) {
        xato = L"Qaytarilgan pul (dollar) xato kiritildi.";
        return false;
    }
    if (summa < 0) {
        xato = L"Qaytarilgan pul (so'm) manfiy bo'lmaydi.";
        return false;
    }
    if (dollar < 0) {
        xato = L"Qaytarilgan pul (dollar) manfiy bo'lmaydi.";
        return false;
    }
    summa = yaxlit(summa, 2);
    dollar = yaxlit(dollar, 2);
    const double E = 1e-9;
    double qarzdan = 0, qarzdan_d = 0, naqddan = summa, naqddan_d = dollar;
    wstring izoh;
    if (sotuvdan) {
        // Pul avval chekning qarz qismidan ayriladi (ko'pi bilan qarzdorning hozirgi qarzicha)
        if (q.qarz_id) {
            double ch = std::max(std::min(q.qarz_sof, q.balans), 0.0);
            double chd = std::max(std::min(q.qarz_sof_d, q.balans_d), 0.0);
            qarzdan = std::min(summa, ch);
            qarzdan_d = std::min(dollar, chd);
        }
        naqddan = yaxlit(summa - qarzdan, 2);
        naqddan_d = yaxlit(dollar - qarzdan_d, 2);
        if (naqddan > q.jami + E || naqddan_d > q.jami_d + E) {
            xato = L"Qaytarilgan pul chek summasidan ko'p bo'lmasin.";
            return false;
        }
        izoh = L"Chek #" + std::to_wstring(q.hujjat) + L" dan qaytarildi";
    } else {
        if (summa > q.jami + E || dollar > q.jami_d + E) {
            xato = L"Qaytarilgan pul qarz summasidan ko'p bo'lmasin.";
            return false;
        }
        izoh = L"Qarz #" + std::to_wstring(q.hujjat) + L" dan qaytarildi — " + q.qarzdor;
    }
    // Ombor: tovar qaytdi (tarixda «Qaytarish» bo'lib turadi)
    baza.sor("UPDATE ombor_mahsulot SET qoldiq = qoldiq + ? WHERE id=?").bog(1, miqdor).bog(2, q.mahsulot).bajar();
    baza.sor("INSERT INTO ombor_omborharakati (tur, miqdor, izoh, sana, mahsulot_id, kiritilgan_birlik,"
             " kiritilgan_miqdor) VALUES ('qaytarish',?,?,?,?,'',NULL)")
        .bog(1, miqdor).bog(2, izoh).bog(3, hozir_utc()).bog(4, q.mahsulot).bajar();
    if (sotuvdan) {
        baza.sor("UPDATE sotuv_sotuvqator SET qaytarilgan = qaytarilgan + ? WHERE id=?").bog(1, miqdor).bog(2, id).bajar();
        baza.sor("UPDATE sotuv_sotuv SET qaytarilgan_summa = qaytarilgan_summa + ?,"
                 " qaytarilgan_summa_dollar = qaytarilgan_summa_dollar + ? WHERE id=?")
            .bog(1, naqddan).bog(2, naqddan_d).bog(3, q.hujjat).bajar();
        if (qarzdan > 0 || qarzdan_d > 0)
            baza.sor("UPDATE qarz_qarz SET qaytarilgan_summa = qaytarilgan_summa + ?,"
                     " qaytarilgan_summa_dollar = qaytarilgan_summa_dollar + ? WHERE id=?")
                .bog(1, qarzdan).bog(2, qarzdan_d).bog(3, q.qarz_id).bajar();
    } else {
        baza.sor("UPDATE qarz_qarzqator SET qaytarilgan = qaytarilgan + ? WHERE id=?").bog(1, miqdor).bog(2, id).bajar();
        baza.sor("UPDATE qarz_qarz SET qaytarilgan_summa = qaytarilgan_summa + ?,"
                 " qaytarilgan_summa_dollar = qaytarilgan_summa_dollar + ? WHERE id=?")
            .bog(1, summa).bog(2, dollar).bog(3, q.hujjat).bajar();
    }
    tr.tasdiq();
    xabar = L"Qaytarish yozildi, tovar omborga qaytdi.";
    if (sotuvdan) {
        if (qarzdan > 0 || qarzdan_d > 0)
            xabar += L" " + qarz_matn(qarzdan, qarzdan_d) + L" " + q.qarzdor + L" ning qarzidan ayrildi.";
        if (naqddan > 0 || naqddan_d > 0) xabar += L" Qo'lga qaytarildi: " + qarz_matn(naqddan, naqddan_d) + L".";
    }
    return true;
}

// ---- holat ----
bool qs_sotuv = true;
long long qs_id = 0;
Joy qs_orqa = J_SOTUVLAR;
long long qs_orqa_p = 0;
QQator qs;

void orqaga() { ot(qs_orqa, qs_orqa_p); }

}  // namespace



Joy qaytarish_qaytadigan_joy() { return qs_orqa; }

void qaytarish_och(bool sotuvdan, long long qator_id, Joy orqa, long long orqa_p) {
    qs_sotuv = sotuvdan;
    qs_orqa = orqa;
    qs_orqa_p = orqa_p;
    ot(J_QAYTARISH, qator_id);
}

void qaytarish_ochildi(long long id) {
    qs_id = id;
    ui.maydonlarni_tozala("qy/");
    if (!qator_ol(qs_sotuv, id, qs)) qs = {};
    if (qs.id && qs_sotuv && (!qs.yakunlangan || qs.bekor)) {
        ui.xabar(L"Faqat yakunlangan chekdan tovar qaytarish mumkin.", X_XATO);
        qs = {};
    } else if (qs.id && !qs_sotuv && !qs.yakunlangan) {
        ui.xabar(L"Faqat yakunlangan hujjatdan tovar qaytariladi. Ochiq savatdagi qatorni o'chiring.", X_XATO);
        qs = {};
    }
    ui.fokus("qy/miqdor");
}

void qaytarish_chiz(R r) {
    if (!qs.id) {
        sarlavha(r, L"Qaytarib berish");
        if (ui.tugma("qy/orqa0", {r.x, r.y, 200, 50}, L"Orqaga", TINCH, "i-chap")) orqaga();
        return;
    }
    float en = std::min(720.f, r.w);
    R s = {r.x + (r.w - en) / 2, r.y, en, r.h};
    wstring hujjat = qs_sotuv ? L"chek #" + std::to_wstring(qs.hujjat)
                              : L"hujjat #" + std::to_wstring(qs.hujjat) + L" · " + qs.qarzdor;
    sarlavha(s, L"Qaytarib berish", L"— " + hujjat + L" · " + mahalliy_vaqt(qs.sana, false));
    // Ko'rsatkichlar
    R k = {s.x, s.y, s.w, 14 + 92 + 14};
    quti(k);
    float kw = (s.w - 28 - 28) / 3, x = s.x + 14, y = s.y + 14;
    korsatkich({x, y, kw, 92}, L"TOVAR", qs.nom, qs.dollar ? L"dollarda kelgan" : L"");
    korsatkich({x + kw + 14, y, kw, 92}, L"OLINGAN", son(qs.miqdor) + L" " + qs.birlik);
    korsatkich({x + 2 * (kw + 14), y, kw, 92}, L"QAYTARISH MUMKIN", son(qs.qolgan()) + L" " + qs.birlik,
               qs.qaytarilgan > 0 ? son(qs.qaytarilgan) + L" " + qs.birlik + L" avval qaytarilgan" : L"",
               qs.qaytarilgan > 0 ? 2 : 0);
    // Forma
    bool qarz_q = qs_sotuv && qs.qarz_id;
    float fb = 14 + (22 + 50 + 22) + 14 + (22 + 50 + 22) + 14 + (qarz_q ? 64 + 14 : 0) + 50 + 14;
    R f = {s.x, k.p() + 14, s.w, fb};
    quti(f);
    x = f.x + 14;
    y = f.y + 14;
    float w = f.w - 28, yw = (w - 14) / 2;
    gfx.matn({x, y, w, 17}, L"Qancha qaytarildi (" + qs.birlik + L")", 12, 600, M->matn2);
    MaydonSoz ms;
    ms.ong = true;
    ms.ishora = L"0";
    ui.maydon("qy/miqdor", {x, y + 22, w, 50}, ms);
    gfx.matn({x, y + 22 + 50 + 4, w, 17}, L"Hammasi emas, bir qismi qaytsa ham bo'ladi — masalan 20 qopdan 5 tasi", 12,
             400, M->matn3);
    y += 22 + 50 + 22 + 14;
    gfx.matn({x, y, yw, 17}, L"Qaytarilgan pul (so'm)", 12, 600, M->matn2);
    gfx.matn({x + yw + 14, y, yw, 17}, L"Qaytarilgan pul ($)", 12, 600, M->matn2);
    ui.maydon("qy/summa", {x, y + 22, yw, 50}, ms);
    ui.maydon("qy/dollar", {x + yw + 14, y + 22, yw, 50}, ms);
    gfx.matn({x, y + 76, yw, 17}, L"Hujjatda qolgani: " + pul(qs.jami) + L" so'm", 12, 400, M->matn3);
    gfx.matn({x + yw + 14, y + 76, yw, 17}, L"Hujjatda qolgani: " + son(qs.jami_d) + L" $", 12, 400, M->matn3);
    y += 22 + 50 + 22 + 14;
    if (qarz_q) {
        // Chekning bir qismi qarzga yozilgan — pul qayerga tushishi oldindan ko'rinsin
        R o = {x, y, w, 64};
        gfx.quti(o, M->ogoh_fon, 9, M->ogoh_chek);
        gfx.matn(o.ichki(14, 8),
                 L"Chekdan " + qarz_matn(qs.qarz_sof, qs.qarz_sof_d) + L" " + qs.qarzdor +
                     L" ning qarziga yozilgan. Qaytarilgan pul avval shu qarzdan ayriladi, qolgani mijozning "
                     L"qo'liga qaytariladi.",
                 13, 500, M->ogoh, CHAP, 1, true);
        y += 64 + 14;
    }
    float b1 = (w - 14) / 3;
    if (ui.tugma("qy/bekor", {x, y, b1, 50}, L"Bekor qilish", TINCH, "i-chap")) {
        orqaga();
        return;
    }
    bool yoz = ui.tugma("qy/yoz", {x + b1 + 14, y, w - b1 - 14, 50}, L"Qaytarishni yozish", YASHIL, "i-kirim") ||
               ui.enter("qy/miqdor") || ui.enter("qy/summa") || ui.enter("qy/dollar");
    if (yoz) {
        wstring xabar, xato;
        bool ok = false;
        try {
            ok = qaytarish_yoz(qs_sotuv, qs.id, ui.qiymat("qy/miqdor"), ui.qiymat("qy/summa"), ui.qiymat("qy/dollar"),
                               xabar, xato);
        } catch (const BazaXato& e) {
            xato = L"Baza xatosi: " + W(e.what());
        }
        if (ok) {
            ombor_keshini_tashla();
            ombor_eskirdi();
            ui.xabar(xabar);
            orqaga();
        } else
            ui.xabar(xato, X_XATO);
    }
}

// ======================= Chek bo'yicha qaytarish (sotuv:chek_izla) =======================

namespace {
long long cq_id = 0;
struct CqQator {
    long long id;
    wstring nom, birlik;
    double miqdor, qaytarilgan;
};
std::vector<CqQator> cq;
string cq_sana;

// Savdo chekining shtrixi: 88 + chek raqami (9) + nazorat (sotuv/chek.py: och)
long long chek_kodini_och(const wstring& matn) {
    wstring t = tozala(matn);
    if (t.size() != 12 || !raqammi(t) || t.rfind(L"88", 0) != 0) return 0;
    if (ean_nazorat(t.substr(0, 11)) != t[11] - L'0') return 0;
    return std::stoll(t.substr(2, 9));
}

bool cq_yukla(long long id) {
    cq.clear();
    Sorov s = baza.sor("SELECT sana FROM sotuv_sotuv WHERE id=? AND yakunlangan=1 AND bekor_qilingan=0");
    s.bog(1, id);
    if (!s.qator()) return false;
    cq_sana = s.matnU(0);
    Sorov q = baza.sor("SELECT id, mahsulot_nomi, birlik, miqdor, qaytarilgan FROM sotuv_sotuvqator"
                       " WHERE sotuv_id=? ORDER BY id");
    q.bog(1, id);
    while (q.qator()) cq.push_back({q.butun(0), q.matn(1), q.matn(2), q.son(3), q.son(4)});
    return true;
}
}  // namespace

void chek_qaytarish_ochildi(long long id) {
    ui.maydonlarni_tozala("cq/");
    cq_id = 0;
    cq.clear();
    if (id) {
        if (cq_yukla(id)) cq_id = id;
        else ui.xabar(L"Chek topilmadi yoki yakunlanmagan.", X_XATO);
    }
    ui.fokus("cq/kod");
}

void chek_qaytarish_chiz(R r) {
    sarlavha(r, L"Chek bo'yicha qaytarish");
    R f = {r.x, r.y, r.w, 14 + 50 + 14};
    quti(f);
    float kb = gfx.matn_eni(L"Chekni topish", 15, 600) + 36;
    MaydonSoz ms;
    ms.ishora = L"Savdo chekining shtrix kodini skanerlang";
    ui.maydon("cq/kod", {f.x + 14, f.y + 14, f.w - 28 - kb - 14, 50}, ms);
    bool top = ui.tugma("cq/top", {f.o() - 14 - kb, f.y + 14, kb, 50}, L"Chekni topish") || ui.enter("cq/kod");
    wstring kod = ui.qiymat("cq/kod");
    if (!ui.skaner.empty()) {  // fokus boshqa joyda bo'lsa ham skaner o'qigani
        kod = ui.skaner;
        ui.skaner.clear();
        top = true;
    }
    if (top && !tozala(kod).empty()) {
        long long id = chek_kodini_och(kod);
        if (!id) {
            ui.xabar(L"Savdo chekining shtrix kodi noto'g'ri.", X_XATO);
            ui.fokus("cq/kod");
        }
        else {
            ot(J_CHEK_QAYTARISH, id);
            return;
        }
        ui.qiymat_qoy("cq/kod", L"");
    }
    float y = f.p() + 14;
    if (!cq_id) {
        gfx.matn({r.x, y, r.w, 20}, L"Shtrix kod chekni topadi. Skanerlashning o'zi pul yoki qoldiqni o'zgartirmaydi.", 14,
                 400, M->matn3);
        return;
    }
    R h = {r.x, y, r.w, 60};
    quti(h);
    wstring sar = L"Chek #" + std::to_wstring(cq_id) + L" · " + mahalliy_vaqt(cq_sana);
    gfx.matn({h.x + 14, h.y, h.w, 60}, sar, 15, 600, M->matn);
    float sw = gfx.matn_eni(sar, 15, 600);
    if (ui.tugma("cq/kor", {h.x + 14 + sw + 14, h.y + 13, gfx.matn_eni(L"Chekni ko'rish", 13, 600) + 22, 34},
                 L"Chekni ko'rish", TINCH, nullptr, true))
        chek_oynasini_och(cq_id);
    y += 60 + 14;
    std::vector<Ustun> u = {{L"Mahsulot", -1}, {L"Sotilgan", 180}, {L"Qaytarilgan", 180}, {L"", 210}};
    float qb = 50;
    R j = {r.x, y, r.w, 38 + std::max<size_t>(cq.size(), 1) * qb};
    gfx.quti(j, M->panel, 12, M->chegara);
    jadval_sarlavha({j.x, j.y, j.w, 38}, u);
    for (size_t i = 0; i < cq.size(); ++i) {
        auto& z = cq[i];
        R row = {j.x, j.y + 38 + i * qb, j.w, qb};
        if (i + 1 < cq.size()) gfx.chiziq(row.x, row.p() - .5f, row.o(), row.p() - .5f, M->chegara);
        auto c = ustunlar(row, u);
        gfx.matn(c[0].ichki(13, 0), z.nom, 15, 400, M->matn);
        gfx.matn(c[1].ichki(13, 0), son(z.miqdor) + L" " + z.birlik, 15, 400, M->matn, ONG);
        gfx.matn(c[2].ichki(13, 0), son(z.qaytarilgan) + L" " + z.birlik, 15, 400, M->matn, ONG);
        if (z.miqdor - z.qaytarilgan > 1e-9) {
            if (ui.tugma("cq/q/" + std::to_string(z.id), {c[3].x + 13, row.y + 8, 140, 34}, L"Qaytarish", ODDIY,
                         "i-kirim", true)) {
                qaytarish_och(true, z.id, J_CHEK_QAYTARISH, cq_id);
                return;
            }
        } else
            gfx.matn(c[3].ichki(13, 0), L"To'liq qaytarilgan", 13, 400, M->matn3);
    }
    gfx.matn({r.x, j.p() + 12, r.w, 20}, L"Qaytarilayotgan mahsulotni tanlang. Miqdor va qaytariladigan pul keyingi "
                                         L"oynada kiritiladi.", 13, 400, M->matn3);
}
