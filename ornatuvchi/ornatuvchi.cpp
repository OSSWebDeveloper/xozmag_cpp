// Xozmag o'rnatuvchisi: ichida Xozmag.exe va sqlite3.dll bor (resurs sifatida).
//
//   Xozmag_Ornatuvchi.exe              — o'rnatadi (administrator: UAC so'raladi)
//   ... /jim                           — savolsiz o'rnatadi (kassa sifatida)
//   ... /server                        — /jim bilan: server kompyuteri sifatida
//   ... /joy:"D:\papka" /yorliqsiz     — boshqa joyga, yorliq va ro'yxatsiz (sinov uchun)
//   Ochirish.exe /ochirish             — o'chiradi (o'rnatilgan papkada turadi)
//
// Dastur: Program Files\Xozmag, yorliqlar hamma foydalanuvchilarga. Baza serverda —
// do'kon kompyuteridagi «Xozmag Server» ochadi; o'chirganda ham bazaga tegilmaydi.
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <string>

#include "versiya_raqam.h"

using std::wstring;

static const wchar_t* NOM = L"Xozmag";
static const wchar_t* ROYXAT = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Xozmag";
static const wchar_t* SERVER_NOMI = L"Xozmag Server";  // xavfsizlik devori qoidasi va Windows vazifasi
static bool jim = false;

static wstring papka_yoli(REFKNOWNFOLDERID id) {
    PWSTR p = nullptr;
    wstring r;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p))) r = p;
    CoTaskMemFree(p);
    return r;
}

static void xabar(const wstring& m, UINT tur = MB_ICONINFORMATION) {
    if (!jim) MessageBoxW(nullptr, m.c_str(), L"Xozmag o'rnatuvchisi", MB_OK | tur);
}

static bool sorash(const wstring& m) {
    return jim || MessageBoxW(nullptr, m.c_str(), L"Xozmag o'rnatuvchisi",
                              MB_YESNO | MB_ICONQUESTION) == IDYES;
}

// Xozmag (kassa yoki server) ochiq bo'lsa fayllarni almashtirib bo'lmaydi.
static bool dastur_yopiq() {
    while (FindWindowW(L"UlugbekXozmag", nullptr) || FindWindowW(L"UlugbekXozmagServer", nullptr)) {
        if (jim) return false;
        if (MessageBoxW(nullptr, L"Xozmag hozir ochiq. Uni yoping va «Qayta urinish» ni bosing.",
                        L"Xozmag o'rnatuvchisi", MB_RETRYCANCEL | MB_ICONWARNING) != IDRETRY)
            return false;
    }
    return true;
}

static bool resursni_yoz(int id, const wstring& fayl) {
    HRSRC r = FindResourceW(nullptr, MAKEINTRESOURCEW(id), (LPCWSTR)RT_RCDATA);
    if (!r) return false;
    HGLOBAL g = LoadResource(nullptr, r);
    DWORD hajm = SizeofResource(nullptr, r);
    const void* p = LockResource(g);
    wstring vaqtli = fayl + L".yangi";
    HANDLE h = CreateFileW(vaqtli.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD yozildi = 0;
    bool ok = WriteFile(h, p, hajm, &yozildi, nullptr) && yozildi == hajm;
    CloseHandle(h);
    if (ok) ok = MoveFileExW(vaqtli.c_str(), fayl.c_str(), MOVEFILE_REPLACE_EXISTING);
    if (!ok) DeleteFileW(vaqtli.c_str());
    return ok;
}

static bool yorliq_yasa(const wstring& lnk, const wstring& exe, const wstring& arg,
                        const wstring& tavsif) {
    IShellLinkW* sl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                                (void**)&sl)))
        return false;
    sl->SetPath(exe.c_str());
    sl->SetArguments(arg.c_str());
    wstring ish = exe.substr(0, exe.find_last_of(L'\\'));
    sl->SetWorkingDirectory(ish.c_str());
    sl->SetIconLocation(exe.c_str(), 0);
    sl->SetDescription(tavsif.c_str());
    IPersistFile* pf = nullptr;
    bool ok = SUCCEEDED(sl->QueryInterface(IID_IPersistFile, (void**)&pf)) &&
              SUCCEEDED(pf->Save(lnk.c_str(), TRUE));
    if (pf) pf->Release();
    sl->Release();
    return ok;
}

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

