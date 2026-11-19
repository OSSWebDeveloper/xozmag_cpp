// «Kassalar» bo'limi — saytdagi config/kassalar.py + templates/kassalar.html:
// server va 2-kassa IP manzillari (qurilmalar_data/kassalar.json), 2-kassa dasturi paketi.
// Sozlama faqat server kompyuterida ochiladi. 2-kassa paketidagi ORNATISH.bat saytdagi kabi
// 2-kassa IP manzilini tarmoq kartasiga qo'shadi (config/kassa_ip.ps1) va dasturni o'rnatadi.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "aloqa.h"
#include "ilova.h"

#include <commdlg.h>
#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <algorithm>

namespace {

// Saytdagi config/kassa_ip.ps1 (o'zgarishsiz): IP qo'shimcha manzil bo'lib qo'shiladi, DHCP qoladi
const char* KASSA_IP_PS1 = R"PS(param([Parameter(Mandatory=$true)][string]$Address,[int]$InterfaceIndex=0)
$ErrorActionPreference='Stop'
$ip=[Net.IPAddress]::Parse($Address)
if ($ip.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork) {throw 'IPv4 kerak'}
if(-not $InterfaceIndex){
 $adapters=@(Get-NetAdapter -Physical | Where-Object Status -eq 'Up')
 $wifi=@($adapters | Where-Object {$_.NdisPhysicalMedium -eq 9})
 if($wifi.Count -eq 1){$InterfaceIndex=$wifi[0].ifIndex}
 elseif($adapters.Count -eq 1){$InterfaceIndex=$adapters[0].ifIndex}
 else {throw 'Bitta faol Wi-Fi/tarmoq kartasini tanlang'}
}
$before=Get-NetIPInterface -InterfaceIndex $InterfaceIndex -AddressFamily IPv4
foreach($store in @('active','persistent')) {
 & netsh interface ipv4 set interface interface=$InterfaceIndex dhcpstaticipcoexistence=enabled store=$store | Out-Null
 if($LASTEXITCODE){throw 'DHCP coexistence sozlanmadi. Administrator sifatida oching'}
}
if(-not(Get-NetIPAddress -InterfaceIndex $InterfaceIndex -IPAddress $Address -ErrorAction SilentlyContinue)) {
 & netsh interface ipv4 add address name=$InterfaceIndex address=$Address mask=255.255.255.0 skipassource=true store=persistent | Out-Null
 if($LASTEXITCODE){throw 'IP qoshilmadi. Administrator sifatida oching'}
}
if($before.Dhcp -eq 'Enabled') {
 Set-NetIPInterface -InterfaceIndex $InterfaceIndex -AddressFamily IPv4 -Dhcp Enabled
}
$limit=(Get-Date).AddSeconds(8)
do {$state=Get-NetIPAddress -InterfaceIndex $InterfaceIndex -IPAddress $Address; if($state.AddressState -eq 'Duplicate'){throw 'Bu IP boshqa qurilmada band'}; if($state.AddressState -eq 'Preferred'){break}; Start-Sleep -Milliseconds 300}while((Get-Date) -lt $limit)
if($state.AddressState -ne 'Preferred'){throw 'IP hali tayyor emas'}
Write-Output 'OK'
)PS";

struct Karta {
    unsigned indeks = 0;
    wstring nom;
    std::vector<wstring> ip;
};

bool ip_oqi(const wstring& s, unsigned& v) {
    IN_ADDR a;
    wstring t = tozala(s);
    if (t.empty() || InetPtonW(AF_INET, t.c_str(), &a) != 1) return false;
    // InetPton "1.2.3" ni ham qabul qilmaydi; qo'shimcha: aynan 4 bo'lak
    if (std::count(t.begin(), t.end(), L'.') != 3) return false;
    v = ntohl(a.S_un.S_addr);
    return true;
}

bool ichkimi(unsigned v) {
    unsigned a = v >> 24, b = (v >> 16) & 255;
    return a == 10 || (a == 172 && b >= 16 && b <= 31) || (a == 192 && b == 168);
}

