// «Printerlar» sahifasi: dastur ochilganda (kassa ham, server ham) shu kompyuterdagi har bir
// printerga bir xil savol beriladi — chek, etiketka yoki ishlatilmaydi. Javob sozlama.ini da
// saqlanadi; yangi printer paydo bo'lsa yoki tanlangan printer yo'qolsa yana so'raladi.
#include "ilova.h"

#include <winspool.h>

#include <algorithm>

namespace {

enum Rol { YOQ, CHEK, ETIKETKA };

struct Printer {
    wstring nom, port;
    bool standart = false, virtual_ = false, ochiq = false;
    Rol rol = YOQ;
};

std::vector<Printer> pr;
bool har_safar = false, pr_boshida = false;

std::vector<wstring> bol(const wstring& s) {
    std::vector<wstring> r;
    size_t a = 0;
    while (a <= s.size()) {
        size_t b = s.find(L'|', a);
        if (b == wstring::npos) b = s.size();
        if (b > a) r.push_back(s.substr(a, b - a));
        a = b + 1;
    }
    return r;
}

wstring kichik(wstring s) {
    for (auto& c : s) c = (wchar_t)towlower(c);
    return s;
}

bool bormi(const wstring& s, std::initializer_list<const wchar_t*> v) {
    for (auto* t : v)
        if (s.find(t) != wstring::npos) return true;
    return false;
}

// PDF, XPS, faks, OneNote — qog'ozga chiqmaydigan printerlar ro'yxat oxirida
bool virtualmi(const wstring& nom, const wstring& port) {
    wstring n = kichik(nom), p = kichik(port);
    return bormi(p, {L"portprompt", L"shrfax", L"xpsport", L"nul:", L"onenote", L"file:"}) ||
           bormi(n, {L"pdf", L"xps", L"fax", L"факс", L"onenote", L"anydesk"});
}

std::vector<Printer> tizim_printerlari() {
    std::vector<Printer> r;
    DWORD kerak = 0, soni = 0, bay = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;
    EnumPrintersW(bay, nullptr, 2, nullptr, 0, &kerak, &soni);
    if (!kerak) return r;
    std::vector<BYTE> buf(kerak);
    if (!EnumPrintersW(bay, nullptr, 2, buf.data(), kerak, &kerak, &soni)) return r;
    wchar_t st[512];
    DWORD sn = 512;
    wstring standart = GetDefaultPrinterW(st, &sn) ? st : L"";
    auto* p = (PRINTER_INFO_2W*)buf.data();
    for (DWORD i = 0; i < soni; ++i) {
        Printer x;
        x.nom = p[i].pPrinterName ? p[i].pPrinterName : L"";
        x.port = p[i].pPortName ? p[i].pPortName : L"";
        x.standart = x.nom == standart;
        x.virtual_ = virtualmi(x.nom, x.port);
        x.ochiq = (p[i].Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE) || (p[i].Status & PRINTER_STATUS_OFFLINE);
        r.push_back(x);
    }
    std::stable_sort(r.begin(), r.end(), [](const Printer& a, const Printer& b) { return a.virtual_ < b.virtual_; });
    return r;
}

// Yangi printer uchun taxminiy javob (nomidan); foydalanuvchi baribir o'zi tasdiqlaydi
Rol taxmin(const Printer& p) {
    if (p.virtual_) return YOQ;
    wstring n = kichik(p.nom);
    if (bormi(n, {L"365", L"label", L"etiket", L"этикет", L"tsc", L"zebra", L"godex", L"xp-4", L"xp-3"}))
        return ETIKETKA;
    if (bormi(n, {L"pos", L"xp-58", L"xp-80", L"58mm", L"80mm", L"receipt", L"чек", L"chek", L"rongta", L"epson tm"}))
        return CHEK;
    return YOQ;
}

void rol_qoy(size_t i, Rol r) {
    // Chek ham, etiketka ham bitta printerga — boshqasida shu vazifa olib tashlanadi
    if (r != YOQ)
        for (auto& p : pr)
            if (p.rol == r) p.rol = YOQ;
    pr[i].rol = r;
}

void saqla() {
    wstring chek, etk, malum;
    for (auto& p : pr) {
        if (p.rol == CHEK) chek = p.nom;
        if (p.rol == ETIKETKA) etk = p.nom;
        malum += (malum.empty() ? L"" : L"|") + p.nom;
    }
    ini_qiymat_yoz(L"chek_printer", chek);
    ini_qiymat_yoz(L"etiketka_printer", etk);
    ini_qiymat_yoz(L"printerlar_malum", malum);
    ini_qiymat_yoz(L"printer_har_safar", har_safar ? L"1" : L"0");
}

}  // namespace

