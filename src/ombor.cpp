// OMBOR — templates/ombor/royxat.html, kirim_forma.html, qoldiq_tuzatish.html, harakatlar.html,
// mahsulot_form.html
#include "db3.h"
#include "excel.h"
#include "ilova.h"

void ombor_eksport(const std::vector<const OmborTovar*>& tl, bool kam);  // excel_sahifa.cpp

namespace {

OmborFiltr filtr;
std::vector<const OmborTovar*> natija;
int sahifa = 0;
const int SAHIFADA = 50;
long long tugagan = 0;
// oynalar
long long kirim_id = 0, tuzat_id = 0, tarix_id = 0, shtrix_id = 0;
double tuzat_eski = 0;     // oyna ochilgandagi qoldiq (saytdagi eski_qoldiq)
wstring shtrix_eski;       // oyna ochilgandagi shtrix (saytdagi «eski»)
wstring tuzat_xato;
int kirim_birlik = 0;  // 0 — sotuv birligi, 1 — olish birligi
std::vector<Harakat> tarix;

void yangila(bool tepaga = true) {
    natija = ombor_filtrla(filtr);
    tugagan = tugagan_soni();
    int sahifalar = std::max(1, ((int)natija.size() + SAHIFADA - 1) / SAHIFADA);
    if (sahifa >= sahifalar) sahifa = sahifalar - 1;
    if (tepaga) ui.scroll_tepaga("om/jadval");
}

bool ikki(const Tovar& t) { return !t.olish_birligi.empty() && t.olish_birligi != t.birlik; }

wstring qadoq_matni(const Tovar& t) {
    if (!ikki(t) || t.olish_miqdori <= 0) return L"";
    double butun = std::floor(t.qoldiq / t.olish_miqdori + 1e-9), ortiq = t.qoldiq - butun * t.olish_miqdori;
    wstring r;
    if (butun) r = son(butun) + L" " + t.olish_birligi;
    if (ortiq > 1e-9 || !butun) r += (r.empty() ? L"" : L" ") + son(ortiq) + L" " + t.birlik;
    return r;
}

// ---------- Kirim oynasi ----------
void kirim_oynasi() {
    Tovar t;
    if (!tovar_ol(kirim_id, t)) { kirim_id = 0; return; }
    bool ik = ikki(t);
    float boy = 58 + 14 + 92 + 14 + (ik ? 60 + 22 + 50 + 14 : 0) + 22 + 50 + 14 + 22 + 50 + 14 + 50 + 14 + (sensor ? 0 : 0);
    if (sensor) boy = std::max(boy, 58.f + 14 + 4 * 57 + 50 + 28);
    bool yop = false;
    R o = oyna("om/k", L"Kirim — " + t.nom, sensor ? 900 : 620, boy, yop);
    if (yop) { kirim_id = 0; return; }
    R b = o.ichki(14);
    R chap = sensor ? R{b.x, b.y, b.w - 330 - 14, b.h} : b;
    float x = chap.x, w = chap.w, y = chap.y;
    korsatkich({x, y, w, 92}, L"HOZIRGI QOLDIQ", son(t.qoldiq) + L" " + t.birlik);
    if (ik) gfx.matn({x + 14, y + 64, w - 28, 18}, qadoq_matni(t), 12, 500, M->matn3);
    y += 92 + 14;
    if (ik) {
        R x2 = {x, y, w, 46};
        gfx.quti(x2, M->kok_fon, 9, M->kok_chek);
        gfx.ikon("i-quti", {x2.x + 12, x2.y + 15, 16, 16}, M->kok);
        gfx.matn({x2.x + 37, x2.y, x2.w - 49, x2.h}, L"1 " + t.olish_birligi + L" = " + son(t.olish_miqdori) + L" " + t.birlik,
                 13, 700, M->kok);
        y += 46 + 14;
        gfx.matn({x, y, w, 17}, L"Qaysi birlikda keldi", 12, 600, M->matn2);
        y += 22;
        float bw = (w - 10) / 2;
        for (int i = 0; i < 2; ++i) {
            R r2 = {x + i * (bw + 10), y, bw, 50};
            bool tan = kirim_birlik == i;
            gfx.quti(r2, tan ? M->urgu_fon : M->panel2, 9, tan ? M->urgu : M->chegara2);
            gfx.matn(r2, i ? t.olish_birligi : t.birlik, 15, 600, tan ? M->urgu_matn : M->matn, MARKAZ);
            if (ui.bosildi("om/k/b" + std::to_string(i), r2)) kirim_birlik = i;
        }
        y += 50 + 14;
    }
    gfx.matn({x, y, w, 17}, L"Miqdor", 12, 600, M->matn2);
    y += 22;
    MaydonSoz m;
    m.ong = true;
    m.ishora = L"0";
    ui.maydon("om/k/miqdor", {x, y, w, 50}, m);
    y += 50 + 14;
    double v = 0;
    bool ok = songa(ui.qiymat("om/k/miqdor"), v) && v > 0;
    gfx.matn({x, y, w, 17}, L"Izoh", 12, 600, M->matn2);
    y += 22;
    MaydonSoz iz;
    iz.ishora = L"Masalan: yetkazib beruvchi, nakladnoy raqami";
    ui.maydon("om/k/izoh", {x, y, w, 50}, iz);
    y += 50 + 14;
    if (sensor) raqamlar("om/k/rq", {b.o() - 330, b.y, 330, 4 * 57.f}, "om/k/miqdor");
    float w1 = (b.w - 20) / 5;
    R pb = {b.x, b.p() - 50, b.w, 50};
    if (ui.tugma("om/k/bekor", {pb.x, pb.y, w1, 50}, L"Bekor qilish", TINCH)) { kirim_id = 0; return; }
    bool qosh = ui.tugma("om/k/qosh", {pb.x + w1 + 10, pb.y, w1 * 2, 50}, L"Omborga qo'shish", YASHIL, "i-kirim") ||
                ui.enter("om/k/miqdor");
    bool etk = ui.tugma("om/k/etk", {pb.x + 3 * w1 + 20, pb.y, w1 * 2, 50}, L"Qo'shish va etiketka", ODDIY, "i-printer");
    if (qosh || etk) {
        if (!ok) {
            ui.xabar(L"Miqdor: noldan katta son yozing.", X_XATO);
            return;
        }
        bool olishda = ik && kirim_birlik == 1;
        double sotuvda = olishda ? yaxlit(v * t.olish_miqdori, 3) : yaxlit(v, 3);
        wstring izoh = tozala(ui.qiymat("om/k/izoh"));
        kirim_qil(t.id, sotuvda, v, olishda ? t.olish_birligi : L"", izoh.empty() ? L"Kirim" : izoh);
        ombor_keshini_tashla();
        ombor_eskirdi();
        ui.xabar(t.nom + L": +" + (olishda ? son(v) + L" " + t.olish_birligi + L" = " : L"") + son(sotuvda) + L" " +
                 t.birlik + L" omborga qo'shildi.");
        kirim_id = 0;
        if (etk) {
            etiketka_och(t.id, etiketka_soni(t, v, olishda ? t.olish_birligi : t.birlik));
            return;
        }
        yangila(false);
    }
}

// ---------- Qoldiqni tuzatish (qoldiq_tuzatish.html) ----------
void tuzat_oynasi() {
    Tovar t;
    if (!tovar_ol(tuzat_id, t)) { tuzat_id = 0; return; }
    bool yop = false;
    float ichw = 620 - 28;
    wstring izoh1 = L"Qolgan yaroqli mahsulot miqdorini " + t.birlik +
                    L" hisobida kiriting. Masalan, 10 tadan 2 tasi singan bo\u2018lsa, yangi qoldiq 8 bo\u2018ladi.";
    wstring izoh2 = L"Sababni bo\u2018sh qoldirish mumkin. Oldingi va yangi qoldiq harakatlar tarixiga yoziladi.";
    float h1 = gfx.matn_boyi(izoh1, 14, 400, ichw), h2 = gfx.matn_boyi(izoh2, 14, 400, ichw);
    float xh = tuzat_xato.empty() ? 0 : ichki_xabar({0, 0, ichw, 0}, tuzat_xato, X_XATO, true) + 14;
    R o = oyna("om/t", L"Qoldiqni tuzatish \u2014 " + t.nom, 620,
               58 + 14 + 92 + 14 + h1 + 14 + xh + 2 * (22 + 50 + 14) + h2 + 14 + 50 + 14, yop);
    if (yop) { tuzat_id = 0; return; }
    R b = o.ichki(14);
    float y = b.y;
    korsatkich({b.x, y, b.w, 92}, L"HOZIRGI QOLDIQ", son(t.qoldiq) + L" " + t.birlik);
    if (ikki(t))  // .qiymat-izoh
        gfx.matn({b.x + 14, y + 64, b.w - 28, 18},
                 qadoq_matni(t) + L" \u00b7 1 " + t.olish_birligi + L" = " + son(t.olish_miqdori) + L" " + t.birlik, 12,
                 500, M->matn3);
    y += 92 + 14;
    gfx.matn({b.x, y, b.w, h1}, izoh1, 14, 400, M->matn2, CHAP, 0, true);
    y += h1 + 14;
    if (xh > 0) y += ichki_xabar({b.x, y, b.w, 0}, tuzat_xato, X_XATO) + 14;
    gfx.matn({b.x, y, b.w, 17}, L"Yangi qoldiq (" + t.birlik + L")", 12, 600, M->matn2);
    y += 22;
    MaydonSoz m;
    m.ong = true;
    ui.maydon("om/t/qoldiq", {b.x, y, b.w, 50}, m);
    y += 50 + 14;
    gfx.matn({b.x, y, b.w, 17}, L"Sabab (ixtiyoriy)", 12, 600, M->matn2);
    y += 22;
    MaydonSoz iz;
    iz.ishora = L"Masalan: singan, yaroqsiz yoki sanoq tuzatildi";
    ui.maydon("om/t/sabab", {b.x, y, b.w, 50}, iz);
    y += 50 + 14;
    gfx.matn({b.x, y, b.w, h2}, izoh2, 14, 400, M->matn2, CHAP, 0, true);
    float w1 = gfx.matn_eni(L"Bekor qilish", 15, 600) + 36, w2 = gfx.matn_eni(L"Qoldiqni saqlash", 15, 600) + 36;
    if (ui.tugma("om/t/bekor", {b.x, b.p() - 50, w1, 50}, L"Bekor qilish", TINCH)) { tuzat_id = 0; return; }
    if (ui.tugma("om/t/ok", {b.x + w1 + 14, b.p() - 50, w2, 50}, L"Qoldiqni saqlash", YASHIL) ||
        ui.enter("om/t/qoldiq") || ui.enter("om/t/sabab")) {
        double v;
        wstring q = tozala(ui.qiymat("om/t/qoldiq"));
        tuzat_xato.clear();
        if (q.empty()) tuzat_xato = L"Bu maydon to'ldirilishi shart.";
        else if (!songa(q, v)) tuzat_xato = L"Son kiriting.";
        else if (v < 0) tuzat_xato = L"Qiymat 0 dan kichik bo'lmasin.";
        else if (std::abs(v * 1000 - std::round(v * 1000)) > 1e-6) tuzat_xato = L"Ko'pi bilan 3 kasr xonasi bo'lsin.";
        wstring sabab = tozala(ui.qiymat("om/t/sabab"));
        if (sabab.size() > 120) tuzat_xato = L"Sabab 120 belgidan oshmasin.";
        if (!tuzat_xato.empty()) return;
        v = yaxlit(v, 3);
        if (!qoldiq_tuzatish(t.id, tuzat_eski, v, sabab, tuzat_xato)) return;
        ombor_keshini_tashla();
        ombor_eskirdi();
        ui.xabar(t.nom + L": qoldiq " + son(v) + L" " + t.birlik + L" bo\u2018ldi.");
        tuzat_id = 0;
        yangila(false);
    }
}

// ---------- Harakatlar tarixi ----------
void tarix_oynasi() {
    Tovar t;
    if (!tovar_ol(tarix_id, t)) { tarix_id = 0; return; }
    bool yop = false;
    R o = oyna("om/h", L"Kirim/chiqim tarixi — " + t.nom, 1040, ui.H * .88f, yop);
    if (yop) { tarix_id = 0; return; }
    R b = o.ichki(14);
    std::vector<Ustun> u = {{L"Sana", 170}, {L"Turi", 120}, {L"Miqdor", -1, true}, {L"Izoh", -2}};
    gfx.quti(b, M->panel, 12, M->chegara);
    gfx.kes(b.ichki(1));
    jadval_sarlavha({b.x, b.y, b.w, 38}, u);
    R tana = {b.x, b.y + 38, b.w, b.h - 38};
    if (tarix.empty()) bosh_holat(tana, "i-tarix", L"Harakat yo'q", L"");
    ui.scroll("om/h/s", tana, tarix.size() * 44.f);
    for (size_t i = 0; i < tarix.size(); ++i) {
        auto& h = tarix[i];
        R row = {tana.x, tana.y + i * 44.f, tana.w - 12, 44};
        auto c = ustunlar(row, u);
        gfx.matn(c[0].ichki(13, 0), h.sana, 15, 400, M->matn);
        Rang nf = h.tur == L"Kirim" ? M->tasdiq_fon : h.tur == L"Chiqim" ? M->xato_fon : M->kok_fon;
        Rang nc = h.tur == L"Kirim" ? M->tasdiq_chek : h.tur == L"Chiqim" ? M->xato_chek : M->kok_chek;
        Rang nr = h.tur == L"Kirim" ? M->tasdiq : h.tur == L"Chiqim" ? M->xato : M->kok;
        ui.nishon(c[1].x + 13, row.y + 12, h.tur, nf, nc, nr);
        gfx.matn(c[2].ichki(13, 0), h.miqdor, 15, 700, M->matn, ONG);
        gfx.matn(c[3].ichki(13, 0), h.izoh, 15, 400, M->matn2);
        gfx.chiziq(row.x, row.p() - .5f, row.o(), row.p() - .5f, M->chegara);
    }
    ui.scroll_tugat();
    gfx.kesishni_ol();
}

// ---------- Shtrix kod ----------
void shtrix_oynasi() {
    Tovar t;
    if (!tovar_ol(shtrix_id, t)) { shtrix_id = 0; return; }
    bool yop = false;
    R o = oyna("om/s", L"Shtrix kod — " + t.nom, 520, 58 + 14 + 22 + 50 + 14 + 50 + 14, yop);
    if (yop) { shtrix_id = 0; return; }
    R b = o.ichki(14);
    gfx.matn({b.x, b.y, b.w, 17}, L"Shtrix kod", 12, 600, M->matn2);
    MaydonSoz m;
    m.ishora = L"Shtrix kod";
    ui.maydon("om/s/kod", {b.x, b.y + 22, b.w, 50}, m);
    float w1 = (b.w - 10) / 3;
    if (ui.tugma("om/s/bekor", {b.x, b.p() - 50, w1, 50}, L"Bekor", TINCH)) { shtrix_id = 0; return; }
    if (ui.tugma("om/s/ok", {b.x + w1 + 10, b.p() - 50, b.w - w1 - 10, 50}, L"Saqlash", YASHIL) || ui.enter("om/s/kod")) {
        wstring x;
        if (shtrix_saqla(t.id, ui.qiymat("om/s/kod"), x, &shtrix_eski)) {
            ui.xabar(L"Shtrix kod saqlandi.");
            shtrix_id = 0;
            yangila(false);
        } else
            ui.xabar(x, X_XATO);
    }
}

}  // namespace