std::vector<Karta> kartalar() {
    std::vector<Karta> r;
    ULONG n = 32 * 1024;
    std::vector<unsigned char> b(n);
    auto* a = (IP_ADAPTER_ADDRESSES*)b.data();
    ULONG bay = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    if (GetAdaptersAddresses(AF_INET, bay, nullptr, a, &n) == ERROR_BUFFER_OVERFLOW) {
        b.resize(n);
        a = (IP_ADAPTER_ADDRESSES*)b.data();
    }
    if (GetAdaptersAddresses(AF_INET, bay, nullptr, a, &n) != NO_ERROR) return r;
    for (; a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        Karta k;
        k.indeks = a->IfIndex;
        k.nom = a->FriendlyName ? a->FriendlyName : L"";
        for (auto* u = a->FirstUnicastAddress; u; u = u->Next) {
            wchar_t s[64] = {};
            InetNtopW(AF_INET, &((sockaddr_in*)u->Address.lpSockaddr)->sin_addr, s, 64);
            k.ip.push_back(s);
        }
        r.push_back(k);
    }
    return r;
}

// Shu kompyuterning ichki tarmoqdagi IPv4 manzillari (config/tarmoq.ip_manzillar)
std::vector<wstring> ichki_ipler() {
    std::vector<wstring> r;
    for (auto& k : kartalar())
        for (auto& ip : k.ip) {
            unsigned v;
            if (ip_oqi(ip, v) && ichkimi(v) && std::find(r.begin(), r.end(), ip) == r.end()) r.push_back(ip);
        }
    return r;
}

wstring json_fayl() { return malumot_papka() + L"\\qurilmalar_data\\kassalar.json"; }

