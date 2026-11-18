// SOTUV — kassa ekrani (templates/sotuv/tahrir.html, kassa_ong_panel.html, kassa_jami_panel.html,
// kassa_tovar_oyna.html, sotuv/qarz_oyna.html, kassa_miqdor_oyna.html va js/kassa.js mantiqi).
#include "ilova.h"

#include "chek_chop.h"

namespace {

long long sotuv_id = 0;  // hujjat raqami: sotuv yoki qarz
bool qarzmi = false;     // «Qarz yozish» ekrani (qarz/qarz_tahrir.html)
Qarzdor qd;
double oldingi = 0, oldingi_d = 0;
string sana;
HujjatTur tur() { return qarzmi ? H_QARZ : H_SOTUV; }
std::vector<Qator> qatorlar;
Tovar tanlangan;
bool tovar_bor = false;
long long yangi_qator = 0;
DWORD yangi_vaqt = 0;
// qarz
long long qarzdor_id = 0;
wstring qarzdor_nomi;
double tolandi = 0, tolandi_d = 0;
// oynalar
bool tovar_oyna = false, qarz_oyna = false;
long long miqdor_qator = 0;
std::vector<Tovar> oyna_tl;
// qidiruv
std::vector<Tovar> hamma;
bool hamma_eskirgan = true;
int dd_nishon = 0;
bool dd_ochiq = false;

void hammani_yukla() {
    if (hamma_eskirgan) {
        hamma = tovarlarni_ol(false);
        hamma_eskirgan = false;
    }
}

void qayta_yukla(bool summalar = true) {
    qatorlar = hujjat_qatorlari(tur(), sotuv_id);
    if (summalar) {
        AvtoHisob h = avto_hisob(qatorlar);
        ui.qiymat_qoy("sot/jami", h.toliq && h.som > 0 ? pul(h.som) : L"");
        ui.qiymat_qoy("sot/jamid", h.toliq && h.dollar > 0 ? pul(h.dollar) : L"");
    }
}

void tozala_tanlov() {
    tovar_bor = false;
    ui.qiymat_qoy("sot/miqdor", L"");
    ui.qiymat_qoy("sot/kod", L"");
    ui.qiymat_qoy("sot/qidir", L"");
    dd_ochiq = false;
}

void tanla(const Tovar& t, double miqdor) {
    tanlangan = t;
    tovar_bor = true;
    ui.qiymat_qoy("sot/miqdor", miqdor > 0 ? son(miqdor) : L"");
    ui.qiymat_qoy("sot/kod", L"");
    ui.qiymat_qoy("sot/qidir", L"");
    dd_ochiq = false;
    tovar_oyna = false;
    if (!sensor) ui.fokus("sot/miqdor");
}

void qosh() {
    if (!tovar_bor) return;
    double m;
    wstring mq = tozala(ui.qiymat("sot/miqdor"));
    if (mq.empty() || !songa(mq, m) || m <= 0) {
        ui.xabar(L"Miqdor: noldan katta son yozing.", X_XATO);
        ui.fokus("sot/miqdor");
        return;
    }
    wstring x;
    long long q = hujjat_qator_qosh(tur(), sotuv_id, tanlangan.id, yaxlit(m, 3), x);
    if (!q) {
        ui.xabar(x, X_XATO);
        return;
    }
    yangi_qator = q;
    yangi_vaqt = GetTickCount();
    SetTimer(ui.h, 78, 1900, nullptr);
    hamma_eskirgan = true;
    tozala_tanlov();
    qayta_yukla();
    for (size_t i = 0; i < qatorlar.size(); ++i)
        if (qatorlar[i].id == q) ui.scroll_korsat("sot/chek", i * 55.f, 55);
    if (!sensor) ui.fokus("sot/kod");
}

// kod / skaner: skanerdan — darrov chekka qo'shiladi
void kod_bilan(const wstring& kod, bool skanerdan) {
    Tovar t;
    double km;
    wstring x;
    if (!kod_bilan_top(kod, t, km, x)) {
        if (!t.faol && t.id) x = t.nom + L" ombordan yashirilgan — avval faollashtiring.";
        ui.xabar(x, X_XATO);
        return;
    }
    if (t.qoldiq <= 0) {
        ui.xabar(t.nom + L" omborda tugagan.", X_XATO);
        return;
    }
    if (skanerdan) {
        double oldindan = 0;
        if (!tovar_bor) songa(ui.qiymat("sot/miqdor"), oldindan);
        tanla(t, km > 0 ? km : oldindan > 0 ? oldindan : 1);
        qosh();
        return;
    }
    tanla(t, km);
    if (km > 0) ui.xabar(t.nom + L": " + son(km) + L" " + t.birlik, X_INFO);
}

wstring qarz_matn(double s, double d) {
    wstring r;
    if (s > 0) r = pul(s) + L" so'm";
    if (d > 0) r += (r.empty() ? L"" : L" va ") + son(d) + L" $";
    return r;
}

double maydon_son(const string& id) {
    double v = 0;
    songa(ui.qiymat(id), v);
    return v;
}

void qarz_yakunla_() {
    long long karta = qd.id;
    if (qarz_bosh_bolsa_ochir(sotuv_id)) {
        ui.xabar(L"Bo'sh qarz bekor qilindi.", X_INFO);
        ot(J_KARTA, karta);
        return;
    }
    double j = 0, jd = 0, k = 0, o = 0, od = 0;
    if (!songa(ui.qiymat("sot/jami"), j) || !songa(ui.qiymat("sot/jamid"), jd) ||
        !songa(ui.qiymat("sot/kurs"), k) || !songa(ui.qiymat("sot/old"), o) || !songa(ui.qiymat("sot/oldd"), od)) {
        ui.xabar(L"Summa noto'g'ri yozilgan — faqat raqam yozing.", X_XATO);
        return;
    }
    wstring x, tolandi_;
    if (!qarz_yakunla(sotuv_id, j, jd, k, o, od, x, tolandi_)) {
        ui.xabar(x, X_XATO);
        return;
    }
    hamma_eskirgan = true;
    ui.xabar(L"Qarz daftarga yozildi." + (tolandi_.empty() ? L"" : L" Oldindan to'langani: " + tolandi_ + L"."));
    ot(J_KARTA, karta);
}

void yakunla() {
    if (qarzmi) {
        qarz_yakunla_();
        return;
    }
    if (qatorlar.empty()) {
        sotuv_bekor(sotuv_id);
        ui.xabar(L"Bo'sh sotuv bekor qilindi.", X_INFO);
        ot(J_BOSH);
        return;
    }
    double j, jd, k;
    if (!songa(ui.qiymat("sot/jami"), j) || !songa(ui.qiymat("sot/jamid"), jd) ||
        !songa(ui.qiymat("sot/kurs"), k)) {
        ui.xabar(L"Summa noto'g'ri yozilgan — faqat raqam yozing.", X_XATO);
        return;
    }
    if (j <= 0 && jd <= 0) {
        ui.xabar(L"Jami summasini yozing — so'mda yoki dollarda.", X_XATO);
        ui.fokus("sot/jami");
        return;
    }
    if (jd > 0 && k <= 0) {
        ui.xabar(L"Dollar summasi yozildi — o'sha kungi kursni ham yozing.", X_XATO);
        ui.fokus("sot/kurs");
        return;
    }
    ChekNatija n;
    wstring x;
    if (!sotuv_yakunla_v2(sotuv_id, foydalanuvchi.id, j, jd, k, qarzdor_id, tolandi, tolandi_d, n, x)) {
        ui.xabar(x, X_XATO);
        return;
    }
    if (!qarzdor_id)
        ui.xabar(L"Chek #" + std::to_wstring(n.sotuv_id) + L" yakunlandi — " + qarz_matn(j, jd) + L".");
    else {
        wstring naqd = qarz_matn(tolandi, tolandi_d);
        ui.xabar(L"Chek #" + std::to_wstring(n.sotuv_id) + L" yakunlandi. " +
                 qarz_matn(j - tolandi, jd - tolandi_d) + L" " + qarzdor_nomi + L" ning daftariga yozildi" +
                 (naqd.empty() ? L"." : L", naqd to'landi: " + naqd + L"."));
    }
    chekni_avtomatik_chop(n.sotuv_id);
    hamma_eskirgan = true;
    sotuv_ochildi(0);
}

// ---------- o'ng panel ----------

void ong_panel(R r) {
    gfx.quti(r, M->panel, 12, M->chegara);
    R p = r.ichki(12);
    float y = p.y;
    // Yangi tovar (F2)
    R yt = {p.x, y, p.w, 50};
    if (ui.tugma("sot/yangi", yt, L"Yangi tovar", ASOSIY, "i-plus")) { tovar_oyna = true; oyna_tl = oyna_tovarlari(); }
    if (!sensor) {
        float tw = gfx.matn_eni(L"Yangi tovar", 15, 600);
        yorliq_belgi(yt.x + (yt.w + 27 + tw) / 2 + 8, yt.y + 25, L"F2", M->asosiy_matn);
    }
    y += 50 + 8;
    // qidiruv
    R qd = {p.x, y, p.w, 50};
    MaydonSoz qs;
    qs.ishora = L"Tovar nomi yoki kodi";
    ui.maydon("sot/qidir", qd, qs);
    if (ui.ozgardi("sot/qidir")) {
        dd_ochiq = !tozala(ui.qiymat("sot/qidir")).empty();
        dd_nishon = 0;
        hammani_yukla();
    }
    y += 50 + 8;
    // KOD
    R km = {p.x, y, p.w, 50};
    bool kfok = ui.fokusda("sot/kod");
    if (kfok) gfx.quti({km.x - 3, km.y - 3, km.w + 6, km.h + 6}, M->urgu_fon, 12);
    gfx.quti(km, M->panel2, 9, kfok ? M->urgu : M->chegara2);
    gfx.matn({km.x + 12, km.y, 40, km.h}, L"KOD", 11, 600, M->matn3);
    MaydonSoz ks;
    ks.ong = true;
    ks.ogirlik = 700;
    ks.ishora = L"0000";
    ks.olcham = 19;
    ui.tahrir("sot/kod", {km.x + 50, km.y, km.w - 50 - 50, km.h}, ks);
    bool izla = ui.tugma("sot/kodtop", {km.o() - 40, km.y + 8, 34, 34}, L"", TINCH, "i-qidir", true);
    if (ui.bosildi("sot/kodquti", {km.x, km.y, 50, km.h})) ui.fokus("sot/kod");
    if (izla || ui.enter("sot/kod")) {
        wstring k = tozala(ui.qiymat("sot/kod"));
        if (!k.empty()) kod_bilan(k, false);
    }
    y += 50 + 8;
    // tanlangan tovar
    R tt = {p.x, y, p.w, 64};
    gfx.quti(tt, tovar_bor ? M->kok_fon : M->panel2, 9, tovar_bor ? M->kok : M->chegara2);
    if (tovar_bor) {
        gfx.matn({tt.x + 13, tt.y + 7, tt.w - 26, 28}, tanlangan.nom, 19, 650, M->matn);
        wstring q = L"Omborda: " + son(tanlangan.qoldiq) + L" " + tanlangan.birlik +
                    (tanlangan.dollarmi() ? L" · dollarda" : L"");
        gfx.matn({tt.x + 13, tt.y + 37, tt.w - 90, 18}, q, 12, 400, M->matn2);
        gfx.matn({tt.o() - 90, tt.y + 37, 77, 18}, tanlangan.kod, 12, 700, M->kok, ONG);
    } else {
        gfx.matn({tt.x + 13, tt.y + 7, tt.w - 26, 28}, L"Tovar tanlanmagan", 15, 400, M->matn3);
        gfx.matn({tt.x + 13, tt.y + 37, tt.w - 26, 18}, L"—", 12, 400, M->matn2);
    }
    y += 64 + 8;
    // MIQDOR
    R mq = {p.x, y, p.w, 56};
    tolov_maydon("sot/miqdor", mq, L"MIQDOR", tovar_bor ? tanlangan.birlik : L"—", 25, {}, {},
                 !kfok && !ui.fokusda("sot/qidir"));
    y += 56 + 8;
    // pastki: C + Qo'shish
    R qq = {p.x, p.p() - 50, p.w, 50};
    if (sensor) raqamlar("sot/rq", {p.x, y, p.w, qq.y - 8 - y}, ui.fokusda("sot/kod") ? "sot/kod" : "sot/miqdor");
    if (ui.tugma("sot/c", {qq.x, qq.y, 50, 50}, L"C", TINCH)) tozala_tanlov();
    R qt = {qq.x + 58, qq.y, qq.w - 58, 50};
    if (ui.tugma("sot/qosh", qt, L"Qo'shish", YASHIL, "i-plus", false, tovar_bor)) qosh();
    if (!sensor) {
        float tw = gfx.matn_eni(L"Qo'shish", 15, 600);
        yorliq_belgi(qt.x + (qt.w + 27 + tw) / 2 + 8, qt.y + 25, L"Enter",
                     tovar_bor ? M->tasdiq_matn : M->matn3);
    }
    if (ui.enter("sot/miqdor")) qosh();
}

// Qidiruv natijalari — chek ustida ochiladi
void qidiruv_royxati(R chek, R ong) {
    if (!dd_ochiq) return;
    auto b = bolakla(ui.qiymat("sot/qidir"));
    std::vector<const Tovar*> nat;
    for (auto& t : hamma) {
        if (mos_keladi(t.izlash, b)) nat.push_back(&t);
        if (nat.size() >= 30) break;
    }
    if (dd_nishon >= (int)nat.size()) dd_nishon = (int)nat.size() - 1;
    if (dd_nishon < 0) dd_nishon = 0;
    if (ui.fokusda("sot/qidir")) {
        if (ui.tugmacha(VK_DOWN)) dd_nishon = std::min(dd_nishon + 1, (int)nat.size() - 1);
        if (ui.tugmacha(VK_UP)) dd_nishon = std::max(dd_nishon - 1, 0);
        if (ui.tugmacha(VK_ESCAPE)) { dd_ochiq = false; return; }
    }
    float en = std::min(560.f, chek.w), qb = 44;
    float boy = nat.empty() ? 50 : std::min(nat.size() * qb, ui.H * .44f);
    R d = {chek.o() - en + 0, ong.y + 58, en, boy + 2};
    gfx.quti({d.x, d.y + 10, d.w, d.h}, {0, 0, 0, .35f}, 12);
    gfx.quti(d, M->panel2, 9, M->chegara2);
    if (nat.empty()) {
        gfx.matn(d, L"Hech narsa topilmadi", 15, 400, M->matn2, MARKAZ);
        return;
    }
    ui.scroll("sot/dd", d.ichki(1), nat.size() * qb);
    for (size_t i = 0; i < nat.size(); ++i) {
        const Tovar& t = *nat[i];
        R q = {d.x + 1, d.y + 1 + i * qb, d.w - 2, qb};
        bool yoq = t.qoldiq <= 0;
        bool hov = !yoq && ui.ustida(q);
        if (hov || (int)i == dd_nishon) gfx.quti(q, M->panel3);
        if (i + 1 < nat.size()) gfx.chiziq(q.x, q.p() - .5f, q.o(), q.p() - .5f, M->chegara);
        float a = yoq ? .45f : 1;
        gfx.matn({q.x + 14, q.y, 50, q.h}, t.kod, 13, 700, M->matn3.shaffof(a));
        float qw = gfx.matn_eni(son(t.qoldiq) + L" " + t.birlik, 12, 400) + 4;
        gfx.matn({q.x + 76, q.y, q.w - 90 - 76 - qw, q.h}, t.nom, 15, 600, M->matn.shaffof(a));
        gfx.matn({q.o() - 14 - qw, q.y, qw, q.h}, son(t.qoldiq) + L" " + t.birlik, 12, 400, M->matn2.shaffof(a), ONG);
        bool tan = !yoq && ui.bosildi("sot/dd/" + std::to_string(t.id), q);
        if (!tan && (int)i == dd_nishon && ui.enter("sot/qidir")) {
            if (yoq) ui.xabar(t.nom + L" omborda tugagan.", X_XATO);
            else tan = true;
        }
        if (tan) {
            tanla(t, -1);
            break;
        }
    }
    ui.scroll_tugat();
}

// ---------- chek (chap ustun) ----------

void chek_chiz(R r) {
    gfx.quti(r, M->panel, 12, M->chegara);
    R bosh = {r.x + 1, r.y + 1, r.w - 2, 62};
    gfx.quti(bosh, M->panel2, 11);
    gfx.quti({bosh.x, bosh.y + 20, bosh.w, bosh.h - 20}, M->panel2);
    gfx.chiziq(r.x, r.y + 63.5f, r.o(), r.y + 63.5f, M->chegara);
    if (qarzmi) {
        gfx.matn({r.x + 16, r.y + 10, r.w - 260, 27}, qd.toliq(), 19, 650, M->matn);
        wstring k = qd.hudud + (qd.telefon.empty() ? L"" : L" · " + qd.telefon) + L" · hujjat #" +
                    std::to_wstring(sotuv_id);
        gfx.matn({r.x + 16, r.y + 36, r.w - 260, 17}, k, 12, 400, M->matn2);
        gfx.matn({r.o() - 216, r.y + 8, 200, 17}, L"Oldingi qarzi", 12, 400, M->matn2, ONG);
        float sw = gfx.matn_eni(L"so'm", 12, 400);
        gfx.matn({r.o() - 16 - sw, r.y + 24, sw + 1, 26}, L"so'm", 12, 400, M->matn2, ONG, 2);
        gfx.matn({r.o() - 216, r.y + 24, 200 - sw - 4, 27}, pul(oldingi), 19, 650, M->matn, ONG);
        if (oldingi_d)
            gfx.matn({r.o() - 216, r.y + 46, 200, 16}, son(oldingi_d) + L" $", 12, 700, M->tasdiq, ONG);
    } else {
    gfx.matn({r.x + 16, r.y + 10, 300, 27}, L"Sotuv", 19, 650, M->matn);
    wstring k = L"chek #" + std::to_wstring(sotuv_id) + L" · " + mahalliy_vaqt(sana).substr(0, 5) + L" " +
                (mahalliy_vaqt(sana).size() > 11 ? mahalliy_vaqt(sana).substr(11) : L"");
    if (!qatorlar.empty()) k += L" · " + std::to_wstring(qatorlar.size()) + L" ta tovar";
    gfx.matn({r.x + 16, r.y + 36, 400, 17}, k, 12, 400, M->matn2);
    float bw = gfx.matn_eni(L"Bekor qilish", 13, 600) + 22 + 16 + 6;
    float sw = gfx.matn_eni(L"Sotuvlar", 13, 600) + 22 + 16 + 6;
    R bk = {r.o() - 16 - bw, r.y + 15, bw, 34};
    if (ui.tugma("sot/bekor", bk, L"Bekor qilish", XATO_U, "i-yopish", true)) {
        if (MessageBoxW(ui.h, L"Sotuv butunlay bekor qilinsinmi? Qo'shilgan tovarlar omborga qaytadi.",
                        L"Ulug'bek Xozmag", MB_OKCANCEL | MB_ICONQUESTION) == IDOK) {
            bool bosh_ = sotuv_bekor(sotuv_id);
            ui.xabar(bosh_ ? L"Bo'sh chek bekor qilindi."
                           : L"Chek #" + std::to_wstring(sotuv_id) + L" bekor qilindi. Ombor o'zgarmadi.",
                     X_INFO);
            qarzdor_id = 0;
            ot(J_BOSH);
            return;
        }
    }
    if (ui.tugma("sot/sotuvlar", {bk.x - 8 - sw, bk.y, sw, 34}, L"Sotuvlar", TINCH, "i-chek", true))
        ot(J_SOTUVLAR);
    }

    R ro = {r.x + 1, r.y + 64, r.w - 2, r.h - 65};
    if (qatorlar.empty()) {
        if (qarzmi) bosh_holat(ro, "i-chek", L"Hali tovar qo'shilmadi", L"O'ng tarafdagi «Yangi tovar» tugmasini bosing");
        else bosh_holat(ro, "i-savdo", L"Hali tovar qo'shilmadi", L"Kodini yozing, skanerlang yoki «Yangi tovar» ni bosing");
        return;
    }
    // jadval sarlavhasi
    R th = {ro.x, ro.y, ro.w, 37};
    gfx.quti(th, M->panel2);
    gfx.chiziq(th.x, th.p() - .5f, th.o(), th.p() - .5f, M->chegara);
    gfx.matn({th.x + 13, th.y, 30, th.h}, L"#", 11, 600, M->matn3);
    gfx.matn({th.x + 56, th.y, 200, th.h}, L"TOVAR", 11, 600, M->matn3);
    gfx.matn({th.o() - 74 - 200, th.y, 200, th.h}, L"MIQDOR", 11, 600, M->matn3, ONG);
    R tana = {ro.x, th.p(), ro.w, ro.p() - th.p()};
    ui.scroll("sot/chek", tana, qatorlar.size() * 55.f);
    DWORD hozir = GetTickCount();
    for (size_t i = 0; i < qatorlar.size(); ++i) {
        auto& q = qatorlar[i];
        R t = {tana.x, tana.y + i * 55.f, tana.w, 55};
        if (q.id == yangi_qator && hozir - yangi_vaqt < 1800) {
            float a = hozir - yangi_vaqt < 630 ? 1 : 1 - (hozir - yangi_vaqt - 630) / 1170.f;
            gfx.quti(t, M->urgu_fon.shaffof(a));
            ui.qayta = true;
        }
        if (i + 1 < qatorlar.size()) gfx.chiziq(t.x, t.p() - .5f, t.o(), t.p() - .5f, M->chegara);
        gfx.matn({t.x + 13, t.y, 40, t.h}, std::to_wstring(i + 1), 15, 400, M->matn3);
        float kx = t.x + 56;
        float kw = std::max(gfx.matn_eni(L"0000", 12, 700) + 4, gfx.matn_eni(q.kod, 12, 700) + 4);
        if (!qarzmi) {
            gfx.matn({kx, t.y, kw, t.h}, q.kod, 12, 700, M->matn3);
            kx += kw + 8;
        }
        // miqdor ustuni: [*] 2,5 kg   [x]
        wstring mtn = son(q.miqdor);
        float bw_ = gfx.matn_eni(q.birlik, 15, 400), mw = gfx.matn_eni(mtn, 15, 400);
        float ox = t.o() - 13 - 34 - 13;  // o'chirish tugmasidan oldin
        gfx.matn({ox - 13 - bw_, t.y, bw_ + 2, t.h}, q.birlik, 15, 400, M->matn3);
        gfx.matn({ox - 13 - bw_ - 4 - mw, t.y, mw + 1, t.h}, mtn, 15, 400, M->matn);
        R yul = {ox - 13 - bw_ - 4 - mw - 12 - 42, t.y + 10.5f, 42, 34};
        float nomen = yul.x - 12 - kx - (q.dollarmi() ? 30 : 0);
        gfx.matn({kx, t.y, nomen, t.h}, q.nom, 15, 700, M->matn);
        if (q.dollarmi())
            ui.nishon(kx + std::min(nomen, gfx.matn_eni(q.nom, 15, 700)) + 8, t.y + 18, L"$", M->tasdiq_fon,
                      M->tasdiq_chek, M->tasdiq);
        if (ui.tugma("sot/yul/" + std::to_string(q.id), yul, L"*", TINCH, nullptr, true, true, 26)) {
            miqdor_qator = q.id;
            ui.qiymat_qoy("sot/kop", L"");
            ui.fokus("sot/kop");
        }
        if (ui.tugma("sot/och/" + std::to_string(q.id), {t.o() - 13 - 34 - 13 + 13, t.y + 10.5f, 34, 34}, L"",
                     XATO_U, "i-chiqar", true)) {
            hujjat_qator_ochir(tur(), q.id);
            qayta_yukla();
            ui.qayta = true;
            break;
        }
    }
    ui.scroll_tugat();
}

// ---------- jami paneli ----------

void qarz_jami_panel(R r) {
    AvtoHisob h = avto_hisob(qatorlar);
    for (auto id : {"sot/jami", "sot/jamid", "sot/kurs", "sot/old", "sot/oldd"}) pul_guruhla(id);
    float g = 10;
    // 1-qator: jami (2), dollar (1.3), kurs (1)
    float b = (r.w - 2 * g) / 4.3f;
    R j = {r.x, r.y, b * 2, 50}, d = {j.o() + g, r.y, b * 1.3f, 50}, k = {d.o() + g, r.y, r.o() - d.o() - g, 50};
    tolov_maydon("sot/jami", j, L"JAMI", L"so'm", 25);
    tolov_maydon("sot/jamid", d, L"JAMI", L"$", 21, M->tasdiq_chek, M->tasdiq);
    tolov_maydon("sot/kurs", k, L"KURS", L"so'm", 15);
    // 2-qator: oldindan (1), oldindan $ (1), yakunlash (1.6)
    float y = r.y + 60;
    float b2 = (r.w - 2 * g) / 3.6f;
    R o = {r.x, y, b2, 50}, od = {o.o() + g, y, b2, 50}, yk = {od.o() + g, y, r.o() - od.o() - g, 50};
    tolov_maydon("sot/old", o, L"OLDINDAN", L"so'm", 19, {}, M->matn3);
    tolov_maydon("sot/oldd", od, L"OLDINDAN", L"$", 19, M->tasdiq_chek, M->tasdiq);
    if (ui.tugma("sot/yakun", yk, L"Yakunlash va daftarga yozish", YASHIL, "i-belgi") || ui.enter("sot/jami") ||
        ui.enter("sot/jamid") || ui.enter("sot/kurs") || ui.enter("sot/old") || ui.enter("sot/oldd"))
        yakunla();
    float py = y + 60;
    if (h.narxsiz) {
        R w = {r.x, py, r.w, 40};
        gfx.quti(w, M->ogoh_fon, 9, M->ogoh_chek);
        gfx.matn(w.ichki(12, 0), std::to_wstring(h.narxsiz) +
                 L" ta mahsulotning sotish narxi yo'q. Jami summani qo'lda kiriting.", 15, 400, M->ogoh);
        py += 50;
    }
    if (ui.tugma("sot/karta", {r.x, py, r.w, 50}, L"Qarzdor kartasi", TINCH, "i-chap")) ot(J_KARTA, qd.id);
}

void jami_panel(R r) {
    if (qarzmi) {
        qarz_jami_panel(r);
        return;
    }
    AvtoHisob h = avto_hisob(qatorlar);
    pul_guruhla("sot/jami");
    pul_guruhla("sot/jamid");
    pul_guruhla("sot/kurs");
    float g = 10;
    float qw = std::max(150.f, qarzdor_id ? gfx.matn_eni(qarzdor_nomi, 11, 600) + 60 : 150.f);
    qw = std::min(qw, 230.f);
    float erk = r.w - qw - 4 * g;
    // flex: jami 2, dollar 1.3, kurs 1, yakun 1.6
    // flex-asos + min-width (CSS: 200 / 150 / 185 / 180)
    float birlik = erk / (2 + 1.3f + 1 + 1.6f);
    float wj = std::max(200.f, birlik * 2), wd = std::max(150.f, birlik * 1.3f), wk = std::max(185.f, birlik);
    float qoldi = erk - wj - wd - wk;
    if (qoldi < 180) {  // joy yetmasa jami maydonlari kichrayadi
        float kam = 180 - qoldi;
        wj -= kam * .5f; wd -= kam * .25f; wk -= kam * .25f;
    }
    float x = r.x;
    R j = {x, r.y, wj, 50}; x += j.w + g;
    R d = {x, r.y, wd, 50}; x += d.w + g;
    R k = {x, r.y, wk, 50}; x += k.w + g;
    R q = {x, r.y, qw, 50}; x += qw + g;
    R y = {x, r.y, r.o() - x, 50};
    tolov_maydon("sot/jami", j, L"JAMI", L"so'm", 25);
    tolov_maydon("sot/jamid", d, L"JAMI", L"$", 21, M->tasdiq_chek, M->tasdiq);
    tolov_maydon("sot/kurs", k, L"KURS", L"so'm", 15);
    // Qarzga
    bool tan = qarzdor_id != 0;
    bool hov = ui.ustida(q);
    Rang fon = tan ? M->ogoh_fon : hov ? M->tugma_hover : (M->kechki ? M->panel3 : M->panel);
    Rang ch = tan ? M->ogoh_chek : hov ? M->tugma_hover_chegara : M->chegara2;
    Rang rang = tan ? M->ogoh : M->matn;
    gfx.quti(q, fon, 9, ch);
    gfx.ikon("i-hamyon", {q.x + 12, q.y + 15.5f, 19, 19}, rang);
    if (tan) {
        double jm = maydon_son("sot/jami"), jdm = maydon_son("sot/jamid");
        gfx.matn({q.x + 39, q.y + 7, q.w - 47, 15}, L"Qarzga · " + qarzdor_nomi, 11, 600, rang.shaffof(.85f));
        wstring qm = qarz_matn(jm - tolandi, jdm - tolandi_d);
        gfx.matn({q.x + 39, q.y + 22, q.w - 47, 21}, qm.empty() ? L"Qarz qolmadi" : qm, 15, 600, rang);
    } else
        gfx.matn({q.x + 39, q.y, q.w - 47, q.h}, L"Qarzga", 15, 600, rang);
    if (ui.bosildi("sot/qarzga", q)) {
        double jm = maydon_son("sot/jami"), jdm = maydon_son("sot/jamid");
        if (jm <= 0 && jdm <= 0)
            ui.xabar(L"Avval «Jami» ga chek summasini yozing — qarzga qoladigani shundan hisoblanadi.", X_XATO);
        else {
            qarz_oyna = true;
            ui.qiymat_qoy("qo/tol", tolandi > 0 ? pul(tolandi) : L"0");
            ui.qiymat_qoy("qo/qarz", pul(jm - tolandi));
            ui.qiymat_qoy("qo/told", tolandi_d > 0 ? son(tolandi_d) : L"0");
            ui.qiymat_qoy("qo/qarzd", son(jdm - tolandi_d));
            ui.qiymat_qoy("qo/qidir", L"");
        }
    }
    if (ui.tugma("sot/yakun", y, L"Sotuvni yakunlash", YASHIL, "i-belgi") || ui.enter("sot/jami") ||
        ui.enter("sot/jamid") || ui.enter("sot/kurs"))
        yakunla();
    if (h.narxsiz) {
        R o = {r.x, r.y + 60, r.w, 40};
        gfx.quti(o, M->ogoh_fon, 9, M->ogoh_chek);
        gfx.matn(o.ichki(12, 0), std::to_wstring(h.narxsiz) +
                 L" ta mahsulotning sotish narxi yo'q. Jami summani qo'lda kiriting.", 15, 400, M->ogoh);
    }
}

// ---------- oynalar ----------

void tovar_oynasi() {
    bool yop = false;
    float en = std::min(1040.f, ui.W - 40);
    R t = oyna("sot/to", L"Skladdan tovar tanlang", 1040, ui.H * .88f, yop);
    if (yop) { tovar_oyna = false; return; }
    R b = t.ichki(14);
    gfx.matn({b.x, b.y, b.w, 20}, L"Eng ko'p sotiladiganlari. Boshqa tovarni o'ng paneldagi qidiruvdan toping.", 15, 400, M->matn3);
    b.y += 30;
    b.h -= 30;
    int ust = std::max(1, int((b.w + 10) / 210));
    float kw = (b.w - (ust - 1) * 10) / ust, kh = 98;
    int qator_soni = ((int)oyna_tl.size() + ust - 1) / ust;
    ui.scroll("sot/to/s", {b.x - 4, b.y - 4, b.w + 8, b.h + 4}, qator_soni * (kh + 10) + 4);
    (void)en;
    for (size_t i = 0; i < oyna_tl.size(); ++i) {
        auto& m = oyna_tl[i];
        R k = {b.x + (i % ust) * (kw + 10), b.y + (i / ust) * (kh + 10), kw, kh};
        bool yoq = m.qoldiq <= 0, hov = !yoq && ui.ustida(k);
        if (hov) k.y -= 2;
        float a = yoq ? .4f : 1;
        gfx.quti(k, hov ? M->panel3 : M->panel2, 9, hov ? M->urgu : M->chegara);
        float kodw = gfx.matn_eni(m.kod, 12, 700) + 2;
        gfx.matn({k.x + 12, k.y + 10, k.w - 32 - kodw, 42}, m.nom, 15, 650, M->matn.shaffof(a), CHAP, 0, true);
        gfx.matn({k.o() - 12 - kodw, k.y + 12, kodw, 17}, m.kod, 12, 700, M->matn3.shaffof(a), ONG);
        float by_ = k.p() - 12 - 19 - 4 - 17;
        gfx.matn({k.x + 12, by_, 60, 17}, m.birlik, 12, 500, M->matn3.shaffof(a));
        float bx_ = k.x + 12 + gfx.matn_eni(m.birlik, 12, 500) + 6;
        if (m.dollarmi()) bx_ += ui.nishon(bx_, by_ - 1, L"$", M->tasdiq_fon, M->tasdiq_chek, M->tasdiq) + 6;
        if (m.narx > 0)
            gfx.matn({bx_, by_, k.o() - 12 - bx_, 17}, pul(m.narx) + (m.dollarmi() ? L" $" : L" so'm"), 12, 700,
                     M->matn2.shaffof(a), ONG);
        if (m.qoldiq > 0)
            ui.nishon(k.x + 12, k.p() - 12 - 19, L"Omborda: " + son(m.qoldiq) + L" " + m.birlik, M->panel3,
                      M->chegara2, M->matn2);
        else
            ui.nishon(k.x + 12, k.p() - 12 - 19, L"Tugagan", M->ogoh_fon, M->ogoh_chek, M->ogoh);
        if (!yoq && ui.bosildi("sot/to/" + std::to_string(m.id), k)) {
            tanla(m, -1);
            break;
        }
    }
    ui.scroll_tugat();
}

std::vector<Qarzdor> qo_royxat;
long long qo_tanlangan = 0;
bool qo_yangi = false;
int qo_hudud = 0;

void qarz_oynasi() {
    bool yop = false;
    double jm = maydon_son("sot/jami"), jdm = maydon_son("sot/jamid");
    bool dollar = jdm > 0;
    float boy = 58 + 14 + (dollar ? 420 : 300) + (sensor ? 240 : 0) + 64;
    if (qo_yangi) boy = std::max(boy, 58.f + 14 + 50 + 10 + 230 + 10 + 50 + 10 + 200 + 64 + 14);
    R t = oyna("qo", L"Pul yetmadi — qolgani qarzga", 900, boy, yop);
    if (yop) { qarz_oyna = false; return; }
    R tana = {t.x + 14, t.y + 14, t.w - 28, t.h - 14 - 64 - 14};
    // --- chap: bo'linish ---
    R ch = {tana.x, tana.y, 260, tana.h};
    float y = ch.y;
    bool xato = false;
    auto bolinish = [&](const string& tol, const string& qarz, double jami_, const wchar_t* bir, bool dol) {
        pul_guruhla(tol);
        pul_guruhla(qarz);
        // autosplit: qaysi biri yozilsa ikkinchisi hisoblanadi
        if (ui.ozgardi(tol) && ui.fokusda(tol)) {
            double v = 0;
            if (songa(ui.qiymat(tol), v)) ui.qiymat_qoy(qarz, dol ? son(std::max(0.0, jami_ - v)) : pul(std::max(0.0, jami_ - v)));
        }
        if (ui.ozgardi(qarz) && ui.fokusda(qarz)) {
            double v = 0;
            if (songa(ui.qiymat(qarz), v)) ui.qiymat_qoy(tol, dol ? son(std::max(0.0, jami_ - v)) : pul(std::max(0.0, jami_ - v)));
        }
        Rang jr = dol ? M->tasdiq : M->matn;
        gfx.matn({ch.x + 2, y, 100, 27}, L"JAMI", 11, 600, M->matn3);
        float bw = gfx.matn_eni(bir, 12, 400) + 2;
        gfx.matn({ch.o() - bw - 2, y, bw, 27}, bir, 12, 400, dol ? M->tasdiq : M->matn2, ONG);
        gfx.matn({ch.x, y, ch.w - bw - 8, 27}, dol ? son(jami_) : pul(jami_), 19, 700, jr, ONG);
        y += 27 + 6;
        double t1 = 0, q1 = 0;
        songa(ui.qiymat(tol), t1);
        songa(ui.qiymat(qarz), q1);
        bool bu_xato = t1 > jami_ + 0.001 || q1 > jami_ + 0.001;
        xato |= bu_xato;
        tolov_maydon(tol, {ch.x, y, ch.w, 50}, L"TO'LADI", bir, 21, bu_xato ? M->xato_chek : dol ? M->tasdiq_chek : Rang{},
                     dol ? M->tasdiq : Rang{});
        y += 50 + 6;
        tolov_maydon(qarz, {ch.x, y, ch.w, 50}, L"QARZGA", bir, 21, bu_xato ? M->xato_chek : M->ogoh_chek, M->ogoh);
        y += 50 + 8;
    };
    bolinish("qo/tol", "qo/qarz", jm, L"so'm", false);
    if (dollar) {
        for (float x = ch.x; x < ch.o(); x += 8) gfx.chiziq(x, y - 1, x + 4, y - 1, M->chegara2);
        y += 8;
        bolinish("qo/told", "qo/qarzd", jdm, L"$", true);
    }
    if (xato) {
        R e = {ch.x, y, ch.w, 52};
        gfx.quti(e, M->xato_fon, 9, M->xato_chek);
        gfx.matn(e.ichki(10, 8), L"To'langan yoki qarzga yozilgan pul chek summasidan ko'p bo'lmaydi.", 12, 600,
                 M->xato, CHAP, 0, true);
        y += 60;
    }
    if (sensor) {
        string n = ui.fokusda("qo/qarz") ? "qo/qarz" : ui.fokusda("qo/told") ? "qo/told" : ui.fokusda("qo/qarzd") ? "qo/qarzd" : "qo/tol";
        raqamlar("qo/rq", {ch.x, y, ch.w, 232}, n);
    }
    // --- o'ng: qarzdorlar ---
    R on = {ch.o() + 14, tana.y, tana.o() - ch.o() - 14, tana.h};
    float oy = on.y;
    MaydonSoz qs;
    qs.ishora = L"Familiya, ism yoki hudud...";
    ui.maydon("qo/qidir", {on.x, oy, on.w, 50}, qs);
    if (ui.ozgardi("qo/qidir") || qo_royxat.empty()) qo_royxat = qarzdorlar(ui.qiymat("qo/qidir"));
    oy += 60;
    int ust = std::max(1, int((on.w + 8) / 198));
    float kw = (on.w - (ust - 1) * 8) / ust, kh = 82;
    int qs_ = ((int)qo_royxat.size() + ust - 1) / ust;
    float setka_h = qo_yangi ? std::min(130.f, qs_ * (kh + 8)) : std::min(230.f, qs_ * (kh + 8));
    if (qo_royxat.empty()) {
        R e = {on.x, oy, on.w, 48};
        gfx.quti(e, {}, 9, M->chegara2);
        gfx.matn(e, L"Bunday qarzdor topilmadi — pastdan yangisini qo'shing.", 15, 400, M->matn3, MARKAZ);
        setka_h = 48;
    } else {
        ui.scroll("qo/setka", {on.x, oy, on.w, setka_h}, qs_ * (kh + 8) - 8);
        for (size_t i = 0; i < qo_royxat.size(); ++i) {
            auto& d = qo_royxat[i];
            R k = {on.x + (i % ust) * (kw + 8), oy + (i / ust) * (kh + 8), kw, kh};
            bool tn = qo_tanlangan == d.id, hov = ui.ustida(k);
            gfx.quti(k, tn ? M->kok_fon : hov ? M->panel3 : M->panel2, 9, tn ? M->kok : hov ? M->urgu : M->chegara);
            gfx.matn({k.x + 11, k.y + 9, k.w - 22, 21}, d.toliq(), 15, 650, M->matn);
            gfx.matn({k.x + 11, k.y + 32, k.w - 22, 17}, d.hudud, 12, 400, M->matn3);
            if (d.balans > 0 || d.balans_d > 0)
                ui.nishon(k.x + 11, k.y + 54, L"Qarzi: " + (d.balans > 0 ? pul(d.balans) + L" so'm " : L"") +
                                                  (d.balans_d > 0 ? son(d.balans_d) + L" $" : L""),
                          M->ogoh_fon, M->ogoh_chek, M->ogoh);
            else
                ui.nishon(k.x + 11, k.y + 54, L"Qarzi yo'q", M->tasdiq_fon, M->tasdiq_chek, M->tasdiq);
            if (ui.bosildi("qo/k/" + std::to_string(d.id), k)) qo_tanlangan = d.id;
        }
        ui.scroll_tugat();
    }
    oy += setka_h + 10;
    if (!qo_yangi) {
        if (ui.tugma("qo/yangi", {on.x, oy, on.w, 50}, L"Yangi qarzdor", ODDIY, "i-odam-plus")) {
            qo_yangi = true;
            qo_hudud = 0;
        }
    } else {
        R f = {on.x, oy, on.w, 12 + 2 * (17 + 5 + 50) + 10 + 10 + 50 + 12};
        gfx.quti(f, M->panel2, 9, M->chegara2);
        float fx = f.x + 12, fw = (f.w - 24 - 10) / 2, fy = f.y + 12;
        const wchar_t* nom[4] = {L"Ism", L"Familiya", L"Telefon", L"Hudud"};
        auto hl = hududlar();
        std::vector<wstring> hn = {L"---------"};
        for (auto& hh : hl) hn.push_back(hh.nom);
        for (int i = 0; i < 4; ++i) {
            R m = {fx + (i % 2) * (fw + 10), fy + (i / 2) * (17 + 5 + 50 + 10), fw, 50};
            gfx.matn({m.x, m.y, m.w, 17}, nom[i], 12, 600, M->matn2);
            m.y += 22;
            MaydonSoz s;
            s.fon = M->panel;
            if (i < 3) ui.maydon("qo/ym/" + std::to_string(i), m, s);
            else ui.tanlov("qo/ym/hudud", m, hn, qo_hudud);
        }
        R sb = {fx, f.p() - 12 - 50, f.w - 24, 50};
        if (ui.tugma("qo/ym/saqla", sb, L"Qo'shish va tanlash", YASHIL, "i-belgi")) {
            Qarzdor q;
            q.ism = ui.qiymat("qo/ym/0");
            q.familiya = ui.qiymat("qo/ym/1");
            q.telefon = ui.qiymat("qo/ym/2");
            q.hudud_id = qo_hudud > 0 ? hl[qo_hudud - 1].id : 0;
            wstring x;
            long long id = qarzdor_saqla(q, x);
            if (!id) ui.xabar(x, X_XATO);
            else {
                ui.xabar(tozala(q.familiya) + L" " + tozala(q.ism) + L" qo'shildi.");
                qo_tanlangan = id;
                qo_yangi = false;
                ui.maydonlarni_tozala("qo/ym/");
                ui.qiymat_qoy("qo/qidir", L"");
                qo_royxat = qarzdorlar(L"");
            }
        }
    }
    // --- pastki qator ---
    R p = {t.x + 1, t.p() - 64, t.w - 2, 63};
    gfx.quti(p, M->panel2, 11);
    gfx.quti({p.x, p.y, p.w, 30}, M->panel2);
    gfx.chiziq(t.x, p.y - .5f, t.o(), p.y - .5f, M->chegara);
    double tl = maydon_son("qo/tol"), tld = maydon_son("qo/told");
    double qz = jm - tl, qzd = dollar ? jdm - tld : 0;
    Qarzdor tq;
    bool bor = qo_tanlangan && qarzdor_ol(qo_tanlangan, tq);
    wstring xul = !bor ? L"Qarzdor tanlanmagan"
                 : xato ? tq.toliq() + L" — summani tekshiring"
                 : (qz <= 0 && qzd <= 0) ? tq.toliq() + L" — hammasi to'langan, qarz qolmadi"
                 : tq.toliq() + L" — qarzga " + qarz_matn(qz, qzd);
    float x = p.o() - 14;
    float tw = gfx.matn_eni(L"Tasdiqlash", 15, 600) + 36 + 27;
    x -= tw;
    bool tasdiq = ui.tugma("qo/tasdiq", {x, p.y + 6.5f, tw, 50}, L"Tasdiqlash", YASHIL, "i-belgi", false,
                           bor && !xato && (qz > 0 || qzd > 0));
    float bw = gfx.matn_eni(L"Bekor qilish", 15, 600) + 36;
    x -= 10 + bw;
    if (ui.tugma("qo/bekor", {x, p.y + 6.5f, bw, 50}, L"Bekor qilish", TINCH)) qarz_oyna = false;
    if (qarzdor_id) {
        float ow = gfx.matn_eni(L"Olib tashlash", 15, 600) + 36 + 27;
        x -= 10 + ow;
        if (ui.tugma("qo/olib", {x, p.y + 6.5f, ow, 50}, L"Olib tashlash", XATO_U, "i-yopish")) {
            qarzdor_id = 0;
            tolandi = tolandi_d = 0;
            qarz_oyna = false;
            ui.xabar(L"Qarz olib tashlandi — chek to'liq naqd.");
        }
    }
    gfx.matn({p.x + 14, p.y, x - p.x - 24, p.h}, xul, 13, 400, M->matn2);
    if (tasdiq) {
        qarzdor_id = qo_tanlangan;
        qarzdor_nomi = tq.toliq();
        tolandi = tl;
        tolandi_d = dollar ? tld : 0;
        qarz_oyna = false;
        ui.xabar(qarzdor_nomi + L" ning daftariga " + qarz_matn(qz, qzd) +
                 L" yoziladi. «Sotuvni yakunlash» ni bosing.", X_INFO);
    }
}

void miqdor_oynasi() {
    Qator* q = nullptr;
    for (auto& x : qatorlar) if (x.id == miqdor_qator) q = &x;
    if (!q) { miqdor_qator = 0; return; }
    ui.oyna_fon();
    float en = std::min(560.f, ui.W * .92f), boy = 26 + 36 + 26 + 22 + 10 + 76 + 16 + 24 + 10 + 4 * 72 + 3 * 12 + 18 + 56 + 26;
    boy = std::min(boy, ui.H * .92f);
    R o = {(ui.W - en) / 2, (ui.H - boy) / 2, en, boy};
    gfx.quti({o.x, o.y + 10, o.w, o.h}, {0, 0, 0, .35f}, 14);
    gfx.quti(o, M->panel, 12, M->chegara);
    R b = o.ichki(26);
    float y = b.y;
    gfx.matn({b.x, y, b.w, 36}, L"Miqdorni ko'paytirish", 26, 700, M->matn);
    y += 36 + 10;
    gfx.matn({b.x, y, b.w, 21}, q->nom, 15, 400, M->matn2);
    y += 26;
    gfx.matn({b.x, y, b.w, 17}, L"Necha baravar ko'paytirilsin?", 12, 600, M->matn2);
    y += 22;
    MaydonSoz s;
    s.olcham = 42;
    s.ong = true;
    ui.maydon("sot/kop", {b.x, y + 10, b.w, 76}, s);
    y += 10 + 76 + 16;
    double k = 0;
    bool ok = songa(ui.qiymat("sot/kop"), k) && k > 0;
    if (ok)
        gfx.matn({b.x, y, b.w, 24}, son(q->miqdor) + L" × " + son(k) + L" = " + son(yaxlit(q->miqdor * k, 3)) + L" " + q->birlik,
                 18, 600, M->matn);
    y += 24 + 10;
    float bh = std::max(50.f, (b.p() - 56 - 18 - y - 3 * 12) / 4);
    R rq = {b.x, y, b.w, bh * 4 + 3 * 12};
    {
        static const wchar_t* T[12] = {L"7", L"8", L"9", L"4", L"5", L"6", L"1", L"2", L"3", L"0", L",", L"⌫"};
        float w = (rq.w - 24) / 3;
        for (int i = 0; i < 12; ++i) {
            R t = {rq.x + (i % 3) * (w + 12), rq.y + (i / 3) * (bh + 12), w, bh};
            if (ui.tugma("sot/kop/" + std::to_string(i), t, T[i], ODDIY, nullptr, false, true, 32)) {
                wstring v = ui.qiymat("sot/kop");
                if (i == 11) { if (!v.empty()) v.pop_back(); } else v += T[i];
                ui.qiymat_qoy("sot/kop", v);
                ui.fokus("sot/kop");
            }
        }
    }
    y = rq.p() + 18;
    float w3 = (b.w - 20) / 3;
    if (ui.tugma("sot/kop/c", {b.x, y, w3, 56}, L"C", TINCH)) ui.qiymat_qoy("sot/kop", L"");
    if (ui.tugma("sot/kop/bekor", {b.x + w3 + 10, y, w3, 56}, L"Bekor qilish", TINCH) || ui.tugmacha(VK_ESCAPE))
        miqdor_qator = 0;
    if (ui.tugma("sot/kop/ok", {b.x + 2 * (w3 + 10), y, w3, 56}, L"Ko'paytirish", YASHIL) || ui.enter("sot/kop")) {
        wstring x;
        if (!ok) ui.xabar(L"Noldan katta son yozing.", X_XATO);
        else if (!hujjat_qator_miqdor(tur(), q->id, yaxlit(q->miqdor * k, 3), x)) ui.xabar(x, X_XATO);
        else {
            miqdor_qator = 0;
            qayta_yukla();
        }
    }
}

}  // namespace

