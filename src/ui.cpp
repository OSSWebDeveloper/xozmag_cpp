#include "ui.h"

#include <windowsx.h>

HINSTANCE hIlova;
HWND hAsosiy;
HFONT fOddiy, fQalin, fKatta, fTugma, fJuda, fMono;
HBRUSH oqFon;

static int dpi = 96;
static std::vector<HWND> modallar;

int dp(int px) { return MulDiv(px, dpi, 96); }

static HFONT shrift(const wchar_t* nom, int pt, bool qalin) {
    return CreateFontW(-MulDiv(pt, dpi, 72), 0, 0, 0, qalin ? FW_BOLD : FW_NORMAL, 0, 0, 0,
                       DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, nom);
}

static LRESULT CALLBACK oyna_jarayoni(HWND h, UINT m, WPARAM w, LPARAM l);

void ui_boshla() {
    HDC dc = GetDC(nullptr);
    dpi = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(nullptr, dc);
    fOddiy = shrift(L"Segoe UI", 11, false);
    fQalin = shrift(L"Segoe UI", 11, true);
    fKatta = shrift(L"Segoe UI", 16, false);
    fTugma = shrift(L"Segoe UI", 12, true);
    fJuda = shrift(L"Segoe UI", 20, true);
    fMono = shrift(L"Consolas", 11, false);
    oqFon = CreateSolidBrush(RANG_FON);

    WNDCLASSEXW wc = {sizeof wc};
    wc.lpfnWndProc = oyna_jarayoni;
    wc.hInstance = hIlova;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = oqFon;
    wc.lpszClassName = L"XozmagOyna";
    wc.hIcon = LoadIconW(hIlova, MAKEINTRESOURCEW(1));
    RegisterClassExW(&wc);
}

// ---------- Boshqaruv elementlari ----------

HWND yorliq(HWND ota, const wstring& matn, int id, HFONT f, DWORD stil) {
    HWND h = CreateWindowExW(0, L"STATIC", matn.c_str(), WS_CHILD | WS_VISIBLE | SS_NOPREFIX | stil,
                             0, 0, 10, 10, ota, (HMENU)(INT_PTR)id, hIlova, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)(f ? f : fOddiy), TRUE);
    return h;
}

HWND maydon(HWND ota, int id, HFONT f, DWORD stil, const wchar_t* ishora) {
    HWND h = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | stil, 0, 0, 10,
                             10, ota, (HMENU)(INT_PTR)id, hIlova, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)(f ? f : fOddiy), TRUE);
    if (ishora) SendMessageW(h, EM_SETCUEBANNER, TRUE, (LPARAM)ishora);
    return h;
}

HWND tugma(HWND ota, const wstring& matn, int id, TugmaRang rang, HFONT f) {
    HWND h = CreateWindowExW(0, L"BUTTON", matn.c_str(),
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, 0, 0, 10, 10, ota,
                             (HMENU)(INT_PTR)id, hIlova, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)(f ? f : fTugma), TRUE);
    SetWindowLongPtrW(h, GWLP_USERDATA, rang);
    return h;
}

HWND belgi(HWND ota, const wstring& matn, int id) {
    HWND h = CreateWindowExW(0, L"BUTTON", matn.c_str(),
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 10, 10,
                             ota, (HMENU)(INT_PTR)id, hIlova, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)fOddiy, TRUE);
    return h;
}

HWND tanlov(HWND ota, int id, const std::vector<wstring>& variantlar, int tanlangan) {
    HWND h = CreateWindowExW(0, L"COMBOBOX", L"",
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 0,
                             0, 10, dp(300), ota, (HMENU)(INT_PTR)id, hIlova, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)fOddiy, TRUE);
    for (auto& v : variantlar) SendMessageW(h, CB_ADDSTRING, 0, (LPARAM)v.c_str());
    SendMessageW(h, CB_SETCURSEL, tanlangan, 0);
    return h;
}

