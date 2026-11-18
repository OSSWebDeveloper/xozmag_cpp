// Ulug'bek Xozmag — sayt bilan bir xil ko'rinish va ishlash, lekin brauzersiz (Win32 + Direct2D).
#include "ilova.h"

#include <shlobj.h>
#include <windowsx.h>

#include "versiya_raqam.h"

Foydalanuvchi foydalanuvchi;
bool sensor = false;
static Joy joriy = J_KIRISH;
static HWND hOyna;
static wstring ini_yoli;

wstring versiya() { return XOZMAG_VERSIYA; }
Joy joriy_joy() { return joriy; }

// ---------- Sozlamalar (brauzerdagi localStorage o'rnida) ----------

static wstring ini_ol(const wchar_t* kalit, const wchar_t* standart) {
    wchar_t b[512];
    GetPrivateProfileStringW(L"xozmag", kalit, standart, b, 512, ini_yoli.c_str());
    return b;
}
static void ini_yoz(const wchar_t* kalit, const wstring& v) {
    WritePrivateProfileStringW(L"xozmag", kalit, v.c_str(), ini_yoli.c_str());
}

static void mavzu_qoy(bool kechki) {
    M = kechki ? &KECHKI : &KUNDUZGI;
    ini_yoz(L"mavzu", kechki ? L"kechki" : L"kunduzgi");
    // EDIT'lar o'z rangini qayta so'rasin
    InvalidateRect(hOyna, nullptr, TRUE);
    EnumChildWindows(hOyna, [](HWND h, LPARAM) -> BOOL { InvalidateRect(h, nullptr, TRUE); return TRUE; }, 0);
}

// ---------- O'tish ----------

void ot(Joy j, long long p) {
    if (j != J_KIRISH && !foydalanuvchi.id) j = J_KIRISH;
    joriy = j;
    switch (j) {
        case J_SOTUV: sotuv_ochildi(p); break;
        case J_QARZ_YOZ: qarz_yozish_ochildi(p); break;
        case J_QARZDORLAR: qarzdorlar_ochildi(p); break;
        case J_HISOBOT: hisobot_ochildi(p); break;
        case J_QARZDOR_ROYXAT: qarzdor_royxat_ochildi(p); break;
        case J_HUDUDLAR: case J_HUDUD: hududlar_ochildi(j == J_HUDUD ? p : 0); break;
        case J_KARTA: karta_ochildi(p); break;
        case J_YANGI_QARZDOR: yangi_qarzdor_ochildi(p); break;
        case J_QIDIRISH: qidirish_ochildi(p); break;
        case J_OMBOR: ombor_ochildi(p); break;
        case J_MAHSULOT: mahsulot_ochildi(p); break;
        case J_SOTUVLAR: sotuvlar_ochildi(p); break;
        case J_AKKAUNT: akkaunt_ochildi(p); break;
        case J_QURILMALAR: qurilmalar_ochildi(p); break;
        case J_EXCEL: excel_yuklash_ochildi(p); break;
        case J_YANGILANISHLAR: yangilanishlar_ochildi(p); break;
        case J_YANGILANISH: yangilanish_ochildi(p); break;
        case J_QATOR: qator_ochildi(p); break;
        default: break;
    }
    SetFocus(hOyna);
    ui.qayta = true;
    InvalidateRect(hOyna, nullptr, FALSE);
}

void chiqish() {
    sozlama_yoz("kirgan", L"");
    foydalanuvchi = {};
    ot(J_KIRISH);
}

// ---------- Yon menyu (.yon-menyu) ----------

struct MenyuBand {
    const wchar_t* nom;
    const char* ikon;
    Joy joy;
};

static bool faolmi(Joy band) {
    switch (band) {
        case J_BOSH: return joriy == J_BOSH;
        case J_SOTUV: return joriy == J_SOTUV || joriy == J_SOTUVLAR;
        case J_HISOBOT: return joriy == J_HISOBOT;
        case J_QARZDORLAR:
            return (joriy >= J_QARZDORLAR && joriy <= J_QIDIRISH) || joriy == J_QARZ_YOZ;
        case J_OMBOR: return joriy == J_OMBOR || joriy == J_MAHSULOT || (joriy >= J_EXCEL && joriy <= J_QATOR);
        default: return joriy == band;
    }
}