bool printer_soralsinmi() {
    if (ini_qiymat(L"printer_har_safar") == L"1") return true;
    auto hozir = tizim_printerlari();
    auto malum = bol(ini_qiymat(L"printerlar_malum"));
    auto bor = [&](const wstring& n) {
        for (auto& p : hozir)
            if (p.nom == n) return true;
        return false;
    };
    for (auto& p : hozir)
        if (std::find(malum.begin(), malum.end(), p.nom) == malum.end()) return true;  // yangi printer
    for (const wchar_t* k : {L"chek_printer", L"etiketka_printer"}) {
        wstring n = ini_qiymat(k);
        if (!n.empty() && !bor(n)) return true;  // tanlangani yo'qolgan
    }
    return false;
}

wstring printer_qisqacha() {
    wstring c = ini_qiymat(L"chek_printer"), e = ini_qiymat(L"etiketka_printer");
    return L"Chek: " + (c.empty() ? L"—" : c) + L"   ·   Etiketka: " + (e.empty() ? L"—" : e);
}

void printer_ochildi(bool boshida) {
    pr_boshida = boshida;
    pr = tizim_printerlari();
    auto malum = bol(ini_qiymat(L"printerlar_malum"));
    wstring chek = ini_qiymat(L"chek_printer"), etk = ini_qiymat(L"etiketka_printer");
    for (size_t i = 0; i < pr.size(); ++i) {
        auto& p = pr[i];
        if (p.nom == chek) p.rol = CHEK;
        else if (p.nom == etk) p.rol = ETIKETKA;
        else if (std::find(malum.begin(), malum.end(), p.nom) == malum.end()) {
            Rol t = taxmin(p);
            bool band = false;
            for (auto& q : pr) band = band || (q.rol == t && t != YOQ);
            if (!band && t != YOQ && (t == CHEK ? chek : etk).empty()) p.rol = t;
        }
    }
    har_safar = ini_qiymat(L"printer_har_safar") == L"1";
    ui.scroll_tepaga("pr/s");
}