HWND royxat(HWND ota, int id, bool virtual_) {
    HWND h = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS |
                                 LVS_SINGLESEL | (virtual_ ? LVS_OWNERDATA : 0),
                             0, 0, 10, 10, ota, (HMENU)(INT_PTR)id, hIlova, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)fOddiy, TRUE);
    ListView_SetExtendedListViewStyle(h, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
                                             LVS_EX_DOUBLEBUFFER);
    return h;
}

void ustun(HWND lv, int i, const wchar_t* nom, int en, bool ong) {
    LVCOLUMNW c = {};
    c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    c.pszText = (LPWSTR)nom;
    c.cx = dp(en);
    c.fmt = ong ? LVCFMT_RIGHT : LVCFMT_LEFT;
    ListView_InsertColumn(lv, i, &c);
}

int lv_qosh(HWND lv, const std::vector<wstring>& q, LPARAM param) {
    LVITEMW it = {};
    it.mask = LVIF_TEXT | LVIF_PARAM;
    it.iItem = ListView_GetItemCount(lv);
    it.pszText = (LPWSTR)(q.empty() ? L"" : q[0].c_str());
    it.lParam = param;
    int i = (int)SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&it);
    for (size_t k = 1; k < q.size(); ++k) {
        LVITEMW s = {};
        s.iSubItem = (int)k;
        s.pszText = (LPWSTR)q[k].c_str();
        SendMessageW(lv, LVM_SETITEMTEXTW, i, (LPARAM)&s);
    }
    return i;
}

LPARAM lv_param(HWND lv, int i) {
    LVITEMW it = {};
    it.mask = LVIF_PARAM;
    it.iItem = i;
    ListView_GetItem(lv, &it);
    return it.lParam;
}

int lv_tanlangan(HWND lv) { return ListView_GetNextItem(lv, -1, LVNI_SELECTED); }