static void yon_menyu(R r) {
    gfx.quti(r, M->panel);
    gfx.chiziq(r.o() - .5f, r.y, r.o() - .5f, r.p(), M->chegara);
    float x = r.x + 8, en = r.w - 16, y = r.y + 10;
    // logo
    R logo = {r.x + (r.w - 46) / 2, y, 46, 46};
    gfx.gradient_quti(logo, M->urgu, M->urgu_quyuq, 12);
    gfx.ikon("i-dokon", {logo.x + 10, logo.y + 10, 26, 26}, M->asosiy_matn);
    y += 46 + 8 + 6;

    static const MenyuBand bandlar[] = {
        {L"Bosh", "i-uy", J_BOSH},           {L"Sotuv", "i-savdo", J_SOTUV},
        {L"Hisobot", "i-savdo", J_HISOBOT},  {L"Qarzdor", "i-odamlar", J_QARZDORLAR},
        {L"Ombor", "i-quti", J_OMBOR},       {L"Qurilmalar", "i-monitor", J_QURILMALAR},
        {L"Kassalar", "i-savdo", J_KASSALAR}, {L"Login / Parol", "i-odamlar", J_AKKAUNT},
    };
    for (auto& b : bandlar) {
        wstring nom = b.nom;
        float matn_boy = gfx.matn_boyi(nom, 11, 500, en - 8);
        R t = {x, y, en, 9 + 22 + 4 + matn_boy + 7};
        bool f = faolmi(b.joy), hov = ui.ustida(t);
        Rang rang = f ? M->urgu_matn : hov ? M->matn : M->matn2;
        if (f) gfx.quti(t, M->urgu_fon, 9);
        else if (hov) gfx.quti(t, M->panel2, 9);
        gfx.ikon(b.ikon, {t.x + (t.w - 22) / 2, t.y + 9, 22, 22}, rang);
        gfx.matn({t.x + 4, t.y + 35, t.w - 8, matn_boy + 2}, nom, 11, 500, rang, MARKAZ, 0, true);
        if (ui.bosildi(string("menyu/") + U(nom), t)) {
            if (b.joy == J_SOTUV) ot(J_SOTUV, 0);
            else ot(b.joy);
        }
        y += t.h + 6;
    }

    // pastki qism: richaglar, chiqish, versiya (joy yetmasa menyudan keyin turadi)
    float pastki_boy = 10 + 14 + 6 + 50 + 2 * (6 + 63);
    float py = std::max(r.p(), y + pastki_boy) - 10;
    py -= 14;
    gfx.matn({x, py, en, 14}, L"V" + versiya(), 10, 600, M->matn3, MARKAZ);
    py -= 6 + 50;
    R ch = {x, py, en, 50};
    bool chov = ui.ustida(ch);
    gfx.quti(ch, chov ? M->xato_fon : M->panel2, 9, M->chegara2);
    Rang crang = chov ? M->xato : M->matn3;
    gfx.ikon("i-chiqish", {ch.x + (ch.w - 22) / 2, ch.y + 7, 22, 22}, crang);
    gfx.matn({ch.x, ch.y + 31, ch.w, 14}, L"Chiqish", 11, 500, crang, MARKAZ);
    if (ui.bosildi("menyu/chiqish", ch)) chiqish();

    auto richag = [&](const string& id, const wchar_t* nom, bool yoniq, Rang rang, Rang chek) {
        py -= 6 + 63;
        R q = {x, py, en, 63};
        bool hov = ui.ustida(q);
        Rang matn = yoniq ? rang : hov ? M->matn : M->matn2;
        gfx.quti(q, M->panel2, 9, yoniq ? chek : hov ? M->chegara2 : M->chegara);
        ui.richag_chiz({q.x + (q.w - 44) / 2, q.y + 10, 44, 24}, yoniq, rang);
        gfx.matn({q.x, q.y + 41, q.w, 13}, nom, 10, 600, matn, MARKAZ);
        return ui.bosildi(id, q);
    };
    if (richag("menyu/sensor", L"SENSOR", sensor, M->tasdiq, M->tasdiq_chek)) {
        sensor = !sensor;
        ini_yoz(L"sensor", sensor ? L"1" : L"0");
    }
    if (richag("menyu/mavzu", L"KECHKI", M->kechki, M->kok, M->kok_chek)) mavzu_qoy(!M->kechki);
}