void ombor_eskirdi() { hamma_eskirgan = true; }

void qarz_yozish_ochildi(long long qarz_id) {
    sotuv_ochildi(-qarz_id);
}

void sotuv_ochildi(long long p) {
    qarzmi = p < 0;
    if (qarzmi) {
        sotuv_id = -p;
        qarzdor_ol(qarz_qarzdori(sotuv_id), qd);
        sana = qarz_sanasi(sotuv_id);
    } else {
        sotuv_id = p ? p : ochiq_sotuv(foydalanuvchi.id);
        sana = sotuv_sanasi(sotuv_id);
    }
    qarzdor_id = 0;
    tolandi = tolandi_d = 0;
    tovar_oyna = qarz_oyna = false;
    miqdor_qator = 0;
    qo_tanlangan = 0;
    qo_yangi = false;
    qo_royxat.clear();
    tozala_tanlov();
    ui.maydonlarni_tozala("sot/");
    ui.maydonlarni_tozala("qo/");
    hamma_eskirgan = true;
    qayta_yukla();
    if (qarzmi) {
        // Shu hujjatdan oldingi qarzi (hozir yozilayotgani hali hisobga kirmagan)
        oldingi = qd.balans;
        oldingi_d = qd.balans_d;
    }
    double k = oxirgi_kurs();
    ui.qiymat_qoy("sot/kurs", k > 0 ? pul(k) : L"");
    ui.scroll_tepaga("sot/chek");
    if (!sensor) ui.fokus("sot/kod");
}