string fayl_matni(const wstring& f) {
    HANDLE h = CreateFileW(f.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return "";
    DWORD n = GetFileSize(h, nullptr), o = 0;
    string s(n, 0);
    if (n) ReadFile(h, s.data(), n, &o, nullptr);
    CloseHandle(h);
    s.resize(o);
    return s;
}

bool fayl_yoz(const wstring& f, const string& s) {
    HANDLE h = CreateFileW(f.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD o = 0;
    bool ok = WriteFile(h, s.data(), (DWORD)s.size(), &o, nullptr) && o == s.size();
    CloseHandle(h);
    return ok;
}

string json_ol(const string& j, const string& k) {
    size_t p = j.find("\"" + k + "\"");
    if (p == string::npos) return "";
    p = j.find(':', p);
    if (p == string::npos) return "";
    ++p;
    while (p < j.size() && j[p] == ' ') ++p;
    if (p < j.size() && j[p] == '"') {
        size_t e = j.find('"', p + 1);
        return e == string::npos ? "" : j.substr(p + 1, e - p - 1);
    }
    size_t e = j.find_first_of(",}", p);
    return j.substr(p, e == string::npos ? string::npos : e - p);
}

struct Sozlama {
    wstring server_ip = L"192.168.153.25", client_ip = L"192.168.153.26";
    unsigned interface_ = 13;
};

Sozlama sozlama_ol() {
    Sozlama s;
    string j = fayl_matni(json_fayl());
    if (j.empty()) return s;
    s.server_ip = W(json_ol(j, "server_ip"));
    s.client_ip = W(json_ol(j, "client_ip"));
    s.interface_ = (unsigned)atoi(json_ol(j, "interface").c_str());
    return s;
}

// Saytdagi KassaForm tekshiruvi
bool tekshir(const wstring& a_m, const wstring& b_m, const wstring& i_m, Sozlama& s, wstring& xato) {
    unsigned a, b;
    if (!ip_oqi(a_m, a)) { xato = L"Server IP: to'g'ri IPv4 manzil kiriting."; return false; }
    if (!ip_oqi(b_m, b)) { xato = L"2-kassa IP: to'g'ri IPv4 manzil kiriting."; return false; }
    double ind = 0;
    if (!songa(i_m, ind) || ind != (long long)ind) { xato = L"Tarmoq kartasi indeksi: butun son kiriting."; return false; }
    if (ind < 1) { xato = L"Tarmoq kartasi indeksi: qiymat 1 dan kichik bo'lmasin."; return false; }
    unsigned tarmoq = a & 0xFFFFFF00u;
    auto loop_link = [](unsigned v) { return (v >> 24) == 127 || (v >> 16) == 0xA9FE; };
    if (!ichkimi(tarmoq) || loop_link(a) || loop_link(b)) { xato = L"Mahalliy tarmoq IPv4 manzilini kiriting."; return false; }
    if (a == b || (b & 0xFFFFFF00u) != tarmoq || (a & 255) == 0 || (a & 255) == 255 || (b & 255) == 0 || (b & 255) == 255) {
        xato = L"IP lar bir /24 tarmoqda, turlicha bo'lsin; .0 va .255 ishlatilmaydi.";
        return false;
    }
    s.server_ip = tozala(a_m);
    s.client_ip = tozala(b_m);
    s.interface_ = (unsigned)ind;
    return true;
}

wstring port_qismi() {
    wstring m = ulangan_manzil();
    size_t p = m.rfind(L':');
    return p == wstring::npos ? std::to_wstring(STANDART_PORT) : m.substr(p + 1);
}

// config/tarmoq.doimiy_manzil: sozlangan server IP haqiqatan kartada bo'lsagina o'sha, aks holda birinchisi
wstring doimiy_manzil() {
    auto bor = ichki_ipler();
    wstring ip = sozlama_ol().server_ip;
    if (std::find(bor.begin(), bor.end(), ip) == bor.end()) ip = bor.empty() ? L"" : bor[0];
    return ip.empty() ? L"" : ip + L":" + port_qismi();
}

bool serverdami() {
    wstring m = ulangan_manzil();
    size_t p = m.rfind(L':');
    wstring h = p == wstring::npos ? m : m.substr(0, p);
    if (h == L"127.0.0.1" || h == L"localhost" || h == L"::1" || h == L"[::1]") return true;
    for (auto& k : kartalar())
        for (auto& ip : k.ip)
            if (ip == h) return true;
    return false;
}

DWORD jarayon(const wstring& cmd, wstring* xato, DWORD kutish = 120000) {
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    wstring fx = wstring(tmp) + L"xozmag_jarayon_" + std::to_wstring(GetTickCount64()) + L".err";
    SECURITY_ATTRIBUTES sa = {sizeof sa, nullptr, TRUE};
    HANDLE hx = CreateFileW(fx.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, 0, nullptr);
    STARTUPINFOW si = {sizeof si};
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdError = hx;
    si.hStdOutput = nullptr;
    PROCESS_INFORMATION pi;
    wstring c = cmd;
    DWORD k = (DWORD)-1;
    if (CreateProcessW(nullptr, c.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        if (WaitForSingleObject(pi.hProcess, kutish) == WAIT_TIMEOUT) TerminateProcess(pi.hProcess, 1);
        GetExitCodeProcess(pi.hProcess, &k);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    if (hx != INVALID_HANDLE_VALUE) CloseHandle(hx);
    if (xato) {
        string e = fayl_matni(fx);
        // PowerShell xatosining birinchi qatori yetarli
        size_t n = e.find('\n');
        if (n != string::npos) e = e.substr(0, n);
        while (!e.empty() && (e.back() == '\r' || e.back() == ' ')) e.pop_back();
        // konsol kodlash sahifasi (OEM)
        int w = MultiByteToWideChar(CP_OEMCP, 0, e.data(), (int)e.size(), nullptr, 0);
        wstring t(w, 0);
        MultiByteToWideChar(CP_OEMCP, 0, e.data(), (int)e.size(), t.data(), w);
        *xato = t;
    }
    DeleteFileW(fx.c_str());
    return k;
}

wstring powershell() {
    wchar_t b[MAX_PATH];
    GetSystemDirectoryW(b, MAX_PATH);
    return wstring(b) + L"\\WindowsPowerShell\\v1.0\\powershell.exe";
}

// IP manzilni tarmoq kartasiga qo'shimcha manzil bo'lib qo'shadi (kassa_ip.ps1)
bool ip_qosh(const wstring& ip, unsigned indeks, wstring& xato) {
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    wstring f = wstring(tmp) + L"xozmag_kassa_ip.ps1";
    if (!fayl_yoz(f, KASSA_IP_PS1)) {
        xato = L"Skriptni yozib bo'lmadi.";
        return false;
    }
    wstring cmd = L"\"" + powershell() + L"\" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"" + f +
                  L"\" -Address " + ip + (indeks ? L" -InterfaceIndex " + std::to_wstring(indeks) : L"");
    wstring e;
    DWORD k = jarayon(cmd, &e, 60000);
    DeleteFileW(f.c_str());
    if (k != 0) {
        xato = e.empty() ? L"IP qo'shilmadi." : e;
        return false;
    }
    return true;
}

wstring exe_papka() {
    wchar_t b[MAX_PATH];
    GetModuleFileNameW(nullptr, b, MAX_PATH);
    wstring e = b;
    return e.substr(0, e.find_last_of(L'\\'));
}

// 2-kassa paketi: Xozmag.exe + sqlite3.dll + kassa.json + ORNATISH.bat (ZIP)
void paket_saqla(const Sozlama& s) {
    wchar_t fayl[MAX_PATH] = L"Xozmag-2-kassa.zip";
    OPENFILENAMEW o = {sizeof o};
    o.hwndOwner = ui.h;
    o.lpstrFilter = L"ZIP arxiv (*.zip)\0*.zip\0";
    o.lpstrFile = fayl;
    o.nMaxFile = MAX_PATH;
    o.lpstrDefExt = L"zip";
    o.lpstrTitle = L"2-kassa dasturini saqlash";
    o.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&o)) return;
    HCURSOR k = SetCursor(LoadCursor(nullptr, IDC_WAIT));
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    wstring p = wstring(tmp) + L"xozmag_2kassa_" + std::to_wstring(GetTickCount64());
    CreateDirectoryW(p.c_str(), nullptr);
    wstring src = exe_papka();
    bool ok = CopyFileW((src + L"\\Xozmag.exe").c_str(), (p + L"\\Xozmag.exe").c_str(), FALSE) &&
              CopyFileW((src + L"\\sqlite3.dll").c_str(), (p + L"\\sqlite3.dll").c_str(), FALSE);
    wstring server = doimiy_manzil();
    ok = ok && fayl_yoz(p + L"\\kassa.json", "{\"server\": \"" + U(server) + "\", \"client_ip\": \"" + U(s.client_ip) + "\"}");
    ok = ok && fayl_yoz(p + L"\\ORNATISH.bat",
                        "@echo off\r\nrem Xozmag 2-kassa: IP manzil qo'shiladi, dastur o'rnatiladi, server manzili yoziladi\r\n"
                        "start \"\" \"%~dp0Xozmag.exe\" --kassa-ornat \"%~dp0kassa.json\"\r\n");
    ok = ok && fayl_yoz(p + L"\\FOYDALANISH.txt",
                        "Xozmag 2-kassa\r\n\r\n1. ZIP ni 2-kassa kompyuterida papkaga oching.\r\n"
                        "2. ORNATISH.bat ni ishga tushiring (administrator huquqi so'raladi).\r\n"
                        "3. Ish stolidagi Xozmag yorlig'i bilan oching. Server: " + U(server) + "\r\n"
                        "Login/parol: kassa\r\n");
    wstring xato;
    if (ok) {
        DeleteFileW(fayl);
        wstring cmd = L"\"" + powershell() + L"\" -NoProfile -NonInteractive -Command \"Compress-Archive -Path '" + p +
                      L"\\*' -DestinationPath '" + fayl + L"' -Force\"";
        ok = jarayon(cmd, &xato, 180000) == 0;
    }
    // vaqtinchalik papka
    for (const wchar_t* n : {L"Xozmag.exe", L"sqlite3.dll", L"kassa.json", L"ORNATISH.bat", L"FOYDALANISH.txt"})
        DeleteFileW((p + L"\\" + n).c_str());
    RemoveDirectoryW(p.c_str());
    SetCursor(k);
    if (ok) ui.xabar(L"2-kassa dasturi saqlandi: " + wstring(fayl));
    else ui.xabar(L"Paketni saqlab bo'lmadi. " + xato, X_XATO);
}

}  // namespace

wstring kassalar_server_ip() { return fayl_matni(json_fayl()).empty() ? L"" : sozlama_ol().server_ip; }

namespace {

// ---- sahifa holati ----
bool ks_server = false;
wstring ks_xato;
std::vector<Karta> ks_kartalar;

}  // namespace

void kassalar_ochildi(long long) {
    ks_server = serverdami();
    ks_xato.clear();
    ks_kartalar = kartalar();
    Sozlama s = sozlama_ol();
    ui.qiymat_qoy("ks/server", s.server_ip);
    ui.qiymat_qoy("ks/client", s.client_ip);
    ui.qiymat_qoy("ks/indeks", s.interface_ ? std::to_wstring(s.interface_) : L"");
}

void kassalar_chiz(R r) {
    float en = std::min(850.f, r.w);
    R s = {r.x + (r.w - en) / 2, r.y, en, r.h};
    sarlavha(s, L"Kassalar");
    if (!ks_server) {
        R q = {s.x, s.y, s.w, 120};
        quti(q);
        gfx.matn(q.ichki(18), L"Kassalar sozlamasi faqat server kompyuterida ochiladi.\nBu kassa " + ulangan_manzil() +
                                  L" serverga ulangan.", 15, 500, M->matn2, CHAP, 0, true);
        return;
    }
    float kontent = 0;
    // 1. Server va kassa IP manzillari
    float kb = 0;
    for (auto& k : ks_kartalar) kb += 20;
    float boy1 = 18 + 24 + 12 + 24 + 12 + 3 * (22 + 50 + 12) + (ks_xato.empty() ? 0 : 30) + 50 + 18 + 20 + kb + 18;
    ui.scroll("ks/s", s, boy1 + 14 + 230);
    R q = {s.x, s.y, s.w - 12, boy1};
    quti(q);
    float x = q.x + 18, w = q.w - 36, y = q.y + 18;
    gfx.matn({x, y, w, 24}, L"Server va kassa IP manzillari", 17, 700, M->matn);
    y += 24 + 12;
    wstring sm = doimiy_manzil();
    gfx.matn({x, y, w, 24}, L"Server: " + (sm.empty() ? L"tarmoq topilmadi" : sm), 15, 700, M->urgu_matn);
    y += 24 + 12;
    const wchar_t* yorliq[] = {L"Server IP", L"2-kassa IP", L"Tarmoq kartasi indeksi"};
    const char* idlar[] = {"ks/server", "ks/client", "ks/indeks"};
    for (int i = 0; i < 3; ++i) {
        gfx.matn({x, y, w, 17}, yorliq[i], 12, 600, M->matn2);
        ui.maydon(idlar[i], {x, y + 22, std::min(w, 360.f), 50});
        y += 22 + 50 + 12;
    }
    if (!ks_xato.empty()) {
        gfx.matn({x, y, w, 24}, ks_xato, 13, 600, M->kechki ? hex("#ffa0a0") : M->xato);
        y += 30;
    }
    bool saqla = ui.tugma("ks/saqla", {x, y, 160, 50}, L"Saqlash", YASHIL) || ui.enter("ks/server") ||
                 ui.enter("ks/client") || ui.enter("ks/indeks");
    bool qosh = ui.tugma("ks/qosh", {x + 172, y, 340, 50}, L"Server IP ni tarmoq kartasiga qo'shish", ODDIY);
    y += 50 + 18;
    gfx.matn({x, y, w, 20}, L"Shu kompyuterning tarmoq kartalari (indeks · nomi · IP):", 12, 600, M->matn3);
    y += 20;
    for (auto& k : ks_kartalar) {
        wstring ipl;
        for (auto& ip : k.ip) ipl += (ipl.empty() ? L"" : L", ") + ip;
        gfx.matn({x, y, w, 20}, std::to_wstring(k.indeks) + L" · " + k.nom + L" · " + ipl, 12, 400, M->matn3);
        y += 20;
    }
    if (saqla || qosh) {
        Sozlama ns;
        if (!tekshir(ui.qiymat("ks/server"), ui.qiymat("ks/client"), ui.qiymat("ks/indeks"), ns, ks_xato)) {
            ui.xabar(ks_xato, X_XATO);
        } else {
            ks_xato.clear();
            CreateDirectoryW((malumot_papka() + L"\\qurilmalar_data").c_str(), nullptr);
            string j = "{\"server_ip\": \"" + U(ns.server_ip) + "\", \"client_ip\": \"" + U(ns.client_ip) +
                       "\", \"interface\": " + std::to_string(ns.interface_) + "}";
            if (!fayl_yoz(json_fayl() + L".tmp", j) ||
                !MoveFileExW((json_fayl() + L".tmp").c_str(), json_fayl().c_str(), MOVEFILE_REPLACE_EXISTING))
                ui.xabar(L"Sozlamalarni saqlab bo'lmadi: " + json_fayl(), X_XATO);
            else if (saqla)
                ui.xabar(L"Sozlamalar saqlandi. Server qayta ishga tushirilmadi.");
            if (qosh) {
                HCURSOR k = SetCursor(LoadCursor(nullptr, IDC_WAIT));
                wstring x2;
                bool ok = ip_qosh(ns.server_ip, ns.interface_, x2);
                SetCursor(k);
                ks_kartalar = kartalar();
                if (ok) ui.xabar(ns.server_ip + L" tarmoq kartasiga qo'shildi (DHCP manzili ham qoladi).");
                else ui.xabar(L"IP qo'shilmadi: " + x2, X_XATO);
            }
        }
    }
    kontent += boy1 + 14;
    // 2. 2-kassa dasturi
    R p = {s.x, q.p() + 14, s.w - 12, 230};
    quti(p);
    x = p.x + 18;
    y = p.y + 18;
    gfx.matn({x, y, w, 24}, L"2-kassa dasturi", 17, 700, M->matn);
    y += 24 + 12;
    gfx.matn({x, y, w, 22}, L"Server: " + (sm.empty() ? L"—" : sm), 15, 600, M->matn);
    y += 22 + 12;
    if (ui.tugma("ks/paket", {x, y, 330, 50}, L"2-kassa dasturini yuklab olish", YASHIL, "i-yuklab-ol"))
        paket_saqla(sozlama_ol());
    y += 50 + 12;
    gfx.matn({x, y, w, 40},
             L"ZIP ni 2-kassa kompyuterida oching va ORNATISH.bat ni ishga tushiring: 2-kassa IP manzili tarmoq "
             L"kartasiga qo'shiladi, dastur o'rnatiladi va server manzili yoziladi. Login/parol: kassa",
             13, 400, M->matn3, CHAP, 0, true);
    ui.scroll_tugat();
    (void)kontent;
}

// ======================= 2-kassa paketidan o'rnatish (Xozmag.exe --kassa-ornat kassa.json) =======================

static bool yorliq(const wstring& lnk, const wstring& exe) {
    IShellLinkW* sl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&sl)))
        return false;
    sl->SetPath(exe.c_str());
    sl->SetWorkingDirectory(exe.substr(0, exe.find_last_of(L'\\')).c_str());
    sl->SetIconLocation(exe.c_str(), 0);
    sl->SetDescription(L"Xozmag kassa");
    IPersistFile* pf = nullptr;
    bool ok = SUCCEEDED(sl->QueryInterface(IID_IPersistFile, (void**)&pf)) && SUCCEEDED(pf->Save(lnk.c_str(), TRUE));
    if (pf) pf->Release();
    sl->Release();
    return ok;
}