// ---------- Kadr ----------

static void chiz() {
    ui.kadr_boshla();
    if (!gfx.chizish_boshla()) return;
    R hammasi = {0, 0, ui.W, ui.H};
    gfx.quti(hammasi, M->fon);
    if (joriy == J_KIRISH) {
        kirish_chiz(hammasi);
    } else {
        const float MENYU = 82;
        R ish = {MENYU, 0, ui.W - MENYU, ui.H};
        // .sahifa { padding: 14px } — Bosh sahifa foni chetigacha boradi
        R sahifa = ish.ichki(14);
        // Menyu avval chiziladi: modal oyna ochilsa u ham xiralashadi va bosilmaydi
        yon_menyu({0, 0, MENYU, ui.H});
        switch (joriy) {
            case J_BOSH: bosh_chiz(ish); break;
            case J_SOTUV: case J_QARZ_YOZ: sotuv_chiz(sahifa); break;
            case J_SOTUVLAR: sotuvlar_chiz(sahifa); break;
            case J_HISOBOT: hisobot_chiz(sahifa); break;
            case J_QARZDORLAR: qarzdorlar_chiz(sahifa); break;
            case J_QARZDOR_ROYXAT: qarzdor_royxat_chiz(sahifa); break;
            case J_HUDUDLAR: case J_HUDUD: hududlar_chiz(sahifa); break;
            case J_KARTA: karta_chiz(sahifa); break;
            case J_YANGI_QARZDOR: yangi_qarzdor_chiz(sahifa); break;
            case J_QIDIRISH: qidirish_chiz(sahifa); break;
            case J_OMBOR: ombor_chiz(sahifa); break;
            case J_MAHSULOT: mahsulot_chiz(sahifa); break;
            case J_AKKAUNT: akkaunt_chiz(sahifa); break;
            case J_QURILMALAR: qurilmalar_chiz(sahifa); break;
            case J_EXCEL: excel_yuklash_chiz(sahifa); break;
            case J_YANGILANISHLAR: yangilanishlar_chiz(sahifa); break;
            case J_YANGILANISH: yangilanish_chiz(sahifa); break;
            case J_QATOR: qator_chiz(sahifa); break;
            case J_KASSALAR: tayyorlanmoqda_chiz(sahifa, L"Kassalar", "i-savdo"); break;
            default: break;
        }
    }
    ui.kadr_tugat();
    gfx.chizish_tugat();
    ui.maydonlarni_chiz();
}

