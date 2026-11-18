// SOZLAMALAR: chek, printer, dollar kursi, baza importi va zaxira.
#include "sahifa.h"

#include <commdlg.h>
#include <shellapi.h>
#include <winhttp.h>

namespace {

enum { ID_DOKON = 10, ID_IZOH, ID_PRINTER, ID_ENI, ID_SAQLA, ID_SINOV, ID_KURS = 20, ID_KURS_SAQLA,
       ID_KURS_BANK, ID_KURS_MATN, ID_BAZA = 30, ID_IMPORT, ID_ZAXIRA, ID_PAPKA, ID_YORLIQ = 40 };

// Markaziy bank kursi: https://cbu.uz/uz/arkhiv-kursov-valyut/json/USD/ -> [{"Rate":"12035.27",...}]
bool bankdan_kurs(double& kurs, wstring& xato) {
    HINTERNET s = WinHttpOpen(L"Xozmag", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                              WINHTTP_NO_PROXY_BYPASS, 0);
    if (!s) { xato = L"Internet kutubxonasi ochilmadi."; return false; }
    WinHttpSetTimeouts(s, 5000, 5000, 8000, 8000);
    HINTERNET c = WinHttpConnect(s, L"cbu.uz", INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET r = c ? WinHttpOpenRequest(c, L"GET", L"/uz/arkhiv-kursov-valyut/json/USD/", nullptr,
                                         WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                         WINHTTP_FLAG_SECURE)
                    : nullptr;
    string javob;
    bool ok = r && WinHttpSendRequest(r, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
              WinHttpReceiveResponse(r, nullptr);
    if (ok) {
        DWORD bor = 0;
        while (WinHttpQueryDataAvailable(r, &bor) && bor) {
            string b(bor, '\0');
            DWORD oqildi = 0;
            if (!WinHttpReadData(r, b.data(), bor, &oqildi) || !oqildi) break;
            javob.append(b.data(), oqildi);
        }
    }
    if (r) WinHttpCloseHandle(r);
    if (c) WinHttpCloseHandle(c);
    WinHttpCloseHandle(s);
    if (!ok) { xato = L"cbu.uz ga ulanib bo'lmadi (internet bormi?)."; return false; }
    size_t p = javob.find("\"Rate\"");
    if (p != string::npos) p = javob.find('"', javob.find(':', p));
    if (p == string::npos) { xato = L"Markaziy bank javobida kurs topilmadi."; return false; }
    kurs = atof(javob.c_str() + p + 1);
    if (kurs <= 0) { xato = L"Kurs noto'g'ri keldi."; return false; }
    return true;
}

class Sozlama : public Sahifa {
public:
    void yarat() override {
        hSarlavha = yorliq(h, L"SOZLAMALAR", 0, fJuda);
        s1 = yorliq(h, L"Chek", ID_YORLIQ, fKatta);
        y[0] = yorliq(h, L"Do'kon nomi (chek sarlavhasi):", ID_YORLIQ);
        y[1] = yorliq(h, L"Chek ostida (manzil, telefon):", ID_YORLIQ);
        y[2] = yorliq(h, L"Chek printeri:", ID_YORLIQ);
        y[3] = yorliq(h, L"Chek eni:", ID_YORLIQ);
        edDokon = maydon(h, ID_DOKON);
        edIzoh = maydon(h, ID_IZOH);
        cbPrinter = tanlov(h, ID_PRINTER, {});
        cbEni = tanlov(h, ID_ENI, {L"58 mm (32 belgi)", L"80 mm (42 belgi)"});
        bSaqla = tugma(h, L"Saqlash", ID_SAQLA, T_KOK);
        bSinov = tugma(h, L"Sinov cheki", ID_SINOV);

        s2 = yorliq(h, L"Dollar kursi", ID_YORLIQ, fKatta);
        edKurs = maydon(h, ID_KURS, fKatta, ES_RIGHT);
        bKurs = tugma(h, L"Saqlash", ID_KURS_SAQLA, T_KOK);
        bBank = tugma(h, L"Markaziy bankdan olish", ID_KURS_BANK);
        hKurs = yorliq(h, L"", ID_KURS_MATN);

        s3 = yorliq(h, L"Baza", ID_YORLIQ, fKatta);
        hBaza = yorliq(h, L"", ID_BAZA);
        bImport = tugma(h, L"Eski bazani import qilish (db.sqlite3)…", ID_IMPORT, T_QIZIL);
        bZaxira = tugma(h, L"Zaxira nusxa olish…", ID_ZAXIRA);
        bPapka = tugma(h, L"Ma'lumotlar papkasini ochish", ID_PAPKA);
    }

    void joylash(int w, int) override {
        const int M = dp(16), G = dp(8), YL = dp(270), ME = std::min(dp(460), w - M * 3 - YL);
        int yy = M;
        joy(hSarlavha, M, yy, dp(400), dp(36));
        yy += dp(50);
        joy(s1, M, yy, dp(300), dp(30));
        yy += dp(36);
        HWND el[4] = {edDokon, edIzoh, cbPrinter, cbEni};
        for (int i = 0; i < 4; ++i) {
            joy(y[i], M, yy + dp(6), YL, dp(24));
            joy(el[i], M + YL, yy, ME, i >= 2 ? dp(300) : dp(34));
            yy += dp(42);
        }
        joy(bSaqla, M + YL, yy, dp(150), dp(42));
        joy(bSinov, M + YL + dp(150) + G, yy, dp(150), dp(42));
        yy += dp(62);
        joy(s2, M, yy, dp(300), dp(30));
        yy += dp(36);
        joy(edKurs, M, yy, dp(200), dp(42));
        joy(bKurs, M + dp(200) + G, yy, dp(140), dp(42));
        joy(bBank, M + dp(340) + G * 2, yy, dp(260), dp(42));
        yy += dp(48);
        joy(hKurs, M, yy, w - M * 2, dp(24));
        yy += dp(44);
        joy(s3, M, yy, dp(300), dp(30));
        yy += dp(36);
        joy(hBaza, M, yy, w - M * 2, dp(90));
        yy += dp(96);
        joy(bImport, M, yy, dp(380), dp(46));
        joy(bZaxira, M + dp(380) + G, yy, dp(220), dp(46));
        joy(bPapka, M + dp(600) + G * 2, yy, dp(280), dp(46));
    }

    void korsatildi() override {
        matn_qoy(edDokon, sozlama("dokon_nomi", L"XOZMAG"));
        matn_qoy(edIzoh, sozlama("chek_izoh", L""));
        SendMessageW(cbPrinter, CB_RESETCONTENT, 0, 0);
        SendMessageW(cbPrinter, CB_ADDSTRING, 0, (LPARAM)L"Windows standart printeri");
        wstring joriy = sozlama("printer", L"");
        int tan = 0, i = 1;
        for (auto& p : printerlar()) {
            SendMessageW(cbPrinter, CB_ADDSTRING, 0, (LPARAM)p.c_str());
            if (p == joriy) tan = i;
            ++i;
        }
        if (!joriy.empty() && tan == 0) {  // saqlangan printer hozir ulanmagan
            SendMessageW(cbPrinter, CB_ADDSTRING, 0, (LPARAM)joriy.c_str());
            tan = i;
        }
        SendMessageW(cbPrinter, CB_SETCURSEL, tan, 0);
        SendMessageW(cbEni, CB_SETCURSEL, sozlama("chek_eni", L"58") == L"80" ? 1 : 0, 0);
        double k = dollar_kursi();
        matn_qoy(edKurs, k > 0 ? pul(k) : L"");
        kurs_matni();
        baza_matni();
    }

    void buyruq(int id, int kod, HWND) override {
        if (kod != BN_CLICKED) return;
        switch (id) {
            case ID_SAQLA: saqla(); xabar(hAsosiy, L"Saqlandi."); break;
            case ID_SINOV: sinov(); break;
            case ID_KURS_SAQLA: {
                double k;
                if (!songa(matn_ol(edKurs), k) || k <= 0) {
                    xabar(hAsosiy, L"Kursni son bilan yozing, masalan 12 050.", true);
                    break;
                }
                kurs_saqla(k);
                kurs_matni();
                break;
            }
            case ID_KURS_BANK: {
                HCURSOR eski = SetCursor(LoadCursor(nullptr, IDC_WAIT));
                double k;
                wstring x;
                bool ok = bankdan_kurs(k, x);
                SetCursor(eski);
                if (!ok) { xabar(hAsosiy, x, true); break; }
                kurs_saqla(k);
                matn_qoy(edKurs, pul(k));
                kurs_matni();
                break;
            }
            case ID_IMPORT: import_qil(); break;
            case ID_ZAXIRA: zaxira(); break;
            case ID_PAPKA:
                ShellExecuteW(hAsosiy, L"open", malumot_papkasi().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                break;
        }
    }

    HBRUSH rang(HDC dc, HWND el) override {
        if (el == s1 || el == s2 || el == s3) SetTextColor(dc, RANG_KOK);
        else return nullptr;
        SetBkColor(dc, RANG_FON);
        return oqFon;
    }

private:
    HWND hSarlavha, s1, s2, s3, y[4], edDokon, edIzoh, cbPrinter, cbEni, bSaqla, bSinov;
    HWND edKurs, bKurs, bBank, hKurs, hBaza, bImport, bZaxira, bPapka;

    void saqla() {
        sozlama_yoz("dokon_nomi", tozala(matn_ol(edDokon)));
        sozlama_yoz("chek_izoh", tozala(matn_ol(edIzoh)));
        int p = (int)SendMessageW(cbPrinter, CB_GETCURSEL, 0, 0);
        wstring nom;
        if (p > 0) {
            wchar_t b[512] = {};
            SendMessageW(cbPrinter, CB_GETLBTEXT, p, (LPARAM)b);
            nom = b;
        }
        sozlama_yoz("printer", nom);
        sozlama_yoz("chek_eni", SendMessageW(cbEni, CB_GETCURSEL, 0, 0) == 1 ? L"80" : L"58");
    }

    void sinov() {
        saqla();
        Chek c;
        c.id = 0;
        c.sana_utc = hozir_utc();
        c.qatorlar = {{L"Sinov tovari: shurup 4x40 qora", L"dona", L"som", 25, 300, true},
                      {L"Sement M400 50 kg", L"qop", L"som", 2, 65000, true}};
        c.naqd = 137500;
        wstring x;
        if (chek_chop(c, x)) xabar(hAsosiy, L"Sinov cheki printerga yuborildi.");
        else xabar(hAsosiy, x, true);
    }

    void kurs_matni() {
        Sorov s = baza.sor("SELECT sana, kurs FROM ombor_dollarkursi WHERE kurs>0 ORDER BY sana DESC LIMIT 1");
        wstring m = L"Kurs hali kiritilmagan. Chekda dollar summasi so'mga qo'shilmaydi — kurs faqat yozib qo'yiladi.";
        if (s.qator()) m = L"Oxirgi kurs: 1 $ = " + pul(s.son(1)) + L" so'm (" + s.matn(0) + L")";
        SetWindowTextW(hKurs, m.c_str());
    }

    void baza_matni() {
        Sanoq q = sanoq();
        wstring m = L"Fayl: " + baza.fayl() + L"\n" + L"Tovarlar: " + guruhla(q.tovar) +
                    L"   ·   Qarzdorlar: " + guruhla(q.qarzdor) + L"   ·   Cheklar: " +
                    guruhla(q.sotuv) + L"   ·   Qarz hujjatlari: " + guruhla(q.qarz) +
                    L"\nJadvallar Django'dagi Xozmag bilan bir xil — eski db.sqlite3 to'g'ridan-to'g'ri o'qiladi.";
        SetWindowTextW(hBaza, m.c_str());
    }

    void import_qil() {
        if (kassa_savat_bormi()) {
            xabar(hAsosiy, L"Avval kassadagi chekni yakunlang yoki tozalang.", true);
            return;
        }
        wchar_t fayl[MAX_PATH] = {};
        OPENFILENAMEW o = {sizeof o};
        o.hwndOwner = hAsosiy;
        o.lpstrFilter = L"Xozmag bazasi (*.sqlite3;*.db)\0*.sqlite3;*.db\0Hamma fayllar\0*.*\0";
        o.lpstrFile = fayl;
        o.nMaxFile = MAX_PATH;
        o.lpstrTitle = L"Eski Xozmag bazasini tanlang (db.sqlite3)";
        o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (!GetOpenFileNameW(&o)) return;
        Sanoq q = sanoq();
        wstring sor = L"Tanlangan bazadagi tovarlar, qarzdorlar, qarzlar, to'lovlar va cheklar shu "
                      L"dasturga ko'chiriladi.\n\nHozirgi ma'lumotlar (" + guruhla(q.tovar) +
                      L" tovar, " + guruhla(q.qarzdor) + L" qarzdor, " + guruhla(q.sotuv) +
                      L" chek) almashtiriladi. Oldin avtomatik zaxira olinadi.\n\nTanlangan fayl "
                      L"o'zgarmaydi (faqat o'qiladi).\n\nDavom etilsinmi?";
        if (!sorash(hAsosiy, sor)) return;
        HCURSOR eski = SetCursor(LoadCursor(nullptr, IDC_WAIT));
        wstring h;
        bool ok = eski_bazani_import(fayl, h);
        SetCursor(eski);
        ombor_eskirdi();
        xabar(hAsosiy, h, !ok);
        korsatildi();
    }

    void zaxira() {
        wchar_t fayl[MAX_PATH];
        wcscpy_s(fayl, (L"xozmag_zaxira_" + fayl_vaqti() + L".sqlite3").c_str());
        OPENFILENAMEW o = {sizeof o};
        o.hwndOwner = hAsosiy;
        o.lpstrFilter = L"SQLite baza (*.sqlite3)\0*.sqlite3\0";
        o.lpstrFile = fayl;
        o.nMaxFile = MAX_PATH;
        o.lpstrDefExt = L"sqlite3";
        o.lpstrTitle = L"Zaxira nusxani qayerga saqlash";
        o.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (!GetSaveFileNameW(&o)) return;
        DeleteFileW(fayl);
        string yol = U(fayl), sql = "VACUUM INTO '";
        for (char c : yol) { sql += c; if (c == '\'') sql += '\''; }
        try {
            baza.bajar(sql + "'");
            xabar(hAsosiy, L"Zaxira saqlandi:\n" + wstring(fayl));
        } catch (const BazaXato& e) {
            xabar(hAsosiy, L"Zaxira olinmadi:\n" + W(e.what()), true);
        }
    }
};

Sozlama sozlamalar;

}  // namespace

Sahifa* sozlama_sahifa() { return &sozlamalar; }