void printer_chiz(R r) {
    float en = std::min(780.f, r.w - 28);
    float qb = 66;
    float tepa = 28 + 54 + 12 + 31 + 6 + 18 + 22;       // logo, sarlavha, izoh
    float past = 16 + 24 + 16 + 50 + 12 + 34 + 16 + 16;  // belgi, tugma, yordam, versiya
    float royxat = std::max<size_t>(pr.size(), 1) * qb;
    float sigadi = r.h - 40 - tepa - past;
    float rb = std::min(royxat, std::max(qb * 2, sigadi));
    float boy = tepa + rb + past;
    R q = {r.x + (r.w - en) / 2, r.y + std::max(14.f, (r.h - boy) / 2 - 10), en, boy};
    gfx.quti({q.x, q.y + 10, q.w, q.h}, {0, 0, 0, M->kechki ? .3f : .1f}, 14);
    gfx.quti(q, M->panel, 12, M->chegara);
    float x = q.x + 26, w = q.w - 52, y = q.y + 28;
    R logo = {q.x + (q.w - 54) / 2, y, 54, 54};
    gfx.gradient_quti(logo, M->urgu, M->urgu_quyuq, 16);
    gfx.ikon("i-printer", {logo.x + 13, logo.y + 13, 28, 28}, M->asosiy_matn);
    y += 54 + 12;
    gfx.matn({x, y, w, 31}, L"Printerlar", 22, 700, M->matn, MARKAZ);
    y += 31 + 6;
    gfx.matn({x, y, w, 18}, L"Har bir printer nima uchun ishlatiladi? Bilmasangiz «Sinov» bilan tekshiring.", 13,
             400, M->matn2, MARKAZ);
    y += 18 + 22;

    R tana = {q.x + 14, y, q.w - 28, rb};
    if (pr.empty()) {
        gfx.matn({x, y, w, rb}, L"Bu kompyuterda printer topilmadi. Printer ulangach dasturni qayta oching.", 15,
                 500, M->matn3, MARKAZ);
    } else {
        ui.scroll("pr/s", tana, royxat);
        const wchar_t* nomlar[] = {L"Ishlatilmaydi", L"Chek", L"Etiketka"};
        const Rol tartib[] = {CHEK, ETIKETKA, YOQ};
        const float enlar[] = {84, 104, 132};
        for (size_t i = 0; i < pr.size(); ++i) {
            auto& p = pr[i];
            R row = {tana.x, tana.y + i * qb, tana.w - (royxat > rb ? 12 : 0), qb};
            if (i) gfx.chiziq(row.x + 12, row.y - .5f, row.o() - 12, row.y - .5f, M->chegara);
            string sid = std::to_string(i);
            // O'ngda: [Chek] [Etiketka] [Ishlatilmaydi] [Sinov]
            float tb = 40, ty = row.y + (qb - tb) / 2;
            float ox = row.o() - 12 - 92;
            if (ui.tugma("pr/sin/" + sid, {ox, ty, 92, tb}, L"Sinov", TINCH, "i-printer", true)) {
                Chek c;
                c.sana_utc = hozir_utc();
                c.qatorlar = {{L"Sinov: " + p.nom, L"dona", L"som", 1, 0, true}};
                wstring xato;
                if (chek_chop(c, xato, p.nom)) ui.xabar(L"«" + p.nom + L"» ga sinov yuborildi.");
                else ui.xabar(xato, X_XATO);
            }
            ox -= 10;
            for (int k = 2; k >= 0; --k) {
                Rol rl = tartib[k];
                ox -= enlar[k];
                bool tanl = p.rol == rl;
                Uslub u = tanl ? (rl == YOQ ? ODDIY : rl == CHEK ? ASOSIY : YASHIL) : TINCH;
                if (ui.tugma("pr/r/" + sid + "/" + std::to_string(k), {ox, ty, enlar[k], tb},
                             nomlar[rl], u, nullptr, true))
                    rol_qoy(i, rl);
                if (tanl && rl == YOQ)
                    gfx.quti({ox, ty, enlar[k], tb}, {}, 9, M->matn2, 1.5f);
                ox -= 6;
            }
            float nx = row.x + 12, nw = ox - nx - 10;
            gfx.matn({nx, row.y + 11, nw, 22}, p.nom, 15, 700, p.virtual_ ? M->matn2 : M->matn);
            wstring iz = p.port;
            if (p.standart) iz = L"Windows standart printeri · " + iz;
            if (p.virtual_) iz += L" · qog'ozga chiqmaydi";
            if (p.ochiq) iz += L" · oflayn";
            gfx.matn({nx, row.y + 35, nw, 19}, iz, 12, 400, p.ochiq ? M->ogoh : M->matn3);
        }
        ui.scroll_tugat();
    }
    y += rb + 16;
    ui.belgi("pr/har", {x, y, w, 24}, L"Har safar dastur ochilganda so'ralsin", har_safar);
    y += 24 + 16;
    bool bos = ui.tugma("pr/davom", {x, y, w, 50}, pr_boshida ? L"Davom etish" : L"Saqlash", YASHIL);
    if (ui.tugmacha(VK_RETURN)) bos = true;
    y += 50 + 12;
    gfx.matn({x, y, w, 34},
             L"Yangi printer ulansa yoki tanlangan printer o'chirilsa, dastur ochilganda yana so'raladi. "
             L"Keyin «Qurilmalar» bo'limida ham o'zgartirsa bo'ladi.",
             12, 400, M->matn3, MARKAZ, 0, true);
    y += 34 + 16;
    gfx.matn({x, y, w, 16}, L"v" + versiya(), 11, 400, M->matn3, MARKAZ);
    if (bos) {
        saqla();
        printerlar_tayyor();
    }
}
