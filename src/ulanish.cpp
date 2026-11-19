// Serverga ulanish sahifasi (kassa) va «Xozmag Server» oynasi.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "aloqa.h"
#include "ilova.h"

#include <commdlg.h>
#include <shlobj.h>

#include <algorithm>

// ======================= Kassa: server manzili =======================

static wstring ulanish_xato;
static bool ulanish_fokus = true, ulanmoqda = false;

void ulanish_ochildi(const wstring& xato) {
    ulanish_xato = xato;
    ulanish_fokus = true;
}

void ulanish_chiz(R r) {
    float en = std::min(400.f, r.w - 28);
    float boy = 440 + (ulanish_xato.empty() ? 0 : 34);
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
    gfx.matn({x, y, w, 18}, L"Serverga ulanish", 13, 400, M->matn2, MARKAZ);
    y += 18 + 16;
    gfx.matn({x, y, w, 17}, L"Server IP manzili", 12, 600, M->matn2);
    y += 17 + 5;
    MaydonSoz s;
    s.ishora = L"192.168.1.5";
    ui.maydon("ulanish/ip", {x, y, w, 50}, s);
    if (ulanish_fokus) {
        if (ui.qiymat("ulanish/ip").empty()) ui.qiymat_qoy("ulanish/ip", ini_qiymat(L"server"));
        ui.fokus("ulanish/ip");
        ulanish_fokus = false;
    }
    y += 50 + 14;
    if (!ulanish_xato.empty()) {
        gfx.matn({x, y - 6, w, 34}, ulanish_xato, 12, 500, M->kechki ? hex("#ffa0a0") : M->xato, CHAP, 0, true);
        y += 34;
    }
    bool bos = ui.tugma("ulanish/tugma", {x, y, w, 50}, ulanmoqda ? L"Ulanmoqda..." : L"Ulanish", YASHIL);
    if (ui.enter("ulanish/ip") || (!ui.fokusda("ulanish/ip") && ui.tugmacha(VK_RETURN))) bos = true;
    y += 50 + 16;
    gfx.matn({x, y, w, 70},
             L"Do'kondagi server kompyuterida «Xozmag Server» ochiq turishi kerak: IP manzil o'sha "
             L"oynada yozilgan. Server shu kompyuterning o'zi bo'lsa — 127.0.0.1",
             12, 400, M->matn3, MARKAZ, 0, true);
    y += 70 + 8;
    gfx.matn({x, y, w, 16}, L"v" + versiya(), 11, 400, M->matn3, MARKAZ);
    if (!bos || ulanmoqda) return;
    wstring manzil = tozala(ui.qiymat("ulanish/ip"));
    if (manzil.empty()) {
        ulanish_xato = L"IP manzilni yozing.";
        return;
    }
    ulanmoqda = true;
    HCURSOR k = SetCursor(LoadCursor(nullptr, IDC_WAIT));
    wstring xato;
    bool ok = ulan(manzil, xato);
    SetCursor(k);
    ulanmoqda = false;
    if (!ok) {
        ulanish_xato = manzil + L" ga ulanib bo'lmadi: " + xato;
        ui.fokus("ulanish/ip");
        return;
    }
    ulanish_xato.clear();
    ulanganda();
}

// ======================= Server oynasi =======================

static DWORD buyruq(const wstring& cmd) {
    STARTUPINFOW si = {sizeof si};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    wstring c = cmd;
    if (!CreateProcessW(nullptr, c.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        return (DWORD)-1;
    WaitForSingleObject(pi.hProcess, 20000);
    DWORD k = (DWORD)-1;
    GetExitCodeProcess(pi.hProcess, &k);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return k;
}

static wstring men_exe() {
    wchar_t b[MAX_PATH];
    GetModuleFileNameW(nullptr, b, MAX_PATH);
    return b;
}

bool administratormi() {
    HANDLE t = nullptr;
    TOKEN_ELEVATION e = {};
    DWORD n = 0;
    bool ha = false;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &t)) {
        if (GetTokenInformation(t, TokenElevation, &e, sizeof e, &n)) ha = e.TokenIsElevated != 0;
        CloseHandle(t);
    }
    return ha;
}

// Kassalar ulana olishi uchun Windows xavfsizlik devorida ruxsat (bir marta qo'shiladi)
void xavfsizlik_devori() {
    if (!administratormi()) return;
    const wchar_t* QOIDA = L"Xozmag Server";
    if (buyruq(L"netsh advfirewall firewall show rule name=\"" + wstring(QOIDA) + L"\"") == 0) return;
    buyruq(L"netsh advfirewall firewall add rule name=\"" + wstring(QOIDA) +
           L"\" dir=in action=allow protocol=TCP profile=any enable=yes program=\"" + men_exe() + L"\"");
}