void ombor_ochildi(long long) {
    ombor_keshini_tashla();
    kirim_id = tuzat_id = tarix_id = shtrix_id = 0;
    sahifa = 0;
    yangila();
}

void ombor_chiz(R r) {
    // skaner: tovar kirim oynasi ochiladi (saytdagi ombor.js kabi)
    if (!ui.skaner.empty() && !kirim_id) {
        Tovar t;
        double km;
        wstring x;
        if (kod_bilan_top(ui.skaner, t, km, x) || t.id) {
            kirim_id = t.id;
            kirim_birlik = 0;
            ui.qiymat_qoy("om/k/miqdor", L"");
            ui.qiymat_qoy("om/k/izoh", L"");
            ui.fokus("om/k/miqdor");
        } else
            ui.xabar(x, X_XATO);
    }
    sarlavha(r, L"Ombor", L"skladdagi tovarlar va qoldiqlar");
    float qh = 14 + 50 + 12 + 24 + 12 + 34 + 14 + (tugagan ? 14 + 40 : 0);
    R q = {r.x, r.y, r.w, qh};
    quti(q);
    float x = q.x + 14, w = q.w - 28, y = q.y + 14, g = 14;
    float bir = (w - 3 * g) / 6;
    MaydonSoz s;
    s.ishora = L"Tovar nomi, kodi yoki shtrix kodi";
    ui.maydon("om/qidir", {x, y, bir * 3, 50}, s);
    int val = filtr.valyuta;
    if (ui.tanlov("om/valyuta", {x + bir * 3 + g, y, bir, 50}, {L"Hamma valyuta", L"So'mda", L"Dollarda"}, val)) {
        filtr.valyuta = val;
        sahifa = 0;
        yangila();
    }
    if (ui.tugma("om/qidirb", {x + bir * 4 + 2 * g, y, bir, 50}, L"Qidirish", ODDIY, "i-qidir") || ui.enter("om/qidir")) {
        filtr.matn = ui.qiymat("om/qidir");
        sahifa = 0;
        yangila();
    }
    if (ui.tugma("om/yangi", {x + bir * 5 + 3 * g, y, bir, 50}, L"Yangi tovar", YASHIL, "i-plus")) ot(J_MAHSULOT, 0);
    y += 50 + 12;
    float bx = x;
    auto belgi_ = [&](const string& id, const wchar_t* t, bool& v) {
        float bw = gfx.matn_eni(t, 14, 500) + 34;
        if (ui.belgi(id, {bx, y, bw, 24}, t, v)) {
            sahifa = 0;
            yangila();
        }
        bx += bw + 18;
    };
    belgi_("om/kam", L"Kam qolganlar birinchi", filtr.kam);
    belgi_("om/tug", L"Faqat tugaganlar", filtr.tugagan);
    belgi_("om/yash", L"Yashirilganlar ham", filtr.yashirilgan);
    gfx.matn({bx, y, 200, 24}, guruhla((long long)natija.size()) + L" ta tovar", 15, 400, M->matn3);
    y += 24 + 12;
    bx = x;
    auto kichik_t = [&](const string& id, const wchar_t* t, Uslub u, const char* ik) {
        float bw = gfx.matn_eni(t, 13, 600) + 22 + (ik ? 22 : 0);
        bool b = ui.tugma(id, {bx, y, bw, 34}, t, u, ik, true);
        bx += bw + 8;
        return b;
    };
    if (kichik_t("om/eks", L"Omborni Excel'ga yuklab olish", YASHIL, "i-yuklab-ol")) ombor_eksport(natija, filtr.kam);
    if (kichik_t("om/exy", L"Excel'dan yangilash", ASOSIY, "i-yuklash")) ot(J_EXCEL);
    {
        // «Yangilanishlar» + qoralamalar nishoni (tugma ichida)
        static long long qor = -1;
        static DWORD vaqt = 0;
        if (GetTickCount() - vaqt > 3000) { qor = qoralamalar_soni(); vaqt = GetTickCount(); }
        wstring nt = qor > 0 ? std::to_wstring(qor) + L" qoralama" : L"";
        float nw = nt.empty() ? 0 : gfx.matn_eni(nt, 11, 600) + 22;
        // matn markazda turadi: nishon uchun ikki tomonga joy (aks holda ustma-ust tushadi)
        // Ichi: ikonka, matn, nishon — bir qatorda, chetlarida 11 px (saytdagi .tugma.kichik)
        float tw = gfx.matn_eni(L"Yangilanishlar", 13, 600);
        float bw = 11 + 16 + 6 + tw + (nt.empty() ? 0 : 6 + nw) + 11;
        bool b = ui.tugma("om/yan", {bx, y, bw, 34}, L"", TINCH, nullptr, true);
        float ix = bx + 11;
        gfx.ikon("i-tarix", {ix, y + 9, 16, 16}, M->matn);
        ix += 16 + 6;
        gfx.matn({ix, y, tw + 2, 34}, L"Yangilanishlar", 13, 600, M->matn);
        ix += tw + 6;
        if (!nt.empty()) ui.nishon(ix, y + 7.5f, nt, M->ogoh_fon, M->ogoh_chek, M->ogoh);
        bx += bw + 8;
        if (b) ot(J_YANGILANISHLAR);
    }
    if (kichik_t("om/sha", L"Bo'sh shablon", TINCH, nullptr)) ombor_shablon();
    if (kichik_t("om/tar", L"Taroziga yuborish", TINCH, nullptr)) {
        ot(J_QURILMALAR);  // Rongta tarozi paneli Qurilmalar sahifasida
        ui.scroll_korsat("qr/s", 500, 700);
        return;
    }
    y += 34 + 14;
    if (tugagan) {
        R o = {x, y, w, 40};
        gfx.quti(o, M->ogoh_fon, 9, M->ogoh_chek);
        gfx.ikon("i-ogoh", {o.x + 12, o.y + 12, 16, 16}, M->kechki ? hex("#ffd382") : M->ogoh);
        gfx.matn({o.x + 37, o.y, o.w - 49, o.h}, guruhla(tugagan) + L" ta tovar tugagan — kirim qilish kerak.", 13, 500,
                 M->kechki ? hex("#ffd382") : M->ogoh);
    }
    r.y = q.p() + 14;
    r.h = ui.H - 14 - r.y;
    int sahifalar = std::max(1, ((int)natija.size() + SAHIFADA - 1) / SAHIFADA);
    float nav = sahifalar > 1 ? 14 + 34 : 0;
    // jadval
    std::vector<Ustun> u = {{L"Kod", 74, true}, {L"Tovar", -3}, {L"Shtrix kod", 130}, {L"Birlik", 150},
                            {L"Kelish narxi", 120, true}, {L"Sotish narxi", 130, true}, {L"Qoldiq", 130, true},
                            {L"", 230}};
    R j = {r.x, r.y, r.w, r.h - nav};
    gfx.quti(j, M->panel, 12, M->chegara);
    gfx.kes(j.ichki(1));
    jadval_sarlavha({j.x, j.y, j.w, 38}, u);
    R tana = {j.x, j.y + 38, j.w, j.h - 38};
    int bosh = sahifa * SAHIFADA, oxir = std::min((int)natija.size(), bosh + SAHIFADA);
    float qb = 76;
    if (natija.empty()) {
        bosh_holat(tana, "i-quti", L"Sklad bo'sh", L"");
        if (ui.tugma("om/bosh/yangi", {tana.x + (tana.w - 180) / 2, tana.y + 130, 180, 50}, L"Tovar qo'shish", YASHIL))
            ot(J_MAHSULOT, 0);
    }
    ui.scroll("om/jadval", tana, (oxir - bosh) * qb);
    for (int i = bosh; i < oxir; ++i) {
        const Tovar& t = natija[i]->t;
        R row = {tana.x, tana.y + (i - bosh) * qb, tana.w - 10, qb};
        if (row.p() < tana.y - 1000 || row.y > tana.p() + 1000) continue;
        bool hov = ui.ustida(row);
        if (t.qoldiq <= 0) gfx.quti(row, hex("#ffb020", .06f));
        if (hov) gfx.quti(row, M->panel2);
        gfx.chiziq(row.x, row.p() - .5f, row.o(), row.p() - .5f, M->chegara);
        auto c = ustunlar(row, u);
        string sid = std::to_string(t.id);
        // Qator bosilishi ichidagi tugmalardan OLDIN so'raladi: bosilgan joydagi oxirgi element
        // (tugma) faol bo'ladi, shuning uchun tugma bosilganda qator kirim oynasini ochmaydi.
        bool qator_bos = ui.bosildi("om/q/" + sid, row);
        gfx.matn(c[0].ichki(13, 0), t.kod, 15, 700, M->matn, ONG);
        // tovar nomi + nishonlar
        R n = c[1].ichki(13, 0);
        float nw = std::min(gfx.matn_eni(t.nom, 15, 700), n.w - (t.dollarmi() ? 30 : 0) - (!t.faol ? 90 : 0));
        gfx.matn({n.x, n.y, nw + 2, n.h}, t.nom, 15, 700, M->matn);
        float nx = n.x + nw + 6;
        if (t.dollarmi()) nx += ui.nishon(nx, row.y + 28, L"$", M->tasdiq_fon, M->tasdiq_chek, M->tasdiq) + 4;
        if (!t.faol) ui.nishon(nx, row.y + 28, L"yashirilgan", M->panel3, M->chegara2, M->matn2);
        // shtrix
        wstring sh = natija[i]->shtrix.empty() ? L"—" : natija[i]->shtrix;
        float shw = std::min(c[2].w - 26, gfx.matn_eni(sh, 13, 600) + 22);
        if (ui.tugma("om/sh/" + sid, {c[2].x + 13, row.y + 21, shw, 34}, sh, TINCH, nullptr, true)) {
            shtrix_id = t.id;
            shtrix_eski = natija[i]->shtrix;
            ui.qiymat_qoy("om/s/kod", natija[i]->shtrix);
            ui.fokus("om/s/kod");
        }
        // birlik
        R bl = c[3].ichki(13, 0);
        if (ikki(t)) {
            gfx.matn({bl.x, row.y + 12, bl.w, 21}, t.birlik, 15, 400, M->matn3);
            ui.nishon(bl.x, row.y + 38, L"1 " + t.olish_birligi + L" = " + son(t.olish_miqdori) + L" " + t.birlik,
                      M->kok_fon, M->kok_chek, M->kok);
        } else
            gfx.matn(bl, t.birlik, 15, 400, M->matn3);
        gfx.matn(c[4].ichki(13, 0),
                 t.kelish_bor ? son(t.kelish_narxi) + (t.kelish_valyutasi == L"dollar" ? L" $" : t.kelish_valyutasi == L"som" ? L" so'm" : L"")
                              : L"—",
                 15, 400, M->matn3, ONG);
        gfx.matn(c[5].ichki(13, 0), t.narx > 0 ? pul(t.narx) + (t.dollarmi() ? L" $" : L" so'm") : L"—", 15, 400,
                 M->matn3, ONG);
        // qoldiq + Tuzatish
        R qd = c[6].ichki(13, 0);
        if (t.qoldiq > 0) {
            gfx.matn({qd.x, row.y + 8, qd.w, 21}, son(t.qoldiq), 15, 700, M->matn, ONG);
            if (ikki(t)) gfx.matn({qd.x - 40, row.y + 8, qd.w - 50, 21}, L"", 12, 400, M->matn3, ONG);
        } else
            ui.nishon(qd.o(), row.y + 9, L"Tugagan", M->ogoh_fon, M->ogoh_chek, M->ogoh, true);
        float tw = gfx.matn_eni(L"Tuzatish", 13, 600) + 22;
        if (ui.tugma("om/tz/" + sid, {qd.o() - tw, row.y + 34, tw, 34}, L"Tuzatish", TINCH, nullptr, true)) {
            tuzat_id = t.id;
            tuzat_eski = t.qoldiq;
            tuzat_xato.clear();
            ui.qiymat_qoy("om/t/qoldiq", son(t.qoldiq));
            ui.qiymat_qoy("om/t/sabab", L"");
            ui.fokus("om/t/qoldiq");
        }
        // amallar
        float ax = c[7].x + 13, ay = row.y + 21;
        float kw = gfx.matn_eni(L"Kirim", 13, 600) + 22 + 22;
        bool kir = ui.tugma("om/kr/" + sid, {ax, ay, kw, 34}, L"Kirim", YASHIL, "i-kirim", true);
        ax += kw + 6;
        if (ui.tugma("om/pr/" + sid, {ax, ay, 38, 34}, L"", TINCH, "i-printer", true)) {
            etiketka_och(t.id, etiketka_soni(t, t.qoldiq, t.birlik));
            break;
        }
        ax += 44;
        if (ui.tugma("om/td/" + sid, {ax, ay, 38, 34}, L"", TINCH, "i-qalam", true)) {
            ot(J_MAHSULOT, t.id);
            break;
        }
        ax += 44;
        if (ui.tugma("om/hr/" + sid, {ax, ay, 38, 34}, L"", TINCH, "i-tarix", true)) {
            tarix_id = t.id;
            tarix = harakatlar(t.id);
            ui.scroll_tepaga("om/h/s");
        }
        if (kir || qator_bos) {  // qator bosilsa kirim (saytdagi kabi)
            kirim_id = t.id;
            kirim_birlik = ikki(t) ? 1 : 0;
            ui.qiymat_qoy("om/k/miqdor", L"");
            ui.qiymat_qoy("om/k/izoh", L"");
            ui.fokus("om/k/miqdor");
        }
    }
    ui.scroll_tugat();
    gfx.kesishni_ol();
    // sahifalash
    if (sahifalar > 1) {
        float ny = j.p() + 14;
        wstring m = std::to_wstring(sahifa + 1) + L" / " + std::to_wstring(sahifalar);
        float mw = gfx.matn_eni(m, 15, 400) + 20;
        float w1 = 40, w2 = gfx.matn_eni(L"‹ Oldingi", 13, 600) + 22, w3 = gfx.matn_eni(L"Keyingi ›", 13, 600) + 22;
        float jami = (sahifa > 0 ? w1 + w2 + 16 : 0) + mw + (sahifa < sahifalar - 1 ? w3 + w1 + 16 : 0);
        float nx = r.x + (r.w - jami) / 2;
        if (sahifa > 0) {
            if (ui.tugma("om/p1", {nx, ny, w1, 34}, L"«", TINCH, nullptr, true)) { sahifa = 0; ui.scroll_tepaga("om/jadval"); }
            nx += w1 + 8;
            if (ui.tugma("om/p2", {nx, ny, w2, 34}, L"‹ Oldingi", TINCH, nullptr, true)) { --sahifa; ui.scroll_tepaga("om/jadval"); }
            nx += w2 + 8;
        }
        gfx.matn({nx, ny, mw, 34}, m, 15, 400, M->matn3, MARKAZ);
        nx += mw;
        if (sahifa < sahifalar - 1) {
            nx += 8;
            if (ui.tugma("om/p3", {nx, ny, w3, 34}, L"Keyingi ›", TINCH, nullptr, true)) { ++sahifa; ui.scroll_tepaga("om/jadval"); }
            nx += w3 + 8;
            if (ui.tugma("om/p4", {nx, ny, w1, 34}, L"»", TINCH, nullptr, true)) { sahifa = sahifalar - 1; ui.scroll_tepaga("om/jadval"); }
        }
    }
    if (kirim_id) kirim_oynasi();
    if (tuzat_id) tuzat_oynasi();
    if (tarix_id) tarix_oynasi();
    if (shtrix_id) shtrix_oynasi();
}

