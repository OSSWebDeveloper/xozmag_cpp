// QARZDORLAR — templates/qarz/qarzdorlar.html, qidirish.html, qarzdor_karta.html, yangi_qarzdor.html
#include "db3.h"
#include "ilova.h"

namespace {

// rejim 0 — «Qarzdorlar» (menyu), 1 — «Oldin qarz olgan» (bosh sahifa)
int rejim = 0;
int korinish = 0;  // 0 tanlov, 1 ro'yxat, 2 hududlar
long long hudud = 0;
std::vector<QarzdorQ> royxat;
std::vector<HududKarta> hk;

wstring pul_d(double s, double d) {
    wstring r = pul(s);
    if (d) r += L" · " + son(d) + L" $";
    return r;
}

void korinishga(int k, long long h = 0) {
    korinish = k;
    hudud = h;
    if (k == 1) royxat = qarzdorlar_royxati(h);
    if (k == 2) hk = hudud_kartalari();
    ui.scroll_tepaga("qz/jadval");
    ui.scroll_tepaga("qz/hud");
}

void tanlov_chiz(R r) {
    sarlavha(r, rejim ? L"Eski qarzdorni topish" : L"Qarzdorlar",
             rejim ? L"qanday izlaymiz?" : L"qanday ko'rmoqchisiz?");
    float en = std::min(720.f, r.w), g = 14, k = (en - g) / 2;
    float x = r.x + (r.w - en) / 2, y = r.y;
    k = std::min(k, (r.h - 80) * 1.f);
    if (tanlov_karta("qz/t1", {x, y, k, k}, rejim ? L"Barcha qarzdorlar" : L"Qarzdorlar ro'yxati",
                     rejim ? L"Hammasi bitta ro'yxatda" : L"Hammasi bitta ro'yxatda, eng ko'p qarzdorlar tepada",
                     "i-odamlar", 2))
        korinishga(1);
    if (tanlov_karta("qz/t2", {x + k + g, y, k, k}, L"Hududlar",
                     L"Avval hududni tanlang, so'ng o'sha hududning qarzdorlari", "i-uy", 3))
        korinishga(2);
    if (rejim) {
        float bw = std::min(320.f, en);
        if (ui.tugma("qz/orqa", {r.x + (r.w - bw) / 2, y + k + g, bw, 50}, L"Orqaga", TINCH, "i-chap"))
            ot(J_BOSH);
    }
}

void hududlar_chiz_(R r) {
    float en = std::min(1100.f, r.w);
    float x = r.x + (r.w - en) / 2, y = r.y + 14 * 6;
    int ust = std::max(1, int((en + 14) / (190 + 14)));
    float kw = (en - (ust - 1) * 14) / ust, kh = 96;
    int qs = ((int)hk.size() + ust - 1) / ust;
    float tagi = 14 + 50 + (rejim ? 0 : 14 + 50);
    float joy = r.p() - y - tagi;
    float boy = qs * (kh + 14) - 14;
    float sk = std::min(boy, joy);
    ui.scroll("qz/hud", {x - 4, y - 4, en + 8, sk + 8}, boy + 8);
    for (size_t i = 0; i < hk.size(); ++i) {
        auto& h = hk[i];
        R t = {x + (i % ust) * (kw + 14), y + (i / ust) * (kh + 14), kw, kh};
        bool qarzli = h.balans > 0 || h.balans_d > 0, hov = ui.ustida(t);
        if (hov) t.y -= 2;
        gfx.quti({t.x, t.y + 2, t.w, t.h}, {0, 0, 0, M->kechki ? .3f : .08f}, 12);
        gfx.quti(t, hov ? (qarzli ? M->ogoh_fon : M->panel2) : (qarzli ? M->ogoh_fon : M->panel), 12,
                 hov ? (qarzli ? M->ogoh : M->urgu) : (qarzli ? M->ogoh_chek : M->chegara));
        gfx.matn({t.x + 16, t.y + 14, t.w - 32, 27}, h.h.nom, 19, 650, M->matn);
        if (h.soni) {
            gfx.matn({t.x + 16, t.y + 42, t.w - 32, 17}, std::to_wstring(h.soni) + L" ta qarzdor", 12, 400, M->matn2);
            gfx.matn({t.x + 16, t.p() - 14 - 21, t.w - 32, 21},
                     pul(h.balans) + L" so'm" + (h.balans_d ? L" · " + son(h.balans_d) + L" $" : L""), 15, 700,
                     M->urgu_matn);
        } else
            gfx.matn({t.x + 16, t.y + 42, t.w - 32, 17}, L"qarzdor yo'q", 12, 400, M->matn3);
        if (ui.bosildi("qz/h/" + std::to_string(h.h.id), t)) {
            korinishga(1, h.h.id);
            break;
        }
    }
    ui.scroll_tugat();
    y += sk + 14;
    if (!rejim) {
        float w = std::min(520.f, en);
        float bw = gfx.matn_eni(L"Hudud qo'shish", 15, 600) + 36 + 27;
        MaydonSoz s;
        s.ishora = L"Yangi hudud nomi";
        ui.maydon("qz/yangi_hudud", {r.x + (r.w - w) / 2, y, w - bw - 14, 50}, s);
        if (ui.tugma("qz/hqosh", {r.x + (r.w - w) / 2 + w - bw, y, bw, 50}, L"Hudud qo'shish", YASHIL, "i-plus") ||
            ui.enter("qz/yangi_hudud")) {
            wstring x2;
            if (hudud_qosh(ui.qiymat("qz/yangi_hudud"), x2)) {
                ui.xabar(L"«" + tozala(ui.qiymat("qz/yangi_hudud")) + L"» hududi qo'shildi.");
                ui.qiymat_qoy("qz/yangi_hudud", L"");
                hk = hudud_kartalari();
            } else
                ui.xabar(x2, X_XATO);
        }
        y += 50 + 14;
    }
    float bw = std::min(320.f, en);
    if (ui.tugma("qz/horqa", {r.x + (r.w - bw) / 2, y, bw, 50}, L"Orqaga", TINCH, "i-chap")) korinishga(0);
}

void royxat_chiz(R r) {
    Hudud h;
    bool hbor = hudud && hudud_ol(hudud, h);
    if (hbor) sarlavha(r, h.nom, rejim ? L"shu hududning qarzdorlari" : L"eng ko'p qarzdorlar tepada");
    else sarlavha(r, rejim ? L"Barcha qarzdorlar" : L"Qarzdorlar ro'yxati", L"eng ko'p qarzdorlar tepada");
    // tugmalar
    R q = {r.x, r.y, r.w, 78};
    quti(q);
    float w = (q.w - 28 - 28) / 3;
    R b1 = {q.x + 14, q.y + 14, w, 50}, b2 = {b1.o() + 14, b1.y, w, 50}, b3 = {b2.o() + 14, b1.y, w, 50};
    if (ui.tugma("qz/r/orqa", b1, L"Orqaga", TINCH, "i-chap")) korinishga(hbor ? 2 : 0);
    if (hbor) {
        if (ui.tugma("qz/r/hamma", b2, L"Barcha qarzdorlar")) korinishga(1);
    } else if (ui.tugma("qz/r/hudud", b2, L"Hudud bo'yicha"))
        korinishga(2);
    if (ui.tugma("qz/r/yangi", b3, L"Yangi qarzdor", YASHIL, rejim ? "i-odam-plus" : "i-plus")) ot(J_YANGI_QARZDOR);
    r.y += 78 + 14;
    r.h -= 78 + 14;
    // jadval
    std::vector<Ustun> u = {{L"Familiya Ism", -2}};
    if (!hbor) u.push_back({L"Hudud", -1});
    u.push_back({L"Telefon", -1});
    if (!rejim) {
        u.push_back({L"Olgan", 140, true});
        u.push_back({L"To'lagan", 140, true});
    }
    u.push_back({L"Qarzi", 140, true});
    u.push_back({L"", 130});
    double js = 0, jd = 0;
    for (auto& x : royxat) js += x.q.balans, jd += x.q.balans_d;
    float tfoot = (!rejim && !royxat.empty()) ? 56 : 0;
    float qb = 56;
    float boy = std::min(r.h, 38 + std::max(1.f, (float)royxat.size()) * qb + tfoot);
    if (royxat.empty()) boy = std::min(r.h, 38.f + 200);
    R j = {r.x, r.y, r.w, boy};
    gfx.quti(j, M->panel, 12, M->chegara);
    gfx.kes(j.ichki(1));
    jadval_sarlavha({j.x, j.y, j.w, 38}, u);
    R tana = {j.x, j.y + 38, j.w, j.h - 38 - tfoot};
    if (royxat.empty()) {
        bosh_holat(tana, "i-odamlar", hbor ? L"Bu hududda qarzdor yo'q" : L"Hali qarzdor yo'q", L"");
        float bw = 240;
        if (ui.tugma("qz/r/bosh", {tana.x + (tana.w - bw) / 2, tana.y + 130, bw, 50}, L"Yangi qarzdor qo'shish", YASHIL))
            ot(J_YANGI_QARZDOR);
    } else {
        ui.scroll("qz/jadval", tana, royxat.size() * qb);
        for (size_t i = 0; i < royxat.size(); ++i) {
            auto& x = royxat[i];
            R t = {tana.x, tana.y + i * qb, tana.w, qb};
            bool hov = ui.ustida(t);
            if (x.qarzli()) gfx.quti(t, hex("#ffb020", .06f));
            if (hov) gfx.quti(t, M->panel2);
            if (i + 1 < royxat.size()) gfx.chiziq(t.x, t.p() - .5f, t.o(), t.p() - .5f, M->chegara);
            auto c = ustunlar(t, u);
            size_t k = 0;
            gfx.matn(c[k++].ichki(13, 0), x.q.toliq(), 15, 700, M->matn);
            if (!hbor) gfx.matn(c[k++].ichki(13, 0), x.q.hudud, 15, 400, M->matn);
            gfx.matn(c[k++].ichki(13, 0), x.q.telefon.empty() ? L"—" : x.q.telefon, 15, 400, M->matn);
            auto son_ustun = [&](R cc, double s, double d, bool qalin, Rang rang) {
                R a = cc.ichki(13, 0);
                if (d) {
                    gfx.matn({a.x, t.y + 8, a.w, 21}, pul(s), 15, qalin ? 700 : 400, rang, ONG);
                    gfx.matn({a.x, t.y + 29, a.w, 17}, son(d) + L" $", 12, 700, M->tasdiq, ONG);
                } else
                    gfx.matn(a, pul(s), 15, qalin ? 700 : 400, rang, ONG);
            };
            if (!rejim) {
                son_ustun(c[k++], x.olgan, x.olgan_d, false, M->matn3);
                son_ustun(c[k++], x.tolagan, x.tolagan_d, false, M->matn3);
            }
            son_ustun(c[k++], x.q.balans, x.q.balans_d, true, M->matn);
            R bt = {c[k].x + 13, t.y + 11, c[k].w - 26, 34};
            if (ui.tugma("qz/r/qy/" + std::to_string(x.q.id), bt, L"Qarz yozish", YASHIL, nullptr, true)) {
                ot(J_QARZ_YOZ, ochiq_qarz(x.q.id, foydalanuvchi.id));
                break;
            }
            if (ui.bosildi("qz/r/q/" + std::to_string(x.q.id), t)) {
                ot(J_KARTA, x.q.id);
                break;
            }
        }
        ui.scroll_tugat();
        if (tfoot) {
            R f = {j.x, j.p() - tfoot, j.w, tfoot};
            gfx.quti(f, M->panel2);
            gfx.chiziq(f.x, f.y + .5f, f.o(), f.y + .5f, M->chegara);
            auto c = ustunlar(f, u);
            size_t qi = c.size() - 2;
            gfx.matn({f.x, f.y, c[qi].x - f.x - 13, f.h}, L"Jami qarz", 15, 600, M->matn, ONG);
            R a = c[qi].ichki(13, 0);
            if (jd) {
                gfx.matn({a.x, f.y + 6, a.w, 27}, pul(js), 19, 700, M->matn, ONG);
                gfx.matn({a.x, f.y + 32, a.w, 17}, son(jd) + L" $", 12, 700, M->tasdiq, ONG);
            } else
                gfx.matn(a, pul(js), 19, 700, M->matn, ONG);
        }
    }
    gfx.kesishni_ol();
}

}  // namespace

