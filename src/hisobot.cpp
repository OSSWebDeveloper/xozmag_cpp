// HISOBOT (templates/sotuv/hisobot.html) va SOTUVLAR (templates/sotuv/royxat.html)
#include "chek_chop.h"
#include "db3.h"
#include "ilova.h"

namespace {

void sana_maydon(const string& id, R r) {
    MaydonSoz s;
    s.ishora = L"kk.oo.yyyy";
    bool fok = ui.fokusda(id);
    if (fok) gfx.quti({r.x - 3, r.y - 3, r.w + 6, r.h + 6}, M->urgu_fon, 12);
    gfx.quti(r, M->panel2, 9, fok ? M->urgu : M->chegara2);
    ui.tahrir(id, {r.x + 13, r.y, r.w - 60, r.h}, s);
    if (ui.bosildi(id + "/q", r)) ui.fokus(id);
    R ik = {r.o() - 32, r.y + (r.h - 18) / 2, 18, 18};
    gfx.quti(ik, {}, 3, M->matn3);
    gfx.chiziq(ik.x, ik.y + 5, ik.o(), ik.y + 5, M->matn3, 1.5f);
}

SYSTEMTIME bugun() {
    SYSTEMTIME t;
    GetLocalTime(&t);
    return t;
}

}  // namespace

// ---- chek oynasi (sotuv:chek) ----
static long long chek_oyna = 0;
void chek_oynasini_och(long long id) { chek_oyna = id; }

void chek_oynasi_chiz() {
    if (!chek_oyna) return;
    Chek c;
    if (!chek_ol(chek_oyna, c)) {
        chek_oyna = 0;
        return;
    }
    wstring m = chek_matni(c);
    int satr = 1;
    for (wchar_t ch : m) satr += ch == L'\n';
    bool yop = false;
    Shtrix sh = shtrix_yasa(savdo_chek_kodi(c.id));
    float shb = sh.eni ? 12 + 56 + 22 + 20 : 0;
    R t = oyna("chk", L"Chek #" + std::to_wstring(c.id), 560, 58 + 28 + satr * 19.f + shb + 50 + 28 + 14, yop);
    if (yop) { chek_oyna = 0; return; }
    R q = t.ichki(14);
    gfx.quti({q.x, q.y, q.w, q.h - 64}, hex("#ffffff"), 6);
    float y = q.y + 14;
    wstring satr_m;
    for (wchar_t ch : m + L"\n") {
        if (ch != L'\n') { satr_m += ch; continue; }
        gfx.matn({q.x + 14, y, q.w - 28, 19}, satr_m, 14, 400, hex("#111111"));
        y += 19;
        satr_m.clear();
    }
    if (sh.eni) {
        y += 12;
        float mod = std::min(2.f, (q.w - 60) / sh.eni), chap = q.x + (q.w - sh.eni * mod) / 2;
        for (auto [bx, bw] : sh.chiziqlar) gfx.quti({chap + bx * mod, y, bw * mod, 56}, hex("#000000"));
        y += 56;
        gfx.matn({q.x, y, q.w, 22}, savdo_chek_kodi(c.id), 14, 600, hex("#111111"), MARKAZ);
        y += 22;
        gfx.matn({q.x, y, q.w, 20}, L"Qaytarish uchun chekni saqlang.", 12, 400, hex("#444444"), MARKAZ);
    }
    if (!c.bekor && ui.tugma("chk/qayt", {q.x, q.p() - 50, 220, 50}, L"Mahsulot qaytarish", ODDIY, "i-kirim")) {
        chek_oyna = 0;
        ot(J_CHEK_QAYTARISH, c.id);
        return;
    }
    if (ui.tugma("chk/chop", {q.o() - 220, q.p() - 50, 220, 50}, L"Chekni chop etish", ASOSIY, "i-printer")) {
        wstring x;
        if (chek_chop(c, x)) ui.xabar(L"Chek printerga yuborildi.");
        else ui.xabar(L"Chop etilmadi: " + x, X_XATO);
    }
}

