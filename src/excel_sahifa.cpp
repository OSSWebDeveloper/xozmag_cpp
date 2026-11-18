// templates/ombor/excel_yuklash.html, yangilanishlar.html, yangilanish.html, qator_tahrir.html
#include "db3.h"
#include "excel.h"
#include "ilova.h"

#include <commdlg.h>

// ======================= Fayl dialoglari =======================

static bool ochish_dialogi(wstring& yol) {
    wchar_t f[MAX_PATH] = {};
    OPENFILENAMEW o = {sizeof o};
    o.hwndOwner = ui.h;
    o.lpstrFilter = L"Excel fayl (*.xlsx)\0*.xlsx\0Hamma fayllar\0*.*\0";
    o.lpstrFile = f;
    o.nMaxFile = MAX_PATH;
    o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&o)) return false;
    yol = f;
    return true;
}

static bool saqlash_dialogi(wstring& yol, const wstring& nom) {
    wchar_t f[MAX_PATH] = {};
    wcsncpy_s(f, nom.c_str(), _TRUNCATE);
    OPENFILENAMEW o = {sizeof o};
    o.hwndOwner = ui.h;
    o.lpstrFilter = L"Excel fayl (*.xlsx)\0*.xlsx\0";
    o.lpstrFile = f;
    o.nMaxFile = MAX_PATH;
    o.lpstrDefExt = L"xlsx";
    o.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&o)) return false;
    yol = f;
    return true;
}

// Ombor ro'yxatidan: filtr bo'yicha yuklab olish (excel_views.eksport) va bo'sh shablon
void ombor_eksport(const std::vector<const OmborTovar*>& tl, bool kam) {
    SYSTEMTIME t;
    GetLocalTime(&t);
    wstring yol;
    if (!saqlash_dialogi(yol, L"Ombor " + sana_yoz(t) + (kam ? L" (kam qolganlar birinchi)" : L"") + L".xlsx")) return;
    std::vector<long long> id;
    for (auto* o : tl) id.push_back(o->t.id);
    wstring x;
    if (excel_eksport(yol, id, x)) ui.xabar(L"Saqlandi: " + yol);
    else ui.xabar(x, X_XATO);
}

void ombor_shablon() {
    wstring yol, x;
    if (!saqlash_dialogi(yol, L"Ombor shabloni.xlsx")) return;
    if (excel_eksport(yol, {}, x)) ui.xabar(L"Saqlandi: " + yol);
    else ui.xabar(x, X_XATO);
}

static void hozirgi_ombor() {
    OmborFiltr f;
    ombor_eksport(ombor_filtrla(f), false);
}

// .xabar ichki bloki (warning/error/info)
static float ichki_xabar(R r, const wstring& matn, XabarTur tur, bool olchash = false) {
    float boy = std::max(40.f, gfx.matn_boyi(matn, 13, 500, r.w - 49) + 18);
    if (olchash) return boy;
    Rang fon = M->ogoh_fon, ch = M->ogoh_chek, rg = M->kechki ? hex("#ffd382") : M->ogoh;
    if (tur == X_XATO) { fon = M->xato_fon; ch = M->xato_chek; rg = M->kechki ? hex("#ffc2c2") : hex("#a01b12"); }
    if (tur == X_INFO) { fon = M->kok_fon; ch = M->kok_chek; rg = M->kechki ? hex("#a9dcf5") : hex("#0a5680"); }
    gfx.quti({r.x, r.y, r.w, boy}, fon, 9, ch);
    gfx.ikon(tur == X_INFO ? "i-quti" : "i-ogoh", {r.x + 12, r.y + 11, 16, 16}, rg);
    gfx.matn({r.x + 37, r.y + 9, r.w - 49, boy - 18}, matn, 13, 500, rg, CHAP, 0, true);
    return boy;
}

static float kichik_tugma_eni(const wstring& t, bool ikon) { return gfx.matn_eni(t, 13, 600) + 22 + (ikon ? 22 : 0); }

// ======================= Excel yuklash =======================

namespace {
wstring ey_fayl;
bool ey_yashir = false;
std::vector<wstring> ey_xato;
}  // namespace

void excel_yuklash_ochildi(long long) {
    ey_fayl.clear();
    ey_yashir = false;
    ey_xato.clear();
    ui.maydonlarni_tozala("ey/");
}