void sotuv_chiz(R r) {
    // skaner (fokus qayerda bo'lmasin) va maydonsiz yozilgan raqamlar
    if (!ui.skaner.empty() && !qarz_oyna && !miqdor_qator) {
        kod_bilan(ui.skaner, true);
        ui.skaner.clear();
    }
    if (!ui.belgilar.empty() && !qarz_oyna && !miqdor_qator && !tovar_oyna) {
        bool raqam = true;
        for (wchar_t c : ui.belgilar) raqam &= c >= L'0' && c <= L'9';
        if (raqam) {
            ui.qiymat_qoy("sot/kod", ui.qiymat("sot/kod") + ui.belgilar);
            ui.fokus("sot/kod");
        }
    }
    if (ui.tugmacha(VK_F2)) { tovar_oyna = true; oyna_tl = oyna_tovarlari(); }

    const float ONG = 370, G = 14;
    R chap = {r.x, r.y, r.w - ONG - G, r.h};
    R ong = {chap.o() + G, r.y, ONG, r.h};
    AvtoHisob h = avto_hisob(qatorlar);
    float pastki = qarzmi ? 10 + 50 + 10 + 50 + 10 + 50 + (h.narxsiz ? 50 : 0) : 10 + 50 + (h.narxsiz ? 50 : 0);
    R chek = {chap.x, chap.y, chap.w, chap.h - pastki};
    chek_chiz(chek);
    jami_panel({chap.x, chek.p() + 10, chap.w, pastki - 10});
    ong_panel(ong);
    qidiruv_royxati(chek, ong);
    if (tovar_oyna) tovar_oynasi();
    if (qarz_oyna) qarz_oynasi();
    if (miqdor_qator) miqdor_oynasi();
}