static const wchar_t* VAZIFA = L"Xozmag Server";
static int avto = -1;  // -1 noma'lum, 0 yo'q, 1 bor

static void avto_tekshir() {
    avto = buyruq(L"schtasks /Query /TN \"" + wstring(VAZIFA) + L"\"") == 0 ? 1 : 0;
}

static bool avto_qoy(bool yoq) {
    if (yoq)
        return buyruq(L"schtasks /Create /F /TN \"" + wstring(VAZIFA) + L"\" /SC ONLOGON /RL HIGHEST /TR \"\\\"" +
                      men_exe() + L"\\\" --server\"") == 0;
    return buyruq(L"schtasks /Delete /F /TN \"" + wstring(VAZIFA) + L"\"") == 0;
}

static std::vector<wstring> ip_lar;
static DWORD ip_vaqt = 0;

static void ip_yangila() {
    if (!ip_lar.empty() && GetTickCount() - ip_vaqt < 10000) return;
    ip_vaqt = GetTickCount();
    ip_lar.clear();
    ULONG n = 32 * 1024;
    std::vector<unsigned char> b(n);
    auto* a = (IP_ADAPTER_ADDRESSES*)b.data();
    ULONG bay = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    if (GetAdaptersAddresses(AF_INET, bay, nullptr, a, &n) == ERROR_BUFFER_OVERFLOW) {
        b.resize(n);
        a = (IP_ADAPTER_ADDRESSES*)b.data();
    }
    if (GetAdaptersAddresses(AF_INET, bay, nullptr, a, &n) != NO_ERROR) return;
    for (; a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        for (auto* u = a->FirstUnicastAddress; u; u = u->Next) {
            wchar_t s[64] = {};
            InetNtopW(AF_INET, &((sockaddr_in*)u->Address.lpSockaddr)->sin_addr, s, 64);
            wstring ip = s;
            if (ip.rfind(L"169.254.", 0) == 0) continue;  // tarmoq yo'q (APIPA)
            // Ethernet / Wi-Fi birinchi, virtual adapterlar keyin
            bool asosiy = a->IfType == IF_TYPE_ETHERNET_CSMACD || a->IfType == IF_TYPE_IEEE80211;
            wstring nom = a->FriendlyName ? a->FriendlyName : L"";
            if (asosiy) ip_lar.insert(ip_lar.begin(), ip + L"\t" + nom);
            else ip_lar.push_back(ip + L"\t" + nom);
        }
    }
}

int server_porti = STANDART_PORT;