void excel_yuklash_chiz(R r) {
    float en = std::min(980.f, r.w);
    R s = {r.x + (r.w - en) / 2, r.y, en, r.h};
    sarlavha(s, L"Omborni Excel'dan yangilash", L"fayl yuklanganda ombor hali o'zgarmaydi");
    float w = (s.w - 14) / 2;
    // chap: forma
    float xb = 0;
    for (auto& x : ey_xato) xb += gfx.matn_boyi(x, 12, 500, w - 28) + 4;
    R f = {s.x, s.y, w, 14 + 22 + 50 + xb + 14 + 22 + 50 + 14 + 82 + 14 + 50 + 14};
    quti(f);
    float x = f.x + 14, fw = f.w - 28, y = f.y + 14;
    gfx.matn({x, y, fw, 17}, L"Excel fayl (.xlsx)", 12, 600, M->matn2);
    y += 22;
    R fm = {x, y, fw, 50};
    gfx.quti(fm, M->panel2, 9, M->chegara2);
    float tw = kichik_tugma_eni(L"Faylni tanlash", false);
    if (ui.tugma("ey/tanla", {fm.x + 8, fm.y + 8, tw, 34}, L"Faylni tanlash", ODDIY, nullptr, true) ||
        ui.bosildi("ey/maydon", fm)) {
        wstring yol;
        if (ochish_dialogi(yol)) {
            ey_fayl = yol;
            ey_xato.clear();
        }
    }
    gfx.matn({fm.x + tw + 18, fm.y, fm.w - tw - 30, fm.h},
             ey_fayl.empty() ? L"Fayl tanlanmagan" : ey_fayl.substr(ey_fayl.find_last_of(L"\\/") + 1), 15, 400,
             ey_fayl.empty() ? M->matn3 : M->matn);
    y += 50;
    for (auto& e : ey_xato) {
        float h = gfx.matn_boyi(e, 12, 500, fw) + 4;
        gfx.matn({x, y + 4, fw, h}, e, 12, 500, M->kechki ? hex("#ffa0a0") : M->xato, CHAP, 0, true);
        y += h;
    }
    y += 14;
    gfx.matn({x, y, fw, 17}, L"Izoh", 12, 600, M->matn2);
    y += 22;
    MaydonSoz iz;
    iz.ishora = L"Masalan: oktabr sanog'i";
    ui.maydon("ey/izoh", {x, y, fw, 50}, iz);
    y += 50 + 14;
    R rg = {x, y, fw, 82};
    bool hov = ui.ustida(rg);
    gfx.quti(rg, ey_yashir ? M->urgu_fon : M->panel2, 9, ey_yashir ? M->urgu_chek : hov ? M->chegara2 : M->chegara);
    ui.richag_chiz({rg.x + 14, rg.y + 14, 44, 24}, ey_yashir, M->urgu);
    gfx.matn({rg.x + 70, rg.y + 10, rg.w - 84, 21}, L"Faylda yo'q tovarlarni yashirish", 15, 650,
             ey_yashir ? M->urgu_matn : M->matn);
    gfx.matn({rg.x + 70, rg.y + 32, rg.w - 84, 44},
             L"Belgilanmasa, faylda bo'lmagan tovarlar o'zgarmaydi. Yashirilgan tovar o'chmaydi — sotuv tarixi saqlanadi, kerak bo'lsa qaytariladi.",
             12, 400, M->matn3, CHAP, 0, true);
    if (ui.bosildi("ey/richag", rg)) ey_yashir = !ey_yashir;
    y += 82 + 14;
    if (ui.tugma("ey/tekshir", {x, y, fw, 50}, L"Faylni tekshirish", ASOSIY, "i-yuklash")) {
        ey_xato.clear();
        if (ey_fayl.empty()) ey_xato.push_back(L"Bu maydon to'ldirilishi shart.");
        else {
            HCURSOR k = SetCursor(LoadCursor(nullptr, IDC_WAIT));
            long long id = fayldan_qoralama(ey_fayl, ey_yashir, ui.qiymat("ey/izoh"), ey_xato);
            SetCursor(k);
            if (id) {
                ui.xabar(L"Fayl o'qildi. Ombor hali o'zgarmadi — o'zgarishlarni ko'rib chiqib «Qo'llash»ni bosing.");
                ot(J_YANGILANISH, id);
                return;
            }
        }
    }
    // o'ng: yo'riqnoma
    R o = {s.x + w + 14, s.y, w, 0};
    float ox = o.x + 14, ow = o.w - 28, oy = o.y + 14;
    const wchar_t* bandlar[5] = {
        L"1. Kod — 4 xonali (0001 … 9999), tovar turgan joyi. Bo'sh bo'lsa navbatdagi bo'sh kod beriladi.",
        L"2. Kod bo'lsa tovar kod bo'yicha, bo'lmasa nomi bo'yicha topiladi.",
        L"3. Faylda yo'q ustun o'zgarmaydi — masalan faqat Kod + Qoldiq faqat qoldiqni yangilaydi.",
        L"4. Qoldiq o'zgarsa tovar tarixiga «Tuzatish» bo'lib yoziladi.",
        L"5. Do'kon dasturining «Products …xlsx» faylini ham to'g'ridan-to'g'ri yuklash mumkin."};
    wstring p = L"Birinchi qatorda sarlavhalar: Kod, Tovar nomi, Birlik, Qoldiq, Narxi, Valyuta. Tartibi muhim emas.";
    float boy = 14 + 26 + gfx.matn_boyi(p, 15, 400, ow) + 8;
    for (auto* b : bandlar) boy += gfx.matn_boyi(b, 15, 400, ow - 16) + 6;
    boy += 14 + 34 + 14;
    o.h = boy;
    quti(o);
    gfx.ikon("i-ogoh", {ox, oy + 4, 18, 18}, M->matn2);
    gfx.matn({ox + 26, oy, ow - 26, 26}, L"Fayl qanday bo'lishi kerak", 15, 650, M->matn);
    oy += 26 + 6;
    float ph = gfx.matn_boyi(p, 15, 400, ow);
    gfx.matn({ox, oy, ow, ph}, p, 15, 400, M->matn, CHAP, 0, true);
    oy += ph + 8;
    for (auto* b : bandlar) {
        float h = gfx.matn_boyi(b, 15, 400, ow - 16);
        gfx.matn({ox + 16, oy, ow - 16, h}, b, 15, 400, M->matn2, CHAP, 0, true);
        oy += h + 6;
    }
    oy += 8;
    float b1 = kichik_tugma_eni(L"Bo'sh shablon", true), b2 = kichik_tugma_eni(L"Hozirgi ombor", true);
    if (ui.tugma("ey/shablon", {ox, oy, b1, 34}, L"Bo'sh shablon", TINCH, "i-yuklab-ol", true)) ombor_shablon();
    if (ui.tugma("ey/hozirgi", {ox + b1 + 8, oy, b2, 34}, L"Hozirgi ombor", TINCH, "i-yuklab-ol", true)) hozirgi_ombor();
    float qy = std::max(f.p(), o.p()) + 14;
    float bw = gfx.matn_eni(L"Omborga qaytish", 15, 600) + 36 + 27;
    if (ui.tugma("ey/ortga", {s.x, qy, bw, 50}, L"Omborga qaytish", TINCH, "i-chap")) ot(J_OMBOR);
}

// ======================= Yangilanishlar ro'yxati =======================

namespace {
std::vector<Yangilanish> yl;
wstring yl_holat;
int yl_sahifa = 0;
}  // namespace

void yangilanishlar_ochildi(long long) {
    yl_holat.clear();
    yl_sahifa = 0;
    yl = yangilanishlar_royxati(yl_holat);
}