void qarzdorlar_chiz(R r) {
    if (korinish == 0) tanlov_chiz(r);
    else if (korinish == 1) royxat_chiz(r);
    else hududlar_chiz_(r);
}

void qarzdor_royxat_ochildi(long long p) {
    rejim = 0;
    korinishga(1, p);
}
void qarzdor_royxat_chiz(R r) { qarzdorlar_chiz(r); }
void hududlar_ochildi(long long p) {
    rejim = 0;
    korinishga(p ? 1 : 2, p);
}
void hududlar_chiz(R r) { qarzdorlar_chiz(r); }
void qidirish_ochildi(long long) {
    rejim = 1;
    korinishga(0);
}
void qidirish_chiz(R r) { qarzdorlar_chiz(r); }
void qarzdorlar_ochildi(long long) {
    rejim = 0;
    korinishga(0);
}

// ======================= Qarzdor kartasi =======================

namespace {
long long karta_id = 0;
QarzdorQ kq;
std::vector<KartaQarz> kqarz;
std::vector<KartaTolov> ktolov;
int tolov_valyuta = 0;

void karta_yukla() {
    kq = qarzdor_hisobi(karta_id);
    kqarz = karta_qarzlari(karta_id);
    ktolov = karta_tolovlari(karta_id);
}

// bitta qarz hujjati jadvali; balandligini qaytaradi
float hujjat_chiz(const KartaQarz& q, float x, float y, float w, bool chiz) {
    float sar = 44, qb = 50;
    float boy = sar + q.q.size() * qb + (q.yakunlangan ? 0 : 56);
    if (!chiz) return boy;
    R r = {x, y, w, boy};
    gfx.quti(r, M->panel, 12, M->chegara);
    R s = {x + 1, y + 1, w - 2, sar - 1};
    gfx.quti(s, M->panel2, 11);
    gfx.quti({s.x, s.y + 20, s.w, s.h - 20}, M->panel2);
    if (!q.q.empty() || !q.yakunlangan) gfx.chiziq(x, y + sar - .5f, r.o(), y + sar - .5f, M->chegara);
    wstring bosh = mahalliy_vaqt(q.sana) + L" · hujjat #" + std::to_wstring(q.id);
    float nx = x + 13;
    gfx.matn({nx, y, w * .6f, sar}, bosh, 11, 600, M->matn3);
    nx += gfx.matn_eni(bosh, 11, 600) + 8;
    float ny = y + (sar - 19) / 2;
    if (!q.yakunlangan) nx += ui.nishon(nx, ny, L"Tugallanmagan", M->ogoh_fon, M->ogoh_chek, M->ogoh) + 6;
    bool qayt = q.qayt > 0 || q.qayt_d > 0;
    for (auto& z : q.q) qayt |= z.qaytarilgan > 0;
    if (qayt) nx += ui.nishon(nx, ny, L"Qaytarish bo'lgan", M->ogoh_fon, M->ogoh_chek, M->ogoh) + 6;
    if (q.sotuv_id) nx += ui.nishon(nx, ny, L"Chek #" + std::to_wstring(q.sotuv_id) + L" dan", M->kok_fon, M->kok_chek, M->kok) + 6;
    if (q.old > 0 || q.old_d > 0)
        nx += ui.nishon(nx, ny, L"Oldindan to'langan: " + (q.old > 0 ? pul(q.old) + L" so'm " : L"") +
                                    (q.old_d > 0 ? son(q.old_d) + L" $" : L""),
                        M->tasdiq_fon, M->tasdiq_chek, M->tasdiq) + 6;
    wstring jm = q.sof ? pul(q.sof) + L" so'm" : L"";
    if (q.sof_d) {
        float dw = gfx.matn_eni(son(q.sof_d) + L" $", 12, 700);
        gfx.matn({r.o() - 13 - dw, y, dw, sar}, son(q.sof_d) + L" $", 12, 700, M->tasdiq, ONG);
        gfx.matn({r.o() - 13 - dw - 200, y, 194, sar}, jm, 11, 600, M->matn3, ONG);
    } else
        gfx.matn({r.o() - 213, y, 200, sar}, jm.empty() ? L"0" : jm, 11, 600, M->matn3, ONG);
    for (size_t i = 0; i < q.q.size(); ++i) {
        auto& z = q.q[i];
        R t = {x, y + sar + i * qb, w, qb};
        if (i + 1 < q.q.size() || !q.yakunlangan) gfx.chiziq(t.x, t.p() - .5f, t.o(), t.p() - .5f, M->chegara);
        float nw = gfx.matn_eni(z.nom, 15, 400);
        gfx.matn({t.x + 13, t.y, w - 260, t.h}, z.nom, 15, 400, M->matn);
        if (z.dollar) ui.nishon(t.x + 13 + std::min(nw, w - 260) + 6, t.y + 15, L"$", M->tasdiq_fon, M->tasdiq_chek, M->tasdiq);
        float bw = gfx.matn_eni(z.birlik, 15, 400);
        gfx.matn({t.o() - 13 - bw, t.y, bw + 1, z.qaytarilgan > 0 ? 30 : t.h}, z.birlik, 15, 400, M->matn3, ONG);
        gfx.matn({t.o() - 200, t.y, 200 - 17 - bw, z.qaytarilgan > 0 ? 30 : t.h}, son(z.qolgan), 15, 400, M->matn, ONG);
        if (z.qaytarilgan > 0)
            gfx.matn({t.o() - 260, t.y + 28, 247, 17}, son(z.qaytarilgan) + L" " + z.birlik + L" qaytarilgan", 12, 400, M->matn2, ONG);
    }
    if (!q.yakunlangan) {
        R t = {x + 13, r.p() - 46, 160, 34};
        if (ui.tugma("kr/dav/" + std::to_string(q.id), t, L"Davom ettirish", ASOSIY, nullptr, true))
            ot(J_QARZ_YOZ, q.id);
    }
    return boy;
}
}  // namespace