static LRESULT CALLBACK jarayon(HWND h, UINT m, WPARAM w, LPARAM l) {
    float ms = gfx.masshtab();
    switch (m) {
        case WM_PAINT: {
            // Kadr ichida MessageBox/fayl oynasi ochilsa Windows qayta chizishni so'raydi.
            // Ichma-ich kadr bosish hodisasini ikkinchi marta «ko'rib» qoladi — ruxsat yo'q.
            static bool chizilmoqda = false, keyin = false;
            if (chizilmoqda) {
                ValidateRect(h, nullptr);
                keyin = true;  // kadr tugagach qayta chiziladi
                return 0;
            }
            struct Bayroq { bool& b; Bayroq(bool& x) : b(x) { b = true; } ~Bayroq() { b = false; } } bayroq(chizilmoqda);
            PAINTSTRUCT ps;
            BeginPaint(h, &ps);
            try {
                chiz();
            } catch (const std::exception& e) {
                gfx.chizish_tugat();
                MessageBoxW(h, (L"Kutilmagan xato:\n" + W(e.what())).c_str(), L"Xozmag", MB_ICONERROR);
            }
            EndPaint(h, &ps);
            if (keyin) {
                keyin = false;
                InvalidateRect(h, nullptr, FALSE);
            }
            return 0;
        }
        case WM_ERASEBKGND: return 1;
        case WM_SIZE:
            gfx.olcham(LOWORD(l), HIWORD(l));
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        case WM_MOUSEMOVE: {
            ui.sichqon(GET_X_LPARAM(l) / ms, GET_Y_LPARAM(l) / ms);
            TRACKMOUSEEVENT t = {sizeof t, TME_LEAVE, h, 0};
            TrackMouseEvent(&t);
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }
        case WM_MOUSELEAVE:
            ui.sichqon(-1, -1);
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        case WM_LBUTTONDOWN:
            SetFocus(h);  // brauzerdagidek: bo'sh joy bosilsa maydondan fokus ketadi
            SetCapture(h);
            ui.bosish(GET_X_LPARAM(l) / ms, GET_Y_LPARAM(l) / ms);
            InvalidateRect(h, nullptr, FALSE);
            UpdateWindow(h);
            return 0;
        case WM_LBUTTONUP:
            ReleaseCapture();
            ui.qoyish(GET_X_LPARAM(l) / ms, GET_Y_LPARAM(l) / ms);
            InvalidateRect(h, nullptr, FALSE);
            UpdateWindow(h);
            return 0;
        case WM_MOUSEWHEEL:
            ui.gildirak_ayl(GET_WHEEL_DELTA_WPARAM(w) * 0.85f);
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        case WM_SETCURSOR:
            if (LOWORD(l) == HTCLIENT && (HWND)w == h) {
                SetCursor(LoadCursor(nullptr, ui.qol_kursor ? IDC_HAND : IDC_ARROW));
                return TRUE;
            }
            break;
        case WM_CTLCOLOREDIT:
            return (LRESULT)ui.edit_rangi((HDC)w, (HWND)l);
        case WM_COMMAND:
            if (HIWORD(w) == EN_CHANGE) {
                ui.ozgardi_qosh((HWND)l);
                InvalidateRect(h, nullptr, FALSE);
            }
            if (HIWORD(w) == EN_SETFOCUS || HIWORD(w) == EN_KILLFOCUS) InvalidateRect(h, nullptr, FALSE);
            return 0;
        case WM_TIMER:
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        case WM_GETMINMAXINFO:
            ((MINMAXINFO*)l)->ptMinTrackSize = {int(900 * ms), int(600 * ms)};
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static wstring papka(REFKNOWNFOLDERID id) {
    PWSTR p = nullptr;
    wstring r;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p))) r = p;
    CoTaskMemFree(p);
    return r;
}