// ======================= Tovar formasi =======================

namespace {
long long mf_id = 0;
int mf_birlik = 0, mf_valyuta = 0, mf_kval = 0, mf_olish = 0;
bool mf_ikki = false, mf_faol = true;
wstring mf_xato[16];
std::vector<wstring> mf_birliklar;
double mf_eski_qoldiq = 0;

int ind(const std::vector<wstring>& v, const wstring& s) {
    for (size_t i = 0; i < v.size(); ++i)
        if (v[i] == s) return (int)i;
    return 0;
}
}  // namespace

void mahsulot_ochildi(long long p) {
    mf_id = p;
    ui.maydonlarni_tozala("mf/");
    for (auto& x : mf_xato) x.clear();
    mf_birliklar = BIRLIKLAR;
    Tovar t;
    wstring shtrix;
    if (p && tovar_ol(p, t)) {
        Sorov s = baza.sor("SELECT COALESCE(shtrix_kod,'') FROM ombor_mahsulot WHERE id=?");
        s.bog(1, p);
        if (s.qator()) shtrix = s.matn(0);
    }
    if (ind(mf_birliklar, t.birlik) == 0 && t.birlik != mf_birliklar[0]) mf_birliklar.push_back(t.birlik);
    ui.qiymat_qoy("mf/nom", t.nom);
    ui.qiymat_qoy("mf/kod", p ? t.kod : L"");
    ui.qiymat_qoy("mf/shtrix", shtrix);
    ui.qiymat_qoy("mf/kelish", t.kelish_bor ? son(t.kelish_narxi) : L"");
    ui.qiymat_qoy("mf/narx", t.narx > 0 ? pul(t.narx) : L"");
    ui.qiymat_qoy("mf/qoldiq", son(t.qoldiq));
    ui.qiymat_qoy("mf/olm", son(t.olish_miqdori));
    ui.qiymat_qoy("mf/qadoq", L"");
    mf_eski_qoldiq = t.qoldiq;
    mf_birlik = ind(mf_birliklar, t.birlik);
    mf_valyuta = t.dollarmi() ? 1 : 0;
    mf_kval = !t.kelish_bor || t.kelish_valyutasi.empty() ? 0 : t.kelish_valyutasi == L"dollar" ? 2 : 1;
    mf_ikki = !t.olish_birligi.empty() && t.olish_birligi != t.birlik;
    mf_olish = mf_ikki ? ind(mf_birliklar, t.olish_birligi) : 0;
    mf_faol = p ? t.faol : true;
    ui.scroll_tepaga("mf/s");
    ui.fokus("mf/nom");
}