// v2.1 va undan oldingisi har bir foydalanuvchiga alohida o'rnatilardi — o'sha nusxa olib tashlanadi
static void eski_nusxani_ochir() {
    wstring eski = papka_yoli(FOLDERID_LocalAppData) + L"\\Programs\\Xozmag";
    DeleteFileW((eski + L"\\Xozmag.exe").c_str());
    DeleteFileW((eski + L"\\sqlite3.dll").c_str());
    DeleteFileW((eski + L"\\Ochirish.exe").c_str());
    RemoveDirectoryW(eski.c_str());
    DeleteFileW((papka_yoli(FOLDERID_Desktop) + L"\\Xozmag.lnk").c_str());
    wstring pusk = papka_yoli(FOLDERID_Programs) + L"\\Xozmag";
    DeleteFileW((pusk + L"\\Xozmag.lnk").c_str());
    DeleteFileW((pusk + L"\\Xozmag'ni o'chirish.lnk").c_str());
    RemoveDirectoryW(pusk.c_str());
    RegDeleteKeyW(HKEY_CURRENT_USER, ROYXAT);
}

static void royxatga_yoz(const wstring& joy) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, ROYXAT, 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr))
        return;
    auto matn = [&](const wchar_t* n, const wstring& v) {
        RegSetValueExW(k, n, 0, REG_SZ, (const BYTE*)v.c_str(), (DWORD)((v.size() + 1) * 2));
    };
    auto son = [&](const wchar_t* n, DWORD v) {
        RegSetValueExW(k, n, 0, REG_DWORD, (const BYTE*)&v, sizeof v);
    };
    matn(L"DisplayName", NOM);
    matn(L"DisplayVersion", XOZMAG_VERSIYA);
    matn(L"Publisher", NOM);
    matn(L"InstallLocation", joy);
    matn(L"DisplayIcon", joy + L"\\Xozmag.exe");
    matn(L"UninstallString", L"\"" + joy + L"\\Ochirish.exe\" /ochirish");
    son(L"NoModify", 1);
    son(L"NoRepair", 1);
    son(L"EstimatedSize", 6000);
    RegCloseKey(k);
}

