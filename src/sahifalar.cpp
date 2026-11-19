#include <algorithm>
#include <map>
#include <string_view>
// Umumiy bezaklar va kichik sahifalar: Kirish, Bosh, Login/Parol.
#include "ilova.h"
#include "aloqa.h"

#include <commdlg.h>

// ======================= Umumiy bezaklar =======================

void sarlavha(R& r, const wstring& matn, const wstring& kichik) {
    float en = gfx.matn_eni(matn, 23, 650);
    gfx.matn({r.x, r.y, en + 2, 32}, matn, 23, 650, M->matn, CHAP, 2);
    if (!kichik.empty())
        gfx.matn({r.x + en + 10, r.y, r.w - en - 10, 29}, kichik, 15, 400, M->matn2, CHAP, 2);
    r.y += 32 + 14;
    r.h -= 32 + 14;
}

// .xabar ichki bloki (warning/error/info)
float ichki_xabar(R r, const wstring& matn, XabarTur tur, bool olchash) {
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

void quti(R r) { gfx.quti(r, M->panel, 12, M->chegara); }

void bosh_holat(R r, const char* ikon, const wstring& b, const wstring& matn) {
    float y = r.y + 40;
    gfx.ikon(ikon, {r.x + (r.w - 38) / 2, y, 38, 38}, M->matn2.shaffof(.35f));
    y += 46;
    gfx.matn({r.x + 20, y, r.w - 40, 28}, b, 19, 600, M->matn, MARKAZ);
    y += 30;
    gfx.matn({r.x + 20, y, r.w - 40, 44}, matn, 15, 400, M->matn2, MARKAZ, 0, true);
}

HWND tolov_maydon(const string& id, R r, const wstring& nomi, const wstring& birlik, float olcham,
                  Rang chegara, Rang nom_rang, bool faol) {
    bool fok = ui.fokusda(id) || faol;
    if (fok) gfx.quti({r.x - 3, r.y - 3, r.w + 6, r.h + 6}, M->urgu_fon, 12);
    gfx.quti(r, M->panel, 9, fok ? M->urgu : chegara.bormi() ? chegara : M->chegara2);
    float nx = gfx.matn_eni(nomi, 11, 600) + 26;
    gfx.matn({r.x + 13, r.y, nx, r.h}, nomi, 11, 600, nom_rang.bormi() ? nom_rang : M->matn3);
    float bx = birlik.empty() ? 13 : gfx.matn_eni(birlik, 12, 400) + 16;
    if (!birlik.empty())
        gfx.matn({r.o() - bx, r.y, bx - 13, r.h}, birlik, 12, 400, nom_rang.bormi() ? nom_rang : M->matn2);
    MaydonSoz s;
    s.olcham = olcham;
    s.ogirlik = 700;
    s.ong = true;
    s.fon = M->panel;
    s.ishora = L"0";
    if (ui.bosildi(id + "/quti", r)) ui.fokus(id);
    return ui.tahrir(id, {r.x + nx, r.y, r.w - nx - bx - 8, r.h}, s);
}

void raqamlar(const string& id, R r, const string& nishon, bool vergul) {
    static const wchar_t* T[12] = {L"7", L"8", L"9", L"4", L"5", L"6", L"1", L"2", L"3", L"0", L",", L"⌫"};
    float g = 7, w = (r.w - 2 * g) / 3, h = (r.h - 3 * g) / 4;
    for (int i = 0; i < 12; ++i) {
        if (i == 10 && !vergul) continue;
        R t = {r.x + (i % 3) * (w + g), r.y + (i / 3) * (h + g), w, h};
        bool hov = ui.ustida(t);
        gfx.quti(t, hov ? M->panel3 : M->panel2, 9, hov ? M->chegara2 : M->chegara);
        gfx.matn(t, T[i], 24, 600, i >= 10 ? M->matn2 : M->matn, MARKAZ);
        if (ui.bosildi(id + "/" + std::to_string(i), t)) {
            wstring v = ui.qiymat(nishon);
            if (i == 11) { if (!v.empty()) v.pop_back(); }
            else v += T[i];
            ui.qiymat_qoy(nishon, v);
            ui.fokus(nishon);
        }
    }
}

bool sonmi(const wstring& s, double& v) { return songa(s, v); }

void tayyorlanmoqda_chiz(R r, const wstring& nom, const char* ikon) {
    sarlavha(r, nom);
    quti({r.x, r.y, r.w, 220});
    bosh_holat({r.x, r.y, r.w, 220}, ikon, nom + L" bo'limi",
               L"Bu bo'lim saytda bor, dasturning keyingi versiyasida shu ko'rinishda qo'shiladi.");
}

// ======================= Kirish =======================

static wstring kirish_xato;
static bool birinchi_yuklash = true;

static long long kirish_soni = -1;  // foydalanuvchilar soni (serverdan bir marta)

void kirish_ochildi() { kirish_soni = -1; }

void kirish_chiz(R r) {
    if (kirish_soni < 0) kirish_soni = foydalanuvchilar_soni();
    bool yangi = kirish_soni == 0;
    float en = std::min(380.f, r.w - 28);
    float boy = yangi ? 550 : 440;
    float xh = kirish_xato.empty() ? 0 : ichki_xabar({0, 0, en - 52, 0}, kirish_xato, X_XATO, true) + 14;
    boy += xh;
    R q = {r.x + (r.w - en) / 2, r.y + (r.h - boy) / 2 - 20, en, boy};
    gfx.quti({q.x, q.y + 10, q.w, q.h}, {0, 0, 0, M->kechki ? .3f : .1f}, 14);
    gfx.quti(q, M->panel, 12, M->chegara);
    float y = q.y + 28, x = q.x + 26, w = q.w - 52;
    R logo = {q.x + (q.w - 54) / 2, y, 54, 54};
    gfx.gradient_quti(logo, M->urgu, M->urgu_quyuq, 16);
    gfx.ikon("i-dokon", {logo.x + 13, logo.y + 13, 28, 28}, M->asosiy_matn);
    y += 54 + 10;
    gfx.matn({x, y, w, 31}, L"Ulug'bek Xozmag", 22, 700, M->matn, MARKAZ);
    y += 31 + 10;
    gfx.matn({x, y, w, 18}, yangi ? L"Birinchi foydalanuvchini yarating" : L"Davom etish uchun kiring",
             13, 400, M->matn2, MARKAZ);
    y += 18 + 16;
    if (xh > 0) y += ichki_xabar({x, y, w, 0}, kirish_xato, X_XATO) + 14;
    auto yorliq = [&](const wchar_t* t) {
        gfx.matn({x, y, w, 17}, t, 12, 600, M->matn2);
        y += 17 + 5;
    };
    yorliq(L"Login");
    MaydonSoz s;
    ui.maydon("kirish/login", {x, y, w, 50}, s);
    y += 50 + 14;
    yorliq(L"Parol");
    s.parol = true;
    ui.maydon("kirish/parol", {x, y, w, 50}, s);
    y += 50 + 14;
    if (yangi) {
        yorliq(L"Parolni takrorlang");
        ui.maydon("kirish/parol2", {x, y, w, 50}, s);
        y += 50 + 14;
    }
    if (birinchi_yuklash) {
        ui.fokus("kirish/login");
        birinchi_yuklash = false;
    }
    bool bos = ui.tugma("kirish/tugma", {x, y, w, 50}, yangi ? L"Yaratish va kirish" : L"Kirish", YASHIL);
    if (ui.enter("kirish/login")) ui.fokus("kirish/parol");
    if (ui.enter("kirish/parol") || ui.enter("kirish/parol2")) bos = true;
    y += 50 + 10;
    gfx.matn({x, y, w, 16}, L"v" + versiya(), 11, 400, M->matn3, MARKAZ);
    if (bos) {
        wstring login = ui.qiymat("kirish/login"), parol = ui.qiymat("kirish/parol");
        if (yangi) {
            if (parol != ui.qiymat("kirish/parol2")) kirish_xato = L"Parollar bir xil emas.";
            else if (!foydalanuvchi_yarat(login, parol, true, kirish_xato)) {}
            else { kirish_xato.clear(); kirish_soni = 1; }
        }
        HCURSOR k = SetCursor(LoadCursor(nullptr, IDC_WAIT));
        if (kirish_xato.empty() && kirish_tekshir(login, parol, foydalanuvchi, kirish_xato)) {
            ini_saqla(L"token", W(sessiya_tokeni()));
            ui.qiymat_qoy("kirish/parol", L"");
            ui.qiymat_qoy("kirish/parol2", L"");
            birinchi_yuklash = true;
            ot(J_BOSH);
        } else
            ui.fokus("kirish/parol");
        SetCursor(k);
    }
    // Qaysi serverga ulangani
    wstring b = L"Server: " + ulangan_manzil();
    float by = q.p() + 16;
    gfx.matn({r.x + 20, by, r.w - 40, 18}, b, 12, 400, M->matn3, MARKAZ);
    float bw = 190;
    if (ui.tugma("kirish/server", {r.x + (r.w - bw) / 2, by + 24, bw, 34}, L"Serverni almashtirish", TINCH,
                 nullptr, true)) {
        uzil();
        ulanish_ochildi(L"");
        ot(J_ULANISH);
    }
}

// ======================= Bosh sahifa =======================

static void tanlov(const string& id, R r, const wchar_t* nom, const char* ikon, int tur, bool kichik,
                   Joy joy, long long param = 0) {
    bool hov = ui.ustida(r);
    if (hov) r.y -= 3;  // .tanlov:hover { transform: translateY(-3px) }
    bool k = M->kechki;
    Rang fon = k ? (hov ? Rang{30 / 255.f, 42 / 255.f, 78 / 255.f, .66f} : Rang{14 / 255.f, 20 / 255.f, 42 / 255.f, .52f})
                 : (hov ? Rang{1, 1, 1, .96f} : Rang{1, 1, 1, .8f});
    Rang ch = k ? Rang{1, 1, 1, hov ? .34f : .16f} : (hov ? hex("#82a8db") : hex("#6d8fbd", .32f));
    if (hov && k) {
        if (tur == 1) ch = hex("#8fbdf8", .7f);
        if (tur == 2) ch = hex("#57c2f5", .7f);
        if (tur == 3) ch = hex("#5ad39e", .7f);
    }
    gfx.quti({r.x, r.y + 10, r.w, r.h}, {0, 0, 0, k ? .18f : .05f}, 12);
    gfx.quti(r, fon, 12, ch);
    Rang bfon = k ? Rang{1, 1, 1, .13f} : hex("#e1ecfb"), brang = k ? hex("#e8eeff") : hex("#245a99");
    if (tur == 1) { bfon = k ? hex("#468ff3", .3f) : hex("#e1ecfb"); brang = k ? hex("#d3e4ff") : hex("#245a99"); }
    if (tur == 2) { bfon = k ? hex("#57c2f5", .28f) : hex("#def1fa"); brang = k ? hex("#d6f1ff") : hex("#196b8e"); }
    if (tur == 3) { bfon = k ? hex("#5ad39e", .28f) : hex("#def3e9"); brang = k ? hex("#d0f5e6") : hex("#176d55"); }
    float b = kichik ? 42 : 56, ik = kichik ? 21 : 28, ora = kichik ? 8 : 10;
    float mb = kichik ? 15 : 21, mboy = mb * 1.4f;
    float jami = b + ora + mboy, y = r.y + (r.h - jami) / 2;
    R bel = {r.x + (r.w - b) / 2, y, b, b};
    gfx.quti(bel, bfon, kichik ? 12 : 16);
    gfx.ikon(ikon, {bel.x + (b - ik) / 2, bel.y + (b - ik) / 2, ik, ik}, brang);
    gfx.matn({r.x + 8, y + b + ora, r.w - 16, mboy}, nom, mb, kichik ? 600 : 650,
             k ? hex("#ffffff") : hex("#173454"), MARKAZ);
    if (ui.bosildi(id, r)) ot(joy, param);
}

void bosh_chiz(R r) {
    // fon: zarrachali rasm + yuqori/pastdan qoraytiruvchi gradient
    if (M->kechki) {
        gfx.rasm("bosh-fon", 202, r, true);
        gfx.gradient_quti({r.x, r.y, r.w, r.h * .45f}, hex("#040816", .5f), hex("#040816", .2f), 0, false);
        gfx.gradient_quti({r.x, r.y + r.h * .45f, r.w, r.h * .55f + 1}, hex("#040816", .2f), hex("#040816", .55f), 0, false);
    } else {
        gfx.quti(r, hex("#eef4ff"));
        gfx.svg_rasm("bosh-kunduzgi", resurs_matni(203), r);
    }
    R s = r.ichki(14);
    float max_en = s.h < 630 / 0.9f ? 660 : 840;
    float en = std::min(max_en, s.w), g = 14;
    float k3 = (en - 2 * g) / 3, k4 = (en - 3 * g) / 4;
    float jami = k3 + g + k4;
    float x = s.x + (s.w - en) / 2, y = s.y + std::max(0.f, (s.h - jami) / 2);
    tanlov("bosh/sotuv", {x, y, k3, k3}, L"Sotuv", "i-savdo", 1, false, J_SOTUV);
    tanlov("bosh/qidir", {x + k3 + g, y, k3, k3}, L"Oldin qarz olgan", "i-qidir", 2, false, J_QIDIRISH);
    tanlov("bosh/yangi", {x + 2 * (k3 + g), y, k3, k3}, L"Yangi qarzdor", "i-odam-plus", 3, false, J_YANGI_QARZDOR);
    y += k3 + g;
    tanlov("bosh/sotuvlar", {x, y, k4, k4}, L"Sotuvlar", "i-chek", 0, true, J_SOTUVLAR);
    tanlov("bosh/qarzdorlar", {x + k4 + g, y, k4, k4}, L"Qarzdorlar", "i-odamlar", 0, true, J_QARZDORLAR);
    tanlov("bosh/ombor", {x + 2 * (k4 + g), y, k4, k4}, L"Ombor", "i-quti", 0, true, J_OMBOR);
    tanlov("bosh/tovar", {x + 3 * (k4 + g), y, k4, k4}, L"Tovar qo'shish", "i-plus", 0, true, J_MAHSULOT, 0);
}

// ======================= Login / Parol =======================

static wstring ak_xato[4];
static bool ak_majburiy = false;  // standart parol (akkaunt.boshlangich_parol)

// ---- Django AUTH_PASSWORD_VALIDATORS (saytdagi settings.py) ----
static wstring kichik_harf(wstring s) {
    if (!s.empty()) CharLowerBuffW(s.data(), (DWORD)s.size());
    return s;
}

// UserAttributeSimilarityValidator (max_similarity 0.7, SequenceMatcher.quick_ratio)
static bool loginga_oxshash(const wstring& parol_, const wstring& login) {
    const double MAX = 0.7;
    wstring parol = kichik_harf(parol_), qiymat = kichik_harf(login);
    std::vector<wstring> qismlar;
    wstring joriy;
    for (wchar_t c : qiymat) {
        if (IsCharAlphaNumericW(c) || c == L'_') joriy += c;
        else { qismlar.push_back(joriy); joriy.clear(); }
    }
    qismlar.push_back(joriy);
    qismlar.push_back(qiymat);
    for (auto& q : qismlar) {
        double pl = (double)parol.size(), ql = (double)q.size();
        if (pl >= 10 * ql && ql < MAX / 2 * pl) continue;  // exceeds_maximum_length_ratio
        std::map<wchar_t, int> sanoq;
        for (wchar_t c : q) ++sanoq[c];
        int mos = 0;
        for (wchar_t c : parol) {
            auto it = sanoq.find(c);
            if (it != sanoq.end() && it->second > 0) { --it->second; ++mos; }
        }
        double nisbat = pl + ql ? 2.0 * mos / (pl + ql) : 1.0;
        if (nisbat >= MAX) return true;
    }
    return false;
}

// CommonPasswordValidator: password.lower().strip() ro'yxatda bo'lmasin
static bool oddiy_parolmi(const wstring& parol) {
    static string matn = resurs_matni(204);
    static std::vector<std::string_view> royxat;
    if (royxat.empty()) {
        size_t b = 0;
        while (b < matn.size()) {
            size_t e = matn.find(char(10), b);
            if (e == string::npos) e = matn.size();
            size_t o = e;
            if (o > b && matn[o - 1] == '\r') --o;  // git CRLF qilib chiqargan bo'lsa ham
            if (o > b) royxat.emplace_back(matn.data() + b, o - b);
            b = e + 1;
        }
    }
    string k = U(kichik_harf(tozala(parol)));
    return std::binary_search(royxat.begin(), royxat.end(), std::string_view(k));
}

// validate_password(parol, user) — xatolar bitta qatorda
static wstring parol_tekshiruvi(const wstring& parol, const wstring& login) {
    std::vector<wstring> x;
    if (loginga_oxshash(parol, login)) x.push_back(L"Parol login bilan juda o'xshash.");
    if (parol.size() < 8) x.push_back(L"Parol juda qisqa. Kamida 8 belgidan iborat bo'lishi kerak.");
    if (oddiy_parolmi(parol)) x.push_back(L"Bu parol juda oddiy (keng tarqalgan).");
    bool raqam = !parol.empty();
    for (wchar_t c : parol) if (!iswdigit(c)) raqam = false;
    if (raqam) x.push_back(L"Parol faqat raqamlardan iborat bo'lmasin.");
    wstring r;
    for (auto& s : x) r += (r.empty() ? L"" : L" ") + s;
    return r;
}

void akkaunt_ochildi(long long) {
    ui.maydonlarni_tozala("akk/");
    ui.qiymat_qoy("akk/login", foydalanuvchi.login);
    for (auto& x : ak_xato) x.clear();
    ak_majburiy = false;
    if (foydalanuvchi.login == L"server" || foydalanuvchi.login == L"kassa") {
        Foydalanuvchi tek;
        wstring x;
        ak_majburiy = kirish_tekshir(foydalanuvchi.login, foydalanuvchi.login, tek, x, false);
    }
}

void akkaunt_chiz(R r) {
    float en = std::min(660.f, r.w);
    R s = {r.x + (r.w - en) / 2, r.y, en, r.h};
    sarlavha(s, L"Login va parolni o'zgartirish");
    if (ak_majburiy) {
        const wchar_t* m = L"Xavfsizlik uchun standart parolni o'zgartirish tavsiya etiladi.";
        float mh = std::max(40.f, gfx.matn_boyi(m, 13, 500, s.w - 24) + 18);
        gfx.quti({s.x, s.y, s.w, mh}, M->panel, 9, M->chegara);
        gfx.matn({s.x + 12, s.y + 9, s.w - 24, mh - 18}, m, 13, 500, M->matn, CHAP, 0, true);
        s.y += mh + 16;
    }
    const wchar_t* yorliqlar[4] = {L"Yangi login", L"Hozirgi parol", L"Yangi parol", L"Yangi parolni takrorlang"};
    const char* idlar[4] = {"akk/login", "akk/eski", "akk/yangi1", "akk/yangi2"};
    float boy = 14 * 2 + 4 * (17 + 5 + 50 + 16) + 50;
    for (auto& x : ak_xato) if (!x.empty()) boy += 20;
    R q = {s.x, s.y, s.w, boy};
    quti(q);
    float y = q.y + 14, x = q.x + 14, w = q.w - 28;
    for (int i = 0; i < 4; ++i) {
        gfx.matn({x, y, w, 17}, yorliqlar[i], 12, 600, M->matn2);
        y += 22;
        MaydonSoz m;
        m.parol = i > 0;
        ui.maydon(idlar[i], {x, y, w, 50}, m);
        y += 50;
        if (!ak_xato[i].empty()) {
            gfx.matn({x, y + 4, w, 16}, ak_xato[i], 12, 500, M->kechki ? hex("#ffa0a0") : M->xato);
            y += 20;
        }
        y += 16;
    }
    float bw = gfx.matn_eni(L"Login va parolni saqlash", 15, 600) + 36;
    bool saqla = ui.tugma("akk/saqla", {x, y, bw, 50}, L"Login va parolni saqlash", YASHIL);
    if (ui.tugma("akk/ortga", {x + bw + 14, y, 110, 50}, L"Ortga", TINCH)) ot(J_BOSH);
    if (!saqla) return;
    for (auto& e : ak_xato) e.clear();
    wstring login = tozala(ui.qiymat("akk/login")), eski = ui.qiymat("akk/eski");
    wstring y1 = ui.qiymat("akk/yangi1"), y2 = ui.qiymat("akk/yangi2");
    if (login.empty()) ak_xato[0] = L"Login bo'sh bo'lishi mumkin emas.";
    if (eski.empty()) ak_xato[1] = L"Bu maydon to'ldirilishi shart.";
    if (y1.empty()) ak_xato[2] = L"Bu maydon to'ldirilishi shart.";
    if (y2.empty()) ak_xato[3] = L"Bu maydon to'ldirilishi shart.";
    else if (y1 != y2) ak_xato[3] = L"Parollar bir xil emas.";
    if (!y1.empty() && (y1 == L"server" || y1 == L"kassa" || y1 == login))
        ak_xato[2] = L"Standart login/paroldan foydalanmang.";
    else if (!y1.empty() && y1 == eski) ak_xato[2] = L"Yangi parol eskisidan farq qilishi kerak.";
    else if (!y1.empty()) ak_xato[2] = parol_tekshiruvi(y1, foydalanuvchi.login);
    for (auto& e : ak_xato) if (!e.empty()) return;
    wstring x2;
    Foydalanuvchi tek;
    if (!kirish_tekshir(foydalanuvchi.login, eski, tek, x2, false)) {
        ak_xato[1] = L"Hozirgi parol noto'g'ri.";
        return;
    }
    if (login != foydalanuvchi.login && !login_almashtir(foydalanuvchi.id, login, x2)) {
        ak_xato[0] = L"Bu login band.";
        return;
    }
    parol_almashtir(foydalanuvchi.id, eski, y1, x2);
    foydalanuvchi.login = login;
    ui.xabar(L"Login va parol muvaffaqiyatli o'zgartirildi.");
    ot(J_BOSH);
}

R oyna(const string& id, const wstring& sarlavha_, float max_en, float boy, bool& yopildi) {
    ui.oyna_fon();
    float en = std::min(max_en, ui.W - 40);
    boy = std::min(boy, ui.H * .88f);
    R o = {(ui.W - en) / 2 - gfx.dx, (ui.H - boy) / 2 - gfx.dy, en, boy};
    gfx.quti({o.x, o.y + 10, o.w, o.h}, {0, 0, 0, .35f}, 14);
    gfx.quti(o, M->panel, 12, M->chegara2);
    R b = {o.x + 1, o.y + 1, o.w - 2, 56};
    gfx.quti(b, M->panel2, 11);
    gfx.quti({b.x, b.y + 20, b.w, b.h - 20}, M->panel2);
    gfx.chiziq(o.x, o.y + 57.5f, o.o(), o.y + 57.5f, M->chegara);
    gfx.matn({o.x + 18, o.y, o.w - 200, 58}, sarlavha_, 19, 650, M->matn);
    float tw = gfx.matn_eni(L"Yopish (Esc)", 13, 600) + 22 + 16;
    yopildi = ui.tugma(id + "/yop", {o.o() - 18 - tw, o.y + 12, tw, 34}, L"Yopish (Esc)", TINCH, "i-yopish", true);
    if (ui.tugmacha(VK_ESCAPE)) yopildi = true;
    return {o.x, o.y + 58, o.w, o.h - 58};
}

void pul_guruhla(const string& id) {
    if (!ui.ozgardi(id)) return;
    wstring s = ui.qiymat(id), raqam, kasr;
    bool ayirgich = false;
    for (wchar_t c : s) {
        if (c == L' ' || c == 0xA0) continue;
        if ((c == L',' || c == L'.') && !ayirgich) { ayirgich = true; continue; }
        if (c < L'0' || c > L'9') return;  // noto'g'ri belgi — tegmaymiz
        (ayirgich ? kasr : raqam) += c;
    }
    wstring g;
    for (size_t i = 0; i < raqam.size(); ++i) {
        if (i && (raqam.size() - i) % 3 == 0) g += L' ';
        g += raqam[i];
    }
    if (ayirgich) g += L"," + kasr;
    if (g != s) {
        ui.qiymat_qoy(id, g);
        HWND e = GetFocus();
        if (e) SendMessageW(e, EM_SETSEL, g.size(), g.size());
    }
}

void yorliq_belgi(float x, float cy, const wstring& t, Rang rang) {
    float w = gfx.matn_eni(t, 10, 600) + 10;
    R r = {x, cy - 9, w, 18};
    gfx.quti(r, {}, 4, rang.shaffof(.55f));
    gfx.matn(r, t, 10, 600, rang.shaffof(.55f), MARKAZ);
}

void korsatkich(R r, const wstring& nomi, const wstring& qiymat, const wstring& dollar, int tur) {
    Rang fon = M->panel2, ch = M->chegara, nr = M->matn3, qr = M->matn;
    float qo = 21;
    if (tur == 1) { fon = M->urgu_fon; ch = M->urgu_chek; nr = M->kechki ? hex("#8fbdf0") : M->urgu; qr = M->urgu_matn; qo = 25; }
    if (tur == 2) { fon = M->ogoh_fon; ch = M->ogoh_chek; nr = qr = M->ogoh; }
    gfx.quti(r, fon, 9, ch);
    gfx.matn({r.x + 14, r.y + 10, r.w - 28, 15}, nomi, 11, 600, nr);
    gfx.matn({r.x + 14, r.y + 26, r.w - 28, qo * 1.35f}, qiymat, qo, 700, qr);
    if (!dollar.empty())
        gfx.matn({r.x + 14, r.y + 26 + qo * 1.35f, r.w - 28, 22}, dollar, tur == 1 ? 17 : 12, 700, M->tasdiq);
}

std::vector<R> ustunlar(R satr, const std::vector<Ustun>& u) {
    float qat = 0, ulush = 0;
    for (auto& x : u) (x.en > 0 ? qat : ulush) += x.en > 0 ? x.en : -x.en;
    float erk = std::max(0.f, satr.w - qat);
    std::vector<R> r;
    float x = satr.x;
    for (auto& c : u) {
        float w = c.en > 0 ? c.en : erk * (-c.en / ulush);
        r.push_back({x, satr.y, w, satr.h});
        x += w;
    }
    return r;
}

void jadval_sarlavha(R r, const std::vector<Ustun>& u) {
    gfx.quti(r, M->panel2);
    gfx.chiziq(r.x, r.p() - .5f, r.o(), r.p() - .5f, M->chegara);
    auto j = ustunlar(r, u);
    for (size_t i = 0; i < u.size(); ++i) {
        wstring t = u[i].nom;
        CharUpperBuffW(t.data(), (DWORD)t.size());
        gfx.matn(j[i].ichki(13, 0), t, 11, 600, M->matn3, u[i].son ? ONG : CHAP);
    }
}

bool tanlov_karta(const string& id, R r, const wstring& nom, const wstring& izoh, const char* ikon, int tur) {
    bool hov = ui.ustida(r);
    if (hov) r.y -= 3;
    Rang ch = M->chegara;
    if (hov) ch = tur == 2 ? M->kok : tur == 3 ? M->tasdiq : M->chegara2;
    gfx.quti(r, hov ? M->panel2 : M->panel, 12, ch);
    Rang bf = M->panel3, br = M->matn2;
    if (tur == 2) { bf = M->kok_fon; br = M->kok; }
    if (tur == 3) { bf = M->tasdiq_fon; br = M->tasdiq; }
    float iz = izoh.empty() ? 0 : gfx.matn_boyi(izoh, 13, 400, r.w - 32) + 10;
    float jami = 56 + 10 + 29 + iz, y = r.y + (r.h - jami) / 2;
    R b = {r.x + (r.w - 56) / 2, y, 56, 56};
    gfx.quti(b, bf, 16);
    gfx.ikon(ikon, {b.x + 14, b.y + 14, 28, 28}, br);
    gfx.matn({r.x + 16, y + 66, r.w - 32, 29}, nom, 21, 650, M->matn, MARKAZ);
    if (!izoh.empty()) gfx.matn({r.x + 16, y + 105, r.w - 32, iz}, izoh, 13, 400, M->matn2, MARKAZ, 0, true);
    return ui.bosildi(id, r);
}

// ======================= Qurilmalar (hozircha: chek printeri) =======================
#include "chek_chop.h"

static std::vector<wstring> q_printerlar, q_etk_royxat;
static int q_printer = 0, q_etk = 0, q_eni = 1;

static float q_tarozi_boy = 600;

void qurilmalar_ochildi(long long) {
    ui.scroll_tepaga("qr/s");
    tarozi_ochildi();
    qurilma_royxat_ochildi();
    q_printerlar = {L"— Chek chop etilmasin —"};
    for (auto& p : printerlar()) q_printerlar.push_back(p);
    wstring joriy = ini_qiymat(L"chek_printer");
    q_printer = 0;
    for (size_t i = 1; i < q_printerlar.size(); ++i)
        if (q_printerlar[i] == joriy) q_printer = (int)i;
    q_etk_royxat = {L"— Etiketka chop etilmasin —"};
    for (auto& p : printerlar()) q_etk_royxat.push_back(p);
    wstring etk = ini_qiymat(L"etiketka_printer");
    q_etk = 0;
    for (size_t i = 1; i < q_etk_royxat.size(); ++i)
        if (q_etk_royxat[i] == etk) q_etk = (int)i;
    q_eni = ini_qiymat(L"chek_eni", L"80") == L"58" ? 0 : 1;
    ui.qiymat_qoy("qr/izoh", ini_qiymat(L"chek_izoh"));
}

void qurilmalar_chiz(R r) {
    float en = std::min(1100.f, r.w);
    R s = {r.x + (r.w - en) / 2, r.y, en, r.h};
    sarlavha(s, L"Yangi ulangan qurilmalar");
    // Saytdagi tartib: shu kassa, chek printeri, Ethernet tarozi, skaner sinovi, USB qurilmalar
    float pw = s.w - 12;
    float h_kassa = shu_kassa_panel(0, 0, pw, false);
    const wchar_t* sinov_izoh = L"Sinov savdo yaratmaydi va ombor qoldig\u2018ini o\u2018zgartirmaydi.";
    float q_boy = 14 + 26 + 10 + 4 * (22 + 50 + 14) + 50 + 10 + 20 + 14;
    float h_skan = skaner_panel(0, 0, pw, false), h_usb = usb_panel(0, 0, pw, false);
    ui.scroll("qr/s", {s.x, s.y, s.w, s.h},
              h_kassa + 14 + q_boy + 14 + q_tarozi_boy + 14 + h_skan + 14 + h_usb + 4);
    shu_kassa_panel(s.x, s.y, pw, true);
    R q = {s.x, s.y + h_kassa + 14, pw, q_boy};
    quti(q);
    float x = q.x + 14, w = q.w - 28, y = q.y + 14;
    gfx.matn({x, y, w, 26}, L"Chek printeri", 20, 700, M->matn);
    y += 26 + 10;
    gfx.matn({x, y, w, 17}, L"Chek printeri (yakunlangan chek shu printerga avtomatik chiqadi)", 12, 600, M->matn2);
    y += 22;
    if (ui.tanlov("qr/printer", {x, y, w, 50}, q_printerlar, q_printer))
        ini_qiymat_yoz(L"chek_printer", q_printer ? q_printerlar[q_printer] : L"");
    y += 50 + 14;
    gfx.matn({x, y, w, 17}, L"Etiketka printeri", 12, 600, M->matn2);
    y += 22;
    if (ui.tanlov("qr/etk", {x, y, w, 50}, q_etk_royxat, q_etk))
        ini_qiymat_yoz(L"etiketka_printer", q_etk ? q_etk_royxat[q_etk] : L"");
    y += 50 + 14;
    gfx.matn({x, y, w, 17}, L"Chek eni", 12, 600, M->matn2);
    y += 22;
    if (ui.tanlov("qr/eni", {x, y, w, 50}, {L"58 mm (32 belgi)", L"80 mm (42 belgi)"}, q_eni))
        ini_qiymat_yoz(L"chek_eni", q_eni ? L"80" : L"58");
    y += 50 + 14;
    gfx.matn({x, y, w, 17}, L"Chek ostidagi yozuv (manzil, telefon)", 12, 600, M->matn2);
    y += 22;
    ui.maydon("qr/izoh", {x, y, w, 50});
    if (ui.ozgardi("qr/izoh")) ini_qiymat_yoz(L"chek_izoh", ui.qiymat("qr/izoh"));
    y += 50 + 14;
    if (ui.tugma("qr/sinov", {x, y, 220, 50}, L"Sinov cheki", ODDIY, "i-printer")) {
        Chek c;
        c.sana_utc = hozir_utc();
        c.qatorlar = {{L"Sinov: shurup 4x40", L"dona", L"som", 25, 300, true}};
        c.naqd = 7500;
        wstring xato;
        if (q_printer == 0) ui.xabar(L"Avval printerni tanlang.", X_XATO);
        else if (chek_chop(c, xato)) ui.xabar(L"Sinov cheki printerga yuborildi.");
        else ui.xabar(xato, X_XATO);
    }
    if (ui.tugma("qr/hammasi", {x + 236, y, 260, 50}, L"Barcha printerlar", ODDIY, "i-printer")) ot(J_PRINTER);
    gfx.matn({x, y + 50 + 10, w, 20}, sinov_izoh, 14, 400, M->matn2);
    R t = {q.x, q.p() + 14, q.w, q_tarozi_boy};
    quti(t);
    float b = tarozi_panel(t.x, t.y, t.w);
    if (std::abs(b - q_tarozi_boy) > .5f) {
        q_tarozi_boy = b;
        ui.qayta = true;
    }
    float yy = t.p() + 14;
    skaner_panel(s.x, yy, pw, true);
    yy += h_skan + 14;
    usb_panel(s.x, yy, pw, true);
    ui.scroll_tugat();
}