int kassa_ornat(const wstring& json) {
    const wchar_t* T = L"Xozmag — 2-kassa";
    string j = fayl_matni(json);
    if (j.empty()) {
        MessageBoxW(nullptr, (L"kassa.json topilmadi: " + json).c_str(), T, MB_ICONERROR);
        return 1;
    }
    wstring server = W(json_ol(j, "server")), client = W(json_ol(j, "client_ip"));
    if (MessageBoxW(nullptr, (L"Bu kompyuter 2-kassa bo'lib sozlanadi.\n\nServer: " + server +
                              (client.empty() ? L"" : L"\nShu kompyuter IP manzili: " + client) +
                              L"\n\nDavom etilsinmi?").c_str(),
                    T, MB_YESNO | MB_ICONQUESTION) != IDYES)
        return 1;
    wstring xato;
    if (!client.empty() && !ip_qosh(client, 0, xato)) {
        MessageBoxW(nullptr, (L"2-kassa IP manzili qo'shilmadi:\n" + xato).c_str(), T, MB_ICONERROR);
        return 1;
    }
    // Dastur Program Files\Xozmag ga (o'rnatuvchi bilan bir joy)
    wchar_t pf[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr, 0, pf);
    wstring joy = wstring(pf) + L"\\Xozmag", src = exe_papka();
    if (lstrcmpiW(joy.c_str(), src.c_str()) != 0) {
        SHCreateDirectoryExW(nullptr, joy.c_str(), nullptr);
        for (const wchar_t* n : {L"Xozmag.exe", L"sqlite3.dll"})
            if (!CopyFileW((src + L"\\" + n).c_str(), (joy + L"\\" + n).c_str(), FALSE)) {
                MessageBoxW(nullptr, (wstring(L"Faylni ko'chirib bo'lmadi: ") + n +
                                      L"\nXozmag ochiq bo'lsa yopib, qayta urinib ko'ring.").c_str(),
                            T, MB_ICONERROR);
                return 1;
            }
    }
    // Server manzili (keyingi ochilishda o'zi ulanadi)
    ini_saqla(L"server", server);
    ini_saqla(L"token", L"");
    CoInitialize(nullptr);
    wchar_t ish[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_COMMON_DESKTOPDIRECTORY, nullptr, 0, ish);
    yorliq(wstring(ish) + L"\\Xozmag.lnk", joy + L"\\Xozmag.exe");
    CoUninitialize();
    MessageBoxW(nullptr, (L"Tayyor. Ish stolidagi Xozmag yorlig'i bilan oching.\nServer: " + server).c_str(), T,
                MB_ICONINFORMATION);
    ShellExecuteW(nullptr, L"open", (joy + L"\\Xozmag.exe").c_str(), nullptr, joy.c_str(), SW_SHOWNORMAL);
    return 0;
}