static int ornat(wstring joy, bool yorliqlar, int server_bol) {
    if (!sorash(L"Xozmag v" XOZMAG_VERSIYA L" kompyuterga o'rnatilsinmi?\n\nDastur: " + joy +
                L"\n\nKassalar do'kondagi server kompyuteriga IP manzil orqali ulanadi. Baza serverda "
                L"turadi — o'rnatish va o'chirish unga tegmaydi."))
        return 1;
    // Bu kompyuter servermi? (baza shu yerda turadi, kassalar shunga ulanadi)
    bool server = server_bol > 0;
    if (server_bol < 0 && !jim)
        server = MessageBoxW(nullptr,
                             L"Bu kompyuter do'kon SERVERI bo'ladimi?\n\n«Ha» — baza shu kompyuterda turadi, "
                             L"«Xozmag Server» ish stoliga qo'yiladi va kassalar shu kompyuterga ulanadi.\n"
                             L"«Yo'q» — bu oddiy kassa, server boshqa kompyuterda.",
                             L"Xozmag o'rnatuvchisi", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES;
    if (!dastur_yopiq()) return 2;
    SHCreateDirectoryExW(nullptr, joy.c_str(), nullptr);
    wchar_t men[MAX_PATH];
    GetModuleFileNameW(nullptr, men, MAX_PATH);
    if (!resursni_yoz(101, joy + L"\\Xozmag.exe") || !resursni_yoz(102, joy + L"\\sqlite3.dll") ||
        !CopyFileW(men, (joy + L"\\Ochirish.exe").c_str(), FALSE)) {
        xabar(L"Fayllar yozilmadi:\n" + joy + L"\n\nPapkaga yozish huquqi bormi?", MB_ICONERROR);
        return 3;
    }
    wstring exe = joy + L"\\Xozmag.exe";
    if (yorliqlar) {
        eski_nusxani_ochir();
        const wchar_t* KASSA_TAVSIF = L"Xozmag — kassa, ombor va qarz daftari";
        const wchar_t* SERVER_TAVSIF = L"Xozmag Server — baza shu kompyuterda, kassalar shunga ulanadi";
        yorliq_yasa(papka_yoli(FOLDERID_PublicDesktop) + L"\\Xozmag.lnk", exe, L"", KASSA_TAVSIF);
        wstring pusk = papka_yoli(FOLDERID_CommonPrograms) + L"\\Xozmag";
        CreateDirectoryW(pusk.c_str(), nullptr);
        yorliq_yasa(pusk + L"\\Xozmag.lnk", exe, L"", KASSA_TAVSIF);
        yorliq_yasa(pusk + L"\\Xozmag Server.lnk", exe, L"--server", SERVER_TAVSIF);
        yorliq_yasa(pusk + L"\\Xozmag'ni o'chirish.lnk", joy + L"\\Ochirish.exe", L"/ochirish",
                    L"Xozmag'ni kompyuterdan o'chirish");
        if (server)
            yorliq_yasa(papka_yoli(FOLDERID_PublicDesktop) + L"\\Xozmag Server.lnk", exe, L"--server", SERVER_TAVSIF);
        royxatga_yoz(joy);
    }
    if (server) {  // kassalar ulana olsin
        buyruq(L"netsh advfirewall firewall delete rule name=\"" + wstring(SERVER_NOMI) + L"\"");
        buyruq(L"netsh advfirewall firewall add rule name=\"" + wstring(SERVER_NOMI) +
               L"\" dir=in action=allow protocol=TCP profile=any enable=yes program=\"" + exe + L"\"");
    }
    if (!jim) {
        wstring m = L"Xozmag v" XOZMAG_VERSIYA L" o'rnatildi.\n\n";
        m += server ? L"«Xozmag Server» hozir ishga tushirilsinmi? Oynasida kassalar uchun IP manzil yoziladi."
                    : L"Hozir ishga tushirilsinmi? Ochilganda server IP manzili so'raladi.";
        if (MessageBoxW(nullptr, m.c_str(), L"Xozmag o'rnatuvchisi", MB_YESNO | MB_ICONINFORMATION) == IDYES)
            ShellExecuteW(nullptr, L"open", exe.c_str(), server ? L"--server" : nullptr, joy.c_str(), SW_SHOWNORMAL);
    }
    return 0;
}

static int ochir() {
    wchar_t men[MAX_PATH];
    GetModuleFileNameW(nullptr, men, MAX_PATH);
    wstring joy = men;
    joy = joy.substr(0, joy.find_last_of(L'\\'));
    if (!sorash(L"Xozmag kompyuterdan o'chirilsinmi?\n\nDastur papkasi: " + joy +
                L"\n\nBaza o'chirilMAYDI: u server kompyuterida (sayt bazasi yoki %LOCALAPPDATA%\\Xozmag) qoladi."))
        return 1;
    if (!dastur_yopiq()) return 2;
    DeleteFileW((joy + L"\\Xozmag.exe").c_str());
    DeleteFileW((joy + L"\\sqlite3.dll").c_str());
    DeleteFileW((papka_yoli(FOLDERID_PublicDesktop) + L"\\Xozmag.lnk").c_str());
    DeleteFileW((papka_yoli(FOLDERID_PublicDesktop) + L"\\Xozmag Server.lnk").c_str());
    wstring pusk = papka_yoli(FOLDERID_CommonPrograms) + L"\\Xozmag";
    DeleteFileW((pusk + L"\\Xozmag.lnk").c_str());
    DeleteFileW((pusk + L"\\Xozmag Server.lnk").c_str());
    DeleteFileW((pusk + L"\\Xozmag'ni o'chirish.lnk").c_str());
    RemoveDirectoryW(pusk.c_str());
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, ROYXAT);
    buyruq(L"netsh advfirewall firewall delete rule name=\"" + wstring(SERVER_NOMI) + L"\"");
    buyruq(L"schtasks /Delete /F /TN \"" + wstring(SERVER_NOMI) + L"\"");
    xabar(L"Xozmag o'chirildi. Baza saqlanib qoldi.");
    // O'zini o'chirish: dastur yopilgach cmd Ochirish.exe ni va bo'sh papkani o'chiradi.
    wstring cmd = L"cmd.exe /c ping -n 3 127.0.0.1 >nul & del /f /q \"" + joy +
                  L"\\Ochirish.exe\" & rmdir \"" + joy + L"\"";
    STARTUPINFOW si = {sizeof si};
    PROCESS_INFORMATION pi;
    if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                       nullptr, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool ochirish = false, yorliqlar = true;
    int server = -1;  // -1 — so'raladi
    wstring joy = papka_yoli(FOLDERID_ProgramFiles) + L"\\Xozmag";
    for (int i = 1; i < argc; ++i) {
        wstring a = argv[i];
        if (a == L"/ochirish") ochirish = true;
        else if (a == L"/jim") jim = true;
        else if (a == L"/yorliqsiz") yorliqlar = false;
        else if (a == L"/server") server = 1;
        else if (a.rfind(L"/joy:", 0) == 0) joy = a.substr(5);
    }
    LocalFree(argv);
    if (jim && server < 0) server = 0;
    int r = ochirish ? ochir() : ornat(joy, yorliqlar, server);
    CoUninitialize();
    return r;
}