namespace {

// ======================= Hisobot =======================

Hisobot hb;
std::vector<std::pair<long long, wstring>> hk;
int hk_tan = 0;

void hisobot_yukla() {
    SYSTEMTIME a = bugun(), b = bugun();
    sana_oqi(ui.qiymat("hs/dan"), a);
    sana_oqi(ui.qiymat("hs/gacha"), b);
    hb = hisobot_ol(a, b, hk_tan > 0 ? hk[hk_tan - 1].first : 0);
}

}  // namespace

void hisobot_ochildi(long long) {
    ui.maydonlarni_tozala("hs/");
    ui.qiymat_qoy("hs/dan", sana_yoz(bugun()));
    ui.qiymat_qoy("hs/gacha", sana_yoz(bugun()));
    hk = kassirlar();
    hk_tan = 0;
    chek_oyna = 0;
    hisobot_yukla();
    ui.scroll_tepaga("hs/sahifa");
}

void hisobot_chiz(R r) {
    float qb = 44;
    float t1 = 38 + std::max(1, (int)hb.kassalar.size()) * qb;
    float t2 = 38 + std::max(1, (int)hb.cheklar.size()) * qb;
    float kontent = 46 + 30 + (14 + 2 * (22 + 50) + 10 + 14) + 14 + (14 + 30 + 3 * 24 + 30 + t1 + 14 + 30 + t2 + 14);
    ui.scroll("hs/sahifa", r, kontent);
    R s = {r.x, r.y, r.w - 12, r.h};
    sarlavha(s, L"Hisobot — kassa savdolari");
    gfx.matn({s.x, s.y - 6, s.w, 20},
             L"Faqat yakunlangan, bekor qilinmagan kassa savdolari. Qaytarilgan summa ayriladi; qarz undiruvi qo'shilmaydi.",
             13, 400, M->matn);
    s.y += 24;
    // filtr formasi
    R f = {s.x, s.y, s.w, 14 + 2 * (22 + 50) + 10 + 14};
    quti(f);
    float x = f.x + 14, w = (f.w - 28 - 14) / 2, y = f.y + 14;
    gfx.matn({x, y, w, 17}, L"Boshlanish sanasi", 12, 600, M->matn2);
    gfx.matn({x + w + 14, y, w, 17}, L"Tugash sanasi", 12, 600, M->matn2);
    y += 22;
    sana_maydon("hs/dan", {x, y, w, 50});
    sana_maydon("hs/gacha", {x + w + 14, y, w, 50});
    y += 50 + 10;
    gfx.matn({x, y, w, 17}, L"Kassir", 12, 600, M->matn2);
    y += 22;
    std::vector<wstring> kv = {L"Barcha kassalar"};
    for (auto& k : hk) kv.push_back(k.second);
    ui.tanlov("hs/kassir", {x, y, w, 50}, kv, hk_tan);
    if (ui.tugma("hs/kor", {x + w + 14, y, gfx.matn_eni(L"Ko'rsatish", 15, 600) + 36, 50}, L"Ko'rsatish", YASHIL) ||
        ui.enter("hs/dan") || ui.enter("hs/gacha"))
        hisobot_yukla();
    s.y = f.p() + 14;
    // natija
    R n = {s.x, s.y, s.w, kontent - (s.y - r.y)};
    quti(n);
    x = n.x + 14;
    w = n.w - 28;
    y = n.y + 14;
    gfx.matn({x, y, w, 26}, L"Jami: " + std::to_wstring(hb.soni) + L" ta chek", 18, 700, M->matn);
    y += 30;
    auto qator = [&](const wstring& b, const wstring& v) {
        float bw = gfx.matn_eni(b, 15, 700);
        gfx.matn({x, y, bw + 2, 22}, b, 15, 700, M->matn);
        gfx.matn({x + bw + 4, y, w, 22}, v, 15, 400, M->matn);
        y += 24;
    };
    qator(L"Naqd so'm:", pul(hb.som) + L" so'm");
    qator(L"Naqd dollar:", son(hb.dollar) + L" $");
    gfx.matn({x, y, w, 26}, L"Kassalar kesimida", 18, 700, M->matn);
    y += 30;
    std::vector<Ustun> u1 = {{L"Kassir", -1}, {L"Cheklar", -1}, {L"So'm", -1}, {L"Dollar", -1}};
    jadval_sarlavha({x, y, w, 38}, u1);
    y += 38;
    if (hb.kassalar.empty()) {
        gfx.matn({x + 13, y, w, qb}, L"Sotuv topilmadi.", 15, 400, M->matn);
        y += qb;
    }
    for (auto& k : hb.kassalar) {
        auto c = ustunlar({x, y, w, qb}, u1);
        gfx.matn(c[0].ichki(13, 0), k.kassir, 15, 400, M->matn);
        gfx.matn(c[1].ichki(13, 0), std::to_wstring(k.soni), 15, 400, M->matn);
        gfx.matn(c[2].ichki(13, 0), pul(k.som), 15, 400, M->matn);
        gfx.matn(c[3].ichki(13, 0), son(k.dollar), 15, 400, M->matn);
        gfx.chiziq(x, y + qb - .5f, x + w, y + qb - .5f, M->chegara);
        y += qb;
    }
    y += 14;
    gfx.matn({x, y, w, 26}, L"Cheklar ro'yxati (oxirgi 500 ta)", 18, 700, M->matn);
    y += 30;
    std::vector<Ustun> u2 = {{L"Chek", -1}, {L"Sana", -1}, {L"Kassir", -1}, {L"Naqd so'm", -1}, {L"Naqd dollar", -1}};
    jadval_sarlavha({x, y, w, 38}, u2);
    y += 38;
    if (hb.cheklar.empty()) gfx.matn({x + 13, y, w, qb}, L"Tanlangan davrda sotuv yo'q.", 15, 400, M->matn);
    for (auto& c : hb.cheklar) {
        auto cc = ustunlar({x, y, w, qb}, u2);
        R l = cc[0].ichki(13, 0);
        wstring id = L"#" + std::to_wstring(c.id);
        R lr = {l.x, l.y + 10, gfx.matn_eni(id, 15, 400) + 2, l.h - 20};
        bool hov = ui.ustida(lr);
        gfx.matn(l, id, 15, 400, hov ? M->urgu_quyuq : M->urgu_matn);
        if (hov) gfx.chiziq(lr.x, lr.p() - 3, lr.o(), lr.p() - 3, M->urgu_matn);
        if (ui.bosildi("hs/c/" + std::to_string(c.id), lr)) chek_oyna = c.id;
        gfx.matn(cc[1].ichki(13, 0), mahalliy_vaqt(c.sana), 15, 400, M->matn);
        gfx.matn(cc[2].ichki(13, 0), c.kassir, 15, 400, M->matn);
        gfx.matn(cc[3].ichki(13, 0), pul(c.som), 15, 400, M->matn);
        gfx.matn(cc[4].ichki(13, 0), son(c.dollar), 15, 400, M->matn);
        gfx.chiziq(x, y + qb - .5f, x + w, y + qb - .5f, M->chegara);
        y += qb;
    }
    ui.scroll_tugat();
    chek_oynasi_chiz();
}