bool bazani_och(const wstring& yol, wstring& xato) {
    try {
        SHCreateDirectoryExW(nullptr, yol.substr(0, yol.find_last_of(L'\\')).c_str(), nullptr);
        baza.och(yol);
        return true;
    } catch (const std::exception& e) {
        xato = W(e.what());
        return false;
    }
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\UlugbekXozmagExe");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND bor = FindWindowW(L"UlugbekXozmag", nullptr);
        if (bor) { ShowWindow(bor, SW_RESTORE); SetForegroundWindow(bor); }
        return 0;
    }
    wstring malumot = papka(FOLDERID_LocalAppData) + L"\\Xozmag";
    SHCreateDirectoryExW(nullptr, malumot.c_str(), nullptr);
    ini_yoli = malumot + L"\\sozlama.ini";
    M = ini_ol(L"mavzu", L"kechki") == L"kunduzgi" ? &KUNDUZGI : &KECHKI;
    sensor = ini_ol(L"sensor", L"0") == L"1";

    // --baza <fayl> — sinov uchun boshqa baza
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    wstring yol = baza_yolini_aniqla();
    for (int i = 1; i + 1 < argc; ++i)
        if (wcscmp(argv[i], L"--baza") == 0) yol = argv[i + 1];
    LocalFree(argv);
    wstring xato;
    if (!bazani_och(yol, xato)) {
        MessageBoxW(nullptr, (L"Bazani ochib bo'lmadi:\n" + yol + L"\n\n" + xato).c_str(), L"Xozmag",
                    MB_ICONERROR);
        if (yol == oz_baza_yoli() || !bazani_och(oz_baza_yoli(), xato)) return 1;
    }

    WNDCLASSEXW wc = {sizeof wc};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = jarayon;
    wc.hInstance = hi;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"UlugbekXozmag";
    wc.hIcon = LoadIconW(hi, MAKEINTRESOURCEW(1));
    wc.hIconSm = (HICON)LoadImageW(hi, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, 0);
    RegisterClassExW(&wc);
    hOyna = CreateWindowExW(WS_EX_CONTROLPARENT, L"UlugbekXozmag", L"Ulug'bek Xozmag",
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
                            1280, 760, nullptr, nullptr, hi, nullptr);
    if (!gfx.boshla(hOyna)) {
        MessageBoxW(nullptr, L"Direct2D ishga tushmadi.", L"Xozmag", MB_ICONERROR);
        return 1;
    }
    ui.boshla(hOyna);

    // Avval kirgan bo'lsa (14 kun ichida, saytdagi sessiya kabi) — to'g'ridan-to'g'ri ichkariga
    wstring kirgan = sozlama("kirgan", L"");
    size_t bo = kirgan.find(L'|');
    if (bo != wstring::npos) {
        long long id = wcstoll(kirgan.c_str(), nullptr, 10);
        long long vaqt = wcstoll(kirgan.c_str() + bo + 1, nullptr, 10);
        if (time(nullptr) - vaqt < 14 * 86400 && foydalanuvchi_ol(id, foydalanuvchi)) {
            sozlama_yoz("kirgan", std::to_wstring(id) + L"|" + std::to_wstring(time(nullptr)));
            joriy = J_BOSH;
        }
    }
    ShowWindow(hOyna, SW_SHOWMAXIMIZED);

    // Skaner: USB skaner klaviatura bo'lib kodni juda tez yozadi va Enter bosadi
    // (static/js/skaner.js dagi qoida). Maydonga tushgan belgilar qaytariladi.
    wstring skan;
    DWORD skan_vaqt = 0;
    HWND skan_maydon = nullptr;
    wstring skan_eski;
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        bool ichida = GetAncestor(msg.hwnd, GA_ROOT) == hOyna;
        // skaner faqat kassa va ombor sahifalarida ushlanadi (saytdagi skaner.js kabi)
        bool skaner_joy = joriy == J_SOTUV || joriy == J_QARZ_YOZ || joriy == J_OMBOR;
        if (ichida && skaner_joy && msg.message == WM_CHAR && msg.wParam >= 32) {
            if (msg.time - skan_vaqt > 40 || skan.empty()) {
                skan.clear();
                skan_maydon = msg.hwnd != hOyna ? msg.hwnd : nullptr;
                skan_eski.clear();
                if (skan_maydon) {
                    int n = GetWindowTextLengthW(skan_maydon);
                    skan_eski.resize(n);
                    if (n) GetWindowTextW(skan_maydon, skan_eski.data(), n + 1);
                }
            }
            skan += (wchar_t)msg.wParam;
            skan_vaqt = msg.time;
            if (msg.hwnd == hOyna) {  // fokus hech qaysi maydonda emas
                ui.belgilar += (wchar_t)msg.wParam;
                InvalidateRect(hOyna, nullptr, FALSE);
            }
        }
        if (ichida && skaner_joy && msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN && skan.size() >= 4 &&
            msg.time - skan_vaqt <= 120) {
            if (skan_maydon && IsWindow(skan_maydon)) SetWindowTextW(skan_maydon, skan_eski.c_str());
            ui.skaner = skan;
            ui.belgilar.clear();
            skan.clear();
            InvalidateRect(hOyna, nullptr, FALSE);
            UpdateWindow(hOyna);
            continue;
        }
        if (ichida && msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_TAB && IsDialogMessageW(hOyna, &msg)) continue;
            if (msg.wParam == VK_RETURN && msg.hwnd != hOyna) {
                wchar_t sinf[16];
                GetClassNameW(msg.hwnd, sinf, 16);
                if (_wcsicmp(sinf, L"Edit") == 0 &&
                    !(GetWindowLongW(msg.hwnd, GWL_STYLE) & ES_WANTRETURN)) {
                    ui.enter_qosh(msg.hwnd);
                    InvalidateRect(hOyna, nullptr, FALSE);
                    UpdateWindow(hOyna);
                    continue;
                }
            }
            ui.tugmacha_qosh((UINT)msg.wParam);
            InvalidateRect(hOyna, nullptr, FALSE);
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    baza.yop();
    if (mutex) CloseHandle(mutex);
    return 0;
}
