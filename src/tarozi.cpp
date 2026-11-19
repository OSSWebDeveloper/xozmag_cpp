// Rongta RLS1100C etiketka tarozisi (Ethernet) — saytdagi config/rongta.py bilan bir xil:
// faqat sotish birligi kg bo'lgan faol, so'mda narxli tovarlar yuboriladi; yuborishdan oldin
// tarozidagi PLU'lar zaxiralanadi, tugagach qayta o'qib solishtiriladi.
//
// Tarozi server kompyuteriga Ethernet orqali ulangan, shuning uchun hamma amal serverda
// bajariladi (kassa A_TAROZI so'rovini yuboradi). Ishlab chiqaruvchining SDK'si x86 DLL
// (rtslabelscale.dll) — u 32 bitli Windows PowerShell yordamchisi orqali chaqiriladi.
#include <winsock2.h>

#include "aloqa.h"
#include "ilova.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>


namespace {

// Yordamchi skript (saytdagi config/rongta_bridge.ps1 + tarozidan qayta o'qib solishtirish).
// Faqat ASCII: Windows PowerShell 5 BOMsiz faylni ANSI deb o'qiydi.
const char* KOPRIK = R"PS(param(
 [Parameter(Mandatory=$true)][string]$DriverDir,
 [Parameter(Mandatory=$true)][string]$Address,
 [ValidateSet('probe','sync')][string]$Mode = 'probe',
 [string]$InputJson,
 [string]$OutputJson,
 [string]$BackupFile,
 [string]$ProgressJson
)
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = New-Object Text.UTF8Encoding($false)
[Environment]::CurrentDirectory = $DriverDir
$env:PATH = "$DriverDir;$env:PATH"
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
public class RongtaNative {
 [DllImport("rtslabelscale.dll", CallingConvention=CallingConvention.StdCall, CharSet=CharSet.Ansi)]
 public static extern int rtscaleConnect(string address, int baud, out int connection);
 [DllImport("rtslabelscale.dll", CallingConvention=CallingConvention.StdCall)]
 public static extern int rtscaleDisConnect(int connection);
 [DllImport("rtslabelscale.dll", CallingConvention=CallingConvention.StdCall)]
 public static extern int rtscaleGetScaleType(int connection, byte[] text, int length);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)]
 public delegate void Callback(IntPtr json, int record, int total);
 [DllImport("rtslabelscale.dll", CallingConvention=CallingConvention.StdCall)]
 public static extern int rtscaleUploadPluData(int connection, Callback callback);
 [DllImport("rtslabelscale.dll", CallingConvention=CallingConvention.StdCall, CharSet=CharSet.Ansi)]
 public static extern int rtscaleDownLoadPLU(int connection, string json, int packet);
 public static List<string> Rows = new List<string>();
 private static void Receive(IntPtr json, int record, int total) {
   Rows.Add(Marshal.PtrToStringAnsi(json));
 }
 public static int Read(int connection) {
   Rows.Clear();
   Callback cb = Receive;
   int result = rtscaleUploadPluData(connection, cb);
   GC.KeepAlive(cb);
   return result;
 }
 public static string ReadJson() { return "[" + string.Join(",", Rows) + "]"; }
}
'@
function SaveJson($path, $text) {
 $temp = $path + '.tmp'
 [IO.File]::WriteAllText($temp, $text, (New-Object Text.UTF8Encoding($false)))
 $saved = $false
 for ($try = 0; $try -lt 7; $try++) {
   try {
     if ([IO.File]::Exists($path)) { [IO.File]::Replace($temp, $path, $null) }
     else { [IO.File]::Move($temp, $path) }
     $saved = $true
     break
   } catch { Start-Sleep -Milliseconds (50 * ($try+1)) }
 }
 if (-not $saved) { [IO.File]::WriteAllText($path, $text, (New-Object Text.UTF8Encoding($false))) }
}
$connection = 0
try {
 $ret = [RongtaNative]::rtscaleConnect($Address, 0, [ref]$connection)
 if ($ret -ne 0) { throw "Tarozi bilan aloqa yo'q ($ret)." }
 $buffer = New-Object byte[] 1024
 $ret = [RongtaNative]::rtscaleGetScaleType($connection, $buffer, $buffer.Length)
 if ($ret -ne 0) { throw "Tarozi protokoli javob bermadi ($ret)." }
 $protocol = [Text.Encoding]::Default.GetString($buffer).Trim([char]0) | ConvertFrom-Json
 if ($Mode -eq 'probe') {
   Write-Output (@{ok=$true; protocol=[string]$protocol.Protocol; address=$Address} | ConvertTo-Json -Compress)
 } else {
   # Eski ro'yxatni o'qish muvaffaqiyatsiz bo'lsa hech narsa yozilmaydi.
   $ret = [RongtaNative]::Read($connection)
   if ($ret -lt 0) { throw "Tarozi bazasini zaxiralab bo'lmadi." }
   SaveJson $BackupFile ([RongtaNative]::ReadJson())
   $inputRows = @(ConvertFrom-Json -InputObject ([IO.File]::ReadAllText($InputJson)))
   $keep = @{}
   foreach ($row in $inputRows) { $keep[[int]$row.LFCode] = $true }
   $oldRows = ConvertFrom-Json -InputObject ([RongtaNative]::ReadJson())
   $remove = @()
   foreach ($row in $oldRows) {
     if (-not $keep.ContainsKey([int]$row.LFCode) -and $row.isDelete -ne 1) {
       $row.isDelete = 1
       $remove += $row
     }
   }
   # Faqat kg katalogi qoladi: qolgan yozuvlar alohida isDelete belgisi bilan yashiriladi.
   # PLU xotirasi, qo'shimcha xabarlar va tarozi sozlamalari ommaviy tozalanmaydi.
   $rows = @($remove) + @($inputRows)
   $total = $rows.Count
   for ($i = 0; $i -lt $total; $i += 4) {
     $end = [Math]::Min($i + 3, $total - 1)
     $batch = @($rows[$i..$end])
     while ($batch.Count -lt 4) { $batch += @{} }
     $json = ConvertTo-Json -InputObject $batch -Depth 8 -Compress
     # SDK'ning eski JSON parseri PowerShell qo'ygan HTML unicode escape'larini ochmaydi.
     $json = $json.Replace('\u0027', "'").Replace('\u003c', '<').Replace('\u003e', '>').Replace('\u0026', '&')
     $ret = [RongtaNative]::rtscaleDownLoadPLU($connection, $json, [int]($i / 4))
     if ($ret -ne 0) { throw "PLU paketi yuborilmadi: $i ($ret). Ayrim mahsulotlar yuborilgan bo'lishi mumkin." }
     if ($i % 40 -eq 0 -or $end -eq $total - 1) {
       SaveJson $ProgressJson (@{holat='yuborilmoqda'; yuborildi=($end+1); jami=$total} | ConvertTo-Json -Compress)
     }
   }
   SaveJson $ProgressJson (@{holat='tekshirilmoqda'; yuborildi=$total; jami=$total} | ConvertTo-Json -Compress)
   $ret = [RongtaNative]::Read($connection)
   if ($ret -lt 0) { throw "Yuborildi, ammo tarozidan qayta o'qib tekshirib bo'lmadi." }
   $after = [RongtaNative]::ReadJson()
   SaveJson $OutputJson $after
   # Qayta o'qilgani yuborilgani bilan solishtiriladi
   $actual = @{}
   foreach ($r in @(ConvertFrom-Json -InputObject $after)) { if ($r.isDelete -ne 1) { $actual[[int]$r.LFCode] = $r } }
   $mismatch = @()
   foreach ($r in $inputRows) {
     $a = $actual[[int]$r.LFCode]
     if (-not $a) { $mismatch += [int]$r.LFCode; continue }
     foreach ($k in 'Code','UnitPrice','WeightUnit','BarCode','Deptment','PluName') {
       if ([string]$a.$k -ne [string]$r.$k) { $mismatch += [int]$r.LFCode; break }
     }
   }
   $extra = @($actual.Keys | Where-Object { -not $keep.ContainsKey($_) }).Count
   Write-Output (@{ok=$true; sent=$total; read=$ret; protocol=[string]$protocol.Protocol; mismatch=$mismatch.Count;
     first=(($mismatch | Select-Object -First 10) -join ', '); extra=$extra} | ConvertTo-Json -Compress)
 }
} catch {
 [Console]::Error.WriteLine($_.Exception.Message)
 exit 1
} finally {
 if ($connection -gt 0) { [void][RongtaNative]::rtscaleDisConnect($connection) }
}
)PS";

