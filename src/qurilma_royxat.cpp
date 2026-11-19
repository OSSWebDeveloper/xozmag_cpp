// Qurilmalar sahifasining saytdagi qismlari (templates/qurilmalar.html, config/qurilmalar.py,
// static/js/qurilmalar.js, rongta.js dagi skaner sinovi):
//  - «Shu kassa» qutisi;
//  - «Skaner va etiketkani sinash» — kod/etiketka qidiruvi, savdo yaratmaydi;
//  - shu kompyuterga ulangan USB qurilmalar (har 10 soniyada; yangi ulangani «Yangi»).
// Sayt ro'yxatni PowerShell Get-PnpDevice bilan oladi; exe o'sha ma'lumotni SetupAPI dan oladi
// va saytdagi `sarala()` qoidalari bilan saralaydi.
#include <winsock2.h>

#include "aloqa.h"
#include "chek_chop.h"
#include "ilova.h"

#include <initguid.h>
#include <devpkey.h>
#include <setupapi.h>
#include <cfgmgr32.h>

#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <regex>
#include <set>
#include <thread>

long long chek_kodini_och(const wstring& matn);  // qaytarish.cpp

namespace {

struct Qurilma {
    wstring id, nom, tur, sinf, port, holat;
    bool tayyor = false;
};

wstring kichik_h(wstring s) {
    if (!s.empty()) CharLowerBuffW(s.data(), (DWORD)s.size());
    return s;
}

bool mos(const wstring& matn, const wchar_t* naqsh) {
    return std::regex_search(matn, std::wregex(naqsh, std::regex::ECMAScript));
}

bool boshlanadi(const wstring& s, const wchar_t* b) { return s.rfind(b, 0) == 0; }

wstring xususiyat(HDEVINFO h, SP_DEVINFO_DATA& d, DWORD p) {
    wchar_t buf[1024];
    DWORD tur = 0;
    if (!SetupDiGetDeviceRegistryPropertyW(h, &d, p, &tur, (BYTE*)buf, sizeof buf - 2, nullptr)) return L"";
    buf[1023] = 0;
    return buf;
}

// config/qurilmalar.py: sarala() — klaviatura/sichqoncha va ichki tizim qismlari chiqariladi
bool saralash(const wstring& ident, const wstring& nom, const wstring& sinf, Qurilma& q) {
    wstring text = kichik_h(nom), cls = kichik_h(sinf), idu = ident;
    CharUpperBuffW(idu.data(), (DWORD)idu.size());
    bool skaner = mos(text, L"barcode|bar.?code|scanner|сканер|skaner|honeywell|datalogic|symbol");
    bool tarozi = mos(text, L"\\bscale\\b|weigh|tarozi|весы|масса-к");
    if (ident.empty()) return false;
    if (!(skaner || tarozi)) {
        if (cls == L"keyboard" || cls == L"mouse") return false;
        if (mos(text, L"keyboard|mouse|touchpad|klaviatura|sichqon|клавиатур|мыш")) return false;
        if (cls == L"hidclass") return false;
        if (mos(text, L"root hub|usb hub|host controller|composite device|usb input device|корневой usb|"
                      L"usb-концентратор|составное usb|usb-устройство ввода"))
            return false;
        if (cls == L"printqueue" &&
            ((idu.size() >= 11 && idu.compare(idu.size() - 11, 11, L"PRINTQUEUES") == 0) || mos(text, L"\\bpdf\\b|xps")))
            return false;
        bool tashqi = boshlanadi(idu, L"USB\\") || boshlanadi(idu, L"USBSTOR\\") || boshlanadi(idu, L"BTHENUM\\") ||
                      boshlanadi(idu, L"BTHLEDEVICE\\");
        static const std::set<wstring> pereferiya = {L"ports", L"printer", L"printqueue", L"image",
                                                     L"camera", L"wpd", L"smartcardreader"};
        if (!tashqi && !pereferiya.count(cls)) return false;
        if (cls == L"system" || cls == L"processor" || cls == L"softwaredevice" || cls == L"volume" ||
            cls == L"computer")
            return false;
    }
    q.tur = skaner ? L"Shtrix kod skaneri"
          : tarozi ? L"Elektron tarozi"
          : cls == L"ports" ? L"COM / LPT port"
          : (cls == L"printer" || cls == L"printqueue") ? L"Printer"
          : (cls == L"camera" || cls == L"image") ? L"Kamera / tasvir qurilmasi"
                                                  : L"Boshqa tashqi qurilma";
    std::wsmatch m;
    wstring nomk = nom;
    if (std::regex_search(nomk, m, std::wregex(L"\\b(?:COM|LPT)\\d+\\b", std::regex::icase))) {
        q.port = m.str(0);
        CharUpperBuffW(q.port.data(), (DWORD)q.port.size());
    }
    q.id = ident;
    q.nom = nom;
    q.sinf = sinf.empty() ? L"Noma’lum" : sinf;
    return true;
}

std::vector<Qurilma> aniqlash(wstring& xato) {
    std::vector<Qurilma> r;
    // Saytdagi Get-PnpDevice nomlarni inglizcha beradi; ruscha Windows'da SetupAPI esa tarjimasini.
    // Bu ish alohida oqimda — faqat shu oqim tili o'zgaradi (sinovda 294 ta qurilma nomi aynan mos).
    SetThreadUILanguage(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
    HDEVINFO h = SetupDiGetClassDevsW(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (h == INVALID_HANDLE_VALUE) {
        xato = L"Windows qurilmalar ro‘yxatini olib bo‘lmadi. Qayta yangilang.";
        return r;
    }
    std::map<wstring, Qurilma> yigma;
    SP_DEVINFO_DATA d{sizeof d};
    for (DWORD i = 0; SetupDiEnumDeviceInfo(h, i, &d); ++i) {
        wchar_t idb[512];
        if (!SetupDiGetDeviceInstanceIdW(h, &d, idb, 512, nullptr)) continue;
        wstring ident = idb;
        wstring sinf = xususiyat(h, d, SPDRP_CLASS);
        wstring nom = xususiyat(h, d, SPDRP_FRIENDLYNAME);
        if (nom.empty()) nom = xususiyat(h, d, SPDRP_DEVICEDESC);
        // USB qurilmaning o'zi aytgan nomi (DEVPKEY_Device_BusReportedDeviceDesc) ustun
        if (boshlanadi(ident, L"USB\\")) {
            wchar_t b[512];
            DEVPROPTYPE t = 0;
            if (SetupDiGetDevicePropertyW(h, &d, &DEVPKEY_Device_BusReportedDeviceDesc, &t, (BYTE*)b, sizeof b - 2,
                                          nullptr, 0) && t == DEVPROP_TYPE_STRING && b[0])
                nom = b;
        }
        if (nom.empty()) nom = L"Noma’lum qurilma";
        Qurilma q;
        if (!saralash(ident, nom, sinf, q)) continue;
        ULONG st = 0, prob = 0;
        bool ok = CM_Get_DevNode_Status(&st, &prob, d.DevInst, 0) == CR_SUCCESS && !(st & DN_HAS_PROBLEM);
        q.tayyor = ok;
        q.holat = ok ? L"Windows aniqlagan" : L"Windows: tekshirish kerak";
        yigma[q.id] = q;
    }
    SetupDiDestroyDeviceInfoList(h);
    for (auto& [k, v] : yigma) r.push_back(v);
    std::sort(r.begin(), r.end(), [](const Qurilma& a, const Qurilma& b) {
        if (a.tur != b.tur) return a.tur < b.tur;
        if (a.nom != b.nom) return a.nom < b.nom;
        return a.id < b.id;
    });
    return r;
}

// ---- holat ----
std::mutex qm;
std::atomic<bool> band{false};
bool natija_bor = false;       // oqim yangi ro'yxat qo'ydi (qm ostida)
std::vector<Qurilma> yangi_r;  // oqim natijasi (qm ostida)
wstring yangi_xato;

std::vector<Qurilma> royxat;
bool olingan = false;          // kamida bir marta olindi
std::map<wstring, wstring> oldingi;  // id -> nom
std::map<wstring, ULONGLONG> yangilar;  // id -> «Yangi» belgisi tugash vaqti
std::set<wstring> ochiq_id;  // «Qurilma identifikatori» ochilganlar
wstring xabar, xato, vaqt_yozuv = L"Ro‘yxat olinmoqda…";
ULONGLONG oxirgi = 0;

wstring skan_natija = L"Hali sinov qilinmadi.";

void boshla() {
    if (band.exchange(true)) return;
    oxirgi = GetTickCount64();
    HWND oyna = ui.h;
    std::thread([oyna] {
        wstring x;
        auto r = aniqlash(x);
        {
            std::lock_guard<std::mutex> l(qm);
            yangi_r = std::move(r);
            yangi_xato = x;
            natija_bor = true;
        }
        band = false;
        InvalidateRect(oyna, nullptr, FALSE);
    }).detach();
}

void natijani_ol() {
    std::vector<Qurilma> r;
    wstring x;
    {
        std::lock_guard<std::mutex> l(qm);
        if (!natija_bor) return;
        natija_bor = false;
        r = std::move(yangi_r);
        x = yangi_xato;
    }
    if (!x.empty()) {
        xato = x;
        vaqt_yozuv = olingan ? L"Yangilanmadi. Quyida oxirgi olingan ro‘yxat." : L"Qurilmalar aniqlanmadi.";
        return;
    }
    ULONGLONG hozir = GetTickCount64();
    std::map<wstring, wstring> joriy;
    for (auto& q : r) joriy[q.id] = q.nom;
    std::vector<wstring> qoshildi, uzildi;
    if (olingan) {
        for (auto& q : r)
            if (!oldingi.count(q.id)) {
                qoshildi.push_back(q.nom);
                yangilar[q.id] = hozir + 60000;
            }
        for (auto& [id, nom] : oldingi)
            if (!joriy.count(id)) uzildi.push_back(nom);
    }
    for (auto it = yangilar.begin(); it != yangilar.end();)
        if (it->second < hozir || !joriy.count(it->first)) it = yangilar.erase(it);
        else ++it;
    if (!qoshildi.empty() || !uzildi.empty()) {
        auto bir = [](const std::vector<wstring>& v) {
            wstring s;
            for (auto& x : v) s += (s.empty() ? L"" : L", ") + x;
            return s;
        };
        xabar.clear();
        if (!qoshildi.empty()) xabar = L"Yangi ulandi: " + bir(qoshildi);
        if (!uzildi.empty()) xabar += (xabar.empty() ? L"" : L". ") + (L"Uzildi: " + bir(uzildi));
    }
    royxat = std::move(r);
    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t v[16];
    swprintf(v, 16, L"%02d:%02d:%02d", t.wHour, t.wMinute, t.wSecond);
    vaqt_yozuv = std::to_wstring(royxat.size()) + L" ta qurilma · Tekshirildi: " + v;
    xato.clear();
    oldingi = std::move(joriy);
    olingan = true;
}

float matn_q(float x, float y, float w, const wstring& m, float o, int q, Rang r) {
    float h = gfx.matn_boyi(m, o, q, w);
    gfx.matn({x, y, w, h}, m, o, q, r, CHAP, 0, true);
    return h;
}

// Bitta qurilma kartasi (balandlikni qaytaradi; chiz=false — faqat o'lchash)
float karta(const Qurilma& q, R r, bool chiz) {
    float iw = r.w - 28, y = 14;
    bool yangi = yangilar.count(q.id) != 0;
    float h_nom = gfx.matn_boyi(q.nom, 19, 700, iw);
    wstring sinf = L"Sinf: " + q.sinf + (q.port.empty() ? L"" : L" · Port: " + q.port);
    float h_sinf = gfx.matn_boyi(sinf, 13, 400, iw);
    bool ochiq = ochiq_id.count(q.id) != 0;
    // overflow-wrap: anywhere — har belgidan keyin ko'rinmas bo'linish nuqtasi
    wstring idb;
    for (wchar_t c : q.id) { idb += c; idb += (wchar_t)0x200B; }
    float h_id = ochiq ? gfx.matn_boyi(idb, 12, 400, iw) + 10 : 0;
    float boy = 14 + 18 + 12 + h_nom + 12 + 20 + 12 + h_sinf + 12 + 20 + h_id + 14;
    if (!chiz) return boy;
    // CSS grid: bir qatordagi kartalar eng balandiga cho'ziladi
    gfx.quti({r.x, r.y, r.w, std::max(boy, r.h)}, M->panel, 12, M->chegara);
    float x = r.x + 14;
    y = r.y + 14;
    gfx.matn({x, y, iw, 18}, q.tur, 13, 400, M->matn2);
    if (yangi) {
        float nw = gfx.matn_eni(L"Yangi", 11, 700) + 16;
        ui.nishon(r.o() - 14 - nw, y, L"Yangi", M->tasdiq_fon, M->tasdiq_chek, M->tasdiq);
    }
    y += 18 + 12;
    y += matn_q(x, y, iw, q.nom, 19, 700, M->matn) + 12;
    gfx.matn({x, y, iw, 20}, q.holat, 14, 500, q.tayyor ? hex("#4ade80") : hex("#f59e0b"));
    y += 20 + 12;
    y += matn_q(x, y, iw, sinf, 13, 400, M->matn2) + 12;
    wstring sar = (ochiq ? L"▾ " : L"▸ ") + wstring(L"Qurilma identifikatori");
    R sr = {x, y, gfx.matn_eni(sar, 13, 500) + 4, 20};
    bool hov = ui.ustida(sr);
    gfx.matn(sr, sar, 13, 500, hov ? M->matn : M->matn2);
    if (ui.bosildi("qu/id/" + U(q.id), sr)) {
        if (ochiq) ochiq_id.erase(q.id);
        else ochiq_id.insert(q.id);
        ui.qayta = true;
    }
    y += 20;
    if (ochiq) matn_q(x, y + 10, iw, idb, 12, 400, M->matn3);
    return boy;
}

}  // namespace

void qurilma_royxat_ochildi() {
    skan_natija = L"Hali sinov qilinmadi.";
    ui.qiymat_qoy("qu/skan", L"");
    xabar.clear();
    boshla();
}

// «Shu kassa» qutisi (qurilmalar.html dagi birinchi bo'lim)
float shu_kassa_panel(float x0, float y0, float w, bool chiz) {
    float iw = w - 28, x = x0 + 14, y = y0 + 14;
    wchar_t komp[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD n = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameW(komp, &n)) komp[0] = 0;
    wstring chek = ini_qiymat(L"chek_printer"), etk = ini_qiymat(L"etiketka_printer");
    wstring p1 = L"Har bir kassaning USB printeri va skaneri o‘z kompyuterida ishlaydi. Tarozi va mahsulot bazasi umumiy.";
    wstring p2 = L"Kompyuter: " + wstring(komp[0] ? komp : L"Noma’lum") + L" · Chek printeri: " +
                 (chek.empty() ? L"Tanlanmagan" : chek) + L" · Etiketka printeri: " +
                 (etk.empty() ? L"Tanlanmagan" : etk) + L" · Server: " + ulangan_manzil();
    wstring p3 = L"Dastur chek va yorliqni shu kompyuterning printeriga to‘g‘ridan-to‘g‘ri chiqaradi, "
                 L"shuning uchun saytdagi USB kassa yordamchisi kerak emas.";
    float h1 = gfx.matn_boyi(p1, 14, 400, iw), h2 = gfx.matn_boyi(p2, 14, 400, iw), h3 = gfx.matn_boyi(p3, 13, 400, iw);
    float boy = 14 + 26 + 10 + h1 + 10 + h2 + 10 + h3 + 14;
    if (!chiz) return boy;
    quti({x0, y0, w, boy});
    gfx.matn({x, y, iw, 26}, L"Shu kassa · " + foydalanuvchi.login, 20, 700, M->matn);
    y += 26 + 10;
    y += matn_q(x, y, iw, p1, 14, 400, M->matn) + 10;
    y += matn_q(x, y, iw, p2, 14, 400, M->matn) + 10;
    matn_q(x, y, iw, p3, 13, 400, M->matn2);
    return boy;
}

// «Skaner va etiketkani sinash» — saytdagi ombor:kod_qidir bilan bir xil qidiruv, savdo yaratmaydi
float skaner_panel(float x0, float y0, float w, bool chiz) {
    float iw = w - 28, x = x0 + 14, y = y0 + 14;
    wstring p = L"Maydonni bosing va do‘kon yoki tarozi etiketkasini skanerlang. Bu tekshiruv savdo yaratmaydi.";
    float hp = gfx.matn_boyi(p, 14, 400, iw), hn = gfx.matn_boyi(skan_natija, 14, 500, iw);
    float boy = 14 + 26 + 10 + hp + 12 + 50 + 12 + hn + 14;
    if (!chiz) return boy;
    quti({x0, y0, w, boy});
    gfx.matn({x, y, iw, 26}, L"Skaner va etiketkani sinash", 20, 700, M->matn);
    y += 26 + 10;
    y += matn_q(x, y, iw, p, 14, 400, M->matn) + 12;
    float bw = gfx.matn_eni(L"Tekshirish", 15, 600) + 36;
    MaydonSoz m;
    m.ishora = L"Etiketkani skanerlang yoki kod yozing";
    HWND hm = ui.maydon("qu/skan", {x, y, iw - bw - 14, 50}, m);
    bool tek = ui.tugma("qu/tek", {x + iw - bw, y, bw, 50}, L"Tekshirish") || ui.enter("qu/skan");
    y += 50 + 12;
    if (tek) {
        wstring k = tozala(ui.qiymat("qu/skan"));
        if (!k.empty()) {
            try {
                long long cid = chek_kodini_och(k);
                if (cid) {
                    Sorov s = baza.sor("SELECT 1 FROM sotuv_sotuv WHERE id=? AND yakunlangan=1 AND bekor_qilingan=0");
                    s.bog(1, cid);
                    skan_natija = s.qator() ? L"Savdo cheki #" + std::to_wstring(cid) +
                                                  L" — qaytarish uchun Sotuvlar → «Chekni skanerlab qaytarish»."
                                            : L"Savdo cheki topilmadi yoki bekor qilingan.";
                } else {
                    Tovar t;
                    double miqdor = -1;
                    wstring xt;
                    if (kod_bilan_top(k, t, miqdor, xt))
                        skan_natija = L"Kod " + t.kod + L": " + t.nom +
                                      (miqdor >= 0 ? L" · Miqdor: " + son(miqdor) + L" " + t.birlik : L"");
                    else
                        skan_natija = xt;
                }
            } catch (const BazaXato& e) {
                skan_natija = W(e.what());
            }
            if (hm) SendMessageW(hm, EM_SETSEL, 0, -1);
            ui.fokus("qu/skan");
            ui.qayta = true;
        }
    }
    matn_q(x, y, iw, skan_natija, 14, 500, M->matn);
    return boy;
}

// USB qurilmalar: sarlavha qutisi, kartalar to'ri, izoh. Balandlikni qaytaradi.
float usb_panel(float x0, float y0, float w, bool chiz) {
    natijani_ol();
    if (chiz) {
        SetTimer(ui.h, 82, 1000, nullptr);  // oqim natijasi va 10 soniyalik tekshiruv uchun uyg'otish
        if (!band && GetTickCount64() - oxirgi >= 10000) boshla();
    }
    float iw = w - 28, x = x0 + 14;
    wstring p1 = L"Shu kassaga ulangan USB qurilmalar. Klaviatura va sichqoncha ko‘rsatilmaydi.";
    wstring p2 = L"Har 10 soniyada tekshiriladi. Shu sahifa ochiq turganda yangi ulangan qurilma «Yangi» deb belgilanadi.";
    float h1 = gfx.matn_boyi(p1, 14, 400, iw), h2 = gfx.matn_boyi(p2, 13, 400, iw);
    float b1 = 14 + h1 + 12 + 50 + 12 + h2 + 14;
    float y = y0;
    if (chiz) {
        quti({x0, y, w, b1});
        float yy = y + 14;
        yy += matn_q(x, yy, iw, p1, 14, 400, M->matn) + 12;
        float bw = gfx.matn_eni(L"Yangilash", 15, 600) + 36;
        if (ui.tugma("qu/yangila", {x, yy, bw, 50}, band ? L"Yangilanmoqda…" : L"Yangilash") && !band) boshla();
        gfx.matn({x + bw + 14, yy, iw - bw - 14, 50}, vaqt_yozuv, 13, 400, M->matn2);
        yy += 50 + 12;
        matn_q(x, yy, iw, p2, 13, 400, M->matn2);
    }
    y += b1;
    for (const wstring* m : {&xato, &xabar}) {
        if (m->empty()) continue;
        float h = gfx.matn_boyi(*m, 14, 500, w);
        if (chiz) gfx.matn({x0, y + 12, w, h}, *m, 14, 500, m == &xato ? hex("#f59e0b") : M->matn, CHAP, 0, true);
        y += 12 + h;
    }
    y += 18;
    // Kartalar to'ri: repeat(auto-fit, minmax(310px, 1fr)), oraliq 14
    int ust = std::max(1, (int)((w + 14) / (310 + 14)));
    float kw = (w - 14 * (ust - 1)) / ust;
    if (royxat.empty() && olingan) {
        wstring m = L"Klaviatura va sichqonchadan boshqa tashqi qurilma topilmadi.";
        if (chiz) {
            quti({x0, y, w, 50});
            gfx.matn({x, y, iw, 50}, m, 14, 400, M->matn);
        }
        y += 50 + 18;
    } else {
        for (size_t i = 0; i < royxat.size(); i += ust) {
            float qb = 0;
            for (size_t j = i; j < std::min(royxat.size(), i + ust); ++j)
                qb = std::max(qb, karta(royxat[j], {0, 0, kw, 0}, false));
            if (chiz)
                for (size_t j = i; j < std::min(royxat.size(), i + ust); ++j)
                    karta(royxat[j], {x0 + (j - i) * (kw + 14), y, kw, qb}, true);
            y += qb + 14;
        }
        if (!royxat.empty()) y += 4;
    }
    // .qurilma-izoh
    wstring i1 = L"Windows’da oddiy klaviatura deb tanilgan skaner avtomatik ajratilmaydi. Uni USB-COM rejimiga "
                 L"o‘tkazish yoki ishlab chiqaruvchi drayverini o‘rnatish kerak bo‘lishi mumkin. Umumiy USB/COM "
                 L"nomidagi qurilmaning turi nomidan aniqlanmasa, u boshqa qurilma yoki port sifatida ko‘rinadi.";
    wstring i2 = L"Quyidagi ro‘yxat qurilmaning Windows’da mavjudligini ko‘rsatadi. Ishlashini etiketka chiqarish "
                 L"va skanerlash bilan tekshiring. Ethernet tarozi yuqoridagi alohida panel orqali boshqariladi.";
    float g1 = gfx.matn_boyi(i1, 14, 400, iw), g2 = gfx.matn_boyi(i2, 14, 400, iw);
    float b2 = 14 + 20 + 10 + g1 + 12 + g2 + 14;
    if (chiz) {
        quti({x0, y, w, b2});
        float yy = y + 14;
        gfx.matn({x, yy, iw, 20}, L"Skaner yoki tarozi ko‘rinmasa", 15, 700, M->matn);
        yy += 20 + 10;
        yy += matn_q(x, yy, iw, i1, 14, 400, M->matn) + 12;
        matn_q(x, yy, iw, i2, 14, 400, M->matn);
    }
    y += b2;
    return y - y0;
}