// ======================= Sotuvlar (kunlik) =======================

namespace {
KunSotuvlari ks;
SYSTEMTIME ks_kun;

void sotuvlar_yukla() {
    SYSTEMTIME k = bugun();
    if (sana_oqi(ui.qiymat("sv/kun"), k)) ks_kun = k;
    else ks_kun = bugun();
    ks = kun_sotuvlari(ks_kun);
}

float chek_boyi(const SotuvKun& s) {
    bool qayt = s.qayt > 0 || s.qayt_d > 0 || s.qarz_qayt > 0 || s.qarz_qayt_d > 0;
    return 50 + s.q.size() * 50.f + (s.qarz_bor ? 16 : 0) + (qayt ? 18 : 0);
}
}  // namespace

void sotuvlar_ochildi(long long) {
    ui.maydonlarni_tozala("sv/");
    ui.qiymat_qoy("sv/kun", sana_yoz(bugun()));
    chek_oyna = 0;
    sotuvlar_yukla();
    ui.scroll_tepaga("sv/cheklar");
}

void sotuvlar_chiz(R r) {
    if (ui.tugma("sv/qaytar", {r.x, r.y, gfx.matn_eni(L"Chekni skanerlab qaytarish", 15, 600) + 36, 50},
                 L"Chekni skanerlab qaytarish")) {
        ot(J_CHEK_QAYTARISH, 0);
        return;
    }
    r.y += 64;
    r.h -= 64;
    SYSTEMTIME b = bugun();
    bool bugunmi = b.wYear == ks_kun.wYear && b.wMonth == ks_kun.wMonth && b.wDay == ks_kun.wDay;
    sarlavha(r, L"Sotuvlar", sana_yoz(ks_kun) + (bugunmi ? L" (bugun)" : L""));
    R q = {r.x, r.y, r.w, 14 + 92 + 14 + 50 + 14};
    quti(q);
    float x = q.x + 14, y = q.y + 14, w = q.w - 28;
    bool qarzga = ks.qarzga > 0 || ks.qarzga_d > 0;
    int n = qarzga ? 3 : 2;
    float formw = (w - 14 * n) * 2 / (n + 2);
    float kw = (w - 14 * n - formw) / n;
    korsatkich({x, y, kw, 92}, L"SOTUVLAR SONI", std::to_wstring(ks.soni));
    korsatkich({x + kw + 14, y, kw, 92}, L"KUNLIK TUSHUM", pul(ks.jami) + L" so'm", ks.jami_d ? son(ks.jami_d) + L" $" : L"", 1);
    if (qarzga)
        korsatkich({x + 2 * (kw + 14), y, kw, 92}, L"QARZGA YOZILDI", ks.qarzga ? pul(ks.qarzga) + L" so'm" : L"0",
                   ks.qarzga_d ? son(ks.qarzga_d) + L" $" : L"", 2);
    float fx = x + n * (kw + 14);
    float kb = gfx.matn_eni(L"Ko'rsatish", 15, 600) + 36;
    gfx.matn({fx, y + 92 - 50 - 22, formw, 17}, L"Kunni tanlang", 12, 600, M->matn2);
    sana_maydon("sv/kun", {fx, y + 92 - 50, formw - kb - 14, 50});
    if (ui.tugma("sv/kor", {fx + formw - kb, y + 92 - 50, kb, 50}, L"Ko'rsatish") || ui.enter("sv/kun")) sotuvlar_yukla();
    y += 92 + 14;
    float bw = gfx.matn_eni(L"Bosh sahifa", 15, 600) + 36 + 27;
    if (ui.tugma("sv/yangi", {x, y, w - bw - 14, 50}, L"Yangi sotuv", YASHIL, "i-plus")) ot(J_SOTUV);
    if (ui.tugma("sv/bosh", {x + w - bw, y, bw, 50}, L"Bosh sahifa", TINCH, "i-uy")) ot(J_BOSH);
    r.y = q.p() + 14;
    r.h = r.p() - r.y;
    R ch = {r.x, r.y, r.w, ui.H - 14 - r.y};
    float jami = 0;
    for (auto& s : ks.s) jami += chek_boyi(s) + 14;
    if (ks.s.empty()) {
        quti({ch.x, ch.y, ch.w, 200});
        bosh_holat({ch.x, ch.y, ch.w, 200}, "i-chek", L"Bu kunda sotuv yo'q", L"");
    } else {
        ui.scroll("sv/cheklar", ch, jami - 14);
        float cy = ch.y, cw = ch.w - 12;
        for (auto& s : ks.s) {
            float boy = chek_boyi(s);
            R t = {ch.x, cy, cw, boy};
            Rang a = Rang{1, 1, 1, 1};
            float op = s.bekor ? .55f : 1;
            (void)a;
            gfx.quti(t, M->panel.shaffof(op), 12, M->chegara.shaffof(op));
            R hb_ = {t.x + 1, t.y + 1, t.w - 2, 49};
            gfx.quti(hb_, M->panel2.shaffof(op), 11);
            gfx.quti({hb_.x, hb_.y + 20, hb_.w, hb_.h - 20}, M->panel2.shaffof(op));
            gfx.chiziq(t.x, t.y + 50.5f, t.o(), t.y + 50.5f, M->chegara);
            wstring vaqt = mahalliy_vaqt(s.sana);
            wstring sar = (vaqt.size() > 11 ? vaqt.substr(11) : vaqt) + L" · chek #" + std::to_wstring(s.id);
            float nx = t.x + 13;
            gfx.matn({nx, t.y, 300, 50}, sar, 11, 600, M->matn3.shaffof(op));
            nx += gfx.matn_eni(sar, 11, 600) + 10;
            if (!s.bekor) {
                float c1 = gfx.matn_eni(L"Chek", 13, 600) + 22;
                if (ui.tugma("sv/ch/" + std::to_string(s.id), {nx, t.y + 8, c1, 34}, L"Chek", TINCH, nullptr, true))
                    chek_oyna = s.id;
                nx += c1 + 6;
                float c2 = gfx.matn_eni(L"Chekni qayta chiqarish", 13, 600) + 22;
                if (ui.tugma("sv/cp/" + std::to_string(s.id), {nx, t.y + 8, c2, 34}, L"Chekni qayta chiqarish", TINCH,
                             nullptr, true)) {
                    Chek c;
                    wstring x2;
                    if (chek_ol(s.id, c) && chek_chop(c, x2)) ui.xabar(L"Chek printerga yuborildi.");
                    else ui.xabar(L"Chop etilmadi: " + x2, X_XATO);
                }
                nx += c2 + 8;
            } else
                nx += ui.nishon(nx, t.y + 15, L"Bekor qilingan", M->xato_fon, M->xato_chek, M->xato) + 6;
            if (s.qaytarilgan) nx += ui.nishon(nx, t.y + 15, L"Qaytarish bo'lgan", M->ogoh_fon, M->ogoh_chek, M->ogoh) + 6;
            if (s.qarz_bor) {
                float nw = ui.nishon(nx, t.y + 15, L"Qarzga: " + s.qarzdor, M->ogoh_fon, M->ogoh_chek, M->ogoh);
                if (ui.bosildi("sv/qz/" + std::to_string(s.id), {nx, t.y + 15, nw, 19})) {
                    ot(J_KARTA, s.qarzdor_id);
                    break;
                }
            }
            // o'ng: summa
            R o = {t.o() - 13 - 320, t.y, 320, s.qarz_bor ? 30 : 50};
            if (s.bekor) gfx.matn(o, L"—", 11, 600, M->matn3, ONG);
            else {
                double um = s.sof + s.qarz_sof, umd = s.sof_d + s.qarz_sof_d;
                wstring v = (um || !umd) ? pul(um) + L" so'm" : L"";
                if (umd) {
                    float dw = gfx.matn_eni(son(umd) + L" $", 12, 700);
                    gfx.matn({o.o() - dw, o.y, dw, o.h}, son(umd) + L" $", 12, 700, M->tasdiq, ONG);
                    gfx.matn({o.x, o.y, o.w - dw - 6, o.h}, v, 11, 600, M->matn3, ONG);
                } else
                    gfx.matn(o, v, 11, 600, M->matn3, ONG);
                if (s.qarz_bor)
                    gfx.matn({o.x - 100, t.y + 28, o.w + 100, 18},
                             L"naqd: " + pul(s.sof) + L" so'm" + (s.sof_d ? L" " + son(s.sof_d) + L" $" : L"") +
                                 L" · qarzga: " + (s.qarz_sof ? pul(s.qarz_sof) + L" so'm" : L"") +
                                 (s.qarz_sof_d ? L" " + son(s.qarz_sof_d) + L" $" : L""),
                             12, 400, M->matn2, ONG);
            }
            float qy = t.y + 50 + (s.qarz_bor ? 16 : 0);
            // «qaytarildi: ...» (naqddan va qarzdan)
            if (s.qayt > 0 || s.qayt_d > 0 || s.qarz_qayt > 0 || s.qarz_qayt_d > 0) {
                wstring m = L"qaytarildi:";
                if (s.qayt > 0) m += L" " + pul(s.qayt) + L" so'm";
                if (s.qayt_d > 0) m += L" " + son(s.qayt_d) + L" $";
                if (s.qarz_qayt > 0 || s.qarz_qayt_d > 0) {
                    if (s.qayt > 0 || s.qayt_d > 0) m += L" ·";
                    m += L" qarzdan";
                    if (s.qarz_qayt > 0) m += L" " + pul(s.qarz_qayt) + L" so'm";
                    if (s.qarz_qayt_d > 0) m += L" " + son(s.qarz_qayt_d) + L" $";
                }
                gfx.matn({t.x + 13, qy - 2, t.w - 26, 18}, m, 12, 500, M->ogoh, ONG);
                qy += 18;
            }
            bool sotuvdan_chiq = false;
            for (size_t i = 0; i < s.q.size(); ++i) {
                auto& z = s.q[i];
                R row = {t.x, qy + i * 50.f, t.w, 50};
                if (i + 1 < s.q.size()) gfx.chiziq(row.x, row.p() - .5f, row.o(), row.p() - .5f, M->chegara);
                const float TUG = 150;  // qaytarish tugmasi ustuni
                float kx = row.x + 13;
                if (!z.kod.empty()) {
                    gfx.matn({kx, row.y, 60, row.h}, z.kod, 13, 700, M->matn3.shaffof(op));
                    kx += 56;
                }
                float nom_w = row.w - (kx - row.x) - 220 - TUG;
                float nw = std::min(gfx.matn_eni(z.nom, 15, 400), nom_w);
                gfx.matn({kx, row.y, nom_w, row.h}, z.nom, 15, 400, M->matn.shaffof(op));
                if (z.valyuta == L"dollar") ui.nishon(kx + 6 + nw, row.y + 15, L"$", M->tasdiq_fon, M->tasdiq_chek, M->tasdiq);
                float ox = row.o() - TUG;
                float bw2 = gfx.matn_eni(z.birlik, 15, 400);
                float sb = z.qaytarilgan > 0 ? 30 : row.h;
                gfx.matn({ox - 13 - bw2, row.y, bw2 + 1, sb}, z.birlik, 15, 400, M->matn3, ONG);
                gfx.matn({ox - 200, row.y, 200 - 18 - bw2, sb}, son(z.miqdor), 15, 400, M->matn.shaffof(op), ONG);
                if (z.qaytarilgan > 0)
                    gfx.matn({ox - 260, row.y + 28, 247, 17}, son(z.qaytarilgan) + L" " + z.birlik + L" qaytarilgan", 12,
                             400, M->matn2, ONG);
                if (!s.bekor && z.miqdor > 1e-9 &&
                    ui.tugma("sv/qy/" + std::to_string(z.id), {ox + 8, row.y + 8, TUG - 21, 34}, L"Qaytarish", TINCH,
                             "i-kirim", true)) {
                    qaytarish_och(true, z.id, J_SOTUVLAR, 0);
                    sotuvdan_chiq = true;
                    break;
                }
            }
            if (sotuvdan_chiq) break;
            cy += boy + 14;
        }
        ui.scroll_tugat();
    }
    chek_oynasi_chiz();
}