struct Tayyor {
    wstring kod, nom;      // nom — tarozidagi (32 belgigacha)
    long long lf = 0;      // LFCode = tovar kodi
    wstring code;          // tarozi PLU (tarozi_kodi, nollarsiz)
    long long narx100 = 0; // so'm * 100
    double narx = 0;
};
struct Otkaz {
    wstring kod, nom, sabab;
};

wstring qisqa_nom(const wstring& nom) {
    // Tarozidagi shriftni almashtirmaymiz: lotincha nomlar ASCII (rongta.short_name)
    wstring s = nom;
    for (auto& c : s)
        if (c == L'\u2018' || c == L'\u2019') c = L'\'';
    int n = NormalizeString(NormalizationKD, s.c_str(), (int)s.size(), nullptr, 0);
    if (n > 0) {
        wstring t(n, 0);
        n = NormalizeString(NormalizationKD, s.c_str(), (int)s.size(), t.data(), n);
        if (n > 0) s = t.substr(0, n);
    }
    for (auto& c : s)
        if (c > 127) c = L'?';
    return s.size() <= 32 ? s : s.substr(0, 24) + L"~" + s.substr(s.size() - 7);
}

// Taroziga ketadiganlar va o'tkaziladiganlar. Tarozi kodi yo'q tovarga yangisi beriladi.
void tayyorla(std::vector<Tayyor>& r, std::vector<Otkaz>& o) {
    struct Q {
        long long id;
        wstring kod, nom, valyuta, tk;
        double narx;
    };
    std::vector<Q> qs;
    {
        Sorov q = baza.sor("SELECT id, kod, nom, valyuta, narx, COALESCE(tarozi_kodi, '') FROM ombor_mahsulot "
                           "WHERE faol=1 AND birlik='kg' ORDER BY kod");
        while (q.qator()) qs.push_back({q.butun(0), q.matn(1), q.matn(2), q.matn(3), q.matn(5), q.son(4)});
    }
    for (auto& m : qs) {
        wstring sabab;
        if (m.valyuta != L"som") sabab = L"Narx so'mda emas";
        else if (m.narx <= 0) sabab = L"Sotish narxi yo'q";
        else if (m.narx > 999999.99) sabab = L"Narx tarozi chegarasidan katta (999 999,99)";
        else if (!raqammi(m.kod) || std::stoll(m.kod) < 1 || std::stoll(m.kod) > KOD_MAX) sabab = L"Kod noto'g'ri";
        else if (qisqa_nom(m.nom).find(L'?') != wstring::npos) sabab = L"Nomi tarozi lotin shriftiga mos emas";
        if (!sabab.empty()) {
            o.push_back({m.kod, m.nom, sabab});
            continue;
        }
        if (m.tk.empty()) {
            Tranzaksiya t;
            long long oxirgi = 0;
            {
                Sorov q = baza.sor("SELECT MAX(CAST(tarozi_kodi AS INTEGER)) FROM ombor_mahsulot WHERE tarozi_kodi IS NOT NULL");
                if (q.qator() && q.bormi(0)) oxirgi = q.butun(0);
            }
            if (oxirgi + 1 > 999999) throw BazaXato("Tarozi shtrix identifikatorlari tugadi.");
            wchar_t b[16];
            swprintf(b, 16, L"%06lld", oxirgi + 1);
            baza.sor("UPDATE ombor_mahsulot SET tarozi_kodi=? WHERE id=? AND tarozi_kodi IS NULL").bog(1, wstring(b)).bog(2, m.id).bajar();
            t.tasdiq();
            m.tk = b;
        }
        Tayyor x;
        x.kod = m.kod;
        x.nom = qisqa_nom(m.nom);
        x.lf = std::stoll(m.kod);
        x.code = std::to_wstring(std::stoll(m.tk));
        x.narx = m.narx;
        x.narx100 = std::llround(m.narx * 100);
        r.push_back(x);
    }
}