static void holat_nishoni(float x, float y, const wstring& h, bool qoralama_matn = false) {
    if (h == L"qoralama") ui.nishon(x, y, qoralama_matn ? L"hali qo'llanmagan" : L"Qoralama", M->ogoh_fon, M->ogoh_chek, M->ogoh);
    else if (h == L"qollandi") ui.nishon(x, y, qoralama_matn ? L"qo'llangan" : L"Qo'llangan", M->tasdiq_fon, M->tasdiq_chek, M->tasdiq);
    else ui.nishon(x, y, qoralama_matn ? L"bekor qilingan" : L"Bekor qilingan", M->panel3, M->chegara2, M->matn2);
}

static void sahifalash(const string& id, R r, int& sahifa, int jami) {
    if (jami <= 1) return;
    wstring m = std::to_wstring(sahifa + 1) + L" / " + std::to_wstring(jami);
    float mw = gfx.matn_eni(m, 15, 400) + 20, w2 = kichik_tugma_eni(L"‹ Oldingi", false), w3 = kichik_tugma_eni(L"Keyingi ›", false);
    float x = r.x + (r.w - ((sahifa > 0 ? w2 + 8 : 0) + mw + (sahifa < jami - 1 ? w3 + 8 : 0))) / 2;
    if (sahifa > 0) {
        if (ui.tugma(id + "/old", {x, r.y, w2, 34}, L"‹ Oldingi", TINCH, nullptr, true)) --sahifa;
        x += w2 + 8;
    }
    gfx.matn({x, r.y, mw, 34}, m, 15, 400, M->matn3, MARKAZ);
    x += mw + 8;
    if (sahifa < jami - 1 && ui.tugma(id + "/key", {x, r.y, w3, 34}, L"Keyingi ›", TINCH, nullptr, true)) ++sahifa;
}

void yangilanishlar_chiz(R r) {
    sarlavha(r, L"Ombor yangilanishlari", L"Excel orqali kiritilgan o'zgarishlar");
    R q = {r.x, r.y, r.w, 62};
    quti(q);
    float x = q.x + 14;
    const std::pair<const wchar_t*, const wchar_t*> tablar[4] = {
        {L"", L"Hammasi"}, {L"qoralama", L"Qoralama"}, {L"qollandi", L"Qo'llangan"}, {L"bekor", L"Bekor qilingan"}};
    for (auto& [k, n] : tablar) {
        float w = kichik_tugma_eni(n, false);
        if (ui.tugma(string("yl/t/") + U(n), {x, q.y + 14, w, 34}, n, yl_holat == k ? ASOSIY : TINCH, nullptr, true)) {
            yl_holat = k;
            yl_sahifa = 0;
            yl = yangilanishlar_royxati(yl_holat);
        }
        x += w + 8;
    }
    float ew = kichik_tugma_eni(L"Excel yuklash", true);
    if (ui.tugma("yl/yukla", {q.o() - 14 - ew, q.y + 14, ew, 34}, L"Excel yuklash", YASHIL, "i-yuklash", true)) ot(J_EXCEL);
    r.y += 62 + 14;
    int sahifalar = std::max(1, ((int)yl.size() + 29) / 30);
    if (yl_sahifa >= sahifalar) yl_sahifa = sahifalar - 1;
    float pastki = 50 + 14 + (sahifalar > 1 ? 34 + 14 : 0);
    std::vector<Ustun> u = {{L"#", 60}, {L"Fayl", -2}, {L"Holat", 150}, {L"O'zgarishlar", -2}, {L"Vaqt", 170}, {L"", 100}};
    float qb = 62;
    int bosh = yl_sahifa * 30, oxir = std::min((int)yl.size(), bosh + 30);
    R j = {r.x, r.y, r.w, std::min(ui.H - 14 - pastki - r.y, 38 + std::max(1, oxir - bosh) * qb + (yl.empty() ? 140 : 0))};
    gfx.quti(j, M->panel, 12, M->chegara);
    gfx.kes(j.ichki(1));
    jadval_sarlavha({j.x, j.y, j.w, 38}, u);
    R tana = {j.x, j.y + 38, j.w, j.h - 38};
    if (yl.empty()) bosh_holat(tana, "i-tarix", L"Hali yangilanish yo'q", L"");
    ui.scroll("yl/s", tana, (oxir - bosh) * qb);
    for (int i = bosh; i < oxir; ++i) {
        auto& y = yl[i];
        R row = {tana.x, tana.y + (i - bosh) * qb, tana.w - 10, qb};
        gfx.chiziq(row.x, row.p() - .5f, row.o(), row.p() - .5f, M->chegara);
        auto c = ustunlar(row, u);
        gfx.matn(c[0].ichki(13, 0), std::to_wstring(y.id), 15, 400, M->matn3);
        if (y.izoh.empty()) gfx.matn(c[1].ichki(13, 0), y.fayl_nomi.empty() ? L"—" : y.fayl_nomi, 15, 700, M->matn);
        else {
            gfx.matn({c[1].x + 13, row.y + 9, c[1].w - 26, 22}, y.fayl_nomi.empty() ? L"—" : y.fayl_nomi, 15, 700, M->matn);
            gfx.matn({c[1].x + 13, row.y + 31, c[1].w - 26, 21}, y.izoh, 15, 400, M->matn3);
        }
        holat_nishoni(c[2].x + 13, row.y + 21, y.holat);
        float nx = c[3].x + 13;
        auto nish = [&](long long n, const wstring& t, Rang f, Rang ch, Rang rg) {
            if (n) nx += ui.nishon(nx, row.y + 21, t, f, ch, rg) + 6;
        };
        nish(y.n_yangi, L"+" + guruhla(y.n_yangi) + L" yangi", M->tasdiq_fon, M->tasdiq_chek, M->tasdiq);
        nish(y.n_ozgardi, guruhla(y.n_ozgardi) + L" o'zgardi", M->kok_fon, M->kok_chek, M->kok);
        nish(y.n_yashirildi, guruhla(y.n_yashirildi) + L" yashirildi", M->ogoh_fon, M->ogoh_chek, M->ogoh);
        nish(y.n_xato, guruhla(y.n_xato) + L" xato", M->xato_fon, M->xato_chek, M->xato);
        if (!y.n_yangi && !y.n_ozgardi && !y.n_yashirildi && !y.n_xato)
            gfx.matn(c[3].ichki(13, 0), L"o'zgarish yo'q", 15, 400, M->matn3);
        if (y.bekor_qilingan.empty()) gfx.matn(c[4].ichki(13, 0), mahalliy_vaqt(y.yaratilgan), 15, 400, M->matn3);
        else {
            gfx.matn({c[4].x + 13, row.y + 9, c[4].w - 13, 22}, mahalliy_vaqt(y.yaratilgan), 15, 400, M->matn3);
            gfx.matn({c[4].x + 13, row.y + 31, c[4].w - 13, 21}, L"bekor: " + mahalliy_vaqt(y.bekor_qilingan), 15, 400, M->matn3);
        }
        if (ui.tugma("yl/och/" + std::to_string(y.id), {c[5].x + 13, row.y + 14, kichik_tugma_eni(L"Ochish", false), 34},
                     L"Ochish", TINCH, nullptr, true)) {
            ot(J_YANGILANISH, y.id);
            break;
        }
    }
    ui.scroll_tugat();
    gfx.kesishni_ol();
    float y = j.p() + 14;
    if (sahifalar > 1) {
        sahifalash("yl/p", {r.x, y, r.w, 34}, yl_sahifa, sahifalar);
        y += 34 + 14;
    }
    float bw = gfx.matn_eni(L"Omborga qaytish", 15, 600) + 36 + 27;
    if (ui.tugma("yl/ortga", {r.x, y, bw, 50}, L"Omborga qaytish", TINCH, "i-chap")) ot(J_OMBOR);
}