void karta_ochildi(long long p) {
    karta_id = p;
    tolov_valyuta = 0;
    ui.maydonlarni_tozala("kr/");
    karta_yukla();
    ui.scroll_tepaga("kr/tarix");
    ui.scroll_tepaga("kr/tolovlar");
}

void karta_chiz(R r) {
    auto& q = kq.q;
    // sarlavha + hudud nishoni
    float nw = gfx.matn_eni(q.toliq(), 23, 650);
    gfx.matn({r.x, r.y, nw + 2, 32}, q.toliq(), 23, 650, M->matn, CHAP, 2);
    ui.nishon(r.x + nw + 10, r.y + 10, q.hudud, M->panel3, M->chegara2, M->matn2);
    r.y += 46;
    r.h -= 46;
    // ko'rsatkichlar
    R qt = {r.x, r.y, r.w, 14 + 86 + 14 + 50 + 14};
    quti(qt);
    float kw = (qt.w - 28 - 3 * 14) / 4, ky = qt.y + 14;
    korsatkich({qt.x + 14, ky, kw, 86}, L"TELEFON", q.telefon.empty() ? L"—" : q.telefon);
    korsatkich({qt.x + 14 + (kw + 14), ky, kw, 86}, L"JAMI OLGAN", pul(kq.olgan), kq.olgan_d ? son(kq.olgan_d) + L" $" : L"");
    korsatkich({qt.x + 14 + 2 * (kw + 14), ky, kw, 86}, L"JAMI TO'LAGAN", pul(kq.tolagan), kq.tolagan_d ? son(kq.tolagan_d) + L" $" : L"");
    korsatkich({qt.x + 14 + 3 * (kw + 14), ky, kw, 86}, L"QOLGAN QARZI", pul(q.balans) + L" so'm",
               q.balans_d ? son(q.balans_d) + L" $" : L"", 1);
    float by = ky + 86 + 14;
    float t1 = gfx.matn_eni(L"Ma'lumotini tahrirlash", 15, 600) + 36 + 27, t2 = gfx.matn_eni(L"Bosh sahifa", 15, 600) + 36 + 27;
    float bw = qt.w - 28 - 28 - t1 - t2;
    if (ui.tugma("kr/yangi", {qt.x + 14, by, bw, 50}, L"Yangi qarz yozish", YASHIL, "i-plus"))
        ot(J_QARZ_YOZ, ochiq_qarz(q.id, foydalanuvchi.id));
    if (ui.tugma("kr/tahrir", {qt.x + 28 + bw, by, t1, 50}, L"Ma'lumotini tahrirlash", TINCH, "i-qalam"))
        ot(J_YANGI_QARZDOR, q.id);
    if (ui.tugma("kr/bosh", {qt.x + 42 + bw + t1, by, t2, 50}, L"Bosh sahifa", TINCH, "i-uy")) ot(J_BOSH);
    r.y += qt.h + 14;
    r.h -= qt.h + 14;
    // ikki ustun: chapda qarzlar tarixi (2), o'ngda to'lov (1)
    float ow = std::max(300.f, (r.w - 14) / 3);
    R chap = {r.x, r.y, r.w - ow - 14, r.h}, ong = {chap.o() + 14, r.y, ow, r.h};
    quti(chap);
    gfx.matn({chap.x + 14, chap.y + 14, chap.w - 28, 27}, L"Qarzlar tarixi", 19, 650, M->matn);
    R tt = {chap.x + 14, chap.y + 14 + 27 + 12, chap.w - 28, chap.h - 14 - 27 - 12 - 14};
    if (kqarz.empty()) {
        bosh_holat(tt, "i-chek", L"Hali qarz yozilmagan", L"");
    } else {
        float jami = 0;
        for (auto& k : kqarz) jami += hujjat_chiz(k, 0, 0, tt.w - 12, false) + 14;
        ui.scroll("kr/tarix", tt, jami - 14);
        float y = tt.y;
        for (auto& k : kqarz) y += hujjat_chiz(k, tt.x, y, tt.w - 12, true) + 14;
        ui.scroll_tugat();
    }
    // to'lov
    quti(ong);
    float x = ong.x + 14, w = ong.w - 28, y = ong.y + 14;
    gfx.matn({x, y, w, 27}, L"To'lov qabul qilish", 19, 650, M->matn);
    y += 27 + 12;
    auto yorliq = [&](const wchar_t* t) { gfx.matn({x, y, w, 17}, t, 12, 600, M->matn2); y += 22; };
    yorliq(L"Summa");
    MaydonSoz s;
    s.ong = true;
    s.ishora = L"0";
    ui.maydon("kr/summa", {x, y, w, 50}, s);
    pul_guruhla("kr/summa");
    y += 50 + 14;
    yorliq(L"Valyuta");
    ui.tanlov("kr/valyuta", {x, y, w, 50}, {L"---------", L"so'm", L"dollar"}, tolov_valyuta);
    y += 50 + 14;
    yorliq(L"Izoh");
    MaydonSoz iz;
    iz.ishora = L"Izoh (ixtiyoriy)";
    ui.maydon("kr/izoh", {x, y, w, 50}, iz);
    y += 50 + 14;
    if (ui.tugma("kr/tolov", {x, y, w, 50}, L"To'lovni yozish", YASHIL, "i-hamyon") || ui.enter("kr/summa")) {
        double v = 0;
        bool dol = tolov_valyuta == 2;
        wstring pm = dol ? L"dollar" : L"so'm";
        double qoldiq = dol ? q.balans_d : q.balans;
        if (!songa(ui.qiymat("kr/summa"), v)) ui.xabar(L"To'lov summasi xato.", X_XATO);
        else if (v <= 0) ui.xabar(L"To'lov summasi noldan katta bo'lishi kerak.", X_XATO);
        else if (qoldiq <= 0) ui.xabar(q.toliq() + L" ning " + pm + L" qarzi yo'q — to'lov yozishning hojati yo'q.", X_XATO);
        else if (v > qoldiq + 0.001)
            ui.xabar(L"Qolgan " + pm + L" qarzi " + (dol ? son(qoldiq) + L" $" : pul(qoldiq) + L" so'm") +
                         L". Bundan ortiq to'lov yozib bo'lmaydi.", X_XATO);
        else {
            tolov_qosh(q.id, yaxlit(v, 2), dol ? L"dollar" : L"som", tozala(ui.qiymat("kr/izoh")));
            karta_yukla();
            double qolgan = dol ? q.balans_d : q.balans;
            ui.xabar(qolgan > 0 ? L"To'lov qabul qilindi. Qolgan " + pm + L" qarzi " +
                                      (dol ? son(qolgan) + L" $" : pul(qolgan) + L" so'm") + L"."
                                : L"To'lov qabul qilindi. " + pm + L" qarzi to'liq yopildi.");
            ui.qiymat_qoy("kr/summa", L"");
            ui.qiymat_qoy("kr/izoh", L"");
        }
    }
    y += 50 + 14;
    if (!ktolov.empty()) {
        std::vector<Ustun> u = {{L"Sana", -1}, {L"Summa", -1, true}, {L"", 60}};
        float boy = std::min(ong.p() - 14 - y, 38 + ktolov.size() * 54.f);
        R j = {x, y, w, boy};
        gfx.quti(j, M->panel, 12, M->chegara);
        gfx.kes(j.ichki(1));
        jadval_sarlavha({j.x, j.y, j.w, 38}, u);
        R tana = {j.x, j.y + 38, j.w, j.h - 38};
        ui.scroll("kr/tolovlar", tana, ktolov.size() * 54.f);
        for (size_t i = 0; i < ktolov.size(); ++i) {
            auto& t = ktolov[i];
            R row = {tana.x, tana.y + i * 54.f, tana.w, 54};
            if (i + 1 < ktolov.size()) gfx.chiziq(row.x, row.p() - .5f, row.o(), row.p() - .5f, M->chegara);
            auto c = ustunlar(row, u);
            gfx.matn(c[0].ichki(13, 0), mahalliy_vaqt(t.sana, false), 15, 400, M->matn);
            wstring bir = t.dollar ? L"$" : L"so'm";
            float bw2 = gfx.matn_eni(bir, 15, 400);
            R a = c[1].ichki(13, 0);
            gfx.matn({a.o() - bw2, a.y, bw2 + 1, a.h}, bir, 15, 400, M->matn3, ONG);
            gfx.matn({a.x, a.y, a.w - bw2 - 5, a.h}, t.dollar ? son(t.summa) : pul(t.summa), 15, 700, M->matn, ONG);
            if (ui.tugma("kr/toch/" + std::to_string(t.id), {c[2].x + 13, row.y + 10, 34, 34}, L"", XATO_U, "i-chiqar", true)) {
                if (MessageBoxW(ui.h, L"Bu to'lov o'chirilsinmi? Qarz summasi ortga qaytadi.", L"Ulug'bek Xozmag",
                                MB_OKCANCEL | MB_ICONQUESTION) == IDOK) {
                    tolov_ochir(t.id);
                    ui.xabar(L"To'lov o'chirildi.", X_INFO);
                    karta_yukla();
                    break;
                }
            }
        }
        ui.scroll_tugat();
        gfx.kesishni_ol();
    }
}