void mahsulot_chiz(R r) {
    wstring bir = mf_birliklar[mf_birlik];
    wstring olish = mf_birliklar[mf_olish];
    float kontent = 80 + 3 * 100 + 4 * 110 + (mf_ikki ? 3 * 110 : 0) + 120 + 100;
    float en = std::min(1100.f, r.w);
    R s0 = {r.x + (r.w - en) / 2, r.y, en, r.h};
    ui.scroll("mf/s", {r.x, r.y, r.w, r.h}, kontent);
    R s = {s0.x, s0.y, s0.w - 12, s0.h};
    float y = s.y;
    gfx.matn({s.x, y, s.w, 32}, mf_id ? L"Mahsulotni tahrirlash" : L"Yangi mahsulot qo'shish", 23, 650, M->matn);
    gfx.matn({s.x, y + 32, s.w, 21}, L"Tovar ma'lumotlarini bir joyda boshqaring — kod, narx va qoldiq.", 15, 400, M->matn2);
    if (mf_id) {
        Tovar t;
        if (tovar_ol(mf_id, t)) ui.nishon(s.o(), y + 8, L"Kod: " + t.kod, M->kok_fon, M->kok_chek, M->kok, true);
    }
    y += 66;
    R f = {s.x, y, s.w, kontent - 66 - 20};
    quti(f);
    float x = f.x + 20, w = f.w - 40, g = 14, ust = (w - g) / 2;
    y = f.y + 20;
    int xi = 0;
    auto bolim = [&](const wchar_t* b, const wchar_t* iz) {
        gfx.matn({x, y, w, 26}, b, 18, 650, M->matn);
        gfx.matn({x, y + 26, w, 19}, iz, 13, 400, M->matn2);
        y += 58;
    };
    auto maydon_ = [&](const string& id, float mx, float mw, const wstring& yorliq, const wstring& izoh, bool ong,
                       const wchar_t* ish, int xid) {
        gfx.matn({mx, y, mw, 17}, yorliq, 12, 600, M->matn2);
        MaydonSoz m;
        m.ong = ong;
        m.ishora = ish;
        ui.maydon(id, {mx, y + 22, mw, 50}, m);
        if (!izoh.empty()) gfx.matn({mx, y + 76, mw, 16}, izoh, 12, 400, M->matn3);
        if (!mf_xato[xid].empty())
            gfx.matn({mx, y + (izoh.empty() ? 76 : 92), mw, 16}, mf_xato[xid], 12, 500,
                     M->kechki ? hex("#ffa0a0") : M->xato);
    };
    auto tanlov_ = [&](const string& id, float mx, float mw, const wstring& yorliq, const wstring& izoh,
                       const std::vector<wstring>& v, int& i, int xid = -1) {
        gfx.matn({mx, y, mw, 17}, yorliq, 12, 600, M->matn2);
        bool o = ui.tanlov(id, {mx, y + 22, mw, 50}, v, i);
        if (!izoh.empty()) gfx.matn({mx, y + 76, mw, 16}, izoh, 12, 400, M->matn3);
        if (xid >= 0 && !mf_xato[xid].empty())
            gfx.matn({mx, y + (izoh.empty() ? 76 : 92), mw, 16}, mf_xato[xid], 12, 500,
                     M->kechki ? hex("#ffa0a0") : M->xato);
        return o;
    };
    (void)xi;
    bolim(L"Asosiy ma'lumotlar", L"Mahsulot nomi va kodlari");
    maydon_("mf/nom", x, w, L"Mahsulot nomi *", L"", false, nullptr, 0);
    y += 22 + 50 + 30;
    maydon_("mf/kod", x, ust, L"Ichki kod (turgan joyi)",
            mf_id ? L"0001–9999. Bo'sh qoldirsangiz avtomatik beriladi; o'zgarsa etiketkani qayta chop eting."
                  : L"0001–9999. Bo'sh qoldirsangiz avtomatik beriladi.",
            false, nullptr, 1);
    maydon_("mf/shtrix", x + ust + g, ust, L"Zavod shtrix kodi (ixtiyoriy)", L"Skanerlang yoki kiriting; 8, 12, 13 yoki 14 raqam.",
            false, nullptr, 2);
    y += 22 + 50 + 44;
    gfx.chiziq(x, y, x + w, y, M->chegara);
    y += 20;
    bolim(L"Narx va valyuta", L"Sotuv va xarid narxini alohida belgilang");
    std::vector<wstring> pullar = {L"so'm", L"dollar"};
    tanlov_("mf/kval", x, ust, L"Xarid narxi valyutasi", L"Tanlanmasa sotuv valyutasi olinadi.",
            {L"---------", L"so'm", L"dollar"}, mf_kval);
    maydon_("mf/kelish", x + ust + g, ust, L"Xarid narxi — 1 " + bir + L" (ixtiyoriy)",
            L"Do'kon sotib olgan tannarx; sotuv narxiga qo'shilmaydi.", true, L"0", 3);
    y += 22 + 50 + 44;
    tanlov_("mf/val", x, ust, L"Sotuv narxi valyutasi", L"So'm va dollar alohida hisoblanadi.", pullar, mf_valyuta);
    maydon_("mf/narx", x + ust + g, ust, L"Sotuv narxi — 1 " + bir,
            pullar[mf_valyuta] + L"da. Kassada yakuniy summa qo'lda yoziladi.", true, L"0", 4);
    pul_guruhla("mf/narx");
    y += 22 + 50 + 44;
    gfx.chiziq(x, y, x + w, y, M->chegara);
    y += 20;
    bolim(L"Ombor va o'lchov birligi", L"Qoldiq har doim sotuv birligida saqlanadi");
    tanlov_("mf/bir", x, ust, L"Sotuv birligi", L"Mahsulot mijozga qaysi birlikda sotiladi?", mf_birliklar, mf_birlik);
    maydon_("mf/qoldiq", x + ust + g, ust, L"Qoldiq (" + bir + L")",
            mf_id ? L"Mavjud qoldiqni o'zgartirish tarixga yoziladi." : L"Boshlang'ich qoldiq.", true, L"0", 5);
    y += 22 + 50 + 44;
    // richag
    R rg = {x, y, w, 66};
    bool hov = ui.ustida(rg);
    gfx.quti(rg, mf_ikki ? M->urgu_fon : M->panel2, 9, mf_ikki ? M->urgu_chek : hov ? M->chegara2 : M->chegara);
    ui.richag_chiz({rg.x + 14, rg.y + 21, 44, 24}, mf_ikki, M->urgu);
    gfx.matn({rg.x + 70, rg.y + 12, rg.w - 84, 21}, L"Mahsulot boshqa birlikda keladi", 15, 650,
             mf_ikki ? M->urgu_matn : M->matn);
    gfx.matn({rg.x + 70, rg.y + 34, rg.w - 84, 18},
             L"Masalan, 1 rulon olinadi, lekin metrda sotiladi. Yoqilganda quyida konvertatsiya ochiladi.", 12, 400, M->matn3);
    if (ui.bosildi("mf/richag", rg)) mf_ikki = !mf_ikki;
    y += 66 + 14;
    if (mf_ikki) {
        tanlov_("mf/olish", x, ust, L"Olish birligi", L"Do'konga qaysi birlikda keladi?", mf_birliklar, mf_olish, 8);
        maydon_("mf/olm", x + ust + g, ust, L"1 " + olish + L"da nechta " + bir + L"?", L"Masalan, 1 rulon = 100 metr.", true,
                L"0", 6);
        y += 22 + 50 + 44;
        if (!mf_id) {
            maydon_("mf/qadoq", x, w, L"Necha " + olish + L" keldi?",
                    L"Masalan, 3 rulon kirsa va har biri 100 metr bo'lsa, 300 metr saqlanadi.", true, L"0", 7);
            y += 22 + 50 + 44;
        }
        double olm = 0;
        songa(ui.qiymat("mf/olm"), olm);
        if (olm > 0 && olish != bir) {
            R qs = {x, y, w, 42};
            gfx.quti(qs, M->kok_fon, 9, M->kok_chek);
            gfx.matn(qs.ichki(12, 0), L"1 " + olish + L" = " + son(olm) + L" " + bir, 15, 400, M->kok);
            y += 56;
        }
    }
    // pastki qator
    gfx.chiziq(x, y, x + w, y, M->chegara);
    y += 14;
    ui.belgi("mf/faol", {x, y + 13, 220, 24}, L"Ro'yxatda ko'rinsin", mf_faol);
    float tb = (w - 240) / 2.7f;
    if (ui.tugma("mf/bekor", {x + w - tb * 2.7f, y, tb, 50}, L"Bekor qilish", TINCH)) {
        ot(J_OMBOR);
        ui.scroll_tugat();
        return;
    }
    bool saqla = ui.tugma("mf/saqla", {x + w - tb * 1.7f + 10, y, tb * 1.7f - 10, 50}, L"Saqlash", YASHIL);
    ui.scroll_tugat();
    if (!saqla) return;
    // ---- saqlash ----
    for (auto& e : mf_xato) e.clear();
    Tovar t;
    if (mf_id) tovar_ol(mf_id, t);
    t.id = mf_id;
    t.nom = tozala(ui.qiymat("mf/nom"));
    t.kod = tozala(ui.qiymat("mf/kod"));
    t.birlik = bir;
    t.valyuta = mf_valyuta ? L"dollar" : L"som";
    double narx = 0, kelish = 0, qoldiq = 0, olm = 1, qadoq = 0;
    if (t.nom.empty()) mf_xato[0] = L"Bu maydon to'ldirilishi shart.";
    if (!songa(ui.qiymat("mf/narx"), narx) || narx < 0) mf_xato[4] = L"Son kiriting.";
    wstring kel = tozala(ui.qiymat("mf/kelish"));
    t.kelish_bor = !kel.empty();
    if (t.kelish_bor && !songa(kel, kelish)) mf_xato[3] = L"Son kiriting.";
    else if (t.kelish_bor && kelish < 0) mf_xato[3] = L"Xarid narxi manfiy bo'lishi mumkin emas.";
    if (!songa(ui.qiymat("mf/qoldiq"), qoldiq) || qoldiq < 0) mf_xato[5] = L"Son kiriting.";
    if (mf_ikki && olish == bir)
        mf_xato[8] = L"Kelgan va sotiladigan birlik bir xil. Birlik o'zgarmasa richagni o'chiring.";
    if (mf_ikki && (!songa(ui.qiymat("mf/olm"), olm) || olm <= 0))
        mf_xato[6] = L"1 " + olish + L" da nechta " + bir + L" borligini yozing (noldan katta son).";
    if (mf_ikki && !mf_id && !songa(ui.qiymat("mf/qadoq"), qadoq)) mf_xato[7] = L"Son kiriting.";
    wstring sh = tozala(ui.qiymat("mf/shtrix"));
    mf_xato[2] = shtrix_xatosi(sh);
    for (auto& e : mf_xato) if (!e.empty()) return;
    t.narx = narx;
    t.kelish_narxi = kelish;
    t.kelish_valyutasi = mf_kval == 2 ? L"dollar" : mf_kval == 1 ? L"som" : t.valyuta;
    t.olish_birligi = mf_ikki ? olish : L"";
    t.olish_miqdori = mf_ikki ? olm : 1;
    if (mf_ikki && !mf_id && qadoq > 0) qoldiq = yaxlit(qadoq * olm, 3);
    wstring xato;
    long long id = tovar_saqla(t, mf_id ? 0 : qoldiq, xato, mf_ikki && !mf_id ? qadoq : 0);
    if (!id) {
        if (xato.find(L"nom") != wstring::npos) mf_xato[0] = xato;
        else if (xato.find(L"od") != wstring::npos) mf_xato[1] = xato;
        else ui.xabar(xato, X_XATO);
        return;
    }
    if (!shtrix_saqla(id, sh, xato)) {
        mf_xato[2] = xato;
        return;
    }
    tovar_faol(id, mf_faol);
    if (mf_id && std::abs(qoldiq - mf_eski_qoldiq) > 1e-9) qoldiq_tuzat(id, yaxlit(qoldiq, 3), L"Qo'lda tuzatildi");
    ombor_keshini_tashla();
    ombor_eskirdi();
    if (mf_id) ui.xabar(L"Saqlandi.");
    else {
        Tovar yt;
        tovar_ol(id, yt);
        wstring xb = yt.nom + L" skladga qo'shildi";
        if (yt.qoldiq) xb += L" — " + son(yt.qoldiq) + L" " + yt.birlik + (ikki(yt) ? L" = " + qadoq_matni(yt) : L"");
        ui.xabar(xb + L".");
    }
    if (mf_id) {
        ot(J_OMBOR);
        return;
    }
    // Kod tayyor: endi etiketkasini chop etib tovarlarga yopishtirish qoladi
    // (nusxalar — kelgan qadoqlar yoki donalar soni, saytdagi kabi)
    etiketka_och(id, mf_ikki && qadoq > 0 ? etiketka_soni(t, qadoq, t.olish_birligi) : etiketka_soni(t, qoldiq, t.birlik));
}
