// Xozmag — qurilish mollari do'koni uchun kassa, ombor va qarz daftari (Windows, C++).
#include "sahifa.h"

#include <shlobj.h>
#include "versiya_raqam.h"  // qurish.ps1 versiya.txt dan yasaydi

#ifndef XOZMAG_VERSIYA
#define XOZMAG_VERSIYA L"0.0.0"
#endif

static Sahifa* bolimlar[B_SONI];
static HWND tugmalar[B_SONI];
static HWND hLogo, hVersiya;
static int joriy = -1;
static wstring papka;
static HBRUSH yonFon;

wstring versiya() { return XOZMAG_VERSIYA; }
wstring malumot_papkasi() { return papka; }

// ---------- Sahifa oynasi ----------

static LRESULT CALLBACK sahifa_jarayoni(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_NCCREATE) {
        auto* s = (Sahifa*)((CREATESTRUCTW*)l)->lpCreateParams;
        s->h = h;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)s);
    }
    auto* s = (Sahifa*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (!s) return DefWindowProcW(h, m, w, l);
    switch (m) {
        case WM_COMMAND:
            try {
                s->buyruq(LOWORD(w), HIWORD(w), (HWND)l);
            } catch (const std::exception& e) {
                xabar(nullptr, L"Kutilmagan xato:\n" + W(e.what()), true);
            }
            return 0;
        case WM_NOTIFY:
            return s->xabarnoma((NMHDR*)l);
        case WM_DRAWITEM:
            if (tugma_chiz(l)) return TRUE;
            break;
        case WM_CTLCOLORSTATIC: {
            HBRUSH b = s->rang((HDC)w, (HWND)l);
            if (b) return (LRESULT)b;
            SetBkColor((HDC)w, RANG_FON);
            return (LRESULT)oqFon;
        }
        case WM_SIZE:
            s->joylash(LOWORD(l), HIWORD(l));
            return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

void sahifa_sinfi() {
    WNDCLASSEXW wc = {sizeof wc};
    wc.lpfnWndProc = sahifa_jarayoni;
    wc.hInstance = hIlova;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = oqFon;
    wc.lpszClassName = L"XozmagSahifa";
    RegisterClassExW(&wc);
}

HWND sahifa_yarat(Sahifa* s, HWND ota) {
    CreateWindowExW(WS_EX_CONTROLPARENT, L"XozmagSahifa", L"", WS_CHILD | WS_CLIPCHILDREN, 0, 0,
                    10, 10, ota, nullptr, hIlova, s);
    s->yarat();
    return s->h;
}

// ---------- Asosiy oyna ----------

static const wchar_t* BOLIM_NOMI[B_SONI] = {L"KASSA", L"QARZ DAFTARI", L"OMBOR", L"HISOBOT",
                                            L"SOZLAMALAR"};

void bolimga_ot(int i) {
    if (i == joriy) {
        bolimlar[i]->korsatildi();
        return;
    }
    if (joriy >= 0) {
        ShowWindow(bolimlar[joriy]->h, SW_HIDE);
        tugma_rangi(tugmalar[joriy], T_YON);
    }
    joriy = i;
    tugma_rangi(tugmalar[i], T_YON_FAOL);
    try {
        bolimlar[i]->korsatildi();
    } catch (const std::exception& e) {
        xabar(nullptr, L"Ma'lumot o'qilmadi:\n" + W(e.what()), true);
    }
    ShowWindow(bolimlar[i]->h, SW_SHOW);
}

static void joylash(int w, int h) {
    int yon = dp(200);
    joy(hLogo, dp(16), dp(14), yon - dp(32), dp(40));
    int y = dp(70);
    for (int i = 0; i < B_SONI; ++i) {
        joy(tugmalar[i], 0, y, yon, dp(62));
        y += dp(62) + dp(2);
    }
    joy(hVersiya, dp(12), h - dp(52), yon - dp(24), dp(44));
    for (auto* s : bolimlar) joy(s->h, yon, 0, w - yon, h);
}

static LRESULT CALLBACK asosiy_jarayoni(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
        case WM_CREATE: {
            hAsosiy = h;
            hLogo = yorliq(h, L"XOZMAG", 0, fJuda);
            for (int i = 0; i < B_SONI; ++i)
                tugmalar[i] = tugma(h, BOLIM_NOMI[i], 1000 + i, T_YON, fTugma);
            hVersiya = yorliq(h, L"v" + versiya() + L"\nC++ / SQLite", 0, fOddiy);
            bolimlar[B_KASSA] = kassa_sahifa();
            bolimlar[B_QARZ] = qarz_sahifa();
            bolimlar[B_OMBOR] = ombor_sahifa();
            bolimlar[B_HISOBOT] = hisobot_sahifa();
            bolimlar[B_SOZLAMA] = sozlama_sahifa();
            for (auto* s : bolimlar) sahifa_yarat(s, h);
            return 0;
        }
        case WM_SIZE:
            joylash(LOWORD(l), HIWORD(l));
            return 0;
        case WM_COMMAND:
            if (LOWORD(w) >= 1000 && LOWORD(w) < 1000 + B_SONI) bolimga_ot(LOWORD(w) - 1000);
            return 0;
        case WM_DRAWITEM:
            if (tugma_chiz(l)) return TRUE;
            break;
        case WM_CTLCOLORSTATIC:
            SetBkColor((HDC)w, RANG_YON);
            SetTextColor((HDC)w, (HWND)l == hLogo ? RGB(255, 255, 255) : RGB(160, 170, 180));
            return (LRESULT)yonFon;
        case WM_GETMINMAXINFO: {
            auto* mm = (MINMAXINFO*)l;
            mm->ptMinTrackSize = {dp(1000), dp(640)};
            return 0;
        }
        case WM_CLOSE: {
            if (kassa_savat_bormi() &&
                !sorash(h, L"Kassada yakunlanmagan chek bor. Baribir chiqilsinmi?"))
                return 0;
            DestroyWindow(h);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static wstring standart_papka() {
    PWSTR p = nullptr;
    wstring r;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) r = p;
    CoTaskMemFree(p);
    return r + L"\\Xozmag";
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int korinish) {
    hIlova = hi;
    // Ma'lumot papkasi: --malumot <papka> (sinov uchun), aks holda %LOCALAPPDATA%\Xozmag
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    papka = standart_papka();
    for (int i = 1; i + 1 < argc; ++i)
        if (wcscmp(argv[i], L"--malumot") == 0) papka = argv[i + 1];
    LocalFree(argv);

    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\XozmagIlovasi");
    if (GetLastError() == ERROR_ALREADY_EXISTS && papka == standart_papka()) {
        HWND bor = FindWindowW(L"XozmagAsosiy", nullptr);
        if (bor) {
            ShowWindow(bor, SW_RESTORE);
            SetForegroundWindow(bor);
        }
        return 0;
    }

    INITCOMMONCONTROLSEX icc = {sizeof icc, ICC_WIN95_CLASSES | ICC_DATE_CLASSES};
    InitCommonControlsEx(&icc);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ui_boshla();
    sahifa_sinfi();
    yonFon = CreateSolidBrush(RANG_YON);

    SHCreateDirectoryExW(nullptr, papka.c_str(), nullptr);
    try {
        baza.och(papka + L"\\xozmag.sqlite3");
    } catch (const std::exception& e) {
        MessageBoxW(nullptr, (L"Bazani ochib bo'lmadi:\n" + papka + L"\\xozmag.sqlite3\n\n" +
                              W(e.what())).c_str(),
                    L"Xozmag", MB_ICONERROR);
        return 1;
    }

    WNDCLASSEXW wc = {sizeof wc};
    wc.lpfnWndProc = asosiy_jarayoni;
    wc.hInstance = hi;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = yonFon;
    wc.lpszClassName = L"XozmagAsosiy";
    wc.hIcon = LoadIconW(hi, MAKEINTRESOURCEW(1));
    wc.hIconSm = (HICON)LoadImageW(hi, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, 0);
    RegisterClassExW(&wc);
    wstring sarlavha = L"Xozmag v" + versiya();
    if (papka != standart_papka()) sarlavha += L"  —  " + papka;
    HWND h = CreateWindowExW(WS_EX_CONTROLPARENT, L"XozmagAsosiy", sarlavha.c_str(),
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
                             dp(1280), dp(760), nullptr, nullptr, hi, nullptr);
    ShowWindow(h, korinish == SW_SHOWMINNOACTIVE ? korinish : SW_SHOWMAXIMIZED);
    bolimga_ot(B_KASSA);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        bool klav = m.message == WM_KEYDOWN || m.message == WM_CHAR || m.message == WM_SYSKEYDOWN;
        if (klav && joriy >= 0 && GetAncestor(m.hwnd, GA_ROOT) == hAsosiy) {
            bool ishlandi = false;
            try {
                ishlandi = bolimlar[joriy]->kalit(m);
            } catch (const std::exception& e) {
                xabar(nullptr, L"Kutilmagan xato:\n" + W(e.what()), true);
                ishlandi = true;
            }
            if (ishlandi) continue;
            if (m.message == WM_KEYDOWN && m.wParam == VK_TAB && IsDialogMessageW(hAsosiy, &m))
                continue;
        }
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    baza.yop();
    if (mutex) CloseHandle(mutex);
    return 0;
}