void lv_tanla(HWND lv, int i) {
    if (i < 0) return;
    ListView_SetItemState(lv, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(lv, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(lv, i, FALSE);
}

void tugma_rangi(HWND h, TugmaRang r) {
    SetWindowLongPtrW(h, GWLP_USERDATA, r);
    InvalidateRect(h, nullptr, TRUE);
}

bool tugma_chiz(LPARAM lp) {
    auto* d = (DRAWITEMSTRUCT*)lp;
    if (d->CtlType != ODT_BUTTON) return false;
    auto rang = (TugmaRang)GetWindowLongPtrW(d->hwndItem, GWLP_USERDATA);
    COLORREF fon = RANG_KULRANG, yozuv = RGB(20, 20, 20), chegara = RGB(120, 125, 130);
    switch (rang) {
        case T_YASHIL: fon = RANG_YASHIL; yozuv = RGB(255, 255, 255); chegara = RGB(27, 94, 32); break;
        case T_KOK: fon = RANG_KOK; yozuv = RGB(255, 255, 255); chegara = RGB(13, 71, 161); break;
        case T_QIZIL: fon = RANG_QIZIL; yozuv = RGB(255, 255, 255); chegara = RGB(150, 20, 20); break;
        case T_YON: fon = RANG_YON; yozuv = RGB(220, 225, 230); chegara = RANG_YON; break;
        case T_YON_FAOL: fon = RANG_KOK; yozuv = RGB(255, 255, 255); chegara = RANG_KOK; break;
        default: break;
    }
    bool bosilgan = d->itemState & ODS_SELECTED, ochiq = !(d->itemState & ODS_DISABLED);
    if (bosilgan) fon = RGB(GetRValue(fon) * 3 / 4, GetGValue(fon) * 3 / 4, GetBValue(fon) * 3 / 4);
    if (!ochiq) { fon = RGB(240, 240, 240); yozuv = RGB(160, 160, 160); chegara = RGB(200, 200, 200); }

    HDC dc = d->hDC;
    RECT r = d->rcItem;
    HBRUSH b = CreateSolidBrush(fon);
    FillRect(dc, &r, b);
    DeleteObject(b);
    HBRUSH c = CreateSolidBrush(chegara);
    FrameRect(dc, &r, c);
    DeleteObject(c);
    if (rang == T_YON_FAOL) {  // tanlangan bo'lim: chap chetda oq chiziq
        RECT ch = {r.left, r.top, r.left + dp(5), r.bottom};
        FillRect(dc, &ch, (HBRUSH)GetStockObject(WHITE_BRUSH));
    }

    wchar_t matn[256];
    GetWindowTextW(d->hwndItem, matn, 256);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, yozuv);
    HFONT eski = (HFONT)SelectObject(dc, (HFONT)SendMessageW(d->hwndItem, WM_GETFONT, 0, 0));
    RECT o = r;
    InflateRect(&o, -dp(6), -dp(4));
    RECT hisob = o;
    DrawTextW(dc, matn, -1, &hisob, DT_CENTER | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
    int bal = hisob.bottom - hisob.top;
    o.top += ((o.bottom - o.top) - bal) / 2;
    DrawTextW(dc, matn, -1, &o, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
    SelectObject(dc, eski);
    if ((d->itemState & ODS_FOCUS) && rang != T_YON && rang != T_YON_FAOL) {
        RECT f = r;
        InflateRect(&f, -dp(3), -dp(3));
        DrawFocusRect(dc, &f);
    }
    return true;
}

wstring matn_ol(HWND h) {
    int n = GetWindowTextLengthW(h);
    wstring s(n, L'\0');
    if (n) GetWindowTextW(h, s.data(), n + 1);
    return s;
}

void matn_qoy(HWND h, const wstring& s) {
    if (matn_ol(h) != s) SetWindowTextW(h, s.c_str());
}

void joy(HWND h, int x, int y, int w, int hgt) {
    SetWindowPos(h, nullptr, x, y, w < 0 ? 0 : w, hgt < 0 ? 0 : hgt, SWP_NOZORDER | SWP_NOACTIVATE);
}

bool belgilanganmi(HWND h) { return SendMessageW(h, BM_GETCHECK, 0, 0) == BST_CHECKED; }
void belgila(HWND h, bool b) { SendMessageW(h, BM_SETCHECK, b ? BST_CHECKED : BST_UNCHECKED, 0); }

void xabar(HWND ota, const wstring& matn, bool xato) {
    MessageBoxW(ota ? ota : faol_oyna(), matn.c_str(), L"Xozmag",
                MB_OK | (xato ? MB_ICONERROR : MB_ICONINFORMATION));
}

bool sorash(HWND ota, const wstring& matn) {
    return MessageBoxW(ota ? ota : faol_oyna(), matn.c_str(), L"Xozmag",
                       MB_YESNO | MB_ICONQUESTION) == IDYES;
}

// ---------- Modal oyna ----------

HWND faol_oyna() { return modallar.empty() ? hAsosiy : modallar.back(); }

static LRESULT CALLBACK oyna_jarayoni(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_NCCREATE) {
        auto* o = (Oyna*)((CREATESTRUCTW*)l)->lpCreateParams;
        o->h = h;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)o);
    }
    auto* o = (Oyna*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (o && o->ishlov) {
        LRESULT r = 0;
        if (o->ishlov(*o, m, w, l, r)) return r;
    }
    switch (m) {
        case WM_CLOSE:
            if (o) o->yop(IDCANCEL);
            return 0;
        case WM_COMMAND:
            if (LOWORD(w) == IDCANCEL && o) o->yop(IDCANCEL);
            return 0;
        case DM_GETDEFID:
            return MAKELONG(IDOK, DC_HASDEFID);
        case WM_DRAWITEM:
            if (tugma_chiz(l)) return TRUE;
            break;
        case WM_CTLCOLORSTATIC:
            SetBkColor((HDC)w, RANG_FON);
            return (LRESULT)oqFon;
    }
    return DefWindowProcW(h, m, w, l);
}

void oyna_yarat(Oyna& o, HWND ota, const wstring& sarlavha, int w, int h) {
    RECT r = {0, 0, w, h};
    DWORD stil = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
    DWORD qo = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    AdjustWindowRectEx(&r, stil, FALSE, qo);
    int ow = r.right - r.left, oh = r.bottom - r.top;
    RECT ot;
    GetWindowRect(ota ? ota : GetDesktopWindow(), &ot);
    int x = ot.left + ((ot.right - ot.left) - ow) / 2, y = ot.top + ((ot.bottom - ot.top) - oh) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    CreateWindowExW(qo, L"XozmagOyna", sarlavha.c_str(), stil, x, y, ow, oh, ota, nullptr, hIlova,
                    &o);
}

int oyna_modal(Oyna& o, HWND ota) {
    modallar.push_back(o.h);
    ShowWindow(o.h, SW_SHOW);
    if (ota) EnableWindow(ota, FALSE);
    MSG m;
    bool chiqish = false;
    while (!o.tugadi) {
        if (!GetMessageW(&m, nullptr, 0, 0)) {
            chiqish = true;
            break;
        }
        if (!IsDialogMessageW(o.h, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }
    if (ota) {
        EnableWindow(ota, TRUE);
        SetActiveWindow(ota);
    }
    modallar.pop_back();
    o.ishlov = nullptr;
    DestroyWindow(o.h);
    if (chiqish) PostQuitMessage(0);
    return o.natija;
}

// ---------- Forma ----------

bool forma(HWND ota, const wstring& sarlavha, std::vector<Maydon>& ms,
           std::function<wstring(std::vector<Maydon>&)> tekshir) {
    const int CHAP = dp(16), YORLIQ = dp(190), MAYDON = dp(300), QATOR = dp(32), ORA = dp(8);
    const int EN = CHAP * 2 + YORLIQ + MAYDON;
    // balandlikni hisoblash
    int y = dp(16);
    std::vector<int> yy;
    for (auto& m : ms) {
        yy.push_back(y);
        if (m.tur == Maydon::MALUMOT) {
            int satr = 1;
            for (wchar_t c : m.qiymat) satr += c == L'\n';
            y += satr * dp(21) + ORA;
        } else
            y += QATOR + ORA;
    }
    int bal = y + dp(12) + dp(44) + dp(16);

    Oyna o;
    oyna_yarat(o, ota, sarlavha, EN, bal);
    HWND birinchi = nullptr;
    for (size_t i = 0; i < ms.size(); ++i) {
        auto& m = ms[i];
        int id = 100 + (int)i;
        if (m.tur == Maydon::MALUMOT) {
            m.h = yorliq(o.h, m.qiymat, id, fQalin);
            joy(m.h, CHAP, yy[i], EN - CHAP * 2, QATOR * 4);
            int satr = 1;
            for (wchar_t c : m.qiymat) satr += c == L'\n';
            joy(m.h, CHAP, yy[i], EN - CHAP * 2, satr * dp(21));
            continue;
        }
        if (m.tur != Maydon::BELGI) {
            HWND yl = yorliq(o.h, m.yorliq + L":", 0);
            joy(yl, CHAP, yy[i] + dp(5), YORLIQ - dp(8), QATOR - dp(5));
        }
        switch (m.tur) {
            case Maydon::MATN:
            case Maydon::SON:
                m.h = maydon(o.h, id, fOddiy, m.tur == Maydon::SON ? ES_RIGHT : 0);
                SetWindowTextW(m.h, m.qiymat.c_str());
                joy(m.h, CHAP + YORLIQ, yy[i], MAYDON, QATOR);
                break;
            case Maydon::TANLOV:
                m.h = tanlov(o.h, id, m.variantlar, m.tanlangan);
                joy(m.h, CHAP + YORLIQ, yy[i], MAYDON, dp(300));
                break;
            case Maydon::BELGI:
                m.h = belgi(o.h, m.yorliq, id);
                belgila(m.h, m.belgilangan);
                joy(m.h, CHAP + YORLIQ, yy[i], MAYDON, QATOR);
                break;
            default: break;
        }
        if (!birinchi) birinchi = m.h;
    }
    HWND ok = tugma(o.h, L"Saqlash", IDOK, T_KOK);
    HWND bekor = tugma(o.h, L"Bekor", IDCANCEL);
    joy(ok, EN - CHAP - dp(150) * 2 - ORA, bal - dp(16) - dp(44), dp(150), dp(44));
    joy(bekor, EN - CHAP - dp(150), bal - dp(16) - dp(44), dp(150), dp(44));

    o.ishlov = [&](Oyna& o, UINT m, WPARAM w, LPARAM, LRESULT&) {
        if (m == WM_COMMAND && LOWORD(w) == IDOK) {
            for (auto& x : ms) {
                if (x.tur == Maydon::MATN || x.tur == Maydon::SON) x.qiymat = matn_ol(x.h);
                if (x.tur == Maydon::TANLOV) x.tanlangan = (int)SendMessageW(x.h, CB_GETCURSEL, 0, 0);
                if (x.tur == Maydon::BELGI) x.belgilangan = belgilanganmi(x.h);
            }
            wstring x = tekshir ? tekshir(ms) : L"";
            if (x.empty())
                o.yop(IDOK);
            else
                xabar(o.h, x, true);
            return true;
        }
        return false;
    };
    if (birinchi) {
        SetFocus(birinchi);
        SendMessageW(birinchi, EM_SETSEL, 0, -1);
    }
    return oyna_modal(o, ota) == IDOK;
}

// ---------- Ro'yxatli oyna ----------

// Qidiruv maydonida yuqori/pastki strelka ro'yxatni siljitadi.
static LRESULT CALLBACK qidiruv_kalit(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR lv) {
    if (m == WM_KEYDOWN && (w == VK_DOWN || w == VK_UP || w == VK_NEXT || w == VK_PRIOR)) {
        HWND r = (HWND)lv;
        if (lv_tanlangan(r) < 0 && ListView_GetItemCount(r) > 0) {
            lv_tanla(r, 0);
            return 0;
        }
        SendMessageW(r, WM_KEYDOWN, w, l);
        return 0;
    }
    return DefSubclassProc(h, m, w, l);
}

int royxat_oyna(HWND ota, RoyxatSozlama& s) {
    const int CHET = dp(14), EN = dp(s.en), BAL = dp(s.boy), TB = dp(44);
    Oyna o;
    oyna_yarat(o, ota, s.sarlavha, EN, BAL);
    int y = CHET;
    HWND qid = nullptr;
    if (s.qidiruv) {
        qid = maydon(o.h, 10, fKatta, 0, s.qidiruv_ishora.c_str());
        joy(qid, CHET, y, EN - CHET * 2, dp(38));
        y += dp(38) + dp(10);
    }
    HWND lv = royxat(o.h, 11);
    for (size_t i = 0; i < s.ustunlar.size(); ++i)
        ustun(lv, (int)i, s.ustunlar[i].first.c_str(), std::abs(s.ustunlar[i].second),
              s.ustunlar[i].second < 0);
    joy(lv, CHET, y, EN - CHET * 2, BAL - y - CHET * 2 - TB);
    if (qid) SetWindowSubclass(qid, qidiruv_kalit, 1, (DWORD_PTR)lv);

    int x = EN - CHET;
    HWND yop = tugma(o.h, L"Yopish", IDCANCEL);
    x -= dp(120);
    joy(yop, x, BAL - CHET - TB, dp(120), TB);
    for (int i = (int)s.tugmalar.size() - 1; i >= 0; --i) {
        HWND t = tugma(o.h, s.tugmalar[i], 200 + i, i == 0 ? T_KOK : T_ODDIY);
        int en = dp(std::max(140, (int)s.tugmalar[i].size() * 10 + 30));
        x -= en + dp(8);
        joy(t, x, BAL - CHET - TB, en, TB);
    }
    if (!s.pastki.empty()) {
        HWND p = yorliq(o.h, s.pastki, 12);
        joy(p, CHET, BAL - CHET - TB + dp(10), x - CHET * 2, TB - dp(10));
    }

    auto toldir = [&]() {
        SendMessageW(lv, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(lv);
        if (s.toldir) s.toldir(lv, qid ? matn_ol(qid) : L"");
        if (ListView_GetItemCount(lv) > 0) lv_tanla(lv, 0);
        SendMessageW(lv, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(lv, nullptr, TRUE);
    };
    int bosilgan = -1;
    auto bos = [&](int i) {
        if (i < 0 || i >= (int)s.tugmalar.size()) return;
        int t = lv_tanlangan(lv);
        LPARAM p = t >= 0 ? lv_param(lv, t) : -1;
        if (s.bosildi && s.bosildi(i, p, lv)) {
            bosilgan = i;
            o.yop(IDOK);
        } else
            toldir();
    };
    o.ishlov = [&](Oyna&, UINT m, WPARAM w, LPARAM l, LRESULT&) {
        if (m == WM_COMMAND) {
            int id = LOWORD(w);
            if (id == 10 && HIWORD(w) == EN_CHANGE) { toldir(); return true; }
            if (id == IDOK) { bos(0); return true; }
            if (id >= 200 && id < 200 + (int)s.tugmalar.size()) { bos(id - 200); return true; }
        }
        if (m == WM_NOTIFY) {
            auto* n = (NMHDR*)l;
            if (n->idFrom == 11 && n->code == NM_DBLCLK) { bos(0); return true; }
        }
        return false;
    };
    toldir();
    SetFocus(qid ? qid : lv);
    oyna_modal(o, ota);
    return bosilgan;
}

void matn_oyna(HWND ota, const wstring& sarlavha, const wstring& matn, const wstring& tugma_,
               std::function<void()> amal) {
    const int EN = dp(560), BAL = dp(640), CHET = dp(14), TB = dp(44);
    Oyna o;
    oyna_yarat(o, ota, sarlavha, EN, BAL);
    HWND e = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                             WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY |
                                 ES_AUTOVSCROLL,
                             CHET, CHET, EN - CHET * 2, BAL - CHET * 3 - TB, o.h, nullptr, hIlova,
                             nullptr);
    SendMessageW(e, WM_SETFONT, (WPARAM)fMono, TRUE);
    wstring crlf;
    for (wchar_t c : matn) {
        if (c == L'\n') crlf += L'\r';
        crlf += c;
    }
    SetWindowTextW(e, crlf.c_str());
    HWND yop = tugma(o.h, L"Yopish", IDCANCEL);
    joy(yop, EN - CHET - dp(120), BAL - CHET - TB, dp(120), TB);
    if (!tugma_.empty()) {
        HWND t = tugma(o.h, tugma_, 300, T_KOK);
        joy(t, EN - CHET - dp(120) - dp(8) - dp(200), BAL - CHET - TB, dp(200), TB);
    }
    o.ishlov = [&](Oyna&, UINT m, WPARAM w, LPARAM, LRESULT&) {
        if (m == WM_COMMAND && LOWORD(w) == 300) {
            if (amal) amal();
            return true;
        }
        if (m == WM_COMMAND && LOWORD(w) == IDOK) return true;
        return false;
    };
    SetFocus(yop);
    oyna_modal(o, ota);
}