wstring yuqori(wstring s) {
    for (auto& c : s) c = (wchar_t)towupper(c);
    return s;
}

bool fayl_bor(const wstring& f) {
    DWORD a = GetFileAttributesW(f.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

wstring muhit(const wchar_t* n) {
    wchar_t b[1024];
    DWORD k = GetEnvironmentVariableW(n, b, 1024);
    return k && k < 1024 ? b : L"";
}

wstring papka() {
    wstring p = malumot_papka() + L"\\qurilmalar_data";
    CreateDirectoryW(p.c_str(), nullptr);
    return p;
}

wstring asosiy_drayver_papka() { return malumot_papka() + L"\\qurilma_drivers\\rongta"; }

// rtslabelscale.dll qayerda: Xozmag papkasi, saytning papkalari, exe yonida, Rongta dasturi o'rnatilgan joy
wstring drayver_papka() {
    std::vector<wstring> joy = {asosiy_drayver_papka()};
    wstring la = muhit(L"LOCALAPPDATA");
    for (const wchar_t* n : {L"\\UlugbekXozmag", L"\\BizaroPOS-Server"})
        joy.push_back(la + n + L"\\qurilma_drivers\\rongta");
    wchar_t b[MAX_PATH];
    GetModuleFileNameW(nullptr, b, MAX_PATH);
    wstring exe = b;
    joy.push_back(exe.substr(0, exe.find_last_of(L'\\')) + L"\\qurilma_drivers\\rongta");
    joy.push_back(muhit(L"USERPROFILE") + L"\\Desktop\\xozmag\\qurilma_drivers\\rongta");
    for (auto& j : joy)
        if (fayl_bor(j + L"\\rtslabelscale.dll")) return j;
    // Rongta dasturining o'z papkasi (Program Files (x86)\...\ yoki bir daraja ichkari)
    for (const wchar_t* pf : {L"ProgramFiles(x86)", L"ProgramFiles"}) {
        wstring ild = muhit(pf);
        if (ild.empty()) continue;
        WIN32_FIND_DATAW d;
        HANDLE h = FindFirstFileW((ild + L"\\*").c_str(), &d);
        if (h == INVALID_HANDLE_VALUE) continue;
        std::vector<wstring> ichki;
        do {
            if (!(d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || d.cFileName[0] == L'.') continue;
            wstring n = yuqori(d.cFileName);
            if (n.find(L"RLS") == wstring::npos && n.find(L"RONGTA") == wstring::npos && n.find(L"SCALE") == wstring::npos)
                continue;
            ichki.push_back(ild + L"\\" + d.cFileName);
        } while (FindNextFileW(h, &d));
        FindClose(h);
        for (auto& i : ichki) {
            if (fayl_bor(i + L"\\rtslabelscale.dll")) return i;
            HANDLE h2 = FindFirstFileW((i + L"\\*").c_str(), &d);
            if (h2 == INVALID_HANDLE_VALUE) continue;
            wstring top;
            do {
                if ((d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && d.cFileName[0] != L'.' &&
                    fayl_bor(i + L"\\" + d.cFileName + L"\\rtslabelscale.dll"))
                    top = i + L"\\" + d.cFileName;
            } while (top.empty() && FindNextFileW(h2, &d));
            FindClose(h2);
            if (!top.empty()) return top;
        }
    }
    return L"";
}

wstring drayver_xabari() {
    return L"Rongta drayveri topilmadi. RLS_setup_V2.0.19.37 o'rnatilgan papkadan rtslabelscale.dll va "
           L"yonidagi .dll fayllarni shu papkaga ko'chiring: " + asosiy_drayver_papka();
}

string fayl_oqi(const wstring& f) {
    HANDLE h = CreateFileW(f.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return "";
    string s;
    DWORD n = GetFileSize(h, nullptr), o = 0;
    s.resize(n);
    if (n) ReadFile(h, s.data(), n, &o, nullptr);
    CloseHandle(h);
    s.resize(o);
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB) s.erase(0, 3);
    return s;
}

bool fayl_yoz(const wstring& f, const string& s, bool bom = false) {
    wstring t = f + L".tmp";
    HANDLE h = CreateFileW(t.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD o = 0;
    if (bom) WriteFile(h, "\xEF\xBB\xBF", 3, &o, nullptr);
    bool ok = WriteFile(h, s.data(), (DWORD)s.size(), &o, nullptr) && o == s.size();
    CloseHandle(h);
    return ok && MoveFileExW(t.c_str(), f.c_str(), MOVEFILE_REPLACE_EXISTING);
}

// JSON'dan oddiy maydon (son yoki matn) — faqat o'zimizning yordamchi chiqargan qisqa javoblar uchun
string json_qiymat(const string& j, const string& kalit) {
    size_t p = j.find("\"" + kalit + "\":");
    if (p == string::npos) return "";
    p += kalit.size() + 3;
    if (p < j.size() && j[p] == '"') {
        string r;
        for (++p; p < j.size() && j[p] != '"'; ++p) {
            if (j[p] == '\\' && p + 1 < j.size()) ++p;
            r += j[p];
        }
        return r;
    }
    size_t e = j.find_first_of(",}", p);
    return j.substr(p, e - p);
}
long long json_son(const string& j, const string& k) {
    string v = json_qiymat(j, k);
    return v.empty() ? 0 : atoll(v.c_str());
}

string json_matn(const wstring& s) {
    string r = "\"";
    for (char c : U(s)) {
        if (c == '"' || c == '\\') r += '\\';
        if ((unsigned char)c < 32) continue;
        r += c;
    }
    return r + "\"";
}

wstring vaqt_belgisi() {
    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t b[32];
    swprintf(b, 32, L"%04d%02d%02d_%02d%02d%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    return b;
}

bool ip_tozami(const wstring& ip) {
    if (ip.empty() || ip.size() > 64) return false;
    for (wchar_t c : ip)
        if (!iswalnum(c) && c != L'.' && c != L':' && c != L'-') return false;
    return true;
}

// 32 bitli PowerShell'da yordamchini ishga tushiradi. chiqish — stdout (JSON), xato — stderr.
bool koprik(const wstring& rejim, const wstring& ip, const std::vector<std::pair<wstring, wstring>>& arg, DWORD kutish,
            string& chiqish, wstring& xato) {
    wstring drv = drayver_papka();
    if (drv.empty()) {
        xato = drayver_xabari();
        return false;
    }
    if (!ip_tozami(ip)) {
        xato = L"Tarozi IP manzili noto'g'ri.";
        return false;
    }
    wstring skript = malumot_papka() + L"\\rongta_bridge.ps1";
    if (!fayl_yoz(skript, KOPRIK, true)) {
        xato = L"Yordamchi skriptni yozib bo'lmadi: " + skript;
        return false;
    }
    wstring tizim = muhit(L"SystemRoot");
    if (tizim.empty()) tizim = L"C:\\Windows";
    wstring ps = tizim + L"\\SysWOW64\\WindowsPowerShell\\v1.0\\powershell.exe";
    if (!fayl_bor(ps)) ps = tizim + L"\\System32\\WindowsPowerShell\\v1.0\\powershell.exe";
    wstring cmd = L"\"" + ps + L"\" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"" + skript +
                  L"\" -DriverDir \"" + drv + L"\" -Address \"" + ip + L"\" -Mode " + rejim;
    for (auto& [k, v] : arg) cmd += L" -" + k + L" \"" + v + L"\"";
    wstring tag = std::to_wstring(GetCurrentThreadId()) + L"_" + std::to_wstring(GetTickCount64());
    wstring fo = papka() + L"\\koprik_" + tag + L".out", fx = papka() + L"\\koprik_" + tag + L".err";
    SECURITY_ATTRIBUTES sa = {sizeof sa, nullptr, TRUE};
    HANDLE ho = CreateFileW(fo.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, 0, nullptr);
    HANDLE hx = CreateFileW(fx.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, 0, nullptr);
    STARTUPINFOW si = {sizeof si};
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = ho;
    si.hStdError = hx;
    si.hStdInput = nullptr;
    PROCESS_INFORMATION pi;
    bool ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (ho != INVALID_HANDLE_VALUE) CloseHandle(ho);
    if (hx != INVALID_HANDLE_VALUE) CloseHandle(hx);
    DWORD kod = (DWORD)-1;
    if (ok) {
        if (WaitForSingleObject(pi.hProcess, kutish) == WAIT_TIMEOUT) {
            TerminateProcess(pi.hProcess, 2);
            WaitForSingleObject(pi.hProcess, 3000);
            kod = 0xFFFE;
        } else
            GetExitCodeProcess(pi.hProcess, &kod);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    chiqish = fayl_oqi(fo);
    string e = fayl_oqi(fx);
    DeleteFileW(fo.c_str());
    DeleteFileW(fx.c_str());
    if (!ok) {
        xato = L"PowerShell ishga tushmadi: " + ps;
        return false;
    }
    if (kod == 0xFFFE) {
        xato = L"Tarozi javob bermadi (vaqt tugadi).";
        return false;
    }
    if (kod != 0) {
        while (!e.empty() && (e.back() == '\n' || e.back() == '\r' || e.back() == ' ')) e.pop_back();
        if (e.size() > 1000) e = e.substr(e.size() - 1000);
        xato = e.empty() ? L"Tarozi javob bermadi." : W(e);
        return false;
    }
    size_t b = chiqish.find('{');
    if (b != string::npos) chiqish = chiqish.substr(b);
    return true;
}

// ---------- Yuborish jarayoni (serverda bitta) ----------

SRWLOCK jq = SRWLOCK_INIT;
struct Jarayon {
    bool band = false;
    wstring holat, xato, progress;
    long long yuborildi = 0, jami = 0;
} jr;
struct JQulf {
    JQulf() { AcquireSRWLockExclusive(&jq); }
    ~JQulf() { ReleaseSRWLockExclusive(&jq); }
};

wstring oxirgi_fayl() { return papka() + L"\\rongta_oxirgi.txt"; }

struct YuborishIshi {
    std::vector<Tayyor> rows;
    size_t otkaz = 0;
    wstring ip, tag;
};

DWORD WINAPI yuborish_ishi(void* p) {
    std::unique_ptr<YuborishIshi> ish((YuborishIshi*)p);
    wstring d = papka(), t = ish->tag;
    wstring fin = d + L"\\" + t + L"_input.json", fout = d + L"\\" + t + L"_after.json",
            fb = d + L"\\" + t + L"_backup.json";
    string j = "[";
    for (size_t i = 0; i < ish->rows.size(); ++i) {
        auto& r = ish->rows[i];
        char b[512];
        snprintf(b, sizeof b,
                 "%s{\"PluName\":%s,\"LFCode\":%lld,\"Code\":%s,\"UnitPrice\":%lld,\"AftertaxUP\":%s,\"WeightUnit\":4,"
                 "\"BarCode\":27,\"Deptment\":2,\"HotKey\":0,\"LabelId\":0,\"ShlefTime\":365,\"ShelfDays\":365,"
                 "\"PackageType\":0,\"isDelete\":0}",
                 i ? "," : "", json_matn(r.nom).c_str(), r.lf, json_matn(r.code).c_str(), r.narx100,
                 sonU(r.narx).c_str());
        j += b;
    }
    j += "]";
    wstring xato;
    string chiqish;
    bool ok = fayl_yoz(fin, j) &&
              koprik(L"sync", ish->ip,
                     {{L"InputJson", fin}, {L"OutputJson", fout}, {L"BackupFile", fb}, {L"ProgressJson", jr.progress}},
                     900000, chiqish, xato);
    if (ok && json_son(chiqish, "mismatch") > 0) {
        ok = false;
        xato = std::to_wstring(json_son(chiqish, "mismatch")) + L" mahsulot tarozidagi ma'lumotga mos kelmadi. "
               L"Birinchi kodlar: " + W(json_qiymat(chiqish, "first"));
    } else if (ok && json_son(chiqish, "extra") > 0) {
        ok = false;
        xato = L"Tarozida yuborilgan kg katalogidan tashqari faol mahsulot qoldi.";
    }
    if (!ok && xato.empty()) xato = L"Yuborishda xato.";
    if (ok) {
        SYSTEMTIME s;
        GetLocalTime(&s);
        wchar_t b[256];
        swprintf(b, 256, L"%02d.%02d.%04d %02d:%02d — %zu ta tovar yuborildi va tekshirildi, %zu ta o'tkazildi",
                 s.wDay, s.wMonth, s.wYear, s.wHour, s.wMinute, ish->rows.size(), ish->otkaz);
        fayl_yoz(oxirgi_fayl(), U(b));
    }
    JQulf q;
    jr.band = false;
    jr.holat = ok ? L"tayyor" : L"xato";
    jr.xato = ok ? L"" : xato + L" Qisman yuborilgan bo'lishi mumkin. Zaxira: " + fb;
    if (ok) jr.yuborildi = jr.jami;
    return 0;
}

void jarayon_yangila() {
    if (!jr.band || jr.progress.empty()) return;
    string s = fayl_oqi(jr.progress);
    if (s.empty()) return;
    jr.holat = W(json_qiymat(s, "holat"));
    jr.yuborildi = json_son(s, "yuborildi");
    if (json_son(s, "jami")) jr.jami = json_son(s, "jami");
}

wstring toza(wstring s) {
    for (auto& c : s)
        if (c == L'\t' || c == L'\n' || c == L'\r') c = L' ';
    return s;
}

wstring jarayon_qatori() {
    JQulf q;
    jarayon_yangila();
    return L"B\t" + wstring(jr.band ? L"1" : L"0") + L"\t" + jr.holat + L"\t" + std::to_wstring(jr.yuborildi) + L"\t" +
           std::to_wstring(jr.jami) + L"\t" + toza(jr.xato) + L"\n";
}

}  // namespace

// Serverda bajariladi (kassa A_TAROZI bilan so'raydi). Javob — tab bilan ajratilgan qatorlar.
string tarozi_bajar(unsigned char amal, const string& soz) {
    wstring ip = W(soz), r;
    try {
        switch (amal) {
            case TA_HOLAT: {
                std::vector<Tayyor> t;
                std::vector<Otkaz> o;
                tayyorla(t, o);
                for (auto& x : t)
                    r += L"T\t" + x.kod + L"\t" + toza(x.nom) + L"\t" + pul(x.narx) + L"\t2" +
                         wstring(6 - std::min<size_t>(6, x.code.size()), L'0') + x.code + L"\n";
                for (auto& x : o) r += L"O\t" + x.kod + L"\t" + toza(x.nom) + L"\t" + x.sabab + L"\n";
                r += L"Z\t" + toza(W(fayl_oqi(oxirgi_fayl()))) + L"\n";
                r += jarayon_qatori();
                break;
            }
            case TA_TEKSHIR: {
                string chiqish;
                wstring xato;
                if (koprik(L"probe", ip, {}, 20000, chiqish, xato))
                    r = L"K\t1\t" + toza(W(json_qiymat(chiqish, "protocol"))) + L"\n";
                else
                    r = L"K\t0\t" + toza(xato) + L"\n";
                break;
            }
            case TA_YUBOR: {
                auto* ish = new YuborishIshi;
                std::vector<Otkaz> o;
                tayyorla(ish->rows, o);
                ish->otkaz = o.size();
                ish->ip = ip;
                ish->tag = vaqt_belgisi();
                if (ish->rows.empty()) {
                    delete ish;
                    r = L"Y\t0\tTaroziga yuboriladigan narxli mahsulot yo'q.\n";
                    break;
                }
                if (drayver_papka().empty()) {
                    delete ish;
                    r = L"Y\t0\t" + toza(drayver_xabari()) + L"\n";
                    break;
                }
                {
                    JQulf q;
                    if (jr.band) {
                        delete ish;
                        r = L"Y\t0\tYuklash davom etmoqda. Tugashini kuting.\n";
                        break;
                    }
                    jr = {};
                    jr.band = true;
                    jr.holat = L"boshlanmoqda";
                    jr.jami = (long long)ish->rows.size();
                    jr.progress = papka() + L"\\" + ish->tag + L".json";
                    DeleteFileW(jr.progress.c_str());
                }
                {
                    std::vector<Otkaz> oo = o;
                    string j = "[";
                    for (size_t i = 0; i < oo.size(); ++i)
                        j += (i ? "," : "") + string("{\"kod\":") + json_matn(oo[i].kod) + ",\"nom\":" +
                             json_matn(oo[i].nom) + ",\"sabab\":" + json_matn(oo[i].sabab) + "}";
                    fayl_yoz(papka() + L"\\" + ish->tag + L"_otkazilgan.json", j + "]");
                }
                HANDLE h = CreateThread(nullptr, 0, yuborish_ishi, ish, 0, nullptr);
                if (!h) {
                    JQulf q;
                    jr.band = false;
                    delete ish;
                    r = L"Y\t0\tYuborishni boshlab bo'lmadi.\n";
                    break;
                }
                CloseHandle(h);
                r = L"Y\t1\n" + jarayon_qatori();
                break;
            }
            case TA_JARAYON: r = jarayon_qatori(); break;
            default: r = L"X\tNoma'lum amal\n";
        }
    } catch (const std::exception& e) {
        r = L"X\t" + toza(W(e.what())) + L"\n";
    }
    return U(r);
}

// ======================= Kassa tomoni: Qurilmalar sahifasidagi panel =======================

namespace {

struct Korinish {
    std::vector<std::vector<wstring>> tayyor, otkaz;
    wstring oxirgi, aloqa, aloqa_xato, umumiy_xato;
    bool aloqa_bor = false, tekshirilgan = false;
    bool band = false;
    wstring holat, jxato;
    long long yuborildi = 0, jami = 0;
    bool ochiq_t = false, ochiq_o = false;
    ULONGLONG oxirgi_sorov = 0;
} tk;

std::vector<wstring> bolib(const wstring& s) {
    std::vector<wstring> r;
    size_t a = 0;
    while (true) {
        size_t b = s.find(L'\t', a);
        r.push_back(s.substr(a, b == wstring::npos ? wstring::npos : b - a));
        if (b == wstring::npos) break;
        a = b + 1;
    }
    return r;
}

void javobni_oqi(const string& j, bool royxat) {
    if (royxat) {
        tk.tayyor.clear();
        tk.otkaz.clear();
    }
    wstring s = W(j);
    size_t a = 0;
    while (a < s.size()) {
        size_t b = s.find(L'\n', a);
        if (b == wstring::npos) b = s.size();
        auto f = bolib(s.substr(a, b - a));
        a = b + 1;
        if (f.empty()) continue;
        auto ol = [&](size_t i) { return i < f.size() ? f[i] : wstring(); };
        if (f[0] == L"T") tk.tayyor.push_back({ol(1), ol(2), ol(3), ol(4)});
        else if (f[0] == L"O") tk.otkaz.push_back({ol(1), ol(2), ol(3)});
        else if (f[0] == L"Z") tk.oxirgi = ol(1);
        else if (f[0] == L"B") {
            bool edi = tk.band;
            tk.band = ol(1) == L"1";
            tk.holat = ol(2);
            tk.yuborildi = _wtoi64(ol(3).c_str());
            tk.jami = _wtoi64(ol(4).c_str());
            tk.jxato = ol(5);
            if (edi && !tk.band) {
                if (tk.holat == L"tayyor") ui.xabar(L"Tarozi katalogi yangilandi va tekshirildi.");
                else if (!tk.jxato.empty()) ui.xabar(L"Taroziga yuborishda xato.", X_XATO);
            }
        } else if (f[0] == L"K") {
            tk.tekshirilgan = true;
            tk.aloqa_bor = ol(1) == L"1";
            tk.aloqa = tk.aloqa_bor ? ol(2) : L"";
            tk.aloqa_xato = tk.aloqa_bor ? L"" : ol(2);
        } else if (f[0] == L"Y") {
            if (ol(1) != L"1") ui.xabar(ol(2), X_XATO);
        } else if (f[0] == L"X") {
            tk.umumiy_xato = ol(1);
        }
    }
}

string sor(unsigned char amal, const wstring& ip) {
    if (baza.uzoqmi()) return uzoq_tarozi(amal, U(ip));
    return tarozi_bajar(amal, U(ip));
}

wstring tarozi_ip() {
    wstring ip = ini_qiymat(L"tarozi_ip");
    if (ip.empty()) ip = muhit(L"XOZMAG_RONGTA_IP");
    return ip.empty() ? L"192.168.1.110" : ip;
}

}  // namespace

void tarozi_ochildi() {
    bool ot = tk.ochiq_t, oo = tk.ochiq_o;
    tk = {};
    tk.ochiq_t = ot;
    tk.ochiq_o = oo;
    ui.qiymat_qoy("tz/ip", tarozi_ip());
    try {
        javobni_oqi(sor(TA_HOLAT, tarozi_ip()), true);
    } catch (const BazaXato& e) {
        tk.umumiy_xato = W(e.what());
    }
}

static float jadval_qator(R r, const std::vector<wstring>& v, const std::vector<float>& en, bool sarl) {
    float x = r.x;
    for (size_t i = 0; i < en.size() && i < v.size(); ++i) {
        bool son_ustun = sarl ? false : (i == 2 && en.size() == 4);
        gfx.matn({x + 10, r.y, en[i] - 20, r.h}, v[i], sarl ? 12 : 14, sarl ? 600 : (i == 0 ? 600 : 400),
                 sarl ? M->matn2 : M->matn, son_ustun ? ONG : CHAP);
        x += en[i];
    }
    gfx.chiziq(r.x, r.p() - .5f, r.o(), r.p() - .5f, M->chegara);
    return r.h;
}

// Qurilmalar sahifasidagi «Rongta RLS1100C · Ethernet» qutisi. y — boshlanishi; qaytaradi — balandligi.
float tarozi_panel(float x0, float y0, float w) {
    // Yuborish davom etsa har soniyada holat so'raladi
    if (tk.band) {
        SetTimer(ui.h, 81, 1000, nullptr);
        if (GetTickCount64() - tk.oxirgi_sorov >= 900) {
            tk.oxirgi_sorov = GetTickCount64();
            try {
                javobni_oqi(sor(TA_JARAYON, L""), false);
                if (!tk.band) javobni_oqi(sor(TA_HOLAT, tarozi_ip()), true);
            } catch (const BazaXato& e) {
                tk.umumiy_xato = W(e.what());
            }
        }
    } else
        KillTimer(ui.h, 81);

    float iw = w - 28, x = x0 + 14, y = y0 + 14;
    gfx.matn({x, y, iw, 24}, L"Rongta RLS1100C · Ethernet", 17, 700, M->matn);
    y += 24 + 6;
    gfx.matn({x, y, iw, 18}, L"Etiketka tarozisi do'kon tarmog'iga (server kompyuteriga) Ethernet kabeli bilan ulangan.",
             13, 400, M->matn2);
    y += 18 + 12;
    gfx.matn({x, y, 220, 17}, L"Tarozi IP manzili", 12, 600, M->matn2);
    y += 22;
    MaydonSoz ms;
    ms.ishora = L"192.168.1.110";
    ui.maydon("tz/ip", {x, y, 220, 50}, ms);
    if (ui.ozgardi("tz/ip")) ini_qiymat_yoz(L"tarozi_ip", tozala(ui.qiymat("tz/ip")));
    wstring ip = tozala(ui.qiymat("tz/ip"));
    if (ip.empty()) ip = tarozi_ip();
    bool tek = ui.tugma("tz/tek", {x + 232, y, 220, 50}, L"Aloqani tekshirish", ODDIY, "i-monitor");
    bool yub = ui.tugma("tz/yub", {x + 464, y, std::min(300.f, iw - 464), 50},
                        tk.band ? L"Yuborilmoqda..." : L"Omborni taroziga yuborish", YASHIL, "i-kirim");
    y += 50 + 12;
    if (tek) {
        HCURSOR k = SetCursor(LoadCursor(nullptr, IDC_WAIT));
        try {
            javobni_oqi(sor(TA_TEKSHIR, ip), false);
        } catch (const BazaXato& e) {
            tk.aloqa_xato = W(e.what());
            tk.tekshirilgan = true;
            tk.aloqa_bor = false;
        }
        SetCursor(k);
    }
    if (yub && !tk.band) {
        if (tk.tayyor.empty()) ui.xabar(L"Taroziga yuboriladigan narxli kg tovar yo'q.", X_XATO);
        else if (MessageBoxW(ui.h, (std::to_wstring(tk.tayyor.size()) + L" ta kg tovar taroziga yuboriladi. Tarozidagi "
                                    L"boshqa mahsulotlar yashiriladi (oldin zaxira olinadi).\n\nDavom etilsinmi?").c_str(),
                             L"Tarozi", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            try {
                javobni_oqi(sor(TA_YUBOR, ip), false);
                tk.oxirgi_sorov = GetTickCount64();
            } catch (const BazaXato& e) {
                ui.xabar(W(e.what()), X_XATO);
            }
        }
    }
    // Aloqa holati
    wstring holat;
    Rang hr = M->matn2;
    if (!tk.tekshirilgan) holat = L"Manzil: " + ip + L" · aloqa hali tekshirilmadi.";
    else if (tk.aloqa_bor) {
        holat = L"Aloqa bor · " + ip + (tk.aloqa.empty() ? L"" : L" · protokol " + tk.aloqa);
        hr = M->tasdiq;
    } else {
        holat = L"Aloqa yo'q: " + tk.aloqa_xato;
        hr = M->kechki ? hex("#ffa0a0") : M->xato;
    }
    float hb = gfx.matn_boyi(holat, 14, 600, iw) + 4;
    gfx.matn({x, y, iw, hb}, holat, 14, 600, hr, CHAP, 0, true);
    y += hb + 6;
    if (!tk.umumiy_xato.empty()) {
        gfx.matn({x, y, iw, 20}, tk.umumiy_xato, 13, 500, M->xato);
        y += 26;
    }
    // Hisob
    std::map<wstring, int> sab;
    for (auto& o : tk.otkaz) sab[o[2]]++;
    wstring hisob = L"Taroziga ketadi: " + std::to_wstring(tk.tayyor.size()) + L" ta kg tovar · o'tkaziladi: " +
                    std::to_wstring(tk.otkaz.size());
    if (!sab.empty()) {
        hisob += L" (";
        bool b = true;
        for (auto& [k, v] : sab) {
            hisob += (b ? L"" : L"; ") + k + L": " + std::to_wstring(v);
            b = false;
        }
        hisob += L")";
    }
    if (!tk.oxirgi.empty()) hisob += L"\nOxirgi yuborish: " + tk.oxirgi;
    float hh = gfx.matn_boyi(hisob, 13, 400, iw) + 4;
    gfx.matn({x, y, iw, hh}, hisob, 13, 400, M->matn3, CHAP, 0, true);
    y += hh + 10;
    // Yuborish jarayoni
    if (tk.band || !tk.jxato.empty() || tk.holat == L"tayyor") {
        float ulush = tk.jami ? std::clamp((float)tk.yuborildi / tk.jami, 0.f, 1.f) : 0;
        if (tk.band || tk.holat == L"tayyor") {
            gfx.quti({x, y, iw, 10}, M->panel2, 5, M->chegara);
            if (ulush > 0) gfx.quti({x, y, iw * ulush, 10}, M->tasdiq, 5);
            y += 16;
        }
        wstring jm = tk.band ? (tk.holat == L"tekshirilmoqda" ? L"Tarozidan qayta o'qib tekshirilmoqda..."
                                                               : L"Yuborilmoqda: " + std::to_wstring(tk.yuborildi) +
                                                                     L" / " + std::to_wstring(tk.jami))
                             : tk.holat == L"tayyor" ? L"Yuborildi va tekshirildi: " + std::to_wstring(tk.jami) + L" ta"
                                                     : tk.jxato;
        float jb = gfx.matn_boyi(jm, 13, 600, iw) + 4;
        gfx.matn({x, y, iw, jb}, jm, 13, 600, tk.band ? M->matn2 : tk.holat == L"tayyor" ? M->tasdiq : M->xato, CHAP,
                 0, true);
        y += jb + 10;
    }
    // Ro'yxatlar (saytdagi <details>)
    auto ochilma = [&](const string& id, const wstring& matn, bool& ochiq) {
        R r = {x, y, iw, 40};
        if (ui.bosildi(id, r)) ochiq = !ochiq;
        gfx.quti(r, ui.ustida(r) ? M->panel2 : M->panel, 8, M->chegara);
        float cx = r.x + 18, cy = r.y + 20;
        if (ochiq) {
            gfx.chiziq(cx - 5, cy - 2.5f, cx, cy + 2.5f, M->matn2, 1.6f);
            gfx.chiziq(cx, cy + 2.5f, cx + 5, cy - 2.5f, M->matn2, 1.6f);
        } else {
            gfx.chiziq(cx - 2.5f, cy - 5, cx + 2.5f, cy, M->matn2, 1.6f);
            gfx.chiziq(cx + 2.5f, cy, cx - 2.5f, cy + 5, M->matn2, 1.6f);
        }
        gfx.matn({r.x + 34, r.y, r.w - 44, r.h}, matn, 14, 600, M->matn);
        y += 40 + 6;
    };
    ochilma("tz/ot", L"Taroziga ketadigan tovarlar: " + std::to_wstring(tk.tayyor.size()), tk.ochiq_t);
    if (tk.ochiq_t) {
        std::vector<float> en = {90, iw - 90 - 170 - 150, 170, 150};
        y += jadval_qator({x, y, iw, 32}, {L"Kod", L"Tarozidagi nomi", L"Narx, so'm/kg", L"Shtrix boshi"}, en, true);
        for (auto& t : tk.tayyor) y += jadval_qator({x, y, iw, 36}, t, en, false);
        if (tk.tayyor.empty()) {
            gfx.matn({x + 10, y, iw, 36}, L"Yo'q", 14, 400, M->matn3);
            y += 36;
        }
        y += 8;
    }
    ochilma("tz/oo", L"O'tkaziladigan kg tovarlar: " + std::to_wstring(tk.otkaz.size()), tk.ochiq_o);
    if (tk.ochiq_o) {
        std::vector<float> en = {90, (iw - 90) * .55f, (iw - 90) * .45f};
        y += jadval_qator({x, y, iw, 32}, {L"Kod", L"Nomi", L"Sabab"}, en, true);
        for (auto& t : tk.otkaz) y += jadval_qator({x, y, iw, 36}, t, en, false);
        if (tk.otkaz.empty()) {
            gfx.matn({x + 10, y, iw, 36}, L"Yo'q", 14, 400, M->matn3);
            y += 36;
        }
        y += 8;
    }
    wstring izoh =
        L"Faqat sotish birligi kg bo'lgan mahsulotlar yuboriladi. Kodlar ombordagidek. Sotish narxi yuboriladi, "
        L"kelish narxi yuborilmaydi. Tarozidagi boshqa mahsulotlar yashiriladi. Yuborishdan oldin zaxira olinadi va "
        L"tugagach ma'lumotlar qayta o'qib tekshiriladi.\n"
        L"Tarozi nomi 32 belgigacha qisqartiriladi. Narxsiz, moslanmagan birlikdagi yoki 999 999,99 so'mdan qimmat "
        L"mahsulotlar o'tkazib yuboriladi. Tarozi sotish narxida 2 kasr xonasi bilan sozlangan bo'lishi kerak.\n"
        L"«Shtrix boshi» — tarozi etiketkasidagi shtrixning birinchi 7 raqami: 2 + tarozi PLU. Keyin og'irlik "
        L"grammda va nazorat raqami keladi.\n"
        L"Tarozi drayveri (rtslabelscale.dll) server kompyuterida kerak. Topilmasa, qaysi papkaga qo'yish "
        L"«Aloqani tekshirish» xabarida yoziladi (saytning qurilma_drivers\\rongta papkasi ham qidiriladi).";
    float ib = gfx.matn_boyi(izoh, 12, 400, iw) + 4;
    gfx.matn({x, y, iw, ib}, izoh, 12, 400, M->matn3, CHAP, 0, true);
    y += ib + 14;
    return y - y0;
}