// ======================= Yangilanish (qoralama) =======================

namespace {
long long yn_id = 0;
Yangilanish yn;
std::vector<YQator> yn_q;
std::vector<Toqnash> yn_toq;
string yn_turi;
int yn_sahifa = 0;
long long yn_tarix = 0;
std::vector<Harakat> yn_harakat;

void yn_yukla() {
    yangilanish_ol(yn_id, yn);
    yn_q = yangilanish_qatorlari(yn_id, yn_turi, ui.qiymat("yn/q"));
    yn_toq = yn.holat == L"qollandi" ? bekor_toqnashuvlari(yn_id) : std::vector<Toqnash>{};
}

wstring val_belgi(const wstring& v) { return v == L"dollar" ? L"$" : v == L"som" ? L"so'm" : L""; }

// ozg-katak: o'zgargan bo'lsa eski (chizilgan) va yangi (qalin)
void ozg_katak(R c, const wstring& eski, const wstring& yangi, bool ozgardi, bool son, const wstring& eq = L"",
               const wstring& yq = L"") {
    R a = c.ichki(13, 0);
    auto tek = [&](const wstring& s, const wstring& q) {
        if (s.empty()) return wstring(L"—");
        double d = 0;
        return (son && songa(s, d) ? (std::fabs(d) >= 1000 ? pul(d) : ::son(d)) : s) + (q.empty() ? L"" : L" " + q);
    };
    if (ozgardi) {
        wstring e = tek(eski, eq);
        float ew = gfx.matn_eni(e, 13, 400);
        float ex = son ? a.o() - ew : a.x;
        gfx.matn({a.x, c.y + 8, a.w, 19}, e, 13, 400, M->matn3, son ? ONG : CHAP);
        gfx.chiziq(ex, c.y + 18, ex + std::min(ew, a.w), c.y + 18, M->matn3);
        gfx.matn({a.x, c.y + 28, a.w, 22}, tek(yangi, yq), 15, 700, M->matn, son ? ONG : CHAP);
    } else {
        wstring y = tek(yangi, L"");
        if (yangi.empty()) gfx.matn(a, L"—", 15, 400, M->matn, son ? ONG : CHAP);
        else if (!yq.empty() && son) {
            float qw = gfx.matn_eni(yq, 15, 400);
            gfx.matn({a.o() - qw, a.y, qw + 1, a.h}, yq, 15, 400, M->matn3, ONG);
            gfx.matn({a.x, a.y, a.w - qw - 5, a.h}, y, 15, 400, M->matn, ONG);
        } else
            gfx.matn(a, y, 15, 400, M->matn, son ? ONG : CHAP);
    }
}
}  // namespace

void yangilanish_ochildi(long long p) {
    yn_id = p;
    yn_turi.clear();
    yn_sahifa = 0;
    yn_tarix = 0;
    ui.maydonlarni_tozala("yn/");
    yn_yukla();
    ui.scroll_tepaga("yn/s");
}