// ======================= Yangi qarzdor / tahrir =======================

namespace {
long long tahrir_id = 0;
int yq_hudud = 0;
std::vector<Hudud> yq_hl;
wstring yq_xato[4];
}  // namespace

void yangi_qarzdor_ochildi(long long p) {
    tahrir_id = p;
    ui.maydonlarni_tozala("yq/");
    yq_hl = hududlar();
    yq_hudud = 0;
    for (auto& x : yq_xato) x.clear();
    if (p) {
        Qarzdor q;
        if (qarzdor_ol(p, q)) {
            ui.qiymat_qoy("yq/0", q.ism);
            ui.qiymat_qoy("yq/1", q.familiya);
            ui.qiymat_qoy("yq/2", q.telefon);
            for (size_t i = 0; i < yq_hl.size(); ++i)
                if (yq_hl[i].id == q.hudud_id) yq_hudud = (int)i + 1;
        }
    }
    ui.fokus("yq/0");
}

void yangi_qarzdor_chiz(R r) {
    float en = std::min(720.f, r.w);
    R s = {r.x + (r.w - en) / 2, r.y, en, r.h};
    sarlavha(s, tahrir_id ? L"Qarzdorni tahrirlash" : L"Yangi qarzdor", tahrir_id ? L"" : L"— ma'lumotini yozamiz");
    const wchar_t* nom[4] = {L"Ism", L"Familiya", L"Telefon", L"Hudud"};
    const wchar_t* ish[3] = {L"Ism", L"Familiya", L"+998 __ ___ __ __"};
    float boy = 14 + 4 * (22 + 50 + 14) + 50 + 14;
    for (auto& x : yq_xato) if (!x.empty()) boy += 20;
    R q = {s.x, s.y, s.w, boy};
    quti(q);
    float x = q.x + 14, w = q.w - 28, y = q.y + 14;
    std::vector<wstring> hn = {L"— Hududni tanlang —"};
    for (auto& h : yq_hl) hn.push_back(h.nom);
    for (int i = 0; i < 4; ++i) {
        gfx.matn({x, y, w, 17}, nom[i], 12, 600, M->matn2);
        y += 22;
        if (i < 3) {
            MaydonSoz m;
            m.ishora = ish[i];
            ui.maydon("yq/" + std::to_string(i), {x, y, w, 50}, m);
        } else
            ui.tanlov("yq/hudud", {x, y, w, 50}, hn, yq_hudud);
        y += 50;
        if (!yq_xato[i].empty()) {
            gfx.matn({x, y + 4, w, 16}, yq_xato[i], 12, 500, M->kechki ? hex("#ffa0a0") : M->xato);
            y += 20;
        }
        y += 14;
    }
    float b1 = (w - 14) / 3;
    if (ui.tugma("yq/bekor", {x, y, b1, 50}, L"Bekor qilish", TINCH)) {
        if (tahrir_id) ot(J_KARTA, tahrir_id);
        else ot(J_BOSH);
    }
    bool saqla = ui.tugma("yq/saqla", {x + b1 + 14, y, w - b1 - 14, 50},
                          tahrir_id ? L"Saqlash" : L"Yaratish va qarz yozish", YASHIL, "i-belgi");
    if (!saqla) return;
    for (auto& e : yq_xato) e.clear();
    Qarzdor qd;
    qarzdor_ol(tahrir_id, qd);
    qd.id = tahrir_id;
    qd.ism = tozala(ui.qiymat("yq/0"));
    qd.familiya = tozala(ui.qiymat("yq/1"));
    qd.telefon = tozala(ui.qiymat("yq/2"));
    qd.hudud_id = yq_hudud > 0 ? yq_hl[yq_hudud - 1].id : 0;
    if (qd.ism.empty()) yq_xato[0] = L"Bu maydon to'ldirilishi shart.";
    if (qd.familiya.empty()) yq_xato[1] = L"Bu maydon to'ldirilishi shart.";
    if (!qd.hudud_id) yq_xato[3] = L"Bu maydon to'ldirilishi shart.";
    for (auto& e : yq_xato) if (!e.empty()) return;
    wstring xato;
    long long id = qarzdor_saqla(qd, xato);
    if (!id) {
        ui.xabar(xato, X_XATO);
        return;
    }
    if (tahrir_id) {
        ui.xabar(L"Ma'lumot saqlandi.");
        ot(J_KARTA, id);
    } else
        ot(J_QARZ_YOZ, ochiq_qarz(id, foydalanuvchi.id));
}
