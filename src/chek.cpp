// Chek matni va uni Windows printeri orqali chop etish (58 / 80 mm chek printerlari).
#include "chek_chop.h"

#include <shlobj.h>
#include <winspool.h>

#include "ui.h"

static int chek_eni() { return ini_qiymat(L"chek_eni", L"80") == L"58" ? 32 : 42; }

void chekni_avtomatik_chop(long long sotuv_id) {
    if (ini_qiymat(L"chek_printer").empty()) return;
    Chek c;
    wstring x;
    if (chek_ol(sotuv_id, c) && !chek_chop(c, x)) ui.xabar(L"Chek chop etilmadi: " + x, X_XATO);
}

static wstring ong(const wstring& chap, const wstring& ong_, int en) {
    int bosh = en - (int)chap.size() - (int)ong_.size();
    return chap + wstring(bosh > 1 ? bosh : 1, L' ') + ong_;
}

static wstring markaz(const wstring& s, int en) {
    int b = (en - (int)s.size()) / 2;
    return wstring(b > 0 ? b : 0, L' ') + s;
}

// Uzun nomni so'zlab qatorlarga bo'ladi.
static std::vector<wstring> bol(const wstring& s, int en) {
    std::vector<wstring> r;
    wstring qator, soz;
    for (wchar_t c : s + L" ") {
        if (c != L' ') { soz += c; continue; }
        if (soz.empty()) continue;
        while ((int)soz.size() > en) {
            if (!qator.empty()) { r.push_back(qator); qator.clear(); }
            r.push_back(soz.substr(0, en));
            soz = soz.substr(en);
        }
        if ((int)(qator.size() + soz.size() + (qator.empty() ? 0 : 1)) > en) {
            r.push_back(qator);
            qator.clear();
        }
        if (!qator.empty()) qator += L' ';
        qator += soz;
        soz.clear();
    }
    if (!qator.empty()) r.push_back(qator);
    return r;
}

wstring chek_matni(const Chek& c) {
    int en = chek_eni();
    wstring chiz(en, L'-'), r;
    wstring dokon = L"Ulug'bek Xozmag";
    for (auto& q : bol(dokon, en)) r += markaz(q, en) + L"\n";
    wstring izoh = ini_qiymat(L"chek_izoh");
    for (auto& q : bol(izoh, en)) r += markaz(q, en) + L"\n";
    r += ong(L"Chek #" + std::to_wstring(c.id), mahalliy_vaqt(c.sana_utc), en) + L"\n";
    if (c.bekor) r += markaz(L"*** BEKOR QILINGAN ***", en) + L"\n";
    r += chiz + L"\n";
    int n = 0;
    for (auto& q : c.qatorlar) {
        auto qatorlar = bol(std::to_wstring(++n) + L". " + q.nom, en);
        for (auto& x : qatorlar) r += x + L"\n";
        wstring v = q.valyuta == L"dollar" ? L" $" : L"";
        wstring miq = L"   " + son(q.miqdor) + L" " + q.birlik;
        if (q.narx_bor && q.narx > 0)
            r += ong(miq + L" x " + pul(q.narx), L"= " + pul(q.narx * q.miqdor) + v, en) + L"\n";
        else
            r += miq + L"\n";
    }
    r += chiz + L"\n";
    double jami = c.naqd + c.qarz, jami_d = c.naqd_d + c.qarz_d;
    r += ong(L"JAMI:", pul(jami) + L" so'm", en) + L"\n";
    if (jami_d > 0) r += ong(L"", pul(jami_d) + L" $", en) + L"\n";
    if (c.qarz > 0 || c.qarz_d > 0) {
        r += ong(L"To'landi:", pul(c.naqd) + L" so'm", en) + L"\n";
        if (c.naqd_d > 0) r += ong(L"", pul(c.naqd_d) + L" $", en) + L"\n";
        if (c.qarz > 0) r += ong(L"Qarzga:", pul(c.qarz) + L" so'm", en) + L"\n";
        if (c.qarz_d > 0) r += ong(L"Qarzga:", pul(c.qarz_d) + L" $", en) + L"\n";
        for (auto& x : bol(L"Qarzdor: " + c.qarzdor, en)) r += x + L"\n";
    }
    r += chiz + L"\n" + markaz(L"Xaridingiz uchun rahmat!", en) + L"\n";
    return r;
}

std::vector<wstring> printerlar() {
    std::vector<wstring> r;
    DWORD kerak = 0, soni = 0;
    EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, 4, nullptr, 0, &kerak, &soni);
    if (!kerak) return r;
    std::vector<BYTE> buf(kerak);
    if (!EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, 4, buf.data(), kerak,
                       &kerak, &soni))
        return r;
    auto* p = (PRINTER_INFO_4W*)buf.data();
    for (DWORD i = 0; i < soni; ++i) r.push_back(p[i].pPrinterName);
    return r;
}

bool chek_chop(const Chek& c, wstring& xato, const wstring& tanlangan) {
    wstring printer = tanlangan.empty() ? ini_qiymat(L"chek_printer") : tanlangan;
    if (printer.empty()) {
        wchar_t b[512];
        DWORD n = 512;
        if (!GetDefaultPrinterW(b, &n)) {
            xato = L"Printer tanlanmagan va Windows'da standart printer yo'q (Sozlamalar → Chek printeri).";
            return false;
        }
        printer = b;
    }
    HDC dc = CreateDCW(L"WINSPOOL", printer.c_str(), nullptr, nullptr);
    if (!dc) {
        xato = L"«" + printer + L"» printeri ochilmadi.";
        return false;
    }
    int en = chek_eni();
    int sahifa_en = GetDeviceCaps(dc, HORZRES), sahifa_boy = GetDeviceCaps(dc, VERTRES);
    // Shriftni chek eniga moslaymiz: `en` ta belgi sahifa eniga sig'sin.
    HFONT f = nullptr;
    int boy = 0;
    wstring namuna(en, L'W');
    for (int bal = sahifa_en / 8; bal > 6; bal -= std::max(1, bal / 20)) {
        if (f) DeleteObject(f);
        f = CreateFontW(-bal, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0,
                        FIXED_PITCH | FF_MODERN, L"Consolas");
        SelectObject(dc, f);
        SIZE o;
        GetTextExtentPoint32W(dc, namuna.c_str(), en, &o);
        boy = o.cy;
        if (o.cx <= sahifa_en) break;
    }
    DOCINFOW di = {sizeof di};
    wstring nom = L"Xozmag chek #" + std::to_wstring(c.id);
    di.lpszDocName = nom.c_str();
    if (StartDocW(dc, &di) <= 0) {
        DeleteDC(dc);
        if (f) DeleteObject(f);
        xato = L"Chop etish boshlanmadi (printer ulanganmi?).";
        return false;
    }
    StartPage(dc);
    SelectObject(dc, f);
    SetBkMode(dc, TRANSPARENT);
    int y = 0;
    wstring matn = chek_matni(c), qator;
    for (wchar_t ch : matn) {
        if (ch != L'\n') { qator += ch; continue; }
        if (y + boy > sahifa_boy) {
            EndPage(dc);
            StartPage(dc);
            SelectObject(dc, f);
            SetBkMode(dc, TRANSPARENT);
            y = 0;
        }
        TextOutW(dc, 0, y, qator.c_str(), (int)qator.size());
        y += boy;
        qator.clear();
    }
    EndPage(dc);
    EndDoc(dc);
    DeleteDC(dc);
    if (f) DeleteObject(f);
    return true;
}