void yangilanish_chiz(R r) {
    if (ui.tugmacha(VK_F5)) yn_yukla();
    bool qoralama = yn.holat == L"qoralama";
    // sarlavha
    wstring sar = (qoralama ? L"Qoralama #" : L"Yangilanish #") + std::to_wstring(yn.id);
    float sw = gfx.matn_eni(sar, 23, 650);
    gfx.matn({r.x, r.y, sw + 2, 32}, sar, 23, 650, M->matn, CHAP, 2);
    float nx = r.x + sw + 10;
    holat_nishoni(nx, r.y + 10, yn.holat, true);
    nx += gfx.matn_eni(qoralama ? L"hali qo'llanmagan" : yn.holat == L"qollandi" ? L"qo'llangan" : L"bekor qilingan", 11, 600) + 26;
    gfx.matn({nx, r.y, r.o() - nx, 29}, yn.fayl_nomi + (yn.izoh.empty() ? L"" : L" · " + yn.izoh), 15, 400, M->matn2, CHAP, 2);
    r.y += 46;
    // ---- ko'rsatkichlar va amallar ----
    wstring sust = L"Faylda " + guruhla(yn.jami_qator) + L" qator, shundan " + guruhla(yn.ozgarmagan) +
                   L" tasi ombordagi bilan bir xil. Yuklangan: " + mahalliy_vaqt(yn.yaratilgan) +
                   (yn.qollangan.empty() ? L"" : L" · qo'llangan: " + mahalliy_vaqt(yn.qollangan)) +
                   (yn.bekor_qilingan.empty() ? L"" : L" · bekor qilingan: " + mahalliy_vaqt(yn.bekor_qilingan)) + L"." +
                   (yn.yoqlarini_yashir ? L" Faylda yo'q tovarlar yashiriladi." : L"");
    wstring ogoh_m, toq_m;
    if (qoralama && yn.n_xato)
        ogoh_m = std::to_wstring(yn.n_xato) + L" ta qatorda xato bor — ular qo'llanmaydi. «Tuzatish» tugmasi bilan to'g'rilang yoki chiqarib tashlang.";
    if (!yn_toq.empty()) {
        toq_m = L"Hozir bekor qilib bo'lmaydi — keyin bu tovarlar yana o'zgargan. Avval keyingi yangilanishlarni bekor qiling:";
        for (size_t i = 0; i < yn_toq.size() && i < 10; ++i)
            toq_m += L"\n" + yn_toq[i].kod + L" — " + yn_toq[i].sabab +
                     (yn_toq[i].keyingi ? L" (#" + std::to_wstring(yn_toq[i].keyingi) + L")" : L"");
        if (yn_toq.size() > 10) toq_m += L"\n… yana " + std::to_wstring(yn_toq.size() - 10) + L" ta";
    }
    float iw = r.w - 28;
    float qh = 14 + 70 + 10 + gfx.matn_boyi(sust, 15, 400, iw) + 14;
    if (!ogoh_m.empty()) qh += 14 + ichki_xabar({0, 0, iw, 0}, ogoh_m, X_OGOH, true);
    if (!toq_m.empty()) qh += 14 + ichki_xabar({0, 0, iw, 0}, toq_m, X_OGOH, true);
    else if (yn.holat == L"qollandi") qh += 14 + 50;
    if (qoralama) qh += 14 + 50;
    R q = {r.x, r.y, r.w, qh};
    quti(q);
    float x = q.x + 14, y = q.y + 14, kw = (iw - 3 * 14) / 4;
    korsatkich({x, y, kw, 70}, L"YANGI TOVARLAR", guruhla(yn.n_yangi));
    korsatkich({x + kw + 14, y, kw, 70}, qoralama ? L"O'ZGARADIGAN" : L"O'ZGARGAN", guruhla(yn.n_ozgardi));
    korsatkich({x + 2 * (kw + 14), y, kw, 70}, qoralama ? L"YASHIRILADIGAN" : L"YASHIRILGAN", guruhla(yn.n_yashirildi));
    korsatkich({x + 3 * (kw + 14), y, kw, 70}, L"XATO QATORLAR", guruhla(yn.n_xato), L"", yn.n_xato && qoralama ? 1 : 0);
    y += 70 + 10;
    float sh = gfx.matn_boyi(sust, 15, 400, iw);
    gfx.matn({x, y, iw, sh}, sust, 15, 400, M->matn3, CHAP, 0, true);
    y += sh;
    std::vector<wstring> xatolar;
    if (qoralama) {
        if (!ogoh_m.empty()) y += 14 + ichki_xabar({x, y + 14, iw, 0}, ogoh_m, X_OGOH);
        y += 14;
        float bx = x;
        bool mumkin = yn.n_yangi || yn.n_ozgardi || yn.n_yashirildi;
        float w1 = gfx.matn_eni(L"Qo'llash", 15, 600) + 36 + 27;
        if (ui.tugma("yn/qolla", {bx, y, w1, 50}, L"Qo'llash", YASHIL, "i-belgi", false, mumkin) &&
            MessageBoxW(ui.h, L"Ombor yangilansinmi? Keyin bekor qilish mumkin.", L"Ulug'bek Xozmag", MB_OKCANCEL | MB_ICONQUESTION) == IDOK) {
            HCURSOR k = SetCursor(LoadCursor(nullptr, IDC_WAIT));
            bool ok = qollash(yn_id, xatolar);
            SetCursor(k);
            if (ok) {
                ombor_keshini_tashla();
                ombor_eskirdi();
                yn_yukla();
                ui.xabar(L"Ombor yangilandi: " + std::to_wstring(yn.n_yangi) + L" ta yangi, " + std::to_wstring(yn.n_ozgardi) +
                         L" ta o'zgargan, " + std::to_wstring(yn.n_yashirildi) + L" ta yashirilgan.");
            }
        }
        bx += w1 + 14;
        if (yn.n_xato) {
            float w2 = gfx.matn_eni(L"Xato qatorlarni chiqarib tashlash", 15, 600) + 36;
            if (ui.tugma("yn/xchiq", {bx, y, w2, 50}, L"Xato qatorlarni chiqarib tashlash", TINCH)) {
                long long n = xatolarni_chiqar(yn_id);
                ui.xabar(std::to_wstring(n) + L" ta xato qator chiqarib tashlandi.", X_INFO);
                yn_yukla();
            }
            bx += w2 + 14;
        }
        float w3 = gfx.matn_eni(L"Qoralamani o'chirish", 15, 600) + 36;
        if (ui.tugma("yn/ochir", {bx, y, w3, 50}, L"Qoralamani o'chirish", XATO_U) &&
            MessageBoxW(ui.h, L"Qoralama o'chirilsinmi? Ombor o'zgarmaydi.", L"Ulug'bek Xozmag", MB_OKCANCEL | MB_ICONQUESTION) == IDOK) {
            wstring xx;
            if (qoralamani_ochir(yn_id, xx)) {
                ui.xabar(L"Qoralama o'chirildi, ombor o'zgarmadi.");
                ot(J_YANGILANISHLAR);
                return;
            }
            ui.xabar(xx, X_XATO);
        }
    } else if (yn.holat == L"qollandi") {
        if (!toq_m.empty()) ichki_xabar({x, y + 14, iw, 0}, toq_m, X_OGOH);
        else {
            y += 14;
            float w1 = gfx.matn_eni(L"↶ Bekor qilish", 15, 600) + 36;
            gfx.matn({x + w1 + 14, y, iw - w1 - 14, 50},
                     L"Yangi tovarlar o'chadi, o'zgarganlar eski qiymatiga qaytadi, yashirilganlar yana ko'rinadi. Qoldiq farqi tarixga yoziladi.",
                     15, 400, M->matn3, CHAP, 1, true);
            if (ui.tugma("yn/bekor", {x, y, w1, 50}, L"↶ Bekor qilish", XATO_U) &&
                MessageBoxW(ui.h, L"Yangilanish bekor qilinsinmi? Ombor shu yangilanishdan oldingi holatiga qaytadi.",
                            L"Ulug'bek Xozmag", MB_OKCANCEL | MB_ICONQUESTION) == IDOK) {
                if (bekor_qil(yn_id, xatolar)) {
                    ombor_keshini_tashla();
                    ombor_eskirdi();
                    ui.xabar(L"Yangilanish bekor qilindi — ombor avvalgi holatiga qaytdi.");
                }
                yn_yukla();
            }
        }
    }
    for (auto& e : xatolar) ui.xabar(e, X_XATO);
    r.y = q.p() + 14;
    // ---- qidiruv va turlar ----
    std::vector<std::pair<string, wstring>> tur = {{"yangi", L"Yangi"}, {"ozgardi", L"O'zgargan"},
                                                   {"yashirildi", L"Yashirilgan"}, {"xato", L"Xato"},
                                                   {"chiqarilgan", L"Chiqarilgan"}};
    long long sonlar[5] = {yn.n_yangi, yn.n_ozgardi, yn.n_yashirildi, yn.n_xato, yn.n_chiqarilgan};
    R f = {r.x, r.y, r.w, 14 + 50 + 10 + 34 + 14};
    quti(f);
    MaydonSoz ms;
    ms.ishora = L"Kod yoki tovar nomi";
    float qw = (f.w - 28 - 14) / 4;
    ui.maydon("yn/q", {f.x + 14, f.y + 14, qw * 3, 50}, ms);
    if (ui.tugma("yn/qidir", {f.x + 14 + qw * 3 + 14, f.y + 14, qw, 50}, L"Qidirish", ODDIY, "i-qidir") || ui.enter("yn/q")) {
        yn_sahifa = 0;
        yn_yukla();
    }
    float tx = f.x + 14, ty = f.y + 14 + 50 + 10;
    float hw = kichik_tugma_eni(L"Hammasi", false);
    if (ui.tugma("yn/t/", {tx, ty, hw, 34}, L"Hammasi", yn_turi.empty() ? ASOSIY : TINCH, nullptr, true)) {
        yn_turi.clear();
        yn_sahifa = 0;
        yn_yukla();
    }
    tx += hw + 8;
    for (int i = 0; i < 5; ++i) {
        if (!sonlar[i]) continue;
        wstring t = tur[i].second + L" · " + guruhla(sonlar[i]);
        float w = kichik_tugma_eni(t, false);
        if (ui.tugma("yn/t/" + tur[i].first, {tx, ty, w, 34}, t, yn_turi == tur[i].first ? ASOSIY : TINCH, nullptr, true)) {
            yn_turi = tur[i].first;
            yn_sahifa = 0;
            yn_yukla();
        }
        tx += w + 8;
    }
    r.y = f.p() + 14;
    // ---- jadval ----
    int sahifalar = std::max(1, ((int)yn_q.size() + 99) / 100);
    if (yn_sahifa >= sahifalar) yn_sahifa = sahifalar - 1;
    float pastki = 50 + 14 + (sahifalar > 1 ? 34 + 14 : 0);
    std::vector<Ustun> u = {{L"Qator", 70, true}, {L"Amal", 120}, {L"Kod", 80, true}, {L"Tovar", -3},
                            {L"Qoldiq", 130, true}, {L"Narxi", 140, true}, {L"Kelish narxi", 140, true}, {L"", 150}};
    int bosh = yn_sahifa * 100, oxir = std::min((int)yn_q.size(), bosh + 100);
    auto qator_boyi = [](const YQator& q) { return 62.f + (q.xato.empty() ? 0 : 18) + (q.ogoh.empty() ? 0 : 18); };
    float kontent = 0;
    for (int i = bosh; i < oxir; ++i) kontent += qator_boyi(yn_q[i]);
    float joy = std::max(160.f, ui.H - 14 - pastki - r.y);
    R j = {r.x, r.y, r.w, std::min(joy, 38 + std::max(kontent, yn_q.empty() ? 140.f : 0.f))};
    gfx.quti(j, M->panel, 12, M->chegara);
    gfx.kes(j.ichki(1));
    jadval_sarlavha({j.x, j.y, j.w, 38}, u);
    R tana = {j.x, j.y + 38, j.w, j.h - 38};
    if (yn_q.empty())
        bosh_holat(tana, "i-belgi", wstring(L"Bu bo'limda qator yo'q") +
                                         (yn.ozgarmagan && yn_turi.empty() ? L" — fayldagi tovarlar ombordagi bilan bir xil" : L""),
                   L"");
    ui.scroll("yn/s", tana, kontent);
    float ry = tana.y;
    for (int i = bosh; i < oxir; ++i) {
        auto& q = yn_q[i];
        float qb = qator_boyi(q);
        R row = {tana.x, ry, tana.w - 10, qb};
        ry += qb;
        if (q.chiqarilgan) gfx.quti(row, M->panel2.shaffof(.5f));
        gfx.chiziq(row.x, row.p() - .5f, row.o(), row.p() - .5f, M->chegara);
        auto c = ustunlar({row.x, row.y, row.w, 62}, u);
        float op = q.chiqarilgan ? .55f : 1;
        gfx.matn(c[0].ichki(13, 0), q.excel_qator ? std::to_wstring(q.excel_qator) : L"—", 15, 400, M->matn3, ONG);
        float ax = c[1].x + 13, ay = row.y + 21;
        if (q.chiqarilgan) ui.nishon(ax, ay, L"chiqarilgan", M->panel3, M->chegara2, M->matn2);
        else if (q.amal == L"yangi") ui.nishon(ax, ay, L"Yangi", M->tasdiq_fon, M->tasdiq_chek, M->tasdiq);
        else if (q.amal == L"ozgardi") ui.nishon(ax, ay, L"O'zgardi", M->kok_fon, M->kok_chek, M->kok);
        else if (q.amal == L"yashirildi") ui.nishon(ax, ay, L"Yashiriladi", M->ogoh_fon, M->ogoh_chek, M->ogoh);
        else ui.nishon(ax, ay, L"Xato", M->xato_fon, M->xato_chek, M->xato);
        Holat& e = q.eski;
        Holat& n = q.amal == L"yashirildi" ? q.eski : q.yangi;
        bool oz = q.amal == L"ozgardi";
        // kod
        if (oz && e["kod"] != n["kod"]) ozg_katak(c[2], e["kod"], n["kod"], true, true);
        else gfx.matn(c[2].ichki(13, 0), q.kod.empty() ? L"—" : q.kod, 15, 700, M->matn.shaffof(op), ONG);
        // tovar
        R t = c[3].ichki(13, 0);
        if (oz && e["nom"] != n["nom"]) ozg_katak(c[3], e["nom"], n["nom"], true, false);
        else gfx.matn(t, n["nom"].empty() ? L"—" : n["nom"], 15, 700, M->matn.shaffof(op));
        if (oz && e["faol"] == L"false" && n["faol"] == L"true")
            ui.nishon(t.o() - gfx.matn_eni(L"yana ko'rinadi", 11, 600) - 16, row.y + 21, L"yana ko'rinadi", M->tasdiq_fon,
                      M->tasdiq_chek, M->tasdiq);
        float ey = row.y + 54;
        if (!q.xato.empty()) {
            gfx.matn({t.x, ey, t.w + 450, 17}, q.xato, 12, 500, M->kechki ? hex("#ffa0a0") : M->xato);
            ey += 18;
        }
        if (!q.ogoh.empty()) gfx.matn({t.x, ey, t.w + 450, 17}, q.ogoh, 12, 500, M->ogoh);
        // qoldiq / narx / kelish narxi
        auto ozgar = [&](std::initializer_list<const char*> m) {
            if (!oz) return false;
            for (auto k : m)
                if (e[k] != q.yangi[k]) return true;
            return false;
        };
        ozg_katak(c[4], e["qoldiq"], n["qoldiq"], ozgar({"qoldiq"}), true, e["birlik"], n["birlik"]);
        ozg_katak(c[5], e["narx"], n["narx"], ozgar({"narx", "valyuta"}), true, val_belgi(e["valyuta"]), val_belgi(n["valyuta"]));
        ozg_katak(c[6], e["kelish_narxi"], n["kelish_narxi"], ozgar({"kelish_narxi", "kelish_valyutasi"}), true,
                  val_belgi(e["kelish_valyutasi"]), val_belgi(n["kelish_valyutasi"]));
        // amallar
        float bx = c[7].x + 13, by = row.y + 14;
        string sid = std::to_string(q.id);
        if (qoralama) {
            if (q.amal != L"yashirildi") {
                bool xt = q.amal == L"xato";
                float w = xt ? kichik_tugma_eni(L"Tuzatish", true) : 38;
                if (ui.tugma("yn/tz/" + sid, {bx, by, w, 34}, xt ? L"Tuzatish" : L"", TINCH, "i-qalam", true)) {
                    ot(J_QATOR, q.id);
                    break;
                }
                bx += w + 6;
            }
            float w = q.chiqarilgan ? kichik_tugma_eni(L"↺ Qaytarish", false) : 38;
            if (ui.tugma("yn/ch/" + sid, {bx, by, w, 34}, q.chiqarilgan ? L"↺ Qaytarish" : L"", TINCH,
                         q.chiqarilgan ? nullptr : "i-yopish", true)) {
                wstring xx;
                if (!qator_chiqar(q.id, xx)) ui.xabar(xx, X_XATO);
                yn_yukla();
                break;
            }
        } else if (q.mahsulot_id) {
            if (ui.tugma("yn/hr/" + sid, {bx, by, 38, 34}, L"", TINCH, "i-tarix", true)) {
                yn_tarix = q.mahsulot_id;
                yn_harakat = harakatlar(q.mahsulot_id);
            }
        }
    }
    ui.scroll_tugat();
    gfx.kesishni_ol();
    float py = j.p() + 14;
    if (sahifalar > 1) {
        int old = yn_sahifa;
        sahifalash("yn/p", {r.x, py, r.w, 34}, yn_sahifa, sahifalar);
        if (old != yn_sahifa) ui.scroll_tepaga("yn/s");
        py += 34 + 14;
    }
    float bw = gfx.matn_eni(L"Yangilanishlar", 15, 600) + 36 + 27;
    if (ui.tugma("yn/ortga", {r.x, py, bw, 50}, L"Yangilanishlar", TINCH, "i-chap")) ot(J_YANGILANISHLAR);
    // tovar tarixi oynasi
    if (yn_tarix) {
        Tovar tv;
        tovar_ol(yn_tarix, tv);
        bool yop = false;
        R o = oyna("yn/h", L"Kirim/chiqim tarixi — " + tv.nom, 1040, ui.H * .88f, yop);
        if (yop) { yn_tarix = 0; return; }
        R b = o.ichki(14);
        std::vector<Ustun> hu = {{L"Sana", 170}, {L"Turi", 120}, {L"Miqdor", -1, true}, {L"Izoh", -2}};
        gfx.quti(b, M->panel, 12, M->chegara);
        gfx.kes(b.ichki(1));
        jadval_sarlavha({b.x, b.y, b.w, 38}, hu);
        R tn = {b.x, b.y + 38, b.w, b.h - 38};
        ui.scroll("yn/h/s", tn, yn_harakat.size() * 44.f);
        for (size_t i = 0; i < yn_harakat.size(); ++i) {
            auto& h = yn_harakat[i];
            R row = {tn.x, tn.y + i * 44.f, tn.w - 12, 44};
            auto cc = ustunlar(row, hu);
            gfx.matn(cc[0].ichki(13, 0), h.sana, 15, 400, M->matn);
            gfx.matn(cc[1].ichki(13, 0), h.tur, 15, 600, M->matn2);
            gfx.matn(cc[2].ichki(13, 0), h.miqdor, 15, 700, M->matn, ONG);
            gfx.matn(cc[3].ichki(13, 0), h.izoh, 15, 400, M->matn2);
            gfx.chiziq(row.x, row.p() - .5f, row.o(), row.p() - .5f, M->chegara);
        }
        ui.scroll_tugat();
        gfx.kesishni_ol();
    }
}