static void server_baza_tanla() {
    wchar_t fayl[MAX_PATH] = {};
    OPENFILENAMEW o = {sizeof o};
    o.hwndOwner = ui.h;
    o.lpstrFilter = L"Xozmag bazasi (*.sqlite3)\0*.sqlite3\0Hamma fayllar\0*.*\0";
    o.lpstrFile = fayl;
    o.nMaxFile = MAX_PATH;
    o.lpstrTitle = L"Baza faylini tanlang (sayt bazasi: BizaroPOS-Server\\db.sqlite3)";
    o.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&o)) return;
    if (!server_mijozlari().empty() &&
        MessageBoxW(ui.h, L"Kassalar ulangan. Baza almashtirilsa ular bir lahzaga uziladi va qayta ulanadi. Davom etilsinmi?",
                    L"Xozmag Server", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;
    wstring xato;
    if (server_baza_almashtir(fayl, xato)) {
        baza_yolini_saqla(fayl);
        ui.xabar(L"Baza almashtirildi.");
    } else
        ui.xabar(L"Bazani ochib bo'lmadi: " + xato, X_XATO);
}

static wstring soat(long long t) {
    time_t v = (time_t)t;
    tm m;
    localtime_s(&m, &v);
    wchar_t b[32];
    swprintf(b, 32, L"%02d:%02d", m.tm_hour, m.tm_min);
    return b;
}

void server_chiz(R r) {
    ip_yangila();
    if (avto < 0) avto_tekshir();
    auto mijozlar = server_mijozlari();
    float en = std::min(760.f, r.w - 28);
    float x0 = r.x + (r.w - en) / 2;
    float scr_boy = r.h;
    float y = r.y + 24;
    // Sarlavha
    R logo = {x0, y, 46, 46};
    gfx.gradient_quti(logo, M->urgu, M->urgu_quyuq, 12);
    gfx.ikon("i-dokon", {logo.x + 10, logo.y + 10, 26, 26}, M->asosiy_matn);
    gfx.matn({x0 + 60, y - 1, en - 60, 28}, L"Xozmag Server", 22, 700, M->matn);
    gfx.matn({x0 + 60, y + 27, en - 60, 18},
             L"Baza shu kompyuterda. Kassalar shu kompyuterga IP manzil orqali ulanadi.", 13, 400, M->matn2);
    ui.nishon(x0 + en, y + 12, L"Ishlamoqda", M->tasdiq_fon, M->tasdiq_chek, M->tasdiq, true);
    y += 46 + 20;

    // IP manzillar
    float ipb = 64 + std::max<size_t>(ip_lar.size(), 1) * 44 + 34;
    R k = {x0, y, en, ipb};
    quti(k);
    float x = k.x + 18, w = k.w - 36, yy = k.y + 16;
    gfx.matn({x, yy, w, 18}, L"Kassalarda «Server IP manzili» ga shuni yozing:", 13, 600, M->matn2);
    yy += 30;
    if (ip_lar.empty()) {
        gfx.matn({x, yy, w, 30}, L"Tarmoq topilmadi — kabel yoki Wi-Fi ulanganini tekshiring.", 15, 600, M->ogoh);
        yy += 44;
    }
    wstring port = server_porti == STANDART_PORT ? L"" : L":" + std::to_wstring(server_porti);
    for (auto& s : ip_lar) {
        size_t t = s.find(L'\t');
        wstring ip = s.substr(0, t), nom = s.substr(t + 1);
        gfx.quti({x, yy, w, 38}, M->panel2, 9, M->chegara);
        gfx.matn({x + 14, yy, 300, 38}, ip + port, 22, 700, M->urgu_matn);
        gfx.matn({x + 300, yy, w - 314, 38}, nom, 12, 500, M->matn3, ONG);
        yy += 44;
    }
    gfx.matn({x, yy + 2, w, 18}, L"Shu kompyuterning o'zida: 127.0.0.1" + port + L"   ·   port " +
             std::to_wstring(server_porti), 12, 400, M->matn3);
    y += ipb + 14;

    // Baza
    R b = {x0, y, en, 104};
    quti(b);
    gfx.matn({b.x + 18, b.y + 14, b.w - 36, 18}, L"Baza", 13, 600, M->matn2);
    gfx.matn({b.x + 18, b.y + 36, b.w - 36 - 170, 50}, baza.fayl(), 13, 500, M->matn, CHAP, 0, true);
    if (ui.tugma("server/baza", {b.o() - 18 - 150, b.y + 36, 150, 40}, L"Bazani tanlash", TINCH, nullptr, true))
        server_baza_tanla();
    y += 104 + 14;

    // Avtomatik ishga tushish
    R a = {x0, y, en, 60};
    quti(a);
    bool v = avto == 1;
    if (ui.belgi("server/avto", {a.x + 18, a.y + 18, a.w - 36, 24},
                 L"Kompyuter yoqilganda server o'zi ishga tushsin", v)) {
        if (!administratormi()) ui.xabar(L"Buning uchun dastur administrator sifatida ochilishi kerak.", X_XATO);
        else if (avto_qoy(v)) ui.xabar(v ? L"Server endi kompyuter bilan birga ochiladi." : L"Avtomatik ochilish o'chirildi.");
        else ui.xabar(L"Windows vazifasini o'zgartirib bo'lmadi.", X_XATO);
        avto_tekshir();
    }
    y += 60 + 14;

    // Ulangan kassalar
    float qb = 50 + std::max<size_t>(mijozlar.size(), 1) * 40 + 10;
    R m = {x0, y, en, std::max(qb, 100.f)};
    quti(m);
    gfx.matn({m.x + 18, m.y + 14, m.w - 36, 18},
             L"Ulangan kassalar: " + std::to_wstring(mijozlar.size()), 13, 600, M->matn2);
    float my = m.y + 44;
    if (mijozlar.empty())
        gfx.matn({m.x + 18, my, m.w - 36, 30}, L"Hozircha hech kim ulanmagan.", 14, 400, M->matn3);
    for (auto& c : mijozlar) {
        gfx.chiziq(m.x + 18, my - .5f, m.o() - 18, my - .5f, M->chegara);
        gfx.matn({m.x + 18, my, 180, 40}, c.ip, 15, 600, M->matn);
        gfx.matn({m.x + 200, my, 220, 40}, c.login.empty() ? L"kirmagan" : c.login, 14, 500,
                 c.login.empty() ? M->matn3 : M->matn2);
        gfx.matn({m.x + 420, my, m.w - 438, 40}, soat(c.ulangan) + L" dan beri · " + std::to_wstring(c.sorovlar) +
                 L" so'rov", 12, 400, M->matn3, ONG);
        my += 40;
    }
    y += m.h + 14;
    gfx.matn({x0, y, en, 16}, L"Oynani yopsangiz server to'xtaydi.   v" + versiya(), 11, 400, M->matn3, MARKAZ);
    (void)scr_boy;
}
