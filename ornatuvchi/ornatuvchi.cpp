// Xozmag o'rnatuvchisi: ichida Xozmag.exe va sqlite3.dll bor (resurs sifatida).
//
//   Xozmag_Ornatuvchi.exe              — o'rnatadi (administrator huquqi kerak emas)
//   ... /jim                           — savolsiz o'rnatadi
//   ... /joy:"D:\papka" /yorliqsiz     — boshqa joyga, yorliq va ro'yxatsiz (sinov uchun)
//   Ochirish.exe /ochirish             — o'chiradi (o'rnatilgan papkada turadi)
//
// Dastur: %LOCALAPPDATA%\Programs\Xozmag. Ma'lumotlar (baza) alohida —
// %LOCALAPPDATA%\Xozmag, o'chirganda ham, qayta o'rnatganda ham tegilmaydi.
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <string>

#include "versiya_raqam.h"

using std::wstring;

static const wchar_t* NOM = L"Xozmag";
static const wchar_t* ROYXAT = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Xozmag";
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

// Xozmag ochiq bo'lsa fayllarni almashtirib bo'lmaydi.
static bool dastur_yopiq() {
    while (FindWindowW(L"XozmagAsosiy", nullptr)) {
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

static void royxatga_yoz(const wstring& joy) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, ROYXAT, 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr))
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

static int ornat(wstring joy, bool yorliqlar) {
    wstring malumot = papka_yoli(FOLDERID_LocalAppData) + L"\\Xozmag";
    if (!sorash(L"Xozmag v" XOZMAG_VERSIYA L" kompyuterga o'rnatilsinmi?\n\nDastur: " + joy +
                L"\nMa'lumotlar (baza): " + malumot +
                L"\n\nIsh stoli va Pusk menyusiga yorliq qo'yiladi. Administrator huquqi kerak "
                L"emas. Oldingi versiya bo'lsa yangilanadi, baza saqlanib qoladi."))
        return 1;
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
        yorliq_yasa(papka_yoli(FOLDERID_Desktop) + L"\\Xozmag.lnk", exe, L"",
                    L"Xozmag — kassa, ombor va qarz daftari");
        wstring pusk = papka_yoli(FOLDERID_Programs) + L"\\Xozmag";
        CreateDirectoryW(pusk.c_str(), nullptr);
        yorliq_yasa(pusk + L"\\Xozmag.lnk", exe, L"", L"Xozmag — kassa, ombor va qarz daftari");
        yorliq_yasa(pusk + L"\\Xozmag'ni o'chirish.lnk", joy + L"\\Ochirish.exe", L"/ochirish",
                    L"Xozmag'ni kompyuterdan o'chirish");
        royxatga_yoz(joy);
    }
    if (!jim && MessageBoxW(nullptr, L"Xozmag v" XOZMAG_VERSIYA L" o'rnatildi.\n\nHozir ishga tushirilsinmi?",
                            L"Xozmag o'rnatuvchisi", MB_YESNO | MB_ICONINFORMATION) == IDYES)
        ShellExecuteW(nullptr, L"open", exe.c_str(), nullptr, joy.c_str(), SW_SHOWNORMAL);
    return 0;
}

static int ochir() {
    wchar_t men[MAX_PATH];
    GetModuleFileNameW(nullptr, men, MAX_PATH);
    wstring joy = men;
    joy = joy.substr(0, joy.find_last_of(L'\\'));
    wstring malumot = papka_yoli(FOLDERID_LocalAppData) + L"\\Xozmag";
    if (!sorash(L"Xozmag kompyuterdan o'chirilsinmi?\n\nDastur papkasi: " + joy +
                L"\n\nBaza va zaxiralar o'chirilMAYDI — ular shu yerda qoladi:\n" + malumot))
        return 1;
    if (!dastur_yopiq()) return 2;
    DeleteFileW((joy + L"\\Xozmag.exe").c_str());
    DeleteFileW((joy + L"\\sqlite3.dll").c_str());
    DeleteFileW((papka_yoli(FOLDERID_Desktop) + L"\\Xozmag.lnk").c_str());
    wstring pusk = papka_yoli(FOLDERID_Programs) + L"\\Xozmag";
    DeleteFileW((pusk + L"\\Xozmag.lnk").c_str());
    DeleteFileW((pusk + L"\\Xozmag'ni o'chirish.lnk").c_str());
    RemoveDirectoryW(pusk.c_str());
    RegDeleteKeyW(HKEY_CURRENT_USER, ROYXAT);
    xabar(L"Xozmag o'chirildi.\n\nMa'lumotlar saqlanib qoldi:\n" + malumot);
    // O'zini o'chirish: dastur yopilgach cmd Ochirish.exe ni va bo'sh papkani o'chiradi.
    wstring buyruq = L"cmd.exe /c ping -n 3 127.0.0.1 >nul & del /f /q \"" + joy +
                     L"\\Ochirish.exe\" & rmdir \"" + joy + L"\"";
    STARTUPINFOW si = {sizeof si};
    PROCESS_INFORMATION pi;
    if (CreateProcessW(nullptr, buyruq.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
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
    wstring joy = papka_yoli(FOLDERID_LocalAppData) + L"\\Programs\\Xozmag";
    for (int i = 1; i < argc; ++i) {
        wstring a = argv[i];
        if (a == L"/ochirish") ochirish = true;
        else if (a == L"/jim") jim = true;
        else if (a == L"/yorliqsiz") yorliqlar = false;
        else if (a.rfind(L"/joy:", 0) == 0) joy = a.substr(5);
    }
    LocalFree(argv);
    int r = ochirish ? ochir() : ornat(joy, yorliqlar);
    CoUninitialize();
    return r;
}