// ======================= Qatorni tuzatish =======================

namespace {
long long qt_id = 0, qt_yid = 0;
YQator qt;
int qt_birlik = 0, qt_val = 0, qt_kval = 0;
wstring qt_xato;
const std::vector<string> QT_MAYDON = {"kod", "nom", "qoldiq", "narx", "kelish_narxi"};
}  // namespace

void qator_ochildi(long long p) {
    qt_id = p;
    qt_xato.clear();
    ui.maydonlarni_tozala("qt/");
    yqator_ol(p, qt, qt_yid);
    Holat h = qator_formasi_uchun(p);
    for (auto& m : QT_MAYDON) ui.qiymat_qoy("qt/" + m, h[m]);
    qt_birlik = 0;
    for (size_t i = 0; i < BIRLIKLAR.size(); ++i)
        if (BIRLIKLAR[i] == h["birlik"]) qt_birlik = (int)i;
    qt_val = h["valyuta"] == L"dollar" ? 1 : 0;
    qt_kval = h["kelish_valyutasi"] == L"som" ? 1 : h["kelish_valyutasi"] == L"dollar" ? 2 : 0;
    ui.fokus("qt/nom");
}

void qator_chiz(R r) {
    float en = std::min(880.f, r.w);
    R s = {r.x + (r.w - en) / 2, r.y, en, r.h};
    sarlavha(s, qt.amal == L"xato" ? L"Qatorni tuzatish" : L"Qatorni tahrirlash",
             qt.excel_qator ? L"Excel'dagi " + std::to_wstring(qt.excel_qator) + L"-qator" : L"");
    if (!qt.xato.empty()) s.y += ichki_xabar(s, L"Xato: " + qt.xato, X_XATO) + 14;
    if (qt.amal == L"ozgardi" && qt.mahsulot_id) {
        Tovar t;
        if (tovar_ol(qt.mahsulot_id, t))
            s.y += ichki_xabar(s, L"Ombordagi tovar: " + t.kod + L" · " + t.nom + L". O'zgarish shu tovarga yoziladi.", X_INFO) + 14;
    }
    float xh = qt_xato.empty() ? 0 : ichki_xabar({0, 0, s.w - 28, 0}, qt_xato, X_XATO, true) + 10;
    R q = {s.x, s.y, s.w, 14 + xh + 5 * (22 + 50 + 30) + 50 + 14};
    quti(q);
    float x = q.x + 14, w = q.w - 28, g = 14, ust = (w - g) / 2, y = q.y + 14;
    if (xh) {
        ichki_xabar({x, y, w, 0}, qt_xato, X_XATO);
        y += xh;
    }
    auto maydon_ = [&](const string& id, float mx, float mw, const wchar_t* yorliq, const wchar_t* izoh) {
        gfx.matn({mx, y, mw, 17}, yorliq, 12, 600, M->matn2);
        ui.maydon("qt/" + id, {mx, y + 22, mw, 50});
        if (izoh) gfx.matn({mx, y + 76, mw, 16}, izoh, 12, 400, M->matn3);
    };
    auto tanlov_ = [&](const string& id, float mx, float mw, const wchar_t* yorliq, const std::vector<wstring>& v, int& i) {
        gfx.matn({mx, y, mw, 17}, yorliq, 12, 600, M->matn2);
        ui.tanlov("qt/" + id, {mx, y + 22, mw, 50}, v, i);
    };
    maydon_("kod", x, ust, L"Kod", L"Bo'sh bo'lsa navbatdagi bo'sh kod beriladi");
    y += 22 + 50 + 30;
    maydon_("nom", x, w, L"Tovar nomi", nullptr);
    y += 22 + 50 + 30;
    tanlov_("birlik", x, ust, L"Birlik", BIRLIKLAR, qt_birlik);
    maydon_("qoldiq", x + ust + g, ust, L"Qoldiq", nullptr);
    y += 22 + 50 + 30;
    maydon_("narx", x, ust, L"Narxi", nullptr);
    tanlov_("valyuta", x + ust + g, ust, L"Valyuta", {L"so'm", L"dollar"}, qt_val);
    y += 22 + 50 + 30;
    maydon_("kelish_narxi", x, ust, L"Kelish narxi", L"Bo'sh — kelish narxi noma'lum");
    tanlov_("kval", x + ust + g, ust, L"Kelish valyutasi", {L"— tovar valyutasi —", L"so'm", L"dollar"}, qt_kval);
    y += 22 + 50 + 30;
    float w1 = gfx.matn_eni(L"Saqlash", 15, 600) + 36 + 27, w2 = gfx.matn_eni(L"Qoralamaga qaytish", 15, 600) + 36;
    bool saqla = ui.tugma("qt/saqla", {x, y, w1, 50}, L"Saqlash", YASHIL, "i-belgi");
    if (ui.tugma("qt/ortga", {x + w1 + 14, y, w2, 50}, L"Qoralamaga qaytish", TINCH)) {
        ot(J_YANGILANISH, qt_yid);
        return;
    }
    gfx.matn({x + w1 + w2 + 28, y, w - w1 - w2 - 28, 50}, L"Ombor hozir o'zgarmaydi — faqat qoralamadagi qator yangilanadi.",
             15, 400, M->matn3, CHAP, 1, true);
    if (!saqla) return;
    Holat h;
    for (auto& m : QT_MAYDON) h[m] = ui.qiymat("qt/" + m);
    h["birlik"] = BIRLIKLAR[qt_birlik];
    h["valyuta"] = qt_val ? L"dollar" : L"som";
    h["kelish_valyutasi"] = qt_kval == 1 ? L"som" : qt_kval == 2 ? L"dollar" : L"";
    qt_xato = qatorni_saqla(qt_id, h);
    if (qt_xato.empty()) {
        YQator yangi;
        long long yid;
        yqator_ol(qt_id, yangi, yid);
        ui.xabar(L"Qator saqlandi (kod " + yangi.kod + L").");
        ot(J_YANGILANISH, qt_yid);
    }
}
